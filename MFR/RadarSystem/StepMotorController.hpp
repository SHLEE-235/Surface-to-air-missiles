#pragma once
#include "PacketProtocol.hpp"
#include <iostream>

class StepMotorController {
private:

    const unsigned char microStep = 16;         //16분주
    const float stepAngle = 1.8;                //nema17 1 pulse -> 1.8
                                                //최종 1 pulse -> 0.1125
    float totalPulse;

public:
    StepMotorController();
    void runSpeedMode(int speed);
    void runAngleMode(double curMotorAngle);
};