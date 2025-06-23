#include "Mfr.h"
#include "logger.h"
#include "IReceiver.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

void Mfr::requestLcInitData()
{
    ReqLcInitData reqData{};
    reqData.radarId = mfrId;

    std::vector<char> packet = serializePacketforSend(reqData);

    packet.insert(packet.begin(), static_cast<char>(LC_INIT_REQ));

    lcCommManager->send(packet);
}

void Mfr::stopDetectionThread()
{
    detectionThreadRunning = false;
    if (detectionThread.joinable())
    {
        detectionThread.join();
    }
}

void Mfr::startDetectionAlgoThread()
{
    stopDetectionThread(); // 기존 스레드가 있다면 정리
    detectionThreadRunning = true;

    detectionThread = std::thread([this]()
                                  {
        while (detectionThreadRunning) 
        {
            // Logger::log("MFR 탐지 알고리즘 스레드가 실행 중입니다.");
            mfrDetectionAlgo();
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // CPU 부하 감소
        } });
}

void Mfr::mfrDetectionAlgo()
{
    std::map<unsigned int, localMockSimData> localTargets;
    std::unordered_map<unsigned int, localMockSimData> localMissiles;

    localTargets = mockTargets;
    localMissiles = mockMissile;

    std::unordered_map<unsigned int, localMockSimData> localDetectedTargets;
    std::unordered_map<unsigned int, localMockSimData> localDetectedMissile;

    std::vector<MfrToLcTargetInfo> detectedTargetList;
    std::vector<MfrToLcMissileInfo> detectedMissileList;

    auto now = std::chrono::system_clock::now();
    unsigned long nowMs = toEpochMillis(now);

    if (mfrMode == ROTATION_MODE)
    {
        std::vector<std::pair<unsigned int, double>> targetDistances;

        for (const auto &[id, target] : localTargets)
        {
            double distance = calcDistance(mfrCoords, target.mockCoords);

            if (distance <= limitDetectionRange && !target.isHit)
            {
                localDetectedTargets[id] = target;
                targetDistances.emplace_back(id, distance);
                // Logger::log("Target ID: " + std::to_string(id) + ", Distance: " + std::to_string(distance) + " m, Sppeed: " + std::to_string(target.speed));
            }
            else
            {
                localDetectedTargets.erase(id);
                auto it = std::remove_if(targetDistances.begin(), targetDistances.end(),
                                         [id](const std::pair<unsigned int, double> &p)
                                         { return p.first == id; });

                targetDistances.erase(it, targetDistances.end());
            }
        }

        std::sort(targetDistances.begin(), targetDistances.end(),
                  [](const auto &a, const auto &b)
                  {
                      return std::get<1>(a) < std::get<1>(b);
                  });

        unsigned char priority = 1;
        for (const auto &[id, distance] : targetDistances)
        {
            const auto &target = localDetectedTargets[id];

            if (priority == 1)
            {
                goalTargetId = id;
            }

            MfrToLcTargetInfo status{};
            status.id = target.mockId;
            status.targetCoords = encode(target.mockCoords);
            status.targetSpeed = target.speed;
            status.targetAngle = target.angle;
            status.targetAngle2 = target.angle2;
            status.firstDetectionTime = nowMs;
            status.prioirty = priority++;
            status.isHit = false;

            detectedTargetList.push_back(status);
            Logger::log("Detected Target ID: " + std::to_string(status.id) +
                        ", Distance: " + std::to_string(distance) +
                        " m, Speed: " + std::to_string(status.targetSpeed));
        }

        for (const auto &[id, missile] : localMissiles)
        {
            long long distance = calcDistance(mfrCoords, missile.mockCoords);

            if (distance <= limitDetectionRange && !missile.isHit)
            {
                localDetectedMissile[id] = missile;

                MfrToLcMissileInfo status{};
                status.id = missile.mockId;
                status.missileCoords = encode(missile.mockCoords);
                status.missileSpeed = missile.speed;
                status.missileAngle = missile.angle;
                status.firstDetectionTime = nowMs;
                status.timeToIntercept = 10;
                status.isHit = false;

                detectedMissileList.push_back(status);
            }
            else
            {
                localDetectedMissile.erase(id);
                detectedMissile.erase(id);
            }
        }
    }

    // ######################################################################################

    else if (mfrMode == ANGLE_MODE)
    {
        // std::cout << "mfrMode: ANGLE_MODE" << std::endl;
        detectedTargetList.clear();
        detectedMissileList.clear();
        // std::cout << "goalTargetId: " << goalTargetId << std::endl;
        if (localTargets.find(goalTargetId) != localTargets.end())
        {
            // std::cout << "[Mfr::mfrDetectionAlgo] 목표 표적 ID: " << goalTargetId << std::endl;
            const auto &goalTarget = localTargets[goalTargetId];
            double baseAz = calcBearing(mfrCoords, goalTarget.mockCoords);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << baseAz;
            // stepMotorManager->sendCommand("STOP_MODE:" + oss.str());

            for (const auto &[id, target] : localTargets)
            {
                long long distance = calcDistance(mfrCoords, target.mockCoords);
                if (distance > limitDetectionRange)
                {
                    continue;
                }

                double az = calcBearing(mfrCoords, target.mockCoords);
                double diff = angleDiff(baseAz, az);

                if (std::abs(diff) <= 15.0)
                {
                    localDetectedTargets[id] = target;

                    MfrToLcTargetInfo status{};
                    status.id = target.mockId;
                    status.targetCoords = encode(target.mockCoords);
                    status.targetSpeed = target.speed;
                    status.targetAngle = target.angle;
                    status.targetAngle2 = target.angle2;
                    status.firstDetectionTime = nowMs;
                    status.prioirty = 1;
                    status.isHit = target.isHit;

                    detectedTargetList.push_back(status);
                }
            }

            for (const auto &[id, missile] : localMissiles)
            {
                long long distance = calcDistance(mfrCoords, missile.mockCoords);
                if (distance > limitDetectionRange)
                {
                    continue;
                }

                double az = calcBearing(mfrCoords, missile.mockCoords);
                double diff = angleDiff(baseAz, az);

                if (std::abs(diff) <= 15.0)
                {
                    localDetectedMissile[id] = missile;

                    MfrToLcMissileInfo status{};
                    status.id = missile.mockId;
                    status.missileCoords = encode(missile.mockCoords);
                    status.missileSpeed = missile.speed;
                    status.missileAngle = missile.angle;
                    status.firstDetectionTime = nowMs;
                    status.isHit = missile.isHit;

                    detectedMissileList.push_back(status);
                }
            }
        }
    }

    if (!detectedTargetList.empty() || !detectedMissileList.empty())
    {
        std::vector<char> packet = serializeDetectionPacket(detectedTargetList, detectedMissileList);
        Logger::log("[Mfr::mfrDetectionAlgo] Sending detection data to LC, Targets: " + std::to_string(detectedTargetList.size()) + ", Missiles: " + std::to_string(detectedMissileList.size()));
        lcCommManager->send(packet);
    }

    detectedTargets = std::move(localDetectedTargets);
    detectedMissile = std::move(localDetectedMissile);
}

