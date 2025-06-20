
// SystemStatus.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Pos2D
{
    long long x = 0;
    long long y = 0;

    bool isValid() const
    {
        return !(x == 0 && y == 0);
    }
};

enum class MFRMode : unsigned int
{
    STOP = 1,
    ROTATE = 2
};
enum class LauncherMode : unsigned int
{
    STOP = 0,
    MOVE = 1,
    WAR = 2
};

struct MFRStatus
{
    unsigned int mfrId = 0;
    MFRMode mode = MFRMode::STOP;
    double degree = 0.0;
    Pos2D position;
    long long height;
};

struct LSStatus
{
    unsigned int launchSystemId = 0;
    LauncherMode mode = LauncherMode::STOP;
    double launchAngle = 0.0;
    Pos2D position;
    long long height = 0;
    int speed = 0;

};

struct LCStatus
{
    unsigned int LCId = 0;
    Pos2D position;
    long long height;

    unsigned long long calculated_time;
};

struct MissileStatus
{
    unsigned int id;
    long long posX;
    long long posY;
    long long altitude;
    int speed;
    double angle;
    unsigned long long detectTime;
    unsigned long long interceptTime;
    bool hit;
};

struct TargetStatus
{
    unsigned int id;
    long long posX;
    long long posY;
    long long altitude;
    int speed;
    double angle1;
    double angle2; // 고도각
    unsigned long long detectTime;
    uint8_t priority;
    bool hit;
};

struct SystemStatus
{
    MFRStatus mfr;
    LSStatus ls;
    LCStatus lc;
    std::vector<MissileStatus> missiles;
    std::vector<TargetStatus> targets;
};
