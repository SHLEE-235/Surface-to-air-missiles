#pragma once
#include "MissileInfo.h"
#include "MFRSendUDPManager.h"

class MockMissile
{
public:
	MockMissile(const MissileInfo &missile_info, std::shared_ptr<MFRSendUDPManager> mfr_send_manager);
	~MockMissile() {}

	void updatePosMissile(); // 미사일 목표 설정

private:
	MissileInfo missile_info_;
	std::shared_ptr<MFRSendUDPManager> mfr_send_manager_;

	void sendData(); // 데이터를 전송
};