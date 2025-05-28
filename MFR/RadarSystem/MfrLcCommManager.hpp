#pragma once
#include "IReceiver.hpp"
#include <vector>

class MfrLcCommManager {
private:
    const unsigned int mfrId = 1;
    int sockfd;
    IReceiver* receiver;

public:
    MfrLcCommManager(IReceiver* receiver);
    void send(const std::vector<char>& packet);
private:
    void initMfrLcCommManager();
    bool connectToLc();
    void startTcpReceiver();    
};
