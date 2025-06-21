#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "MotorManagerInterface.h"
class LSMotorManager :  public MotorManagerInterface
{
private:
    int uart_fd = -1;

    std::string device;
    speed_t uartBaudRate;
    bool initUart();

public:
    LSMotorManager(std::string& device, int& uartBaudRate);
    ~LSMotorManager();
    bool rotateToAngle(double targetAngle);
};
