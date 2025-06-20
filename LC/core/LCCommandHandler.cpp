#include "LCCommandHandler.h"
#include "LCManager.h"
#include "Serializer.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <climits>
#include "GeolocationConverter.h"

namespace LCCommandHandler
{
    using namespace Common;
    using namespace GeolocationConverter;

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
        // case CommandType::FIRE_COMMAND_ECC_TO_LC:
        // {
        //     const auto& payload = std::get<FireCommand>(msg.payload);
        //     std::cout << "[ECC] 발사 명령 수신 → lsId=" << payload.lsId
        //               << ", targetId=" << payload.targetId << "\n";

        //     SystemStatus snapshot = manager.getStatusCopy();
        //     const auto& ls = snapshot.ls; // 발사대 정보
        //     const auto& targets = snapshot.targets; // 타겟 목록

        //     TargetStatus selectedTarget{};
        //     bool found = false;

        //     // ***** 미사일 발사 지점(발사대)의 현재 위치 정보 추출 및 ECEF 변환 *****
        //     // 이 값들은 ECEF 계산에 사용될 뿐, cmd.start_x,y,z에는 원본 long long이 그대로 들어감.
        //     double launch_lat_deg = static_cast<double>(ls.position.x) / 1e8;
        //     double launch_lon_deg = static_cast<double>(ls.position.y) / 1e8;
        //     double launch_alt_meters = static_cast<double>(ls.height);

        //     double launch_lat_rad = degreesToRadians(launch_lat_deg);
        //     double launch_lon_rad = degreesToRadians(launch_lon_deg);
        //     Eigen::Vector3d launch_ecef = geodeticToEcef(launch_lat_rad, launch_lon_rad, launch_alt_meters);
        //     // *******************************************************************

        //     // 타겟 관련 변수 초기화
        //     double tg_lat_deg_initial = 0.0;
        //     double tg_lon_deg_initial = 0.0;
        //     double tg_alt_initial = 0.0;
        //     Eigen::Vector3d tg_ecef_initial;

        //     double initial_bearing = 0.0; // 요격 실패 시 fallback으로 사용할 초기 방위각

        //     if (payload.targetId == 0) { // 가장 가까운 타겟 찾기 (ECEF 기준)
        //         double minDistSq = std::numeric_limits<double>::max();
                
        //         for (const auto& t : targets) {
        //             double current_tg_lat_deg = static_cast<double>(t.posX) / 1e8;
        //             double current_tg_lon_deg = static_cast<double>(t.posY) / 1e8;
        //             double current_tg_alt = static_cast<double>(t.altitude);

        //             double current_tg_lat_rad = degreesToRadians(current_tg_lat_deg);
        //             double current_tg_lon_rad = degreesToRadians(current_tg_lon_deg);
        //             Eigen::Vector3d current_tg_ecef = geodeticToEcef(current_tg_lat_rad, current_tg_lon_rad, current_tg_alt);

        //             Eigen::Vector3d delta_ecef = current_tg_ecef - launch_ecef; // 발사대 ECEF 위치 기준
        //             double distSq = delta_ecef.squaredNorm();

        //             if (distSq < minDistSq) {
        //                 minDistSq = distSq;
        //                 selectedTarget = t;
        //                 found = true;
        //                 // 가장 가까운 타겟의 초기 정보를 저장
        //                 tg_lat_deg_initial = current_tg_lat_deg;
        //                 tg_lon_deg_initial = current_tg_lon_deg;
        //                 tg_alt_initial = current_tg_alt;
        //                 tg_ecef_initial = current_tg_ecef;
        //             }
        //         }
        //     } else { // 특정 타겟 ID 찾기
        //         for (const auto& t : targets) {
        //             if (t.id == payload.targetId) {
        //                 selectedTarget = t;
        //                 found = true;
        //                 // 선택된 타겟의 초기 정보를 저장
        //                 tg_lat_deg_initial = static_cast<double>(t.posX) / 1e8;
        //                 tg_lon_deg_initial = static_cast<double>(t.posY) / 1e8;
        //                 tg_alt_initial = static_cast<double>(t.altitude);

        //                 double tg_lat_rad = degreesToRadians(tg_lat_deg_initial);
        //                 double tg_lon_rad = degreesToRadians(tg_lon_deg_initial);
        //                 tg_ecef_initial = geodeticToEcef(tg_lat_rad, tg_lon_rad, tg_alt_initial);
        //                 break;
        //             }
        //         }
        //     }

        //     if (!found) {
        //         std::cerr << "[LC] 대상 타겟 없음 → targetId=" << payload.targetId << "\n";
        //         break;
        //     }