// util
double Mfr::deg2rad(const double &deg)
{
    return deg * M_PI / 180.0;
}

double Mfr::calcDistance(const Pos3D &mfrCoord, const Pos3D &mockCoord)
{
    // 위도, 경도 (단위: degree)
    double mfrLatitudeDeg = mfrCoord.latitude;
    double mfrLongitudeDeg = mfrCoord.longitude;
    double mockLatitudeDeg = mockCoord.latitude;
    double mockLongitudeDeg = mockCoord.longitude;

    // 위도, 경도 (단위: radian)
    double mfrLatitudeRad = deg2rad(mfrLatitudeDeg);
    double mockLatitudeRad = deg2rad(mockLatitudeDeg);

    // 위도, 경도 변화율 (단위: radian)
    double deltaLatitude = deg2rad(mockLatitudeDeg - mfrLatitudeDeg);
    double deltaLongitude = deg2rad(mockLongitudeDeg - mfrLongitudeDeg);

    // Haversine Formula 적용
    double sinDelataLatitude = std::sin(deltaLatitude / 2.0);
    double sinDelataLongitude = std::sin(deltaLongitude / 2.0);

    double a = sinDelataLatitude * sinDelataLatitude + std::cos(mfrLatitudeRad) * std::cos(mockLatitudeRad) * sinDelataLongitude * sinDelataLongitude;
    double c = 2.0 * std::asin(std::sqrt(a));

    double horizontal = EARTH_RADIUS_M * c;
    double vertical = mockCoord.altitude - mfrCoord.altitude;
    double dist = std::sqrt(horizontal * horizontal + vertical * vertical);

    return dist;
}

EncodedPos3D Mfr::encode(const Pos3D &p)
{
    EncodedPos3D e;

    e.latitude = std::llround(p.latitude * SCALE);   // 37.67213612 → 3767213612
    e.longitude = std::llround(p.longitude * SCALE); // 127.23644256 → 12723644256
    e.altitude = std::llround(p.altitude);           // 9000.12345678 → 900012345678
    return e;
}

