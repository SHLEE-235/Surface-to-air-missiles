#include "MockTarget.h"

#include <cmath>

MockTarget::MockTarget(const TargetInfo &target_info, std::shared_ptr<MFRSendUDPManager> mfr_send_manager) : target_info_(target_info), mfr_send_manager_(mfr_send_manager)
{
	// Constructor implementation
}

MockTarget::~MockTarget()
{
	// Destructor implementation
}

void MockTarget::updatePos()
{
	// 위치 업데이트 로직 (예: 속도와 각도를 기반으로 위치 계산)
	target_info_.x += std::cos(target_info_.angle * M_PI / 180.0) * target_info_.speed * 0.1;
	target_info_.y += std::sin(target_info_.angle * M_PI / 180.0) * target_info_.speed * 0.1;
	target_info_.z += target_info_.speed * 0.01; // 고도 증가 예제

	// 데이터 전송
	sendData();
}

void MockTarget::sendData()
{
	// 타겟 데이터를 전송
	char buffer[1024];
	std::cout << "target size : " << sizeof(target_info_) << std::endl;
	std::memcpy(buffer, &target_info_, sizeof(target_info_));
	mfr_send_manager_->sendData(buffer, sizeof(target_info_));
	std::cout << "Target data sent." << std::endl;
}