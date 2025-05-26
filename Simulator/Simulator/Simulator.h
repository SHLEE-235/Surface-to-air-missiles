#include <thread>
#include <memory>

#include "LSRecvUDPManager.h"
#include "MFRSendUDPManager.h"
#include "MockTargetManager.h"
#include "MockMissileManager.h"

class Simulator
{
private:
	std::unique_ptr<LSRecvUDPManager> ls_recv_manager_;
	std::shared_ptr<MFRSendUDPManager> mfr_send_manager_;

	std::shared_ptr<MockTargetManager> mock_target_manager_;
	std::unique_ptr<MockMissileManager> mock_missile_manager_;

	std::thread recv_thread_;
	std::thread flight_target_thread_;

public:
	Simulator();
	~Simulator();

	bool init();
	void start();
};