#pragma once
#include "IReceiver.hpp"
#include <vector>

class MfrSimCommManager {
private:
    int sockfd;
    IReceiver* receiver;

public:
    MfrSimCommManager(IReceiver* recv = nullptr) : receiver(recv) {}    
    bool connectToSim();
    void startUdpReceiver();
    void recv(const std::vector<char>& packet);
    void setReceiver(IReceiver* recv);
};