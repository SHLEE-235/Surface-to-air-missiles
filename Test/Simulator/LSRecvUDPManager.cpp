#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 데이터 구조체 정의
struct LaunchData
{
	float latitude;	 // 위도
	float longitude; // 경도
	float z;		 // 고도
	float angle;	 // 발사각
	int speed;		 // 속도
};

int main()
{
	const char *server_ip = "127.0.0.1"; // 서버 IP (로컬호스트)
	const int server_port = 3001;		 // 서버 포트

	// 송신할 데이터 초기화
	LaunchData data;
	data.latitude = 37.5694f; // 소수점 4자리
	data.longitude = 126.9803f;
	data.z = 1000.0f;
	data.angle = 45.5f;
	data.speed = 1200;

	// UDP 소켓 생성
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket");
		return -1;
	}

	// 서버 주소 설정
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
	{
		perror("inet_pton");
		close(sock);
		return -1;
	}

	// 데이터 송신
	ssize_t sent_bytes = sendto(sock, &data, sizeof(data), 0,
								(struct sockaddr *)&server_addr, sizeof(server_addr));
	if (sent_bytes < 0)
	{
		perror("sendto");
		close(sock);
		return -1;
	}

	std::cout << "Sent LaunchData to " << server_ip << ":" << server_port << std::endl;
	std::cout << "Data: latitude=" << data.latitude << ", longitude=" << data.longitude
			  << ", z=" << data.z << ", angle=" << data.angle << ", speed=" << data.speed << std::endl;

	// 소켓 닫기
	close(sock);
	return 0;
}