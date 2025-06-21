#pragma once
#include "MotorManagerInterface.h"
#include <iostream>

class TestMotorManager : public MotorManagerInterface {
public:
    bool rotateToAngle(double targetAngle) override {
        std::cout << "[TestMotorManager] Simulated rotation to " << targetAngle << " completed.\n";
        return true;
    }
};
