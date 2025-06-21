// StatusLoader.cpp
/*#include "StatusLoader.h"
#include "INIReader.h"
#include <iostream>
SystemStatus loadSystemStatus(const std::string& iniPath) {
    INIReader reader(iniPath);
    SystemStatus status;

    if (reader.ParseError() < 0)
    {
        std::cerr << "[ERROR] " << iniPath << " 불러오기 실패\n";
        return status;
    }

    status.mfr.mfrId = reader.GetInteger("MFR", "mfrId", 0);
    status.mfr.mode = static_cast<MFRMode>(reader.GetInteger("MFR", "mode", 0));
    status.mfr.degree = reader.GetReal("MFR", "degree", 0.0);
    status.mfr.position.x = reader.GetInteger("MFR", "posX", 0);
    status.mfr.position.y = reader.GetInteger("MFR", "posY", 0);

    status.ls.launchSystemId = reader.GetInteger("LS", "launchSystemId", 0);
    status.ls.mode = static_cast<LauncherMode>(reader.GetInteger("LS", "mode", 0));
    status.ls.launchAngle = reader.GetReal("LS", "launchAngle", 0.0);
    status.ls.position.x = reader.GetInteger("LS", "posX", 0);
    status.ls.position.y = reader.GetInteger("LS", "posY", 0);

    status.lc.LCId = reader.GetInteger("LC", "LCId", 0);
    status.lc.position.x = reader.GetInteger("LC", "posX", 0);
    status.lc.position.y = reader.GetInteger("LC", "posY", 0);

    for (int i = 0; i < 2; ++i)
    {
        std::string section = "Missile_" + std::to_string(i);
        if (!reader.HasSection(section)) break;

        MissileStatus m;
        m.id = reader.GetInteger(section, "id", 0);
        m.posX = reader.GetInteger(section, "posX", 0);
        m.posY = reader.GetInteger(section, "posY", 0);
        m.altitude = reader.GetInteger(section, "altitude", 0);
        m.speed = reader.GetReal(section, "speed", 0.0);
        m.angle = reader.GetReal(section, "angle", 0.0);
        m.interceptTime = reader.GetInteger(section, "interceptTime", 0);
        m.detectTime = reader.GetInteger(section, "interceptTime", 0);
        m.hit = reader.GetBoolean(section, "hit", false);

        status.missiles.push_back(m);
    }

    for (int i = 0; i < 50; ++i)
    {
        std::string section = "Target_" + std::to_string(i);
        if (!reader.HasSection(section)) break;

        TargetStatus t;
        t.id = reader.GetInteger(section, "id", 0);
        t.posX = reader.GetInteger(section, "posX", 0);
        t.posY = reader.GetInteger(section, "posY", 0);
        t.altitude = reader.GetInteger(section, "altitude", 0);
        t.speed = reader.GetReal(section, "speed", 0.0);
        t.angle1 = reader.GetReal(section, "angle", 0.0);
        t.angle2 = reader.GetReal(section, "angle", 0.0);
        t.detectTime = reader.GetInteger(section, "detectTime", 0);
        t.priority = reader.GetInteger(section, "priority", 0);
        t.hit = reader.GetBoolean(section, "hit", false);

        status.targets.push_back(t);
    }

    return status;
}

*/