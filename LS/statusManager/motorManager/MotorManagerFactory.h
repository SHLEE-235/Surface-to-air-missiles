#pragma once
#include "MotorManagerInterface.h"
#include "LSMotorManager.h"
#include "TestMotorManager.h"
#include <memory>
#include <string>

class MotorManagerFactory {
public:
    static std::unique_ptr<MotorManagerInterface> create(const std::string& type,
                                                        std::string& device,
                                                        int uartBaudRate) {
        if (type == "REAL") {
            return std::make_unique<LSMotorManager>(device, uartBaudRate);
        } else if (type == "TEST") {
            return std::make_unique<TestMotorManager>();
        } else {
            throw std::invalid_argument("Unknown MotorManager type: " + type);
        }
    }
};