#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>

struct MockMissile
{
	uint32_t id;
	float x;	   // 위도
	float y;	   // 경도
	float z;	   // 고도
	float angle;   // 발사각
	int32_t speed; // 속도
};

constexpr double DEGREE_PER_METER_LAT = 1.0 / 111320.0;
constexpr double DEGREE_PER_METER_LON = 1.0 / (111320.0 * cos(37.5665 * M_PI / 180.0));

void sendMissileStatus(int client, MockMissile &missile)
{
	char buffer[sizeof(MockMissile)];
	std::memcpy(buffer, &missile.id, 4);
	std::memcpy(buffer + 4, &missile.x, 4);
	std::memcpy(buffer + 8, &missile.y, 4);
	std::memcpy(buffer + 12, &missile.z, 4);
	std::memcpy(buffer + 16, &missile.angle, 4);
	std::memcpy(buffer + 20, &missile.speed, 4);
	send(client, buffer, sizeof(MockMissile), 0);
}

int main()
{
	int server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == -1)
	{
		std::cerr << "Could not create socket" << std::endl;
		return 1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9001);
	addr.sin_addr.s_addr = inet_addr("0.0.0.0");

	if (bind(server, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Bind failed" << std::endl;
		close(server);
		return 1;
	}

	listen(server, 1);

	std::cout << "Server started on 127.0.0.1:9001" << std::endl;

	while (true)
	{
		int client = accept(server, nullptr, nullptr);
		if (client < 0)
		{
			std::cerr << "Accept failed" << std::endl;
			close(server);
			return 1;
		}
		std::cout << "Client connected." << std::endl;

		MockMissile missile;
		missile.id = 101001;
		missile.x = 37.5665f;
		missile.y = 126.9780f;
		missile.z = 0.0f;
		missile.angle = 0.0f;
		missile.speed = 10;

		char recvbuf[128];
		while (true)
		{
			int len = recv(client, recvbuf, sizeof(recvbuf) - 1, 0);
			std::cout << "len : " << len << std::endl;
			if (len <= 0)
				break;
			recvbuf[len] = 0;

			if (std::string(recvbuf).find("getStatus") != std::string::npos)
			{
				missile.y += static_cast<float>(DEGREE_PER_METER_LON);
				sendMissileStatus(client, missile);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		close(client);
		std::cout << "Client disconnected. Waiting for new connection..." << std::endl;
	}

	close(server);
	return 0;
}