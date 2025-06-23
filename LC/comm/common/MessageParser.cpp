#include "MessageParser.h"
#include "SystemStatus.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <iomanip>
#include <arpa/inet.h>

namespace {

using namespace Common;

unsigned long be64toh(const uint8_t* data) {
    unsigned long val = 0;
    for (int i = 0; i < 8; ++i)
        val = (val << 8) | data[i];
    return val;
}

// 4바이트 빅엔디안 → uint32_t
uint32_t be32toh(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8)  |
           (static_cast<uint32_t>(data[3]));
}

CommonMessage parsePositionRequest(const std::vector<uint8_t>& data, SenderType sender) {
    CommonMessage msg;

    // 최소 1바이트(commandType) 확인
    if (data.size() < 1) {
        msg.ok = false;
        return msg;
    }

    msg.sender = sender;
    msg.commandType = CommandType::POSITION_REQUEST_MFR_TO_LC;
    msg.payload = std::monostate{};  // payload 없음
    msg.ok = true;

    return msg;
}

CommonMessage parseRadarCommand(const std::vector<uint8_t>& data, CommonMessage& msg) {
    RadarModeCommand rc;

    if (data.size() < 11) {
        msg.ok = false;
        return msg;
    }

    std::memcpy(&rc.radarId, &data[1], 4);             // [1~4]
    rc.radarMode = data[5];                            // [5]
    rc.flag = data[6];                                 // [6] → priority_select와 동일한 역할
    std::memcpy(&rc.targetId, &data[7], 4);            // [7~10]
    rc.priority_select = rc.flag;                      // 이름 통일용 (구조체 유지 목적)

    std::cout << "[RadarCommand] radarId (mfrId): " << rc.radarId << "\n";
    std::cout << "[RadarCommand] radarMode: " << static_cast<int>(rc.radarMode) << "\n";
    std::cout << "[RadarCommand] priority_select: " << static_cast<int>(rc.priority_select) << "\n";
    std::cout << "[RadarCommand] targetId: " << rc.targetId << "\n";

    msg.payload = rc;
    msg.commandType = static_cast<CommandType>(data[0]);
    msg.ok = true;
    return msg;
}

CommonMessage parseLSStatus(const std::vector<uint8_t>& data, CommonMessage& msg) {
    // 최소 길이 검증
    if (data.size() < 42) {  // 1 + 4 + 8 + 8 + 8 + 8 + 4 + 1
        msg.ok = false;
        return msg;
    }

    LSReport ls;

    // 리틀엔디안 시스템이라면 memcpy로 그대로 복사하면 됨
    std::memcpy(&ls.lsId,   &data[1],  sizeof(uint32_t));
    std::memcpy(&ls.posX,   &data[5],  sizeof(int64_t));
    std::memcpy(&ls.posY,   &data[13], sizeof(int64_t));
    std::memcpy(&ls.height, &data[21], sizeof(int64_t));

    unsigned long angleBits;
    std::memcpy(&angleBits, &data[29], sizeof(unsigned long));
    std::memcpy(&ls.launchAngle, &angleBits, sizeof(double));

    std::memcpy(&ls.speed, &data[37], sizeof(uint32_t));

    ls.mode = static_cast<OperationMode>(data[41]);

    // 구성
    msg.payload = ls;
    msg.commandType = static_cast<CommandType>(data[0]);
    msg.ok = true;
    return msg;
}

CommonMessage parseLauncherCommand(const std::vector<uint8_t>& data, CommonMessage& msg) {
    if (data.size() < 6) { // cmd(1) + lsId(4) + lsMode(1)
        msg.ok = false;
        return msg;
    }

    Common::LauncherCommand lc;
    msg.commandType = static_cast<CommandType>(data[0]);  

    std::memcpy(&lc.lsId, &data[1], 4);     // 4바이트 launcherId
    lc.lsMode = data[5];                    // 1바이트 mode

    // ✅ 로그 출력
    std::cout << "[LauncherCommand] lsId: " << lc.lsId
              << ", lsMode: " << static_cast<int>(lc.lsMode) << "\n";

    msg.payload = lc;
    msg.ok = true;
    return msg;
}

CommonMessage parseFireCommand(const std::vector<uint8_t>& data, CommonMessage& msg) {
    if (data.size() < 8) { // 1 (commandType) + 4 (lsId) + 4 (targetId) = 최소 9이지만 targetId는 중첩된 걸로 4만 계산
        msg.ok = false;
        return msg;
    }

    FireCommand fc;
    std::memcpy(&fc.lsId, &data[1], 4);
    std::memcpy(&fc.targetId, &data[5], 4);

    std::cout << "[FireCommand] lsId: " << fc.lsId << "\n";
    std::cout << "[FireCommand] targetId: " << fc.targetId << "\n";

    msg.payload = fc;
    msg.commandType = static_cast<CommandType>(data[0]);
    msg.ok = true;
    return msg;
}