Pos3D Mfr::decode(const EncodedPos3D &e)
{
    return Pos3D{
        static_cast<double>(e.latitude) / SCALE,
        static_cast<double>(e.longitude) / SCALE,
        static_cast<double>(e.altitude)};
}

unsigned long Mfr::toEpochMillis(const std::chrono::system_clock::time_point &tp)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               tp.time_since_epoch())
        .count();
}

std::vector<char> Mfr::serializeDetectionPacket(const std::vector<MfrToLcTargetInfo> &targets, const std::vector<MfrToLcMissileInfo> &missiles)
{
    std::vector<char> buffer;

    buffer.push_back(static_cast<char>(DETECTED_INFO));
    const char *idPtr = reinterpret_cast<const char *>(&mfrId);
    buffer.insert(buffer.end(), idPtr, idPtr + sizeof(mfrId));
    buffer.push_back(static_cast<unsigned char>(targets.size()));
    buffer.push_back(static_cast<unsigned char>(missiles.size()));

    for (const auto &target : targets)
    {
        size_t offset = buffer.size();
        buffer.resize(offset + sizeof(MfrToLcTargetInfo));
        std::memcpy(buffer.data() + offset, &target, sizeof(MfrToLcTargetInfo));
    }

    for (const auto &missile : missiles)
    {
        size_t offset = buffer.size();
        buffer.resize(offset + sizeof(MfrToLcMissileInfo));
        std::memcpy(buffer.data() + offset, &missile, sizeof(MfrToLcMissileInfo));
    }

    return buffer;
}

double Mfr::calcBearing(const Pos3D &mfrCoord, const Pos3D &mockCoord)
{
    double mfrLatitudeDeg = mfrCoord.latitude;
    double mockLatitudeDeg = mockCoord.latitude;

    double mfrLatitudeRad = deg2rad(mfrLatitudeDeg);
    double mockLatitudeRad = deg2rad(mockLatitudeDeg);

    double deltaLongitude = deg2rad(mockCoord.longitude - mfrCoord.longitude);

    double y = std::sin(deltaLongitude) * std::cos(mockLatitudeRad);
    double x = std::cos(mfrLatitudeRad) * std::sin(mockLatitudeRad) - std::sin(mfrLatitudeRad) * std::cos(mockLatitudeRad) * std::cos(deltaLongitude);
    double theta = std::atan2(y, x) * 180.0 / M_PI; // rad → deg

    return std::fmod(theta + 360.0, 360.0);
}

double Mfr::angleDiff(const double &baseAngle, const double &targetAngle)
{
    double diff = targetAngle - baseAngle;

    if (diff > 180.0)
    {
        diff -= 360.0;
    }
    else if (diff < -180.0)
    {
        diff += 360.0;
    }

    return diff;
}

void Mfr::parsingSimData(const std::vector<char> &payload)
{
    if (payload.size() != sizeof(MockSimData) && payload.size() != sizeof(MockSimData))
    {
        // std::cerr << "[Mfr::handleSimDataPayload] SimData 크기 오류. 받은 크기: " << payload.size() << std::endl;
        return;
    }

    MockSimData data;
    std::memcpy(&data, payload.data(), sizeof(TargetSimData));

    localMockSimData localSimData;
    localSimData.mockId = data.mockId;
    localSimData.mockCoords = decode(data.mockCoords);
    localSimData.angle = data.angle;
    localSimData.angle2 = data.angle2;
    localSimData.speed = data.speed;
    localSimData.isHit = data.isHit;

    if (localSimData.mockId >= 104001 && localSimData.mockId <= 104999) // 표적 정보
    {
        Logger::log("Target Detected! ID: " + std::to_string(localSimData.mockId));
        addMockTarget(localSimData);
    }
    else if (localSimData.mockId >= 105001 && localSimData.mockId <= 105999) // 미사일 정보
    {
        Logger::log("Missile Detected! ID: " + std::to_string(localSimData.mockId));
        addMockMissile(localSimData);
    }
    else
    {
        Logger::log("[Mfr::handleSimDataPayload] 알 수 없는 ID 범위: " + std::to_string(localSimData.mockId));
    }
}

void Mfr::sendMfrStatus()
{
    MfrStatus status{};
    status.radarId = mfrId;
    status.radarPos = encode(mfrCoords);
    status.radarMode = mfrMode;
    status.radarAngle = goalMotorAngle;
    // std::cout << "mfrMode: " << mfrMode << std::endl;
    std::vector<char> packet = serializePacketforSend(status);

    packet.insert(packet.begin(), static_cast<char>(STATUS_RES));

    lcCommManager->send(packet);
}

