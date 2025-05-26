#include "MockTargetManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

// 생성자 수정: MFRSendUDPManager 포인터를 받도록 변경
MockTargetManager::MockTargetManager(std::shared_ptr<MFRSendUDPManager> mfr_send_manager)
	: mfr_send_manager_(mfr_send_manager)
{
	// Constructor implementation
}

MockTargetManager::~MockTargetManager()
{
	// Destructor implementation
}

void MockTargetManager::RaedTargetIni()
{
	std::ifstream file("target_list.ini");
	if (!file.is_open())
	{
		std::cerr << "Failed to open target_list.ini" << std::endl;
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> tokens;

		while (std::getline(ss, token, ','))
		{
			tokens.push_back(token);
		}

		if (tokens.size() == 6)
		{
			try
			{
				unsigned int id = std::stoul(tokens[0]);
				float x = std::stof(tokens[1]);
				float y = std::stof(tokens[2]);
				float z = std::stof(tokens[3]);
				float angle = std::stof(tokens[4]);
				int speed = std::stoi(tokens[5]);

				// TargetInfo 객체 생성 및 데이터 설정
				TargetInfo targetInfo;
				targetInfo.mockType = 0;
				targetInfo.id = id;
				targetInfo.x = x;
				targetInfo.y = y;
				targetInfo.z = z;
				targetInfo.angle = angle;
				targetInfo.speed = speed;

				// MockTarget 객체 생성 및 TargetInfo, MFRSendUDPManager 설정
				std::shared_ptr<MockTarget> target = std::make_shared<MockTarget>(targetInfo, mfr_send_manager_);
				std::cout << "add " << std::endl;

				// 타겟 추가
				addTarget(target);
			}
			catch (const std::invalid_argument &e)
			{
				std::cerr << "Invalid argument: " << e.what() << std::endl;
			}
			catch (const std::out_of_range &e)
			{
				std::cerr << "Out of range: " << e.what() << std::endl;
			}
		}
		else
		{
			std::cerr << "Invalid line format: " << line << std::endl;
		}
	}

	file.close();

	std::cout << "Target list loaded successfully." << std::endl;
}

void MockTargetManager::addTarget(std::shared_ptr<MockTarget> &target)
{
	targets.push_back(target);
}

void MockTargetManager::flitghtTarget()
{
	for (auto &target : targets)
	{
		std::cout << "update " << std::endl;
		target->updatePos();
	}
}