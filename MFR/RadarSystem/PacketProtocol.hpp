#pragma once
#include <cstdint>
#pragma pack(push, 1)

struct PacketHeader 
{
    unsigned char cmdType;
};

struct Pos2D 
{
    long long x;
    long long y;
};

struct Pos3D
{
    long long x;
    long long y;
    long long z;
};

struct SimData // 4 + (8*3) + 4 + 8 = 40 bytes
{
    unsigned int mockId;
    Pos3D mockCoords;
    int speed;
    double angle;    
};

struct MfrToLcMissileInfo   // 4 + (8*3) + 4 + 8 + 8 + 8 + 1 = 57 bytes
{
    unsigned int id;
    Pos3D missileCoords;
    int missileSpeed;
    double missileAngle;
    unsigned long long firstDetectionTime;
    unsigned long long timeToIntercept;
    bool isHit;
};

struct MfrToLcTargetInfo    // 4 + (8*3) + 4 + 8 + 8 + 1 + 1 = 50 bytes
{
    unsigned int id;       
    Pos3D targetCoords;
    int targetSpeed;
    double targetAngle;
    unsigned long long firstDetectionTime;
    unsigned char prioirty;
    bool isHit;
};


enum recvPacketType : uint8_t
{
    SIM_MOCK_DATA = 0x01,
    STATUS_REQ = 0x11,
    MODE_CHANGE = 0x12,
    LC_INIT_RES = 0x13
};

enum sendPacketType : uint8_t 
{
    STATUS_RES = 0x21,
    DETECTED_INFO = 0x22,
    LC_INIT_REQ = 0x23
};

enum RadarMode : uint8_t 
{
    ROTATION_MODE = 0,
    ANGLE_MODE
};

struct MfrStatus 
{
    unsigned int radarId;
    Pos2D radarPos;
    RadarMode radarMode;
    double radarAngle;    
};

struct LcInitData
{
    unsigned int radarId;
    Pos2D lcCoord;
};

struct ReqLcInitData
{
    unsigned int radarId;
};

struct ModeCahngeData
{
    unsigned int radarId;
    unsigned char modeData;
    unsigned char isPriorityMode;
    unsigned int targetId;
};