#include <cmath>
#include <thread>
#include <iostream>
#include <cstring>
#include <memory>

#include "MockMissile.h"

MockMissile::MockMissile(const MissileInfo &missile_info, std::shared_ptr<MFRSendUDPManager> mfr_send_manager)
	: missile_info_(missile_info), mfr_send_manager_(mfr_send_manager) {}

void MockMissile::updatePosMissile()
{
	while (true)
	{
		// 위치 업데이트 로직 (예: 속도와 각도를 기반으로 위치 계산)
		missile_info_.x += std::cos(missile_info_.angle * M_PI / 180.0) * missile_info_.speed * 0.1;
		missile_info_.y += std::sin(missile_info_.angle * M_PI / 180.0) * missile_info_.speed * 0.1;
		missile_info_.z += missile_info_.speed * 0.01; // 고도 증가 예제

		// 데이터 전송
		sendData();

		// 0.1초 대기
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void MockMissile::sendData()
{
	// 미사일 데이터를 전송
	char buffer[1024];
	std::memcpy(buffer, &missile_info_, sizeof(missile_info_));
	mfr_send_manager_->sendData(buffer, sizeof(missile_info_));
	// std::cout << "Missile data sent." << std::endl;
}