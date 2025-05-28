#include "MfrLcCommManager.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>

#define LC_SERVER_IP "192.168.1.110"
#define LC_SERVER_PORT 9999
#define BUFFER_SIZE 1024

MfrLcCommManager::MfrLcCommManager(IReceiver* receiver) : receiver(receiver), sockfd(-1) 
{
    initMfrLcCommManager();
}

void MfrLcCommManager::initMfrLcCommManager()
{
    std::cout << "[Mfr::startTcp] TCP 통신 수신 시작" << std::endl;
    if(connectToLc())
    {
        startTcpReceiver();
    }
}

bool MfrLcCommManager::connectToLc() 
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        std::cerr << "[MfrLcCommManager::connectToLc] 소켓 생성 실패" << std::endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(LC_SERVER_PORT);
    inet_pton(AF_INET, LC_SERVER_IP, &serverAddr.sin_addr);
    
    if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        perror("[MfrLcCommManager::connectToLc] LC 서버 연결 실패");
        close(sockfd);
        sockfd = -1;
        return false;
    }

    std::cout << "[MfrLcCommManager] LC 서버에 연결 성공" << std::endl;
    return true;
}

void MfrLcCommManager::startTcpReceiver()
{
    std::thread([this]() {
        char buffer[BUFFER_SIZE];

        std::cout << "[MfrLcCommManager::startTcpReceiver] TcpReceiver 스레드 시작" << std::endl;

        while (true)
        {
            ssize_t len = read(this->sockfd, buffer, sizeof(buffer));
            if (len <= 0)
            {
                std::cerr << "[MfrLcCommManager::startTcpReceiver] 연결 종료 또는 수신 실패" << std::endl;
                break;
            }

            std::vector<char> packet(buffer, buffer + len);
            std::cout << "[MfrLcCommManager::startTcpReceiver] 수신된 패킷: " << len << " bytes" << std::endl;

            if (this->receiver)
            {
                this->receiver->callBackData(packet);
            }
            else
            {
                std::cerr << "[MfrLcCommManager::startTcpReceiver] receiver가 null임" << std::endl;
            }
        }
    }).detach();
}

void MfrLcCommManager::send(const std::vector<char>& packet) 
{
    if (sockfd < 0) 
    {
        std::cerr << "[MfrLcCommManager::send] 소켓이 미 Open" << std::endl;
        return;
    }

    ssize_t sent = write(sockfd, packet.data(), packet.size());
    if (sent != packet.size()) 
    {
        std::cerr << "[MfrLcCommManager::send] 전송 실패" << std::endl;
    }

    else 
    {
        std::cout << "[MfrLcCommManager::send] send() 성공, size: " << sent << " bytes" << std::endl;
    }
}