        //     // 요격 솔루션을 찾지 못했을 때 사용할 초기 방위각 계산 (폴백)
        //     Eigen::Vector3d vector_launch_to_tg_enu_initial = ecefToEnu(tg_ecef_initial, launch_ecef, launch_lat_rad, launch_lon_rad);
        //     initial_bearing = radiansToDegrees(std::atan2(vector_launch_to_tg_enu_initial.x(), vector_launch_to_tg_enu_initial.y()));
        //     if (initial_bearing < 0.0) initial_bearing += 360.0;


        //     LaunchCommand cmd;
        //     cmd.launcherId = ls.launchSystemId;

        //     std::cout << "[LC] 발사대 위치 (Lat/Lon/Alt): (" << launch_lat_deg << ", "
        //               << launch_lon_deg << ", " << launch_alt_meters << ")\n";
        //     std::cout << "[LC] 타겟 초기 위치 (Lat/Lon/Alt): (" << tg_lat_deg_initial << ", "
        //               << tg_lon_deg_initial << ", " << tg_alt_initial << ")\n";

        //     // 2. 타겟의 속도 벡터 계산 (ENU 기준 -> ECEF 변환)
        //     const double targetSpeed_mps = static_cast<double>(selectedTarget.speed) * 1000.0 / 3600.0; // km/h -> m/s
        //     double targetHeadingRad_XY = degreesToRadians(selectedTarget.angle1); // 수평 방위각
        //     double targetPitchRad_XZ = degreesToRadians(selectedTarget.angle2);   // 수직 고각 (상승각)

        //     // 타겟의 ENU 속도 벡터 (동, 북, 상)
        //     double cos_pitch = std::cos(targetPitchRad_XZ);
        //     double vx_enu_t = targetSpeed_mps * std::sin(targetHeadingRad_XY) * cos_pitch; // 동 (East)
        //     double vy_enu_t = targetSpeed_mps * std::cos(targetHeadingRad_XY) * cos_pitch; // 북 (North)
        //     double vz_enu_t = targetSpeed_mps * std::sin(targetPitchRad_XZ);              // 상 (Up)
        //     Eigen::Vector3d target_vel_enu(vx_enu_t, vy_enu_t, vz_enu_t);
        //     std::cout << "[LC] 타겟 ENU 속도 벡터 → East: " << vx_enu_t << " m/s, North: " << vy_enu_t << " m/s, Up: " << vz_enu_t << " m/s\n";

        //     // ENU 속도 벡터를 ECEF 속도 벡터로 변환 (타겟의 현재 위치 기준 ENU 프레임)
        //     double sin_lat_tg_initial = std::sin(degreesToRadians(tg_lat_deg_initial));
        //     double cos_lat_tg_initial = std::cos(degreesToRadians(tg_lat_deg_initial));
        //     double sin_lon_tg_initial = std::sin(degreesToRadians(tg_lon_deg_initial));
        //     double cos_lon_tg_initial = std::cos(degreesToRadians(tg_lon_deg_initial));

        //     Eigen::Matrix3d R_enu_to_ecef;
        //     R_enu_to_ecef << -sin_lon_tg_initial,         -sin_lat_tg_initial * cos_lon_tg_initial,  cos_lat_tg_initial * cos_lon_tg_initial,
        //                       cos_lon_tg_initial,         -sin_lat_tg_initial * sin_lon_tg_initial,  cos_lat_tg_initial * sin_lon_tg_initial,
        //                       0,                             cos_lat_tg_initial,                       sin_lat_tg_initial;

        //     Eigen::Vector3d target_vel_ecef = R_enu_to_ecef * target_vel_enu;
        //     std::cout << "[LC] 타겟 ECEF 속도 벡터 → X: " << target_vel_ecef.x() << " m/s, Y: " << target_vel_ecef.y() << " m/s, Z: " << target_vel_ecef.z() << " m/s\n";


        //     // 3. 요격 시간 및 각도 계산 (반복법)
        //     const double missileSpeed_mps = static_cast<double>(snapshot.ls.speed) * 1000.0 / 3600.0;
        //     double bestTime = -1.0;
        //     double interceptAngleXY = 0.0; // 방위각 (XY 평면)
        //     double interceptAngleXZ = 0.0; // 고각 (XZ 평면)
        //     bool foundSolution = false;

        //     // 1초부터 120초까지 0.1초 간격으로 시뮬레이션
        //     for (double t = 1.0; t <= 120.0; t += 0.01)
        //     {
        //         // 타겟의 예측 ECEF 위치
        //         Eigen::Vector3d tg_ecef_future = tg_ecef_initial + target_vel_ecef * t;

        //         // 발사대에서 미래 타겟까지의 ECEF 벡터
        //         Eigen::Vector3d vector_launch_to_tg_ecef = tg_ecef_future - launch_ecef;
        //         double dist_to_future_ecef = vector_launch_to_tg_ecef.norm(); // 거리 계산

