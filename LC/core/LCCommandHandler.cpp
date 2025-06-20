#include "LCCommandHandler.h"
#include "LCManager.h"
#include "Serializer.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <climits>

namespace LCCommandHandler
{
    using namespace Common;

    // ECC 명령 처리
    void handleECCCommand(const CommonMessage &msg, LCManager &manager)
    {
        switch (msg.commandType)
        {
        case CommandType::STATUS_REQUEST_ECC_TO_LC: {
            static int eccRequestCounter = 0;
            eccRequestCounter++;

            // if (eccRequestCounter % 10 == 0) {
            //     std::cout << "[ECC] STATUS_REQUEST 수신 → 상태 전송\n";
            // }
            manager.sendStatus();
            break;
        }
        //0x02
        case CommandType::SET_RADAR_MODE_ECC_TO_LC:
        {
            const auto& payload = std::get<RadarModeCommand>(msg.payload);

            std::cout << "[ECC] 레이더 모드 변경 수신 → "
                    << "radarId=" << payload.radarId
                    << ", mode=" << static_cast<int>(payload.radarMode)
                    << ", priority_select=" << static_cast<int>(payload.priority_select)
                    << ", targetId=" << payload.targetId << "\n";

            // 직렬화 및 전송
            auto packet = Serializer::serializeRadarModeChange(payload);
            if (manager.hasMFRSender()) {
                manager.sendToMFR(packet);
                std::cout << "레이더에게 모드변경 요청, 전송 Byte : " << packet.size() << "\n";  
            } else {
                std::cerr << "[LCCommandHandler] MFR 송신자 없음. 전송 실패\n";
            }
            break;
        }
        //0x03
        case CommandType::SET_LAUNCHER_MODE_ECC_TO_LC:
        {
            const auto &payload = std::get<LauncherCommand>(msg.payload);
            std::cout << "[ECC] 발사대 모드 변경 수신 → lsId=" << payload.lsId
                      << ", mode=" << static_cast<int>(payload.lsMode) << "\n";

                      LauncherModeCommand cmd;
                      cmd.launcherId = payload.lsId;
                      cmd.newMode = static_cast<OperationMode>(payload.lsMode);
                      
                      auto packet = Serializer::serializeModeChangeCommand(cmd);
                      if (manager.hasLSSender())
                      {
                manager.sendToLS(packet);
            }
            break;
        }
        //0x04
        case CommandType::FIRE_COMMAND_ECC_TO_LC:
        {
            auto loop_start = std::chrono::high_resolution_clock::now();
            const auto& payload = std::get<FireCommand>(msg.payload);
            std::cout << "[ECC] 발사 명령 수신 → lsId=" << payload.lsId
                    << ", targetId=" << payload.targetId << "\n";

            SystemStatus snapshot = manager.getStatusCopy();
            const auto& ls = snapshot.ls;
            const auto& targets = snapshot.targets;

            TargetStatus selectedTarget{};
            bool found = false;

            if (payload.targetId == 0) {
                long long minDistSq = LLONG_MAX;
                for (const auto& t : targets) {
                    long long dx = t.posX - ls.position.x;
                    long long dy = t.posY - ls.position.y;
                    long long distSq = dx * dx + dy * dy;
                    if (distSq < minDistSq) {
                        minDistSq = distSq;
                        selectedTarget = t;
                        found = true;
                    }
                }
            } else {
                for (const auto& t : targets) {
                    if (t.id == payload.targetId) {
                        selectedTarget = t;
                        found = true;
                        break;
                    }
                }
            }
            

            if (!found) {
                std::cerr << "[LC] 대상 타겟 없음 → targetId=" << payload.targetId << "\n";
                break;
            }
            
            LaunchCommand cmd;
            cmd.launcherId = ls.launchSystemId;

            // 위도/경도 변환
            double lat_ls = static_cast<double>(ls.position.x) / 1e7;
            double lon_ls = static_cast<double>(ls.position.y) / 1e7;
            double lat_tg = static_cast<double>(selectedTarget.posX) / 1e7;
            double lon_tg = static_cast<double>(selectedTarget.posY) / 1e7;

            const double meters_per_deg_lat = 111320.0;
            const double meters_per_deg_lon = 111320.0 * std::cos(lat_ls * M_PI / 180.0);

            double dy = (lat_tg - lat_ls) * meters_per_deg_lat;
            double dx = (lon_tg - lon_ls) * meters_per_deg_lon;
            double dz = 0;
            double dist_m = std::sqrt(dx * dx + dy * dy);

            std::cout << "[LC] 발사대 위치: (" << lat_ls << ", " << lon_ls << ")\n";
            std::cout << "[LC] 타겟 위치:   (" << lat_tg << ", " << lon_tg << ")\n";
            std::cout << "[LC] 거리 계산 결과 → dx: " << dx << "m, dy: " << dy << "m, 총거리: " << dist_m << "m\n";

            double initial_bearing = std::atan2(dx, dy) * 180.0 / M_PI;
            if (initial_bearing < 0.0) initial_bearing += 360.0;

            const double missileSpeed = static_cast<double>(snapshot.ls.speed) * 1000.0 / 3600.0;
            const double targetSpeed = static_cast<double>(selectedTarget.speed) * 1000.0 / 3600.0;

            std::cout << "[LC] 타겟 속도: " << selectedTarget.speed << " km/h (" << targetSpeed << " m/s)\n";
            std::cout << "[LC] 타겟 헤딩 : " << selectedTarget.angle1 << "도\n";

            double targetHeadingRad = selectedTarget.angle1 * M_PI / 180.0;
            double vx_t = targetSpeed * std::sin(targetHeadingRad);
            double vy_t = targetSpeed * std::cos(targetHeadingRad);

            std::cout << "[LC] 타겟 속도 벡터 → vx: " << vx_t << " m/s, vy: " << vy_t << " m/s\n";

            double bestTime = -1.0;
            double interceptAngle = initial_bearing;
            bool foundSolution = false;

            for (double t = 1.0; t <= 2000.0; t += 0.1) {
                double future_lat = lat_tg + (vy_t * t) / meters_per_deg_lat;
                double future_lon = lon_tg + (vx_t * t) / meters_per_deg_lon;

                double dx_f = (future_lon - lon_ls) * meters_per_deg_lon;
                double dy_f = (future_lat - lat_ls) * meters_per_deg_lat;
                double dist_to_future = std::sqrt(dx_f * dx_f + dy_f * dy_f);

                double required_time = dist_to_future / missileSpeed;

                if (std::abs(required_time - t) < 0.2) {
                    interceptAngle = std::atan2(dx_f, dy_f) * 180.0 / M_PI;
                    if (interceptAngle < 0.0) interceptAngle += 360.0;
                    bestTime = t;
                    foundSolution = true;

                    double altitude_change = std::tan(selectedTarget.angle2 * M_PI / 180.0) * targetSpeed * t;
                    double target_altitude = selectedTarget.altitude + altitude_change;
                    dz = target_altitude - ls.height;
                    double horizontal_distance = std::sqrt(dx_f * dx_f + dy_f * dy_f);  // 실제 평면 거리
                    cmd.launchAngleXZ = std::atan2(dz, horizontal_distance) * 180.0 / M_PI;

                    std::cout << "[고도 디버깅]\n";
                    std::cout << "  타겟 초기 고도: " << selectedTarget.altitude << " m\n";
                    std::cout << "  타겟 고도 변화량: " << altitude_change << " m\n";
                    std::cout << "  타겟 예측 고도: " << target_altitude << " m\n";
                    std::cout << "  타겟 상승 각: " << selectedTarget.angle2 << " m\n";
                    std::cout << "  발사대 고도: " << ls.height << " m\n";
                    std::cout << "  dz = 타겟고도 - 발사대고도 = " << dz << " m\n";
                    std::cout << "  고도 각도 (XZ): " << cmd.launchAngleXZ << " 도\n";

                    std::cout << "[Intercept] t=" << t << "s, 위치=(" << future_lat << ", " << future_lon << "), 각도=" << interceptAngle << "도\n";
                    break;
                }
            }
            
            if (foundSolution) {
                cmd.launchAngleXY = interceptAngle;
                std::cout << "[LC] 벡터 기반 요격 계산 결과\n";
                std::cout << "  조준 각도 (XY): " << cmd.launchAngleXY << "도 (진북 기준)\n";
                std::cout << "  조준 각도 (XZ): " << cmd.launchAngleXZ << "도 (수직 기준)\n";
                std::cout << "  추정 요격 시간: " << bestTime << " 초\n";
                
                auto loop_end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = loop_end - loop_start;
                cmd.start_x = static_cast<long long>(
                    (std::cos(cmd.launchAngleXY * M_PI / 180.0) * missileSpeed * (elapsed.count() + 0.15) * 0.001 / 111.32) * 1e7
                        + snapshot.ls.position.x);

                double lat_deg = static_cast<double>(snapshot.ls.position.x) / 1e7;
                cmd.start_y = static_cast<long long>(
                    (std::sin(cmd.launchAngleXY * M_PI / 180.0) * missileSpeed * (elapsed.count() + 0.15) * 0.001 / (111.32 * std::cos(lat_deg * M_PI / 180.0))) * 1e7
                        + snapshot.ls.position.y);

                cmd.start_z = static_cast<long long>(snapshot.ls.height);
            } else {
                cmd.launchAngleXY = initial_bearing;
                cmd.launchAngleXZ = 0.0;
                cmd.start_x = 0;
                cmd.start_y = 0;
                cmd.start_z = 0;
                std::cerr << "[LC] 요격 불가: fallback 각도 적용 → " << initial_bearing << " 도\n";
            }

            auto packet = Serializer::serializeLaunchCommand(cmd);
            if (manager.hasLSSender()) {
                manager.sendToLS(packet);
            } else {
                std::cerr << "[LC] LS 송신자 없음. 전송 실패\n";
            }

            std::cout << std::dec;
            std::cout << "------------------------------------------------------" << std::endl;
            std::cout << "발사명령 정보\n"
                    << "  lsId: " << cmd.launcherId
                    << ", launchAngleXY: " << cmd.launchAngleXY
                    << ", launchAngleXZ: " << cmd.launchAngleXZ << " (수직 기준)\n";
            std::cout << "------------------------------------------------------" << std::endl;

            break;
        }



        //0x05
        case CommandType::MOVE_COMMAND_ECC_TO_LC:
        {
            const auto &payload = std::get<MoveCommand>(msg.payload);
            std::cout << "[ECC] 이동 명령 수신 → lsId=" << payload.lsId
                      << ", x=" << payload.posX << ", y=" << payload.posY << "\n";

            MoveCommandLS cmd;
            cmd.launcherId = payload.lsId;
            cmd.newX = payload.posX;
            cmd.newY = payload.posY;

            auto packet = Serializer::serializeMoveCommandLS(cmd);
            if (manager.hasLSSender())
            {
                manager.sendToLS(packet);
            }
            break;
        }

        default:
            std::cerr << "[ECC] 알 수 없는 명령\n";
            break;
        }
    }

