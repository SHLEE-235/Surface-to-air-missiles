#include "MFRSendUDPManager.h"

MFRSendUDPManager::MFRSendUDPManager(/* args */)
{
	// Constructor implementation
}

MFRSendUDPManager::~MFRSendUDPManager()
{
	// Destructor implementation
}

bool MFRSendUDPManager::MFRSocketOpen(const std::string &ip, int port)
{
	// UDP 소켓 생성
	mfr_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (mfr_socket_ < 0)
	{
		perror("socket");
		return false;
	}

	// 서버 주소 설정
	memset(&client_addr_, 0, sizeof(client_addr_));
	client_addr_.sin_family = AF_INET;
	client_addr_.sin_port = htons(port);
	if (inet_pton(AF_INET, ip.c_str(), &client_addr_.sin_addr) <= 0)
	{
		perror("inet_pton");
		close(mfr_socket_);
		mfr_socket_ = -1;
		return false;
	}

	std::cout << "MFRSocketOpen: Socket opened for IP " << ip << " on port " << port << std::endl;
	return true;
}

bool MFRSendUDPManager::sendData(const char *data, int dataSize)
{
	if (mfr_socket_ < 0)
	{
		std::cerr << "Socket is not open. Call MFRSocketOpen first." << std::endl;
		return false;
	}

	// 데이터 전송
	ssize_t sent_bytes = sendto(mfr_socket_, data, dataSize, 0,
								(struct sockaddr *)&client_addr_, sizeof(client_addr_));
	if (sent_bytes < 0)
	{
		perror("sendto");
		return false;
	}

	std::cout << "missile sendData: Sent " << sent_bytes << " bytes to "
			  << inet_ntoa(client_addr_.sin_addr) << ":" << ntohs(client_addr_.sin_port) << std::endl;
	return true;
}