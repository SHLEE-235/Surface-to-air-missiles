#include "LSStatusManager.h"
#include "ConfigParser.h"
#include "MotorManagerFactory.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <arpa/inet.h>
#include <byteswap.h>
#include <cstdint>

LSStatusManager::LSStatusManager(const std::string &launcherConfigPath)
{
    init(launcherConfigPath);
}

LSStatusManager::~LSStatusManager()
{
    {
        std::lock_guard<std::mutex> lock(threadJoinMutex);
        moveFlag = false;
        moveCV.notify_all();
        if (moveThread.joinable())
            moveThread.join();
    }
}

void LSStatusManager::init(const std::string &launcherConfigPath)
{
    try
    {

        status.id = ConfigParser::getInt("Launcher", "ID", launcherConfigPath);
        status.position.x = ConfigParser::getLongLong("Launcher", "PositionX", launcherConfigPath);
        status.position.y = ConfigParser::getLongLong("Launcher", "PositionY", launcherConfigPath);
        status.position.z = ConfigParser::getLongLong("Launcher", "PositionZ", launcherConfigPath);
        status.angle = ConfigParser::getDouble("Launcher", "LaunchAngle", launcherConfigPath);
        status.speed = ConfigParser::getInt("Launcher", "LaunchSpeed", launcherConfigPath);

        std::string modeStr = ConfigParser::getValue("Launcher", "Mode", launcherConfigPath);
        if (modeStr == "STOP_MODE")
            status.mode = OperationMode::STOP_MODE;
        else if (modeStr == "MOVE_MODE")
            status.mode = OperationMode::MOVE_MODE;
        else if (modeStr == "WAR_MODE")
            status.mode = OperationMode::WAR_MODE;
        else
            throw std::invalid_argument("Invalid Launcher Mode: " + modeStr);

        // Motor 핀 설정 읽기
        std::string motorType = ConfigParser::getValue("Motor", "Type", launcherConfigPath);
        std::string device = ConfigParser::getValue("Motor", "Device", launcherConfigPath);
        uint32_t uartBaudRate = static_cast<uint32_t>(ConfigParser::getInt("Motor", "BaudRate", launcherConfigPath));

        motorManager = MotorManagerFactory::create(motorType, device, uartBaudRate);

        std::cout << "[LSStatusManager] Initialized from config: " << launcherConfigPath << "\n";
    }

    catch (const std::exception &e)
    {
        std::cerr << "[LSStatusManager] Config error: " << e.what() << "\n";
    }
}

void LSStatusManager::updatePosition(long long x, long long y)
{
    std::cout << "[LSStatusManager] Position updated: (" << x << ", " << y << ")\n";
    status.position.x = x;
    status.position.y = y;
}

void LSStatusManager::updateLaunchAngle(double angle)
{
    std::cout << "[LSStatusManager] Launch angle updated: " << angle << "°\n";
    status.angle = angle;
}

void LSStatusManager::changeMode(OperationMode mode)
{
    std::cout << "[LSStatusManager] Mode changed from "
              << static_cast<int>(status.mode) << " to " << static_cast<int>(mode) << "\n";
    status.mode = mode;
}

void LSStatusManager::positionBriefing(long long &x, long long &y, long long &z) const
{
    x = this->status.position.x;
    y = this->status.position.y;
    z = this->status.position.z;
    return;
}

void LSStatusManager::speedBriefing(int &speed) const
{
    speed = status.speed;
    return;
}

void LSStatusManager::serializeStatus(std::vector<uint8_t> &out) const
{
    out.clear();
    out.reserve(sizeof(LSStatus) + 1);

    const LSStatus &s = status;

    auto append = [&](const void *data, size_t size)
    {
        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        out.insert(out.end(), bytes, bytes + size);
    };

    // CommandType
    uint8_t commandType = 0x41;
    append(&commandType, sizeof(commandType));

    // ID (uint32_t - 그대로 저장: 리틀엔디안)
    append(&s.id, sizeof(s.id));

    // Position X/Y/Z (int64_t - 그대로 저장: 리틀엔디안)
    append(&s.position.x, sizeof(s.position.x));
    append(&s.position.y, sizeof(s.position.y));
    append(&s.position.z, sizeof(s.position.z));

    // Angle (double → 그대로 저장: 리틀엔디안)
    append(&s.angle, sizeof(s.angle));

    // Speed (int → uint32_t로 명시적 캐스팅 후 저장)
    uint32_t speedLE = static_cast<uint32_t>(s.speed);
    append(&speedLE, sizeof(speedLE));

    // Mode
    uint8_t mode_byte = static_cast<uint8_t>(s.mode);
    append(&mode_byte, sizeof(mode_byte));
}


void LSStatusManager::moveLS(long long x, long long y)
{
    {
        std::lock_guard<std::mutex> lock(threadJoinMutex); // 동시에 여러 스레드 join 방지
        // 이전 이동 종료 요청
        moveFlag = false;
        moveCV.notify_all();

        // 이전 스레드 종료
        if (moveThread.joinable())
        {
            moveThread.join();
        }

        // 새로운 이동 시작
        moveFlag = true;
        changeMode(OperationMode::MOVE_MODE);
        moveThread = std::thread(&LSStatusManager::moveRoutine, this, x, y);
    }
}

void LSStatusManager::moveRoutine(long long destX, long long destY)
{
    constexpr double speed_m_per_s = 13.8889; // 50 km/h
    constexpr int update_interval_ms = 1000;

    while (moveFlag)
    {
        long long curX = status.position.x;
        long long curY = status.position.y;

        double dx = static_cast<double>(destX - curX);
        double dy = static_cast<double>(destY - curY);
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < speed_m_per_s)
        {
            updatePosition(destX, destY);
            changeMode(OperationMode::STOP_MODE);
            moveFlag = false;
            break;
        }

        double dirX = dx / dist;
        double dirY = dy / dist;

        long long nextX = static_cast<long long>(curX + dirX * speed_m_per_s);
        long long nextY = static_cast<long long>(curY + dirY * speed_m_per_s);
        updatePosition(nextX, nextY);

        std::unique_lock<std::mutex> lock(moveMutex);
        moveCV.wait_for(lock, std::chrono::milliseconds(update_interval_ms), [&]()
                        { return !moveFlag; });
    }
}

bool LSStatusManager::rotateToAngle(double targetAngle)
{
    if (!motorManager)
    {
        std::cerr << "[LSStatusManager] MotorManager not initialized!\n";
        return false;
    }

    // 성공 여부 판단
    bool success = motorManager->rotateToAngle(targetAngle);

    if (success)
    {
        updateLaunchAngle(targetAngle);
        std::cout << "[LSStatusManager] Rotation succeeded\n";
    }
    else
    {
        std::cerr << "[LSStatusManager] Rotation failed\n";
    }

    return success;
}