    // MFR 명령 처리
    void handleMFRCommand(const CommonMessage &msg, LCManager &manager)
    {
        switch (msg.commandType)
        {
        case CommandType::STATUS_RESPONSE_MFR_TO_LC:
            manager.onRadarStatusReceived(std::get<RadarStatus>(msg.payload));
            break;
        case CommandType::DETECTION_MFR_TO_LC:  {
            manager.onRadarDetectionReceived(std::get<RadarDetection>(msg.payload));
            break;
        }
        case CommandType::POSITION_REQUEST_MFR_TO_LC:
        {
            manager.onLCPositionRequest();
            break;
        }
        default:
            std::cerr << "[MFR] 알 수 없는 명령\n";
            break;
        }
    }

    // LS 명령 처리
    void handleLSCommand(const CommonMessage &msg, LCManager &manager)
    {
        switch (msg.commandType)
        {
        case CommandType::LS_STATUS_UPDATE_LS_TO_LC:
        {
            // std::cout <<"data received from LS\n" ;
            const auto &status = std::get<Common::LSReport>(msg.payload);
            manager.onLSStatusReceived(status); // ✅ MFR처럼 위임 방식으로 통일
            break;
        }
        default:
            std::cerr << "[LS] 처리되지 않은 명령 수신: " << static_cast<int>(msg.commandType) << "\n";
            break;
        }
    }
    // LC 위치 요청 처리
        // void handleLCPositionRequest(LCManager &manager)
        // {
        //     manager.onLCPositionRequest();  // 내부에서 직렬화, sendToMFR()까지 처리
        // }
        // 통합 명령 처리 진입점
        void handleCommand(SenderType sender, const CommonMessage &msg, LCManager &manager)
        {
            switch (sender)
            {
            case SenderType::ECC:
                handleECCCommand(msg, manager);
                break;
            case SenderType::MFR:
                handleMFRCommand(msg, manager);
                break;
            case SenderType::LS:
                handleLSCommand(msg, manager);
                break;
            default:
                std::cerr << "[LCCommandHandler] 알 수 없는 송신자\n";
                break;
            }
        }


