#ifndef MOCK_TARGET_MANAGER_H
#define MOCK_TARGET_MANAGER_H

#include <vector>
#include <memory>

#include "MockTarget.h"
#include "TargetInfo.h"

#include "MFRSendUDPManager.h"

class MockTargetManager
{
public:
	MockTargetManager(std::shared_ptr<MFRSendUDPManager> mfr_send_manager); // 생성자 수정
	~MockTargetManager();

	void RaedTargetIni();
	void addTarget(std::shared_ptr<MockTarget> &target);
	void flitghtTarget();

private:
	std::vector<std::shared_ptr<MockTarget>> targets;
	std::shared_ptr<MFRSendUDPManager> mfr_send_manager_; // MFRSendUDPManager 추가
};

#endif