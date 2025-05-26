#pragma once

#include <iostream>

class StepMotorController {
private:
    float targetRps;
    int stepsPerRev;
    float stepsPerAngle;
    int microStep;
    int pulsePerRev;
    float pulseIntervalUs;
    float totalPulse;

public:
    void runSpeedMode(int speed);
    void runAngleMode(double angle);
};