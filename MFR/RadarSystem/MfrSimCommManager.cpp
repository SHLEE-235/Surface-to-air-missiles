#include "MfrSimCommManager.hpp"
#include "PacketProtocol.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <iostream>

#define SIM_UDP_PORT 9000
#define BUFFER_SIZE 1024

MfrSimCommManager::MfrSimCommManager(IReceiver* receiver) : receiver(receiver), sockfd(-1) 
{
    initMfrSimCommManager();
}

void MfrSimCommManager::initMfrSimCommManager()
{
    std::cout << "[Mfr::startUdp] UDP 통신 수신 시작" << "\n";
    if (connectToSim()) 
    {
        startUdpReceiver();        
    }
}

bool MfrSimCommManager::connectToSim()
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("UDP 소켓 생성 실패");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SIM_UDP_PORT);

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        perror("UDP bind 실패");
        close(sockfd);
        return false;
    }

    return true;
}

void MfrSimCommManager::startUdpReceiver()
{
    std::thread([this]() {
        char buffer[BUFFER_SIZE];
        
        std::cout << "[MfrSimCommManager::startUdpReceiver] UdpReceiver 스레드 시작" << std::endl;

        while (true) 
        {
            //std::cout << "SIMDATA SIZE: " << sizeof(SimData) << std::endl;
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            ssize_t len = recvfrom(this->sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen);
            if (len > 0) 
            {
                if (len == sizeof(uint8_t) + sizeof(SimData)) 
                {
                    SimData data;
                    std::memcpy(&data, buffer + 1, sizeof(SimData)); // +1로 수정
                    if (this->receiver != nullptr) 
                    {
                        std::vector<char> packet(buffer, buffer + len);
                        receiver->callBackData(packet);
                    }
                    else
                    {
                        std::cout << "[MfrSimCommManager::startUdpReceiver] receiver null" << "\n";
                    }
                }
                else
                {
                    std::cerr << "패킷 크기 이상 (" << len << " bytes)" << std::endl;
                }
            }
        }
    }).detach();
}