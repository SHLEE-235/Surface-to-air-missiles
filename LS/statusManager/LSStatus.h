#pragma once
#include "info.h"
#include <string>
#include <termios.h>
#pragma pack(push, 1)

struct Pos3D {
    long long x;
    long long y;
    long long z;
};

struct LSStatus {
    unsigned int id;
    Pos3D position;
    double angle;
    int speed;
    OperationMode mode;

    std::string device;
    int uartBaudRate = B9600;
};

#pragma pack(pop)