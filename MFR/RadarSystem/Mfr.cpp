#include "Mfr.hpp"
#include <thread>

Mfr::Mfr()
    : mfrId(1), mfrX(10), mfrY(10), mfrMode(0), targetMotorAngle(0.0),
      lcCommManager(this), 
      simCommManager(this)
{}

void Mfr::startUdp()
{
    std::cout << "[Mfr::startUdp] UDP 통신 수신 시작" << "\n";
    if (simCommManager.connectToSim()) 
    {
        simCommManager.startUdpReceiver();
    }
}

void Mfr::startTcp()
{
    std::cout << "[Mfr::startTcp] TCP 통신 수신 시작" << "\n";
    if (lcCommManager.connectToLc()) 
    {
        lcCommManager.startTcpReceiver();
    }
}

//
//  표적 탐지 알고리즘 관련
//
std::vector<MockTarget*> Mfr::MfrDetectionAlgo()
{
    const double detectionRange = 100.0;
    std::vector<MockTarget*> detectedTargets;
    
    for (auto& target : mockTargets) 
    {
        long long dx = target.targetX- mfrX;
        long long dy = target.targetY - mfrY;
        double distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance <= detectionRange) 
        {
            detectedTargets.push_back(&target);
        }
    }
    
    return detectedTargets;
}

void Mfr::callBackData(const std::vector<char>& packet)
{
    if (packet.size() < 1) 
    {
        std::cerr << "[Mfr::recvData] 이상 패킷" << std::endl;
        return;
    }

    uint8_t cmd = static_cast<uint8_t>(packet[0]);

    std::vector<char> dataPayload(packet.begin() + 1, packet.end());

    switch (cmd) 
    {
        case SIM_TARGET_DATA:
            handleSimDataPayload(dataPayload);
            break;

        case SIM_MISSILE_DATA:
            handleSimDataPayload(dataPayload);
            break;

        case STATUS_REQ:
            std::cout << "[Mfr::recvData] 상태 요청 명령 수신됨" << std::endl; 
            //sendToLc(mfrStatus);           
            break;
        case MODE_CHANGE:
            controlMotor(dataPayload);
            break;

        default:
            std::cerr << "[Mfr::recvData] 미정의 명령어 타입: 0x"
                      << std::hex << static_cast<int>(cmd) << std::dec << std::endl;
            break;
    }
}


//
//  모의 객체 관리 관련
//
void Mfr::handleSimDataPayload(const std::vector<char>& payload)
{
    if (payload.size() != sizeof(SimData)) 
    {
        std::cerr << "[Mfr::handleSimDataPayload] SimData 크기 오류. 받은 크기: " << payload.size() << std::endl;
        return;
    }
    
    SimData data;
    std::memcpy(&data, payload.data(), sizeof(SimData));

    std::cout << "[Mfr::handleSimDataPayload] SimData 수신 → ID: " << data.mockId
              << ", X: " << data.x
              << ", Y: " << data.y
              << ", Z: " << data.z
              << ", Angle: " << data.angle
              << ", Speed: " << data.speed << std::endl;

    if (data.mockId >= 101001 && data.mockId <= 101999) 
    {
        MockTarget target{data.mockId, data.x, data.y, static_cast<float>(data.angle), data.speed};
        addMockTarget(target);
        std::cout << "[Mfr::handleSimDataPayload] → 모의 표적 등록 완료" << std::endl;
    }
    else if (data.mockId >= 102001 && data.mockId <= 102999) 
    {
        MockMissile missile{data.mockId, data.x, data.y, data.z, static_cast<float>(data.angle), data.speed};
        addMockMissile(missile);
        std::cout << "[Mfr::handleSimDataPayload] → 모의 미사일 등록 완료" << std::endl;
    }
    else 
    {
        std::cerr << "[Mfr::handleSimDataPayload] 알 수 없는 ID 범위: " << data.mockId << std::endl;
    }
}

void Mfr::addMockTarget(const MockTarget& target) 
{
    mockTargets.push_back(target);
}

MockTarget* Mfr::getMockTargetById(unsigned int id) 
{
    for (auto& t : mockTargets) 
    {
        if (t.id == id)
        {
            return &t;
        }
    }
    return nullptr;
}

void Mfr::removeMockTargetById(unsigned int id) 
{
    mockTargets.erase(
        std::remove_if(mockTargets.begin(), mockTargets.end(),
            [id](const MockTarget& t) {
                 return t.id == id; 
                }),
        mockTargets.end());
}

void Mfr::clearMockTargets() 
{
    mockTargets.clear();
}

void Mfr::addMockMissile(const MockMissile& missile) 
{
    mockMissile.push_back(missile);
}

MockMissile* Mfr::getMockMissileById(unsigned int id)
{
    for (auto& m : mockMissile) 
    {
        if (m.id == id)
        {
            return &m;
        }
    }
    return nullptr;
}

void Mfr::removeMockMissileById(unsigned int id) 
{
    mockMissile.erase(
        std::remove_if(mockMissile.begin(), mockMissile.end(),
            [id](const MockMissile& m) {
                 return m.id == id;
                }),
        mockMissile.end());
}

void Mfr::clearMockMissiles() 
{
    mockMissile.clear();
}

//
//  Radar 관리 데이터 관련
//
void Mfr::manageMfrStatus()
{
    mfrStatus.radarId = mfrId;
    mfrStatus.radarPos.x = mfrX;
    mfrStatus.radarPos.y = mfrY;
    mfrStatus.radarMode = static_cast<RadarMode>(mfrMode);
    mfrStatus.radarAngle = targetMotorAngle;
}

//
//  모터 제어 관련
//
void Mfr::controlMotor(const std::vector<char>& payload) 
{

    auto [mode, value] = motorDataParser(payload);

    switch(mode) 
    {
        case ROTATION_MODE:
            stepMotorManager.runSpeedMode(static_cast<int>(value));
            break;
        break;
        case ANGLE_MODE:
            stepMotorManager.runAngleMode(value);
            break;
        default:
           break;
    }
}

std::pair<uint8_t, float> Mfr::motorDataParser(const std::vector<char>& payload)
{
    uint8_t mode = static_cast<uint8_t>(payload[0]);

    double value = 0.0f;
    std::memcpy(&value, &payload[1], sizeof(float));

    return { mode, value };
}


std::vector<char> Mfr::serializeMfrStatus(const MfrStatus& status)
{
    std::vector<char> buffer(sizeof(MfrStatus));
    std::memcpy(buffer.data(), &status, sizeof(MfrStatus));
    return buffer;
}

void Mfr::startMfrStatusSender()
{
    std::thread([this]() {
        while (true)
        {
            MfrStatus status{};
            status.radarId = 1;
            status.radarPos = { 100, 200 };
            status.radarMode = RadarMode::ROTATION_MODE;
            status.radarAngle = 45.0f;

            std::vector<char> packet = serializeMfrStatus(status);
            lcCommManager.send(packet);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
}