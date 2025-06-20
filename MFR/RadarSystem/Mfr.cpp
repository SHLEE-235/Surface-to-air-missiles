#include "Mfr.hpp"

Mfr::Mfr() : goalTargetId(101001), mfrMode(ROTATION_MODE), goalMotorAngle(135.0)
{
    stepMotorManager = new StepMotorController();
    lcCommManager = new MfrLcCommManager(this);
    simCommManager = new MfrSimCommManager(this);

    startDetectionAlgoThread();
    requestLcInitData();
}

Mfr::~Mfr()
{
    delete stepMotorManager;
    delete lcCommManager;
    delete simCommManager;
}

Pos3D Mfr::decode(const EncodedPos3D& e) 
{                
    return Pos3D
    {
        static_cast<double>(e.latitude)  / SCALE,
        static_cast<double>(e.longitude) / SCALE,
        static_cast<double>(e.altitude)
    };
}

EncodedPos3D Mfr::encode(const Pos3D& p) 
{
    EncodedPos3D e;
    
    e.latitude  = std::llround(p.latitude  * SCALE);  // 37.67213612 → 3767213612
    e.longitude = std::llround(p.longitude * SCALE);  // 127.23644256 → 12723644256
    e.altitude  = std::llround(p.altitude);  // 9000.12345678 → 900012345678
    return e;
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
        case SIM_MOCK_DATA:
            parsingSimData(dataPayload);
            break;

        case STATUS_REQ:
            // std::cout << "[Mfr::recvData] 상태 요청 명령 수신됨" << std::endl;
            sendMfrStatus();
            // std::cout << "[Mfr::recvData] 상태 요청 명령 전송됨" << std::endl;
            break;

        case MODE_CHANGE:
            std::cout << "[Mfr::recvData] 모드 변경 명령 수신됨" << std::endl; 
            parsingModeChangeData(dataPayload);
            break;

        case LC_INIT_RES:
            // std::cout << "[Mfr::recvData] LC 최초 정보 수신" << std::endl;
            parsingLcInitData(dataPayload);
            break;

        default:
            std::cerr << "[Mfr::recvData] 미정의 명령어 타입: 0x"
                      << std::hex << static_cast<int>(cmd) << std::dec << std::endl;
            break;
    }
}

template <typename T>
std::vector<char>Mfr::serializePacketforSend(const T& status)
{
    std::vector<char> buffer(sizeof(T));
    std::memcpy(buffer.data(), &status, sizeof(T));
    return buffer;
}