void Mfr::parsingModeChangeData(const std::vector<char> &payload)
{
    if (payload.size() < 5)
    {
        // std::cerr << "[Mfr::parsingModeChangeData] ModeChangeData 크기 오류. 받은 크기: " << payload.size() << std::endl;
        return;
    }

    // std::cout << "[Mfr::parsingModeChangeData] packet size: " << payload.size() << std::endl;

    unsigned int radarId;
    std::memcpy(&radarId, &payload[0], sizeof(radarId));

    unsigned char modeData = payload[4];

    if (!(radarId == mfrId))
    {
        // std::cerr << "[Mfr::parsingModeChangeData] mfrId 미일치. 받은 mfrId: " << radarId << std::endl;
        return;
    }

    unsigned int targetId = 0;
    if (modeData == ROTATION_MODE)
    {
        mfrMode = ROTATION_MODE;
        motorRotationFlag = true;
        // stepMotorManager->runSpeedMode(motorTargetRPM);
        // std::cout << "[Mfr::parsingModeChangeData] 모드 변경: ROTATION_MODE" << std::endl;
    }

    else if (modeData == ANGLE_MODE)
    {
        mfrMode = ANGLE_MODE;
        motorRotationFlag = false;
        // std::cout << "[Mfr::parsingModeChangeData] ANGLE_MODE" << std::endl;
        if (payload.size() < 5 + sizeof(unsigned int) + sizeof(unsigned char))
        {
            // std::cerr << "[Mfr::parsingModeChangeData] targetId 포함인데도 길이가 부족함" << std::endl;
            return;
        }

        unsigned char isPriorityMode = payload[5];
        if (isPriorityMode == PRIORITY_TARGET) // MFR 판단 ID
        {
            // std::cout << "priority  mode 유지" << std::endl;
        }

        else if (isPriorityMode == CUSTOMIZE_TARGET)
        {
            std::memcpy(&targetId, &payload[6], sizeof(unsigned int));
            goalTargetId = targetId;
            // std::cout << "[Mfr::parsingModeChangeData] Priority targetId: " << goalTargetId << std::endl;
        }
        // stepMotorManager->runAngleMode(goalMotorAngle);
    }
}

void Mfr::parsingLcInitData(const std::vector<char> &payload)
{
    LcInitData status{};
    std::memcpy(&status, payload.data(), sizeof(LcInitData));

    lcCoords.latitude = status.lcCoord.longitude;
    lcCoords.longitude = status.lcCoord.latitude;
    lcCoords.altitude = status.lcCoord.altitude;
}

template <typename T>
std::vector<char> Mfr::serializePacketforSend(const T &status)
{
    std::vector<char> buffer(sizeof(T));
    std::memcpy(buffer.data(), &status, sizeof(T));
    return buffer;
}

// shared data
void Mfr::addMockTarget(const localMockSimData &target)
{
    // Logger::log("[Mfr::addMockTarget] Mock Target ID: " + std::to_string(target.mockId) +
    //             ", Latitude: " + std::to_string(target.mockCoords.latitude) +
    //             ", Longitude: " + std::to_string(target.mockCoords.longitude) +
    //             ", Altitude: " + std::to_string(target.mockCoords.altitude) +
    //             ", Angle1: " + std::to_string(target.angle) +
    //             ", Angle2: " + std::to_string(target.angle2) +
    //             ", Speed: " + std::to_string(target.speed) +
    //             ", is Hit?: " + (target.isHit ? "true" : "false"));
    mockTargets[target.mockId] = target;
}

void Mfr::addMockMissile(const localMockSimData &missile)
{
    // std::lock_guard<std::mutex> lock(mockMissileMutex);
    mockMissile[missile.mockId] = missile;
    detectedMissile[missile.mockId] = missile;
}

// public
void Mfr::callBackData(const std::vector<char> &packet)
{
    if (packet.size() < 1)
    {
        // std::cerr << "[Mfr::recvData] 이상 패킷" << std::endl;
        return;
    }

    uint8_t cmd = static_cast<uint8_t>(packet[0]);

    std::vector<char> dataPayload(packet.begin() + 1, packet.end());
    switch (cmd)
    {
    case SIM_MOCK_DATA:
        parsingSimData(dataPayload);
        break;

    case STATUS_REQ:
        sendMfrStatus();
        break;

    case MODE_CHANGE:
        parsingModeChangeData(dataPayload);
        break;

    case LC_INIT_RES:
        parsingLcInitData(dataPayload);
        break;

    default:
        // std::cerr << "[Mfr::recvData] 미정의 명령어 타입: 0x"
        //   << std::hex << static_cast<int>(cmd) << std::dec << std::endl;
        break;
    }
}