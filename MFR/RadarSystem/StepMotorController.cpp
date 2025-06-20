#include "StepMotorController.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>

StepMotorController::StepMotorController()
{
    // if (loadMfrConfig("../config/MFR.ini", mfrConfig)) 
    // {
    //     std::cout << "Device: " << mfrConfig.device << ", BaudRate: " << mfrConfig.uartBaudRate << std::endl;

    //     device = mfrConfig.device;
    //     uartBaudRate = mfrConfig.uartBaudRate;

    //     if(!initUart())
    //     {
    //         std::cerr << "[StepMotorController::initUart] Uart Init Failed" << std::endl;
    //     }
    // }

    // else
    // {
    //     std::cerr << "[StepMotorController::StepMotorController] ini Read Failed" << std::endl;
    // }
}

StepMotorController::~StepMotorController()
{
    if (uart_fd >= 0)            
    {
        close(uart_fd);
    }
}

bool StepMotorController::initUart()
{
    uart_fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (uart_fd < 0)
    {
        std::cerr << "[UART] Failed to open device " << device << std::endl;
        return false;
    }

    struct termios tty;
    if (tcgetattr(uart_fd, &tty) != 0)
    {
        std::cerr << "[UART] Error getting termios attributes" << std::endl;
        return false;
    }

    cfsetospeed(&tty, uartBaudRate);
    cfsetispeed(&tty, uartBaudRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                         // disable break processing
    tty.c_lflag = 0;                                // no signaling chars, no echo
    tty.c_oflag = 0;                                // no remapping, no delays
    tty.c_cc[VMIN]  = 1;                            // read blocks
    tty.c_cc[VTIME] = 1;                            // timeout 0.1s

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);              // no parity
    tty.c_cflag &= ~CSTOPB;                         // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                        // no hardware flow control

    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0) {
        std::cerr << "[UART] Error setting termios attributes" << std::endl;
        return false;
    }

    return true;
}

void StepMotorController::sendCommand(const std::string& cmd)
{
    if (uart_fd < 0)
    {
        // std::cerr << "[UART] UART not initialized" << std::endl;
        return;
    }
    std::string commandWithNewline = cmd + "\n";
    write(uart_fd, commandWithNewline.c_str(), commandWithNewline.size());
}