        //         double required_time = dist_to_future_ecef / missileSpeed_mps;

        //         // 요격 조건 (시간 차이가 허용 오차 범위 내)
        //         if (std::abs(required_time - t) < 0.1) // 0.1초 오차 허용
        //         {
        //             bestTime = t;
        //             foundSolution = true;

        //             // 발사대 기준으로 ENU (East-North-Up) 벡터 변환
        //             Eigen::Vector3d vector_launch_to_tg_enu = ecefToEnu(tg_ecef_future, launch_ecef, launch_lat_rad, launch_lon_rad);

        //             // ENU 벡터에서 방위각 (XY) 및 고각 (XZ) 계산
        //             // 방위각 (Yaw/Azimuth): East (X), North (Y)
        //             // atan2(East, North) 사용 (atan2(x,y)는 Y축(북) 기준 X축(동)으로의 각도)
        //             interceptAngleXY = radiansToDegrees(std::atan2(vector_launch_to_tg_enu.x(), vector_launch_to_tg_enu.y()));
        //             if (interceptAngleXY < 0.0) interceptAngleXY += 360.0;

        //             // 고각 (Pitch/Elevation): Up (Z), 수평 거리 (sqrt(X^2 + Y^2))
        //             double horizontal_dist_enu = std::sqrt(vector_launch_to_tg_enu.x() * vector_launch_to_tg_enu.x() +
        //                                                    vector_launch_to_tg_enu.y() * vector_launch_to_tg_enu.y());
        //             interceptAngleXZ = radiansToDegrees(std::atan2(vector_launch_to_tg_enu.z(), horizontal_dist_enu));

        //             std::cout << "[LC] 요격 ENU 벡터 (E, N, U): (" << vector_launch_to_tg_enu.x() << ", "
        //                       << vector_launch_to_tg_enu.y() << ", " << vector_launch_to_tg_enu.z() << ") m\n";
        //             std::cout << "[Intercept] bestTime=" << bestTime << "s, Intercept Angle XY=" << interceptAngleXY
        //                       << " deg, Intercept Angle XZ=" << interceptAngleXZ << " deg\n";
        //             break;
        //         }
        //     }

        //     if (foundSolution) {
        //         cmd.launchAngleXY = interceptAngleXY;
        //         cmd.launchAngleXZ = interceptAngleXZ;
        //         std::cout << "[LC] ECEF 기반 요격 계산 결과\n";
        //         std::cout << "  조준 각도 (XY): " << cmd.launchAngleXY << "도 (진북 기준)\n";
        //         std::cout << "  조준 각도 (XZ): " << cmd.launchAngleXZ << "도 (수평 기준)\n";
        //         std::cout << "  추정 요격 시간: " << bestTime << " 초\n";

        //         // ***** 수정된 부분: cmd.start_x,y,z는 발사대 원본 위치 정보를 그대로 할당 *****
        //         // 미사일 발사 위치는 발사대 위치로 고정되며, ECEF 계산은 각도 도출에만 사용됨.
        //         cmd.start_x = ls.position.x; // 원본 long long (1e8 스케일)
        //         cmd.start_y = ls.position.y; // 원본 long long (1e8 스케일)
        //         cmd.start_z = ls.height;     // 원본 long long (미터)
        //         // *****************************************************************************

        //         std::cout << "시작 위치 (원본 long long Lat/Lon/Alt): " << cmd.start_x <<", " << cmd.start_y << ", " << cmd.start_z << std::endl;

        //     }
            
        //     else
        //     {
        //         // 요격 불가 시 fallback 로직 (초기 방위각 사용, 고각 0)
        //         cmd.launchAngleXY = initial_bearing;
        //         cmd.launchAngleXZ = 0.0;
        //         // ***** 수정된 부분: fallback 시에도 cmd.start_x,y,z는 발사대 원본 위치 정보를 그대로 할당 *****
        //         cmd.start_x = ls.position.x;
        //         cmd.start_y = ls.position.y;
        //         cmd.start_z = ls.height;
        //         // *********************************************************************************************
        //         std::cerr << "[LC] 요격 불가: fallback 각도 적용 → " << cmd.launchAngleXY << " 도\n";
        //     }

        //     auto packet = Serializer::serializeLaunchCommand(cmd);
        //     if(manager.hasLSSender())
        //     {
        //         manager.sendToLS(packet);
        //     }

        //     else
        //     {
        //         std::cerr << "[LC] LS 송신자 없음. 전송 실패\n";
        //     }

