#pragma once

class MotorManagerInterface
{
public:
    virtual ~MotorManagerInterface() = default;

    // 목표 각도로 회전 요청하고 성공 여부 반환
    virtual bool rotateToAngle(double targetAngle) = 0;
};
