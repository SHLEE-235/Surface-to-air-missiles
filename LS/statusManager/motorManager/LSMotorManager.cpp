#include "LSMotorManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>
LSMotorManager::LSMotorManager(std::string& device, int& uartBaudRate)
{
    this->device = device;
    this->uartBaudRate = static_cast<speed_t>(uartBaudRate);
    if(!initUart())
    {
        std::cerr << "[StepMotorController::initUart] Uart Init Failed" << std::endl;
    }
}

LSMotorManager::~LSMotorManager()
{
    if (uart_fd >= 0)
    {
        close(uart_fd);
    }
}

bool LSMotorManager::initUart()
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

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN] = 1;                         // read blocks
    tty.c_cc[VTIME] = 1;                        // timeout 0.1s

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);      // no parity
    tty.c_cflag &= ~CSTOPB;                 // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                // no hardware flow control

    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0)
    {
        std::cerr << "[UART] Error setting termios attributes" << std::endl;
        return false;
    }

    return true;
}


bool LSMotorManager::rotateToAngle(double targetAngle)
{
    if (uart_fd < 0)
    {
        std::cerr << "[UART] UART not initialized" << std::endl;
        return false;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << targetAngle;
    std::string cmd = "STOP_MODE:" + oss.str() + "\n";
    write(uart_fd, cmd.c_str(), cmd.size());
    return true;
}