void Mfr::requestLcInitData()
{
    ReqLcInitData reqData{};
    reqData.radarId = mfrId;

    std::vector<char> packet = serializePacketforSend(reqData);
    
    packet.insert(packet.begin(), static_cast<char>(LC_INIT_REQ));

    lcCommManager->send(packet);
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

std::vector<char> Mfr::serializeDetectionPacket(const std::vector<MfrToLcTargetInfo>& targets, const std::vector<MfrToLcMissileInfo>& missiles)
{
    std::vector<char> buffer;

    buffer.push_back(static_cast<char>(DETECTED_INFO));
    const char* idPtr = reinterpret_cast<const char*>(&mfrId);
    buffer.insert(buffer.end(), idPtr, idPtr + sizeof(mfrId));
    buffer.push_back(static_cast<unsigned char>(targets.size()));
    buffer.push_back(static_cast<unsigned char>(missiles.size()));

    for (const auto& target : targets)
    {
        size_t offset = buffer.size();
        buffer.resize(offset + sizeof(MfrToLcTargetInfo));
        std::memcpy(buffer.data() + offset, &target, sizeof(MfrToLcTargetInfo));
    }

    for (const auto& missile : missiles)
    {
        size_t offset = buffer.size();
        buffer.resize(offset + sizeof(MfrToLcMissileInfo));
        std::memcpy(buffer.data() + offset, &missile, sizeof(MfrToLcMissileInfo));
    }

    return buffer;
}

void Mfr::parsingModeChangeData(const std::vector<char>& payload) 
{
    if (payload.size() < 5)
    {
        std::cerr << "[Mfr::parsingModeChangeData] ModeChangeData 크기 오류. 받은 크기: " << payload.size() << std::endl;
        return;
    }

    std::cout << "[Mfr::parsingModeChangeData] packet size: " << payload.size() << std::endl;

    unsigned int radarId;
    std::memcpy(&radarId, &payload[0], sizeof(radarId));

    unsigned char modeData = payload[4];    

    if(!(radarId == mfrId))
    {
        std::cerr << "[Mfr::parsingModeChangeData] mfrId 미일치. 받은 mfrId: " << radarId << std::endl;
        return;
    }

    unsigned int targetId = 0;
    if(modeData == ROTATION_MODE)
    {
        mfrMode = ROTATION_MODE;
        motorRotationFlag = true;
        // stepMotorManager->runSpeedMode(motorTargetRPM);
        std::cout << "[Mfr::parsingModeChangeData] 모드 변경: ROTATION_MODE" << std::endl;
    }

    else if(modeData == ANGLE_MODE)
    {
        mfrMode = ANGLE_MODE;
        motorRotationFlag = false;
        std::cout << "[Mfr::parsingModeChangeData] ANGLE_MODE" << std::endl;
        if (payload.size() < 5 + sizeof(unsigned int) + sizeof(unsigned char))
        {
            std::cerr << "[Mfr::parsingModeChangeData] targetId 포함인데도 길이가 부족함" << std::endl;
            return;
        }

        unsigned char isPriorityMode = payload[5];
        if(isPriorityMode == PRIORITY_TARGET)       // MFR 판단 ID
        { 
            std::cout << "priority  mode 유지" << std::endl;
        }

        else if(isPriorityMode == CUSTOMIZE_TARGET)
        {
            std::memcpy(&targetId, &payload[6], sizeof(unsigned int));            
            goalTargetId = targetId;
            std::cout << "[Mfr::parsingModeChangeData] Priority targetId: " << goalTargetId << std::endl;
        }
        // stepMotorManager->runAngleMode(goalMotorAngle);        
    }
}

void Mfr::parsingLcInitData(const std::vector<char>& payload)
{
    LcInitData status{};
    std::memcpy(&status, payload.data(), sizeof(LcInitData));

    lcCoords.latitude = status.lcCoord.longitude;
    lcCoords.longitude = status.lcCoord.latitude;
    lcCoords.altitude = status.lcCoord.altitude;
}

void Mfr::parsingSimData(const std::vector<char>& payload)
{
    if (payload.size() != sizeof(MockSimData) && payload.size() != sizeof(MockSimData)) 
    {
        std::cerr << "[Mfr::handleSimDataPayload] SimData 크기 오류. 받은 크기: " << payload.size() << std::endl;
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
    
    // std::cout << "[Mfr::handleSimDataPayload] Sim Data 수신 -> ID: " << localSimData.mockId
    //           << ", Latitude: " << localSimData.mockCoords.latitude
    //           << ", Longitude: " << localSimData.mockCoords.longitude
    //           << ", Altitude: " << localSimData.mockCoords.altitude
    //           << ", Angle1: " << localSimData.angle
    //           << ", Angle2: " << localSimData.angle2
    //           << ", Speed: " << localSimData.speed 
    //           << ", is Hit?: " << localSimData.isHit
    //           << std::endl;

    if (localSimData.mockId >= 104001 && localSimData.mockId <= 104999)         // 표적 정보
    {
        // std::cout << "Target Detected!@!" 
        //     << ", ID: " << localSimData.mockId        
        //     << std::endl;
            
        addMockTarget(localSimData);
    }

    else if (localSimData.mockId >= 105001 && localSimData.mockId <= 105999)    // 미사일 정보
    {
        addMockMissile(localSimData);
    }

    else 
    {
        std::cerr << "[Mfr::handleSimDataPayload] 알 수 없는 ID 범위: " << localSimData.mockId << std::endl;
    }
}

uint64_t Mfr::toEpochMillis(std::chrono::system_clock::time_point tp)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

double Mfr::deg2rad(double deg)
{ 
    return deg * M_PI / 180.0; 
}

void Mfr::startDetectionAlgoThread()
{
    detectionThreadRunning = true;

    detectionThread = std::thread([this]() {
        while (detectionThreadRunning) 
        {
            mfrDetectionAlgo();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}

void Mfr::stopDetectionAlgoThread()
{
    detectionThreadRunning = false;
    if (detectionThread.joinable())
    {
        detectionThread.join();
    }
}

double Mfr::calcDistance(Pos3D mfrCoord, Pos3D mockCoord)
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

double Mfr::calcBearing(Pos3D mfrCoord, Pos3D mockCoord)
{
    double mfrLatitudeDeg = mfrCoord.latitude;
    double mockLatitudeDeg = mockCoord.latitude;

    double mfrLatitudeRad = deg2rad(mfrLatitudeDeg);
    double mockLatitudeRad = deg2rad(mockLatitudeDeg);

    double deltaLongitude = deg2rad(mockCoord.longitude - mfrCoord.longitude);

    double y = std::sin(deltaLongitude) * std::cos(mockLatitudeRad);
    double x = std::cos(mfrLatitudeRad) * std::sin(mockLatitudeRad) - std::sin(mfrLatitudeRad) * std::cos(mockLatitudeRad) * std::cos(deltaLongitude);
    double theta = std::atan2(y, x) * 180.0/M_PI;  // rad → deg

    return std::fmod(theta + 360.0, 360.0);
}

double Mfr::angleDiff(double baseAngle, double targetAngle)
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

void Mfr::mfrDetectionAlgo()
{
    std::unordered_map<unsigned int, localMockSimData> localTargets;
    std::unordered_map<unsigned int, localMockSimData> localMissiles;

    {
        std::lock_guard<std::shared_mutex> lock(mockTargetMutex);
        localTargets = mockTargets;
    }

    {
        std::lock_guard<std::shared_mutex> lock(mockMissileMutex);
        localMissiles = mockMissile;
    }

    std::unordered_map<unsigned int, localMockSimData> localDetectedTargets;
    std::unordered_map<unsigned int, localMockSimData> localDetectedMissile;

    std::vector<MfrToLcTargetInfo> detectedTargetList;
    std::vector<MfrToLcMissileInfo> detectedMissileList;

    auto now = std::chrono::system_clock::now();
    uint64_t nowMs = toEpochMillis(now);
    
    if (mfrMode == ROTATION_MODE)
    {
        // if(!motorRotationFlag)
        // {
        //     stepMotorManager->sendCommand("ROTATION_MODE");
        // }        

        std::vector<std::pair<unsigned int, long long>> targetDistances;

        for (const auto& [id, target] : localTargets)
        {
            double distance = calcDistance(mfrCoords, target.mockCoords);
            
            if (distance <= limitDetectionRange && !target.isHit)
            {
                localDetectedTargets[id] = target;
                targetDistances.emplace_back(id, distance);
            }
            else
            {
                localDetectedTargets.erase(id);
                auto it = std::remove_if(targetDistances.begin(), targetDistances.end(),
                    [id](const std::pair<int, double>& p) { return p.first == id; });

                targetDistances.erase(it, targetDistances.end());
            }
        }
        
        std::sort(targetDistances.begin(), targetDistances.end(), [](const auto& a, const auto& b) 
        { 
            return a.second < b.second;
        });

        unsigned char priority = 1;
        for (const auto& [id, distance] : targetDistances)
        {
            const auto& target = localDetectedTargets[id];

            if(priority == 1)
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
        }

        for (const auto& [id, missile] : localMissiles)
        {
            long long distance = calcDistance(mfrCoords, missile.mockCoords);
            std::cout << "Missile Num: " << localMissiles.size() << std::endl;
            std::cout << "Missile Distance: " << distance << std::endl;

            // std::cout << "Missile ID: " << missile.mockId
            // << ", Missile Coords: " << missile.mockCoords.altitude << ", " << missile.mockCoords.latitude <<", " << missile.mockCoords.longitude
            // << std::endl;

            if (distance <= limitDetectionRange && !missile.isHit)
            {
                std::cout << "missile detected " << std::endl;
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
                std::cout << "missile undetected " << std::endl;
                localDetectedMissile.erase(id);
                detectedMissile.erase(id);
            }
        }
    }

    //
    // ######################################################################################
    //

    else if (mfrMode == ANGLE_MODE)
    {
        std::cout << "mfrMode: ANGLE_MODE" << std::endl;
        //detectedTargetList.clear();
        //detectedMissileList.clear();
        std::cout << "goalTargetId: " << goalTargetId << std::endl;
        if (localTargets.find(goalTargetId) != localTargets.end())
        {
            std::cout << "[Mfr::mfrDetectionAlgo] 목표 표적 ID: " << goalTargetId << std::endl;
            const auto& goalTarget = localTargets[goalTargetId];
            double baseAz = calcBearing(mfrCoords, goalTarget.mockCoords);
            
            // std::ostringstream oss;
            // oss << std::fixed << std::setprecision(2) << baseAz;
            // stepMotorManager->sendCommand("STOP_MODE:" + oss.str());

            for (const auto& [id, target] : localTargets)
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
                    status.isHit = false;

                    detectedTargetList.push_back(status);
                }
            }

            for (const auto& [id, missile] : localMissiles)
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
                    status.isHit = false;
                    
                    detectedMissileList.push_back(status);
                }
            }
        }
    }
    
    if (!detectedTargetList.empty() || !detectedMissileList.empty())
    {
        std::vector<char> packet = serializeDetectionPacket(detectedTargetList, detectedMissileList);       
        lcCommManager->send(packet);
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedTargetMutex);
        detectedTargets = std::move(localDetectedTargets);
    }    

    {
        std::lock_guard<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile = std::move(localDetectedMissile);
    }

}