        //     std::cout << std::dec;
        //     std::cout << "------------------------------------------------------" << std::endl;
        //     std::cout << "발사명령 정보\n"
        //               << "  lsId: " << cmd.launcherId
        //               << ", launchAngleXY: " << cmd.launchAngleXY
        //               << ", launchAngleXZ: " << cmd.launchAngleXZ << " (수직 기준)\n";
        //     std::cout << "------------------------------------------------------" << std::endl;

        //     break;
        // }

        // case CommandType::FIRE_COMMAND_ECC_TO_LC:
        // {
        //     const auto& payload = std::get<FireCommand>(msg.payload);
        //     std::cout << "[ECC] 발사 명령 수신 → lsId=" << payload.lsId
        //               << ", targetId=" << payload.targetId << "\n";

        //     SystemStatus snapshot = manager.getStatusCopy();
        //     const auto& ls = snapshot.ls;
        //     const auto& targets = snapshot.targets;

        //     TargetStatus selectedTarget{};
        //     bool found = false;

        //     double launch_lat_deg = static_cast<double>(ls.position.x) / 1e8;
        //     double launch_lon_deg = static_cast<double>(ls.position.y) / 1e8;
        //     double launch_alt_meters = static_cast<double>(ls.height);

        //     double launch_lat_rad = degreesToRadians(launch_lat_deg);
        //     double launch_lon_rad = degreesToRadians(launch_lon_deg);
        //     Eigen::Vector3d launch_ecef = geodeticToEcef(launch_lat_rad, launch_lon_rad, launch_alt_meters);

        //     double tg_lat_deg_initial = 0.0;
        //     double tg_lon_deg_initial = 0.0;
        //     double tg_alt_initial = 0.0;
        //     Eigen::Vector3d tg_ecef_initial;

        //     double initial_bearing = 0.0; // Fallback initial bearing

        //     if (payload.targetId == 0) { // Find closest target (ECEF based)
        //         double minDistSq = std::numeric_limits<double>::max();
                
        //         for (const auto& t : targets) {
        //             double current_tg_lat_deg = static_cast<double>(t.posX) / 1e8;
        //             double current_tg_lon_deg = static_cast<double>(t.posY) / 1e8;
        //             double current_tg_alt = static_cast<double>(t.altitude);

        //             double current_tg_lat_rad = degreesToRadians(current_tg_lat_deg);
        //             double current_tg_lon_rad = degreesToRadians(current_tg_lon_deg);
        //             Eigen::Vector3d current_tg_ecef = geodeticToEcef(current_tg_lat_rad, current_tg_lon_rad, current_tg_alt);

        //             Eigen::Vector3d delta_ecef = current_tg_ecef - launch_ecef;
        //             double distSq = delta_ecef.squaredNorm();

        //             if (distSq < minDistSq) {
        //                 minDistSq = distSq;
        //                 selectedTarget = t;
        //                 found = true;
        //                 tg_lat_deg_initial = current_tg_lat_deg;
        //                 tg_lon_deg_initial = current_tg_lon_deg;
        //                 tg_alt_initial = current_tg_alt;
        //                 tg_ecef_initial = current_tg_ecef;
        //             }
        //         }
        //     } else { // Find specific target by ID
        //         for (const auto& t : targets) {
        //             if (t.id == payload.targetId) {
        //                 selectedTarget = t;
        //                 found = true;
        //                 tg_lat_deg_initial = static_cast<double>(t.posX) / 1e8;
        //                 tg_lon_deg_initial = static_cast<double>(t.posY) / 1e8;
        //                 tg_alt_initial = static_cast<double>(t.altitude);

        //                 double tg_lat_rad = degreesToRadians(tg_lat_deg_initial);
        //                 double tg_lon_rad = degreesToRadians(tg_lon_deg_initial);
        //                 tg_ecef_initial = geodeticToEcef(tg_lat_rad, tg_lon_rad, tg_alt_initial);
        //                 break;
        //             }
        //         }
        //     }

        //     if (!found) {
        //         std::cerr << "[LC] 대상 타겟 없음 → targetId=" << payload.targetId << "\n";
        //         break;
        //     }

        //     Eigen::Vector3d vector_launch_to_tg_enu_initial = ecefToEnu(tg_ecef_initial, launch_ecef, launch_lat_rad, launch_lon_rad);
        //     initial_bearing = radiansToDegrees(std::atan2(vector_launch_to_tg_enu_initial.x(), vector_launch_to_tg_enu_initial.y()));
        //     if (initial_bearing < 0.0) initial_bearing += 360.0;


        //     LaunchCommand cmd;
        //     cmd.launcherId = ls.launchSystemId;

