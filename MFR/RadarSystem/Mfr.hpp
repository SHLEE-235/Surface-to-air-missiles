#pragma once

#include "StepMotorController.hpp"
#include "MfrLcCommManager.hpp"
#include "MfrSimCommManager.hpp"
#include "IReceiver.hpp"
#include "PacketProtocol.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>



class Mfr : public IReceiver {
private:
    unsigned int mfrId;
    float mfrX;
    float mfrY;
    int mfrMode;
    double targetMotorAngle;

    MfrStatus mfrStatus;
    StepMotorController stepMotorManager;
    MfrLcCommManager lcCommManager;
    MfrSimCommManager simCommManager;

    std::vector<MockTarget> mockTargets;
    std::vector<MockMissile> mockMissile;   // shared ptr

public:
    Mfr();
    
    void startTcp();
    void startUdp();

    void sendToLc(const std::vector<char>& packet);
    std::vector<MockTarget*> MfrDetectionAlgo();
    
    void callBackData(const std::vector<char>& packet) override;

    void handleSimDataPayload(const std::vector<char>& packet);

    void addMockTarget(const MockTarget& target);
    MockTarget* getMockTargetById(unsigned int id);
    void removeMockTargetById(unsigned int id);
    void clearMockTargets();

    void addMockMissile(const MockMissile& missile);
    MockMissile* getMockMissileById(unsigned int id);
    void removeMockMissileById(unsigned int id);
    void clearMockMissiles();

    void manageMfrStatus();

    void controlMotor(const std::vector<char>& payload);
    std::pair<uint8_t, float> motorDataParser(const std::vector<char>& payload);

    std::vector<char> serializeMfrStatus(const MfrStatus& status);
    void startMfrStatusSender();
};