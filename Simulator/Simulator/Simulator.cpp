#include "Simulator.h"

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <stdexcept>
#include <thread>

#include "MissileInfo.h"
#include "LSRecvUDPManager.h"
#include "MFRSendUDPManager.h"

Simulator::Simulator()
{
}

Simulator::~Simulator()
{
	// Destructor implementation
}

bool Simulator::init()
{
	// Constructor implementation
	ls_recv_manager_ = std::make_unique<LSRecvUDPManager>();
	mfr_send_manager_ = std::make_shared<MFRSendUDPManager>();
	if (!ls_recv_manager_ || !mfr_send_manager_)
	{
		std::cerr << "Failed to allocate memory for LSRecvUDPManager or MFRSendUDPManager." << std::endl;
		return false;
	}

	if (!ls_recv_manager_->LSSocketOpen(3001))
	{
		std::cerr << "Failed to initialize ls_recv_manager socket." << std::endl;
		return false;
	}

	if (!mfr_send_manager_->MFRSocketOpen("192.168.2.176", 9000))
	{
		std::cerr << "Failed to initialize mfr_send_manager socket." << std::endl;
		return false;
	}

	// Initialize the mock target manager
	mock_target_manager_ = std::make_shared<MockTargetManager>(mfr_send_manager_);
	mock_target_manager_->RaedTargetIni();

	// Initialize the mock missile
	mock_missile_manager_ = std::make_unique<MockMissileManager>(mock_target_manager_, mfr_send_manager_);

	return true;
}

void Simulator::start()
{
	recv_thread_ = std::thread([this]()
							   {
        char buffer[1024];
        while (true) {
            if (ls_recv_manager_->receiveData(buffer, sizeof(buffer))) {
                if (strlen(buffer) > 0) {
                    // 수신된 데이터를 LaunchData 구조체로 파싱
                    MissileInfo received_data;
                    if (sizeof(received_data) <= sizeof(buffer)) {
						
						{
							size_t offset = 0;
							// Latitude (long long)
							if (offset + sizeof(long long) <= sizeof(buffer))
							{
								std::memcpy(&received_data.x, buffer + offset, sizeof(long long));
								offset += sizeof(long long);
							}

							// Longitude (long long)
							if (offset + sizeof(long long) <= sizeof(buffer))
							{
								std::memcpy(&received_data.y, buffer + offset, sizeof(long long));
								offset += sizeof(long long);
							}

							// Altitude (z, long long)
							if (offset + sizeof(long long) <= sizeof(buffer))
							{
								std::memcpy(&received_data.z, buffer + offset, sizeof(long long));
								offset += sizeof(long long);
							}

							// Speed (int)
							if (offset + sizeof(int) <= sizeof(buffer))
							{
								std::memcpy(&received_data.speed, buffer + offset, sizeof(int));
								offset += sizeof(int);
							}

							// Angle (double)
							if (offset + sizeof(double) <= sizeof(buffer))
							{
								std::memcpy(&received_data.angle, buffer + offset, sizeof(double));
								offset += sizeof(double);
							}

							std::cout << "offset size : " << offset << std::endl;
						}

						// LaunchData 구조체 출력
						std::cout
							<< "Received LaunchData:" << std::endl;
						std::cout << "  ID: " << received_data.id << std::endl;
                        std::cout << "  Latitude: " << received_data.x << std::endl;
                        std::cout << "  Longitude: " << received_data.y << std::endl;
                        std::cout << "  Altitude (z): " << received_data.z << std::endl;
                        std::cout << "  Angle: " << received_data.angle << std::endl;
                        std::cout << "  Speed: " << received_data.speed << std::endl;

						// 미사일 발사 명령
						mock_missile_manager_->flightMissile(received_data);
					} else {
                        std::cerr << "Received data size does not match LaunchData structure size." << std::endl;
                    }
                }
                memset(buffer, 0, sizeof(buffer)); // Clear the buffer after processing
            } else {
                // Optional: Add a small delay to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } });
	recv_thread_.detach();

	flight_target_thread_ = std::thread([this]()
										{
		while (true)
		{
			mock_target_manager_->flitghtTarget();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		} });
	flight_target_thread_.detach();
}