    // void handleLSCommand(const CommonMessage& msg, LCManager& manager) {
    //     switch (msg.commandType) {
    //     case CommandType::SET_LAUNCHER_MODE: {  // 0x03
    //         const auto& payload = std::get<LauncherCommand>(msg.payload);
    //         std::cout << "[ECC] 발사대 모드 변경 수신 → lsId=" << payload.lsId
    //                 << ", mode=" << static_cast<int>(payload.lsMode) << "\n";

    //         LSStatus status = manager.getStatusCopy().ls;
    //         status.lsId = payload.lsId;
    //         status.mode = payload.lsMode;
    //         // manager.updateStatus(status);
    //         break;
    //     }
    //     case CommandType::FIRE_COMMAND: {
    //         const auto& payload = std::get<FireCommand>(msg.payload);
    //         std::cout << "[LS] 발사 명령: lsId=" << payload.lsId
    //                   << ", targetId=" << payload.targetId << "\n";
    //         // 실제 발사 로직 필요시 추가
    //         break;
    //     }
    //     case CommandType::MOVE_COMMAND: {
    //         const auto& payload = std::get<MoveCommand>(msg.payload);
    //         std::cout << "[LS] 이동 명령: lsId=" << payload.lsId
    //                   << ", x=" << payload.posX << ", y=" << payload.posY << "\n";
    //         // 실제 이동 로직 필요시 추가
    //         break;
    //     }
    //     case CommandType::STATUS_REQUEST:
    //         std::cout << "[LS] 상태 요청 수신 → 응답 전송\n";
    //         manager.sendStatus();
    //         break;
    //     default:
    //         std::cerr << "[LS] 알 수 없는 명령 수신\n";
    //         break;
    //     }
    // }

  

} // namespace LCCommandHandler