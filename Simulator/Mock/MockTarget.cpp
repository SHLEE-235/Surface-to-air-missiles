#include "MockTarget.h"

#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>
#include <cstring>

constexpr double DEGREE_TO_INT = 1e7;                     // 위도/경도를 정수로 저장할 때 사용하는 스케일
constexpr double METERS_PER_DEGREE_LAT = 111320.0;        // 위도 1도당 거리 (m)

MockTarget::MockTarget(const TargetInfo &target_info, std::shared_ptr<MFRSendUDPManager> mfr_send_manager)
	: target_info_(target_info), mfr_send_manager_(mfr_send_manager)
{
}

MockTarget::~MockTarget()
{
}

void MockTarget::updatePos()
{
	static const auto start_time = std::chrono::steady_clock::now();

	// 초기 위치 기록
	const double init_lat = static_cast<double>(target_info_.x) / DEGREE_TO_INT;
	const double init_lon = static_cast<double>(target_info_.y) / DEGREE_TO_INT;
	const double init_alt = static_cast<double>(target_info_.z); // m 단위

	const double speed_mps = target_info_.speed * 0.27778;
	const double angle_rad = target_info_.angle * M_PI / 180.0;
	const double angle_z_rad = target_info_.angle2 * M_PI / 180.0;

	static double last_logged = 0.0;

	while (true)
	{
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - start_time;
		double elapsed_sec = elapsed.count();

		// 총 이동 거리
		double distance = speed_mps * elapsed_sec;

		// 현재 위도 기준 경도 m/deg 계산
		double meters_per_deg_lon = METERS_PER_DEGREE_LAT * std::cos(init_lat * M_PI / 180.0);

		// 현재 위치 계산 (실수 단위)
		double delta_lat = std::cos(angle_rad) * distance / METERS_PER_DEGREE_LAT;
		double delta_lon = std::sin(angle_rad) * distance / meters_per_deg_lon;
		double delta_alt = std::tan(angle_z_rad) * distance;

		// 정수로 변환해 갱신
		target_info_.x = static_cast<long long>((init_lat + delta_lat) * DEGREE_TO_INT);
		target_info_.y = static_cast<long long>((init_lon + delta_lon) * DEGREE_TO_INT);
		target_info_.z = static_cast<long long>(init_alt + delta_alt);

		// 4초마다 출력
		if (elapsed_sec - last_logged >= 4.0)
		{
			std::cout << "[4 sec update] Target moved " << distance << " meters.\n";
			std::cout << " → Current lat: " << (init_lat + delta_lat)
					  << ", lon: " << (init_lon + delta_lon)
					  << ", alt: " << (init_alt + delta_alt) << " m\n\n";
			last_logged = elapsed_sec;
		}

		sendData();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}



void MockTarget::sendData()
{
	char buffer[1024];
	std::memcpy(buffer, &target_info_, sizeof(target_info_));
	mfr_send_manager_->sendData(buffer, sizeof(target_info_));
}

bool MockTarget::downTargetStatus(const MissileInfo &missileInfo)
{
	const int missile_range = 200; // m 기준

	// 위도/경도 간 거리 차이(m) 계산
	double lat1 = static_cast<double>(missileInfo.x) / DEGREE_TO_INT;
	double lon1 = static_cast<double>(missileInfo.y) / DEGREE_TO_INT;
	double alt1 = static_cast<double>(missileInfo.z);
	
	double lat2 = static_cast<double>(target_info_.x) / DEGREE_TO_INT;
	double lon2 = static_cast<double>(target_info_.y) / DEGREE_TO_INT;
	double alt2 = static_cast<double>(target_info_.z);

	double avg_lat = (lat1 + lat2) / 2.0;
	double meters_per_deg_lon = METERS_PER_DEGREE_LAT * std::cos(avg_lat * M_PI / 180.0);

	double dx = (lon1 - lon2) * meters_per_deg_lon;
	double dy = (lat1 - lat2) * METERS_PER_DEGREE_LAT;
	double dz = alt1 - alt2;

	double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

	if (distance <= missile_range)
	{
		target_info_.is_hit = true;
		return true;
	}
	return false;
}