void Mfr::addMockTarget(const localMockSimData& target) 
{
    std::lock_guard<std::shared_mutex> lock(mockTargetMutex);
    mockTargets[target.mockId] = target;
}

//
//  detected 된 데이터에서만 조회
//
localMockSimData* Mfr::getMockTargetById(unsigned int id) 
{
    std::lock_guard<std::shared_mutex> lock(detectedTargetMutex);
    auto it = detectedTargets.find(id);
    return (it != detectedTargets.end()) ? &it->second : nullptr;
}

void Mfr::removeMockTargetById(unsigned int id) 
{
    {
        std::lock_guard<std::shared_mutex> lock(mockTargetMutex);
        mockTargets.erase(id);
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedTargetMutex);
        detectedTargets.erase(id);
    }
}

void Mfr::clearMockTargets() 
{
    {
        std::lock_guard<std::shared_mutex> lock(mockTargetMutex);
        mockTargets.clear();
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedTargetMutex);
        detectedTargets.clear();
    }
}

void Mfr::addMockMissile(const localMockSimData& missile) 
{
    {      
        std::lock_guard<std::shared_mutex> lock(mockMissileMutex);
        mockMissile[missile.mockId] = missile;
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile[missile.mockId] = missile;
    }
}

//
//  detected 된 데이터에서만 조회
//
localMockSimData* Mfr::getMockMissileById(unsigned int id)
{
    std::lock_guard<std::shared_mutex> lock(detectedMissileMutex);
    auto it = detectedMissile.find(id);
    return (it != detectedMissile.end()) ? &it->second : nullptr;
}

void Mfr::removeMockMissileById(unsigned int id) 
{
    {
        std::lock_guard<std::shared_mutex> lock(mockMissileMutex);
        mockMissile.erase(id);
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile.erase(id);
    }
}

void Mfr::clearMockMissiles() 
{
    {
        std::lock_guard<std::shared_mutex> lock(mockMissileMutex);
        mockMissile.clear();
    }

    {
        std::lock_guard<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile.clear();
    }
}