//0x05 -> 0x31
CommonMessage parseMoveCommand(const std::vector<uint8_t>& data, CommonMessage& msg) {
    if (data.size() < 18) {
        msg.ok = false;
        return msg;
    }

    MoveCommand mv;
    std::memcpy(&mv.lsId, &data[1], 4);         // ✅ 4바이트 복사
    std::memcpy(&mv.posX, &data[5], 8);         // ✅ X 위치
    std::memcpy(&mv.posY, &data[13], 8);        // ✅ Y 위치

    msg.payload = mv;
    msg.commandType = static_cast<CommandType>(data[0]);
    msg.ok = true;
    std::cout << std::dec;
    std::cout << "발사대 이동 명령 송신\n" ;
    std::cout << "lsId: " << mv.lsId << ", posX: " << mv.posX << ", posY: " << mv.posY << "\n";

    return msg;
}

CommonMessage parseRadarStatus(const std::vector<uint8_t>& data, SenderType sender) {
    CommonMessage msg;
    if (data.size() < 37) { // 4+8+8+8+1+8+4+4 = 최소 45, 넉넉히 잡아도 37 이상
        msg.ok = false;
        return msg;
    }

    msg.sender = sender;
    msg.commandType = CommandType::STATUS_RESPONSE_MFR_TO_LC;

    RadarStatus rs;
    std::memcpy(&rs.radarId,       &data[1], 4);
    std::memcpy(&rs.posX,          &data[5], 8);
    std::memcpy(&rs.posY,          &data[13], 8);
    std::memcpy(&rs.height,        &data[21], 8);  
    rs.radarMode     = data[29];
    std::memcpy(&rs.radarAngle,    &data[30], 8);

    msg.payload = rs;
    msg.ok = true;
    return msg;
}

//0x22
CommonMessage parseRadarDetection(const std::vector<uint8_t>& data, SenderType sender) {
    CommonMessage msg;
    msg.sender = sender;
    msg.commandType = CommandType::DETECTION_MFR_TO_LC;
    msg.ok = true;

    RadarDetection det;

    size_t offset = 1; // CommandType 뒤부터 시작

    // ✅ radarId: 4바이트
    std::memcpy(&det.radarId, &data[offset], 4);
    offset += 4;

    // ✅ numTargets: 1바이트, numMissiles: 1바이트
    uint8_t numTargets = data[offset++];
    uint8_t numMissiles = data[offset++];

    // std::cout << "[Parser] CommandType: " << static_cast<int>(msg.commandType) << "\n";
    // std::cout << "[Parser] Radar ID: " << det.radarId << "\n";
    // std::cout << "[Parser] Number of Targets: " << static_cast<int>(numTargets) << "\n";
    // std::cout << "[Parser] Number of Missiles: " << static_cast<int>(numMissiles) << "\n";

    std::cout << std::dec; // 10진수 출력 설정

    // ✅ Target 파싱 (50바이트씩)
    for (int i = 0; i < numTargets && offset + 58 <= data.size(); ++i) {
        RadarDetection::Target t;
        std::memcpy(&t.id,         &data[offset],      4);
        std::memcpy(&t.posX,       &data[offset + 4],  8);
        std::memcpy(&t.posY,       &data[offset + 12], 8);
        std::memcpy(&t.altitude,   &data[offset + 20], 8);
        std::memcpy(&t.speed,      &data[offset + 28], 4);
        std::memcpy(&t.angle1,     &data[offset + 32], 8);  // 기존 angle → angle1
        std::memcpy(&t.angle2,     &data[offset + 40], 8);  // 새로 추가된 angle2
        std::memcpy(&t.detectTime, &data[offset + 48], 8);
        t.priority = data[offset + 56];
        t.hit = data[offset + 57];
        det.targets.push_back(t);
        offset += 58;
    }

    // ✅ Missile 파싱 (57바이트씩)
    for (int i = 0; i < numMissiles && offset + 57 <= data.size(); ++i) {
        RadarDetection::Missile m;
        std::memcpy(&m.id,           &data[offset],      4);
        std::memcpy(&m.posX,         &data[offset + 4],  8);
        std::memcpy(&m.posY,         &data[offset + 12], 8);
        std::memcpy(&m.altitude,     &data[offset + 20], 8);
        std::memcpy(&m.speed,        &data[offset + 28], 4);
        std::memcpy(&m.angle,        &data[offset + 32], 8);
        std::memcpy(&m.detectTime,   &data[offset + 40], 8);
        std::memcpy(&m.interceptTime,&data[offset + 48], 8);
        m.hit = data[offset + 56];
        det.missiles.push_back(m);
        offset += 57;
    }
    //

    for (size_t i = 0; i < det.targets.size(); ++i) {
        std::cout << "[Target#" << i << "] angle1: " << det.targets[i].angle1
                  << ", angle2: " << det.targets[i].angle2 << std::endl;
    }
    
    msg.payload = det;
    return msg;
}
 


} // anonymous namespace