        //     std::cout << "[LC] 발사대 위치 (Lat/Lon/Alt): (" << launch_lat_deg << ", "
        //               << launch_lon_deg << ", " << launch_alt_meters << ")\n";
        //     std::cout << "[LC] 타겟 초기 위치 (Lat/Lon/Alt): (" << tg_lat_deg_initial << ", "
        //               << tg_lon_deg_initial << ", " << tg_alt_initial << ")\n";

        //     // Target velocity calculation (ENU -> ECEF)
        //     const double targetSpeed_mps = static_cast<double>(selectedTarget.speed) * 1000.0 / 3600.0; // km/h -> m/s
        //     double targetHeadingRad_XY = degreesToRadians(selectedTarget.angle1); // Horizontal bearing (e.g., from True North clockwise)
        //     double targetPitchRad_XZ = degreesToRadians(selectedTarget.angle2);   // Vertical elevation (e.g., from horizontal upward)

        //     // Target's ENU velocity vector (East, North, Up)
        //     double cos_pitch = std::cos(targetPitchRad_XZ);
        //     double vx_enu_t = targetSpeed_mps * std::sin(targetHeadingRad_XY) * cos_pitch; // East
        //     double vy_enu_t = targetSpeed_mps * std::cos(targetHeadingRad_XY) * cos_pitch; // North
        //     double vz_enu_t = targetSpeed_mps * std::sin(targetPitchRad_XZ);              // Up
        //     Eigen::Vector3d target_vel_enu(vx_enu_t, vy_enu_t, vz_enu_t);
        //     std::cout << "[LC] 타겟 ENU 속도 벡터 → East: " << vx_enu_t << " m/s, North: " << vy_enu_t << " m/s, Up: " << vz_enu_t << " m/s\n";

        //     // Convert ENU velocity vector to ECEF velocity vector (at target's initial position)
        //     double sin_lat_tg_initial = std::sin(degreesToRadians(tg_lat_deg_initial));
        //     double cos_lat_tg_initial = std::cos(degreesToRadians(tg_lat_deg_initial));
        //     double sin_lon_tg_initial = std::sin(degreesToRadians(tg_lon_deg_initial));
        //     double cos_lon_tg_initial = std::cos(degreesToRadians(tg_lon_deg_initial));

        //     Eigen::Matrix3d R_enu_to_ecef;
        //     R_enu_to_ecef << -sin_lon_tg_initial,         -sin_lat_tg_initial * cos_lon_tg_initial,  cos_lat_tg_initial * cos_lon_tg_initial,
        //                       cos_lon_tg_initial,         -sin_lat_tg_initial * sin_lon_tg_initial,  cos_lat_tg_initial * sin_lon_tg_initial,
        //                       0,                             cos_lat_tg_initial,                       sin_lat_tg_initial;

        //     Eigen::Vector3d target_vel_ecef = R_enu_to_ecef * target_vel_enu;
        //     std::cout << "[LC] 타겟 ECEF 속도 벡터 → X: " << target_vel_ecef.x() << " m/s, Y: " << target_vel_ecef.y() << " m/s, Z: " << target_vel_ecef.z() << " m/s\n";


        //     // 3. 요격 시간 및 각도 계산 (뉴턴-랩슨 메서드)
        //     const double missileSpeed_mps = static_cast<double>(snapshot.ls.speed) * 1000.0 / 3600.0;
        //     double bestTime = -1.0;
        //     double interceptAngleXY = 0.0;
        //     double interceptAngleXZ = 0.0;
        //     bool foundSolution = false;

        //     // 뉴턴-랩슨 초기 추측값: 초기 타겟까지의 거리를 미사일 속도로 나눈 값
        //     Eigen::Vector3d initial_vec_ecef = tg_ecef_initial - launch_ecef;
        //     double initial_dist = initial_vec_ecef.norm();
        //     double current_t = initial_dist / missileSpeed_mps; // 초기 추정 시간

        //     // 뉴턴-랩슨 파라미터
        //     const double tolerance = 0.001; // 1ms (밀리초) 오차 허용
        //     const int max_iterations = 100; // 최대 반복 횟수

        //     for (int i = 0; i < max_iterations; ++i)
        //     {
        //         // 시간 t에서의 타겟 ECEF 위치 예측
        //         Eigen::Vector3d tg_ecef_at_t = tg_ecef_initial + target_vel_ecef * current_t;

        //         // 발사대에서 시간 t에서의 타겟까지의 ECEF 벡터
        //         Eigen::Vector3d R_vec = tg_ecef_at_t - launch_ecef;
        //         double D_t = R_vec.norm(); // 거리 D(t)

        //         // 함수 f(t) = D(t) / V_m - t
        //         double f_t = (D_t / missileSpeed_mps) - current_t;

        //         // 수렴 조건 체크
        //         if (std::abs(f_t) < tolerance) {
        //             bestTime = current_t;
        //             foundSolution = true;
                    
