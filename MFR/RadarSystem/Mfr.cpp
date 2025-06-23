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

Pos3D Mfr::decode(const EncodedPos3D &e)
{
    return Pos3D{
        static_cast<double>(e.latitude) / SCALE,
        static_cast<double>(e.longitude) / SCALE,
        static_cast<double>(e.altitude)};
}

EncodedPos3D Mfr::encode(const Pos3D &p)
{
    EncodedPos3D e;

    e.latitude = std::llround(p.latitude * SCALE);   // 37.67213612 → 3767213612
    e.longitude = std::llround(p.longitude * SCALE); // 127.23644256 → 12723644256
    e.altitude = std::llround(p.altitude);           // 9000.12345678 → 900012345678
    return e;
}

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

template <typename T>
std::vector<char> Mfr::serializePacketforSend(const T &status)
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

    // std::cout << "[Mfr::handleSimDataPayload] Sim Data 수신 -> ID: " << localSimData.mockId
    //           << ", Latitude: " << localSimData.mockCoords.latitude
    //           << ", Longitude: " << localSimData.mockCoords.longitude
    //           << ", Altitude: " << localSimData.mockCoords.altitude
    //           << ", Angle1: " << localSimData.angle
    //           << ", Angle2: " << localSimData.angle2
    //           << ", Speed: " << localSimData.speed
    //           << ", is Hit?: " << localSimData.isHit
    //           << std::endl;

    if (localSimData.mockId >= 104001 && localSimData.mockId <= 104999) // 표적 정보
    {
        // std::cout << "Target Detected!@!"
        //     << ", ID: " << localSimData.mockId
        //     << std::endl;
        MfrToLcTargetInfo targetInfo;
        targetInfo.id = localSimData.mockId;
        targetInfo.targetCoords = encode(localSimData.mockCoords);
        targetInfo.targetSpeed = localSimData.speed;
        targetInfo.targetAngle = localSimData.angle;
        targetInfo.targetAngle2 = localSimData.angle2;
        targetInfo.firstDetectionTime = 0;
        targetInfo.prioirty = 0; // 기본 우선순위
        targetInfo.isHit = localSimData.isHit;

        addMockTarget(targetInfo);
    }
    else if (localSimData.mockId >= 105001 && localSimData.mockId <= 105999) // 미사일 정보
    {
        MfrToLcMissileInfo missileInfo;
        missileInfo.id = localSimData.mockId;
        missileInfo.missileCoords = encode(localSimData.mockCoords);
        missileInfo.missileSpeed = localSimData.speed;
        missileInfo.missileAngle = localSimData.angle;
        missileInfo.timeToIntercept = 0; // 초기값 설정
        missileInfo.isHit = localSimData.isHit;

        addMockMissile(missileInfo);
    }
}
unsigned long Mfr::toEpochMillis(const std::chrono::system_clock::time_point &tp)
{
    auto duration = tp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

double Mfr::deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

void Mfr::startDetectionAlgoThread()
{
    detectionThreadRunning = true;

    detectionThread = std::thread([this]()
                                  {
        while (detectionThreadRunning) 
        {
            mfrDetectionAlgo();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } });
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
    double theta = std::atan2(y, x) * 180.0 / M_PI; // rad → deg

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

#ifdef BUILD_FOR_PETALINUX
void Mfr::mfrDetectionAlgo()
{
    std::map<unsigned int, localMockSimData> localTargets;
    std::map<unsigned int, localMockSimData> localMissiles;

    localTargets = mockTargets;
    localMissiles = mockMissile;

    std::map<unsigned int, localMockSimData> localDetectedTargets;
    std::map<unsigned int, localMockSimData> localDetectedMissile;

    std::vector<MfrToLcTargetInfo> detectedTargetList;
    std::vector<MfrToLcMissileInfo> detectedMissileList;

    auto now = std::chrono::system_clock::now();
    unsigned long nowMs = toEpochMillis(now);

    if (mfrMode == ROTATION_MODE)
    {
        if (!motorRotationFlag)
        {
            stepMotorManager->sendCommand("ROTATION_MODE");
        }

        std::vector<std::pair<unsigned int, long long>> targetDistances;

        for (const auto &[id, target] : localTargets)
        {
            double distance = calcDistance(mfrCoords, target.mockCoords);

            if (distance <= limitDetectionRange && !target.isHit)
            {
                localDetectedTargets[id] = target;
                targetDistances.emplace_back(id, distance);
                // std::cout << target.speed << std::endl;
            }
            else
            {
                localDetectedTargets.erase(id);
                auto it = std::remove_if(targetDistances.begin(), targetDistances.end(),
                                         [id](const std::pair<int, double> &p)
                                         { return p.first == id; });

                targetDistances.erase(it, targetDistances.end());
            }
        }

        std::sort(targetDistances.begin(), targetDistances.end(), [](const auto &a, const auto &b)
                  { return a.second < b.second; });

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
            status.prioirty = priority++;
            status.isHit = false;

            detectedTargetList.push_back(status);
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

    //
    // ######################################################################################
    //

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
            stepMotorManager->sendCommand("STOP_MODE:" + oss.str());

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
                    status.prioirty = 1;
                    status.isHit = false;

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

    detectedTargets = std::move(localDetectedTargets);
    detectedMissile = std::move(localDetectedMissile);
}

void Mfr::addMockTarget(const localMockSimData &target)
{
    // std::lock_guard<std::mutex> lock(mockTargetMutex);
    mockTargets[target.mockId] = target;
}

//
//  detected 된 데이터에서만 조회
//
localMockSimData *Mfr::getMockTargetById(unsigned int id)
{
    auto it = detectedTargets.find(id);
    return (it != detectedTargets.end()) ? &it->second : nullptr;
}

void Mfr::removeMockTargetById(unsigned int id)
{
    // std::lock_guard<std::mutex> lock(mockTargetMutex);
    mockTargets.erase(id);
    detectedTargets.erase(id);
}

void Mfr::clearMockTargets()
{
    // std::lock_guard<std::mutex> lock(mockTargetMutex);
    mockTargets.clear();
    detectedTargets.clear();
}

void Mfr::addMockMissile(const localMockSimData &missile)
{
    mockMissile[missile.mockId] = missile;
    detectedMissile[missile.mockId] = missile;
}

//
//  detected 된 데이터에서만 조회
//
localMockSimData *Mfr::getMockMissileById(unsigned int id)
{
    auto it = detectedMissile.find(id);
    return (it != detectedMissile.end()) ? &it->second : nullptr;
}

void Mfr::removeMockMissileById(unsigned int id)
{
    mockMissile.erase(id);
    detectedMissile.erase(id);
}

void Mfr::clearMockMissiles()
{
    mockMissile.clear();
    detectedMissile.clear();
}
#else
void Mfr::mfrDetectionAlgo()
{
    std::unordered_map<unsigned int, MfrToLcTargetInfo> localTargets;
    std::unordered_map<unsigned int, MfrToLcMissileInfo> localMissiles;

    localTargets = mockTargets;
    localMissiles = mockMissile;

    std::unordered_map<unsigned int, MfrToLcTargetInfo> localDetectedTargets;
    std::unordered_map<unsigned int, MfrToLcMissileInfo> localDetectedMissile;

    std::vector<MfrToLcTargetInfo> detectedTargetList;
    std::vector<MfrToLcMissileInfo> detectedMissileList;

    if (mfrMode == ROTATION_MODE)
    {
        // if(!motorRotationFlag)
        // {
        //     stepMotorManager->sendCommand("ROTATION_MODE");
        // }

        std::vector<std::pair<unsigned int, long long>> targetDistances;

        for (const auto &[id, target] : localTargets)
        {
            double distance = calcDistance(mfrCoords, decode(target.targetCoords));

            if (distance <= limitDetectionRange && !target.isHit)
            {
                std::cout << "target hit?:" << target.isHit << std::endl;
                localDetectedTargets[id] = target;
                targetDistances.emplace_back(id, distance);
            }
            else
            {
                localDetectedTargets.erase(id);
                auto it = std::remove_if(targetDistances.begin(), targetDistances.end(),
                                         [id](const std::pair<int, double> &p)
                                         { return p.first == id; });

                targetDistances.erase(it, targetDistances.end());
            }
        }

        std::sort(targetDistances.begin(), targetDistances.end(), [](const auto &a, const auto &b)
                  { return a.second < b.second; });

        unsigned char priority = 1;
        for (const auto &[id, distance] : targetDistances)
        {
            const auto &target = localDetectedTargets[id];

            if (priority == 1)
            {
                goalTargetId = id;
            }

            detectedTargetList.push_back(target);
        }

        for (const auto &[id, missile] : localMissiles)
        {
            long long distance = calcDistance(mfrCoords, decode(missile.missileCoords));

            if (distance <= limitDetectionRange)
            {
                //std::cout << "missile hit?:" << missile.isHit << std::endl;
                localDetectedMissile[id] = missile;
                detectedMissileList.push_back(missile);
            }
            else
            {
                localDetectedTargets.erase(id);
                auto it = std::remove_if(targetDistances.begin(), targetDistances.end(),
                                         [id](const std::pair<int, double> &p)
                                         { return p.first == id; });

                targetDistances.erase(it, targetDistances.end());
            }
        }
    }

    //
    // ######################################################################################
    //

    else if (mfrMode == ANGLE_MODE)
    {
        std::cout << "mfrMode: ANGLE_MODE" << std::endl;
        detectedTargetList.clear();
        detectedMissileList.clear();
        std::cout << "goalTargetId: " << goalTargetId << std::endl;
        if (localTargets.find(goalTargetId) != localTargets.end())
        {
            std::cout << "[Mfr::mfrDetectionAlgo] 목표 표적 ID: " << goalTargetId << std::endl;
            const auto &goalTarget = localTargets[goalTargetId];
            auto decode_coords = decode(goalTarget.targetCoords);
            double baseAz = calcBearing(mfrCoords, decode_coords);

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << baseAz;
            // stepMotorManager->sendCommand("STOP_MODE:" + oss.str());

            for (const auto &[id, target] : localTargets)
            {
                long long distance = calcDistance(mfrCoords, decode_coords);
                if (distance > limitDetectionRange)
                {
                    continue;
                }

                double az = calcBearing(mfrCoords, decode_coords);
                double diff = angleDiff(baseAz, az);

                if (std::abs(diff) <= 15.0)
                {
                    localDetectedTargets[id] = target;
                    detectedTargetList.push_back(target);
                }
            }

            for (const auto &[id, missile] : localMissiles)
            {
                auto missileCoords = decode(missile.missileCoords);
                long long distance = calcDistance(mfrCoords, missileCoords);
                if (distance > limitDetectionRange)
                {
                    continue;
                }

                double az = calcBearing(mfrCoords, missileCoords);
                double diff = angleDiff(baseAz, az);

                if (std::abs(diff) <= 15.0)
                {
                    localDetectedMissile[id] = missile;
                    detectedMissileList.push_back(missile);
                }
            }
        }
    }

    if (!detectedTargetList.empty() || !detectedMissileList.empty())
    {
        std::vector<char> packet = serializeDetectionPacket(detectedTargetList, detectedMissileList);
        for (const auto &target : detectedTargetList)
        {
            // Convert epoch milliseconds to time_t
            time_t time = target.firstDetectionTime / 1000;
            // Convert to local time
            struct tm *ltm = localtime(&time);

            char timeStr[32];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", ltm);

            // Print milliseconds part
            int ms = target.firstDetectionTime % 1000;

            // std::cout << "send Mock Target  Time ulong: " << target.firstDetectionTime << "!!!!!!!!!!!!!!!" << std::endl;
            // std::cout << "Detected Target ID: " << target.id
            //           << " First Detection Time: " << timeStr
            //           << "." << std::setfill('0') << std::setw(3) << ms
            //           << std::endl;
        }
        lcCommManager->send(packet);
    }

    detectedTargets = std::move(localDetectedTargets);
    detectedMissile = std::move(localDetectedMissile);
}
//
//  detected 된 데이터에서만 조회
//
void Mfr::addMockTarget(const MfrToLcTargetInfo &target)
{
    std::unique_lock<std::shared_mutex> lock(mockTargetMutex);

    MfrToLcTargetInfo tmpTarget = target;

    // Add first detection time only for new targets
    if (mockTargets.find(target.id) == mockTargets.end())
    {
        auto now = std::chrono::system_clock::now();
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();

        // Set first detection time in milliseconds since epoch
        tmpTarget.firstDetectionTime = static_cast<uint64_t>(nowMs);
    } else {
        // If the target already exists, keep the existing first detection time
        tmpTarget.firstDetectionTime = mockTargets[target.id].firstDetectionTime;
    }

    //std::cout << "Real First Mock Target : " << tmpTarget.firstDetectionTime << "!!!!!!!!!!!!!!!" << std::endl;
    mockTargets[target.id] = tmpTarget;
}

//
//  detected 된 데이터에서만 조회
//
MfrToLcTargetInfo *Mfr::getMockTargetById(unsigned int id)
{
    std::shared_lock<std::shared_mutex> lock(detectedTargetMutex);
    auto it = detectedTargets.find(id);
    return (it != detectedTargets.end()) ? &it->second : nullptr;
}

void Mfr::removeMockTargetById(unsigned int id)
{
    {
        std::unique_lock<std::shared_mutex> lock(mockTargetMutex);
        mockTargets.erase(id);
    }

    {
        std::unique_lock<std::shared_mutex> lock(detectedTargetMutex);
        detectedTargets.erase(id);
    }
}

void Mfr::clearMockTargets()
{
    {
        std::unique_lock<std::shared_mutex> lock(mockTargetMutex);
        mockTargets.clear();
    }

    {
        std::unique_lock<std::shared_mutex> lock(detectedTargetMutex);
        detectedTargets.clear();
    }
}

void Mfr::addMockMissile(const MfrToLcMissileInfo &missile)
{
    {
        std::unique_lock<std::shared_mutex> lock(mockMissileMutex);
        mockMissile[missile.id] = missile;
    }

    {
        std::unique_lock<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile[missile.id] = missile;
    }
}

//
//  detected 된 데이터에서만 조회
//
MfrToLcMissileInfo *Mfr::getMockMissileById(unsigned int id)
{
    std::shared_lock<std::shared_mutex> lock(detectedMissileMutex);
    auto it = detectedMissile.find(id);
    return (it != detectedMissile.end()) ? &it->second : nullptr;
}

void Mfr::removeMockMissileById(unsigned int id)
{
    {
        std::unique_lock<std::shared_mutex> lock(mockMissileMutex);
        mockMissile.erase(id);
    }

    {
        std::unique_lock<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile.erase(id);
    }
}

void Mfr::clearMockMissiles()
{
    {
        std::unique_lock<std::shared_mutex> lock(mockMissileMutex);
        mockMissile.clear();
    }

    {
        std::unique_lock<std::shared_mutex> lock(detectedMissileMutex);
        detectedMissile.clear();
    }
}
#endif