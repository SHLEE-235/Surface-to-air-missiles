#ifndef MOCK_MISSILE_MANAGER_H
#define MOCK_MISSILE_MANAGER_H

#include <thread>

#include "MissileInfo.h"
#include "MockTargetManager.h"
#include "MFRSendUDPManager.h"

class MockMissileManager
{
private:
	std::shared_ptr<MockTargetManager> mock_target_manager_;
	std::shared_ptr<MFRSendUDPManager> mfr_send_manager_;

	bool is_flight_ = false;
	std::thread flight_thread_;

	int last_missile_id_ = 0;		// 마지막 미사일 ID
	MissileInfo last_missile_info_; // 마지막 미사일 정보

	void updateMissileID();

public:
	// 생성자 선언 (정의 제거)
	MockMissileManager(std::shared_ptr<MockTargetManager> target_manager, std::shared_ptr<MFRSendUDPManager> mfr_send_manager);

	// 비행 미사일 관리 함수
	void flightMissile(const MissileInfo &MissileInfo);
};

#endif // MOCK_MISSILE_MANAGER_H