        //             // 요격 각도 계산 (이 시점의 미래 타겟 위치 기준)
        //             Eigen::Vector3d vector_launch_to_tg_enu = ecefToEnu(tg_ecef_at_t, launch_ecef, launch_lat_rad, launch_lon_rad);
        //             interceptAngleXY = radiansToDegrees(std::atan2(vector_launch_to_tg_enu.x(), vector_launch_to_tg_enu.y()));
        //             if (interceptAngleXY < 0.0) interceptAngleXY += 360.0;
        //             double horizontal_dist_enu = std::sqrt(vector_launch_to_tg_enu.x() * vector_launch_to_tg_enu.x() +
        //                                                    vector_launch_to_tg_enu.y() * vector_launch_to_tg_enu.y());
        //             interceptAngleXZ = radiansToDegrees(std::atan2(vector_launch_to_tg_enu.z(), horizontal_dist_enu));
                    
        //             std::cout << "[LC] 요격 ENU 벡터 (E, N, U): (" << vector_launch_to_tg_enu.x() << ", "
        //                       << vector_launch_to_tg_enu.y() << ", " << vector_launch_to_tg_enu.z() << ") m\n";
        //             std::cout << "[Intercept] bestTime=" << bestTime << "s, Intercept Angle XY=" << interceptAngleXY
        //                       << " deg, Intercept Angle XZ=" << interceptAngleXZ << " deg (Newton-Raphson Converged)\n";
        //             break;
        //         }

        //         // f'(t) 계산 (도함수)
        //         // D_t가 0에 가까워지면 문제가 생길 수 있으므로 작은 값으로 체크
        //         double R_dot_Vt = R_vec.dot(target_vel_ecef);
        //         double dD_dt = (D_t > 1e-6) ? (R_dot_Vt / D_t) : 0.0; // D_t가 0이면 0으로 처리 (타겟이 발사대 위치에 있는 경우)
                
        //         double f_prime_t = (1.0 / missileSpeed_mps) * dD_dt - 1.0;

        //         // f'(t)가 0에 가까우면 수렴하지 않거나 특이점일 수 있음. 반복 중단.
        //         if (std::abs(f_prime_t) < 1e-9) { // 0으로 나누는 것을 방지
        //             std::cerr << "[LC] Newton-Raphson: Derivative too small, stopping iterations.\n";
        //             break;
        //         }

        //         // 다음 추측값 계산
        //         current_t = current_t - f_t / f_prime_t;

        //         // 시간이 음수가 되면 비물리적이므로 양수로 유지
        //         if (current_t < 0.0) {
        //             current_t = 0.0; // 또는 초기 추측값으로 재설정하거나, 반복 중단.
        //             std::cerr << "[LC] Newton-Raphson: Predicted time became negative, stopping.\n";
        //             break;
        //         }
        //     }

        //     if (foundSolution) {
        //         cmd.launchAngleXY = interceptAngleXY;
        //         cmd.launchAngleXZ = interceptAngleXZ;
        //         std::cout << "[LC] ECEF 기반 요격 계산 결과 (Newton-Raphson)\n";
        //         std::cout << "  조준 각도 (XY): " << cmd.launchAngleXY << "도 (진북 기준)\n";
        //         std::cout << "  조준 각도 (XZ): " << cmd.launchAngleXZ << "도 (수평 기준)\n";
        //         std::cout << "  추정 요격 시간: " << bestTime << " 초\n";

        //         // cmd.start_x,y,z는 발사대 원본 위치 정보를 그대로 할당 (사용자 요구사항)
        //         cmd.start_x = ls.position.x;
        //         cmd.start_y = ls.position.y;
        //         cmd.start_z = ls.height;

        //         std::cout << "시작 위치 (원본 long long Lat/Lon/Alt): " << cmd.start_x <<", " << cmd.start_y << ", " << cmd.start_z << std::endl;

        //     } else {
        //         // 뉴턴-랩슨이 수렴에 실패했거나 해를 찾지 못한 경우 (fallback)
        //         cmd.launchAngleXY = initial_bearing;
        //         cmd.launchAngleXZ = 0.0;
        //         // fallback 시에도 cmd.start_x,y,z는 발사대 원본 위치 정보를 그대로 할당
        //         cmd.start_x = ls.position.x;
        //         cmd.start_y = ls.position.y;
        //         cmd.start_z = ls.height;
        //         std::cerr << "[LC] 요격 불가: Newton-Raphson 수렴 실패 또는 해 없음. fallback 각도 적용 → " << cmd.launchAngleXY << " 도\n";
        //     }