namespace Common {

CommonMessage MessageParser::parse(const std::vector<uint8_t>& data, SenderType sender) {
    CommonMessage msg;
    msg.sender = sender;

    if (data.empty()) {
        std::cerr << "[Parser] 데이터가 비어 있음\n";
        msg.ok = false;
        return msg;
    }
    // debug
    /*
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    */

    uint8_t cmd = data[0];

    if (sender == SenderType::ECC) {
        msg.commandType = static_cast<CommandType>(cmd);

        switch (msg.commandType) {
        case CommandType::STATUS_REQUEST_ECC_TO_LC:
            msg.ok = true;
            break;
        case CommandType::SET_RADAR_MODE_ECC_TO_LC:
            return parseRadarCommand(data, msg);
        case CommandType::SET_LAUNCHER_MODE_ECC_TO_LC:
            return parseLauncherCommand(data, msg);
        case CommandType::FIRE_COMMAND_ECC_TO_LC:
            return parseFireCommand(data, msg);
        case CommandType::MOVE_COMMAND_ECC_TO_LC:
            return parseMoveCommand(data, msg);
        default:
            std::cerr << "[Parser] 알 수 없는 ECC commandType: " << static_cast<int>(cmd) << "\n";
            msg.ok = false;
            break;
        }
    }

    if (sender == SenderType::MFR) {
        msg.commandType = static_cast<CommandType>(cmd);
        switch (msg.commandType) {
            //0x21
        case CommandType::STATUS_RESPONSE_MFR_TO_LC: {
            // data를 hex로 출력
            // std::cout << "[STATUS_RESPONSE_MFR_TO_LC] data (hex): ";
            // for (size_t i = 0; i < data.size(); ++i) {
            //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
            // }
            // std::cout << std::dec << std::endl;
            return parseRadarStatus(data, sender);
        }
            //0x22
        case CommandType::DETECTION_MFR_TO_LC:
            return parseRadarDetection(data, sender);
            //0x23
        case CommandType::POSITION_REQUEST_MFR_TO_LC:
            return parsePositionRequest(data, sender);
        default:
            std::cerr << "[Parser] Unknown MFR commandType: " << static_cast<int>(cmd) << "\n";
            msg.ok = false;
            return msg;
        }
    }


    //0x41
    if (sender == SenderType::LS) {
        msg.commandType = static_cast<CommandType>(cmd);

        switch (msg.commandType) {
        case CommandType::LS_STATUS_UPDATE_LS_TO_LC: {
            return parseLSStatus(data, msg);  // 예: LS가 상태 갱신 전송
        }
        // case CommandType::LS_STATUS_RESPONSE_LS_TO_LC:
        //     return parseLSStatus(data, msg);  // 예: LS가 이동 완료 등 응답
        default:
            std::cerr << "[Parser] 알 수 없는 LS → LC commandType: "
                    << static_cast<int>(cmd) << "\n";
            msg.ok = false;
            break;
        }
    }

    return msg;
}
//0x23 

CommonMessage parseMoveCommandLS(const std::vector<uint8_t>& data, CommonMessage& msg) {
    if (data.size() < 1 + 4 + 8 + 8) {
        msg.ok = false;
        return msg;
    }
    MoveCommand mv;
    std::memcpy(&mv.lsId, &data[1], 4);
    std::memcpy(&mv.posX, &data[5], 8);
    std::memcpy(&mv.posY, &data[13], 8);
    msg.payload = mv;
    msg.commandType = static_cast<CommandType>(data[0]);

    msg.ok = true;
    return msg;
}

CommonMessage parseModeChangeCommand(const std::vector<uint8_t>& data, CommonMessage& msg) {
    if (data.size() < 1 + 4 + 1) {
        msg.ok = false;
        return msg;
    }
    LauncherModeCommand mc;
    std::memcpy(&mc.launcherId, &data[1], 4);
    mc.newMode = static_cast<OperationMode>(data[5]);

    msg.payload = mc;  // ✅ 이것도 variant에 포함된 타입이어야 함
    msg.commandType = static_cast<CommandType>(data[0]);
    msg.ok = true;
    return msg;
}

} // namespace Common
