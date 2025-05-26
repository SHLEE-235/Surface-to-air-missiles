#ifndef MOCK_TARGET_H
#define MOCK_TARGET_H

#include <string>
#include <memory>

#include "TargetInfo.h"
#include "MFRSendUDPManager.h"

class MockTarget
{
public:
	MockTarget(const TargetInfo &target_info, std::shared_ptr<MFRSendUDPManager> mfr_send_manager);
	~MockTarget();

	void updatePos(); // 위치 업데이트

private:
	TargetInfo target_info_;
	std::shared_ptr<MFRSendUDPManager> mfr_send_manager_ = nullptr;

	void sendData(); // 데이터를 전송
};

#endif // MOCK_TARGET_H