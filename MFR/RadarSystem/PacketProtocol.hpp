#pragma once
#include <cstdint>
#pragma pack(push, 1)

struct PacketHeader 
{
    uint8_t cmdType;
};

struct Pos2D
{
    long long x;
    long long y;
};

struct SimData 
{
    uint32_t mockId;
    float x;
    float y;
    float z;
    float angle;
    int speed;
};

enum PacketType : uint8_t 
{
    SIM_TARGET_DATA = 0,
    SIM_MISSILE_DATA = 1,
    STATUS_REQ = 0x02,
    STATUS_RESP = 0x03,
    MODE_CHANGE = 0x04
};

enum RadarMode : uint8_t
{
    ROTATION_MODE = 0,
    ANGLE_MODE
};

struct MfrStatus
{
    uint8_t radarId;
    Pos2D radarPos;
    RadarMode radarMode;
    float radarAngle;    
};

struct MockTarget {
    unsigned int id;
    float targetX;
    float targetY;
    float targetZ;
    float targetAngle;
    int targetSpeed;    
};

#pragma once

struct MockMissile {
    unsigned int id;
    float missileX;
    float missileY;
    float missileZ;
    float missileAngle;
    int missileSpeed;    
};