        //     auto packet = Serializer::serializeLaunchCommand(cmd);
        //     if(manager.hasLSSender())
        //     {
        //         manager.sendToLS(packet);
        //     }
        //     else
        //     {
        //         std::cerr << "[LC] LS 송신자 없음. 전송 실패\n";
        //     }

        //     std::cout << std::dec;
        //     std::cout << "------------------------------------------------------" << std::endl;
        //     std::cout << "발사명령 정보\n"
        //               << "  lsId: " << cmd.launcherId
        //               << ", launchAngleXY: " << cmd.launchAngleXY
        //               << ", launchAngleXZ: " << cmd.launchAngleXZ << " (수직 기준)\n";
        //     std::cout << "------------------------------------------------------" << std::endl;

        //     break;
        // }

        case CommandType::FIRE_COMMAND_ECC_TO_LC:
        {
            const auto& payload = std::get<FireCommand>(msg.payload);
            std::cout << "[ECC] 발사 명령 수신 → lsId=" << payload.lsId
                    << ", targetId=" << payload.targetId << "\n";

            SystemStatus snapshot = manager.getStatusCopy();
            const auto& ls = snapshot.ls;
            const auto& targets = snapshot.targets;

            TargetStatus selectedTarget{};
            bool found = false;

            double launch_lat_deg = static_cast<double>(ls.position.x) / 1e8;
            double launch_lon_deg = static_cast<double>(ls.position.y) / 1e8;
            double launch_alt_meters = static_cast<double>(ls.height);

            double launch_lat_rad = degreesToRadians(launch_lat_deg);
            double launch_lon_rad = degreesToRadians(launch_lon_deg);
            Eigen::Vector3d launch_ecef = geodeticToEcef(launch_lat_rad, launch_lon_rad, launch_alt_meters);

            double tg_lat_deg_initial = 0.0;
            double tg_lon_deg_initial = 0.0;
            double tg_alt_initial = 0.0;
            Eigen::Vector3d tg_ecef_initial;
            double initial_bearing = 0.0;

            if (payload.targetId == 0)
            {
                double minDistSq = std::numeric_limits<double>::max();
                for (const auto& t : targets)
                {
                    double current_lat = static_cast<double>(t.posX) / 1e8;
                    double current_lon = static_cast<double>(t.posY) / 1e8;
                    double current_alt = static_cast<double>(t.altitude);
                    Eigen::Vector3d tg_ecef = geodeticToEcef(
                        degreesToRadians(current_lat), degreesToRadians(current_lon), current_alt);
                    Eigen::Vector3d delta = tg_ecef - launch_ecef;
                    double distSq = delta.squaredNorm();
                    if (distSq < minDistSq)
                    {
                        minDistSq = distSq;
                        selectedTarget = t;
                        found = true;
                        tg_lat_deg_initial = current_lat;
                        tg_lon_deg_initial = current_lon;
                        tg_alt_initial = current_alt;
                        tg_ecef_initial = tg_ecef;
                    }
                }
            }

            else
            {
                for (const auto& t : targets)
                {
                    if (t.id == payload.targetId)
                    {
                        selectedTarget = t;
                        found = true;
                        tg_lat_deg_initial = static_cast<double>(t.posX) / 1e8;
                        tg_lon_deg_initial = static_cast<double>(t.posY) / 1e8;
                        tg_alt_initial = static_cast<double>(t.altitude);
                        tg_ecef_initial = geodeticToEcef(
                            degreesToRadians(tg_lat_deg_initial),
                            degreesToRadians(tg_lon_deg_initial),
                            tg_alt_initial);
                        break;
                    }
                }
            }

            if (!found)
            {
                std::cerr << "[LC] 대상 타겟 없음 → targetId=" << payload.targetId << "\n";
                break;
            }

            Eigen::Vector3d vec_enu_init = ecefToEnu(tg_ecef_initial, launch_ecef, launch_lat_rad, launch_lon_rad);
            initial_bearing = radiansToDegrees(std::atan2(vec_enu_init.x(), vec_enu_init.y()));
            if (initial_bearing < 0.0) initial_bearing += 360.0;

            LaunchCommand cmd;
            cmd.launcherId = ls.launchSystemId;

            std::cout << "[LC] 발사대 위치 (Lat/Lon/Alt): (" << launch_lat_deg << ", "
                    << launch_lon_deg << ", " << launch_alt_meters << ")\n";
            std::cout << "[LC] 타겟 초기 위치 (Lat/Lon/Alt): (" << tg_lat_deg_initial << ", "
                    << tg_lon_deg_initial << ", " << tg_alt_initial << ")\n";

            // 타겟 속도 계산
            double targetSpeed_mps = static_cast<double>(selectedTarget.speed) * 1000.0 / 3600.0;
            double heading_rad = degreesToRadians(selectedTarget.angle1);
            double pitch_rad = degreesToRadians(selectedTarget.angle2);
            double cos_pitch = std::cos(pitch_rad);

            double vx_enu = targetSpeed_mps * std::sin(heading_rad) * cos_pitch;
            double vy_enu = targetSpeed_mps * std::cos(heading_rad) * cos_pitch;
            double vz_enu = targetSpeed_mps * std::sin(pitch_rad);
            Eigen::Vector3d vel_enu(vx_enu, vy_enu, vz_enu);

            // 타겟 ENU → ECEF
            double sin_lat = std::sin(degreesToRadians(tg_lat_deg_initial));
            double cos_lat = std::cos(degreesToRadians(tg_lat_deg_initial));
            double sin_lon = std::sin(degreesToRadians(tg_lon_deg_initial));
            double cos_lon = std::cos(degreesToRadians(tg_lon_deg_initial));

            Eigen::Matrix3d R_enu_to_ecef;
            R_enu_to_ecef << -sin_lon, -sin_lat * cos_lon, cos_lat * cos_lon,
                            cos_lon, -sin_lat * sin_lon, cos_lat * sin_lon,
                            0,        cos_lat,            sin_lat;
            Eigen::Vector3d vel_ecef = R_enu_to_ecef * vel_enu;
            std::cout << "[DEBUG] 타겟 속도 ECEF: " << vel_ecef.transpose() << "\n";
            std::cout << "[DEBUG] 발사 → 타겟 초기 방향 벡터: " << (tg_ecef_initial - launch_ecef).normalized().transpose() << "\n";
            std::cout << "[DEBUG] 속도 벡터 방향: " << vel_ecef.normalized().transpose() << "\n";


            Eigen::Vector3d r = tg_ecef_initial - launch_ecef;
            Eigen::Vector3d v_t = vel_ecef;
            double V_m = static_cast<double>(snapshot.ls.speed) * 1000.0 / 3600.0;

            double a = v_t.squaredNorm() - V_m * V_m;
            double b = 2.0 * r.dot(v_t);
            double c = r.squaredNorm();

            double discriminant = b * b - 4 * a * c;
            double bestTime = -1.0;
            bool foundSolution = false;

            if (discriminant >= 0)
            {
                double sqrt_disc = std::sqrt(discriminant);
                double t1 = (-b + sqrt_disc) / (2 * a);
                double t2 = (-b - sqrt_disc) / (2 * a);
                if (t1 > 0 && t2 > 0) bestTime = std::min(t1, t2);
                else if (t1 > 0) bestTime = t1;
                else if (t2 > 0) bestTime = t2;
                if (bestTime > 0) foundSolution = true;
            }

            if (foundSolution)
            {
                Eigen::Vector3d tg_future_ecef = tg_ecef_initial + vel_ecef * bestTime;
                Eigen::Vector3d vec_enu = ecefToEnu(tg_future_ecef, launch_ecef, launch_lat_rad, launch_lon_rad);

                double angleXY = radiansToDegrees(std::atan2(vec_enu.x(), vec_enu.y()));
                if (angleXY < 0.0) angleXY += 360.0;
                double horizDist = std::sqrt(vec_enu.x() * vec_enu.x() + vec_enu.y() * vec_enu.y());
                double angleXZ = radiansToDegrees(std::atan2(vec_enu.z(), horizDist));

                cmd.launchAngleXY = angleXY;
                cmd.launchAngleXZ = angleXZ;

                std::cout << "[LC] 요격 계산 결과\n";
                std::cout << "  요격 시간: " << bestTime << "s\n";
                std::cout << "  발사 각도 XY (방위각): " << angleXY << " 도\n";
                std::cout << "  발사 각도 XZ (고각): " << angleXZ << " 도\n";
            }
            else
            {
                cmd.launchAngleXY = initial_bearing;
                cmd.launchAngleXZ = 0.0;
                std::cerr << "[LC] 요격 실패 → fallback 각도 사용: " << cmd.launchAngleXY << " 도\n";
            }

            cmd.start_x = ls.position.x;
            cmd.start_y = ls.position.y;
            cmd.start_z = ls.height;

            auto packet = Serializer::serializeLaunchCommand(cmd);
            if (manager.hasLSSender())
            {
                manager.sendToLS(packet);
            }
            else
            {
                std::cerr << "[LC] LS 송신자 없음. 전송 실패\n";
            }

            std::cout << "------------------------------------------------------" << std::endl;
            std::cout << "발사명령 정보\n"
                    << "  lsId: " << cmd.launcherId
                    << ", launchAngleXY: " << cmd.launchAngleXY
                    << ", launchAngleXZ: " << cmd.launchAngleXZ << std::endl;
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