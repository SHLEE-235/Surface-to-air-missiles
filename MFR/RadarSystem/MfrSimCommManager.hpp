#pragma once
#include "IReceiver.hpp"
#include <vector>

class MfrSimCommManager {
private:
    int sockfd;
    IReceiver* receiver;

public:
    MfrSimCommManager(IReceiver* receiver);
private:
    void initMfrSimCommManager();
    bool connectToSim();
    void startUdpReceiver();
};