#pragma once

#include <cstddef>

#include "Pad.h"

struct UserCmd;

struct GlobalVars {
    float realtime; // 0x0000
    int framecount; // 0x0004
    float absoluteFrameTime; // 0x0008
    PAD(4);
    float currenttime;// 0x0010
    float frametime; // 0x0014
    const int maxClients;// 0x0018
    const int tickCount;// 0x001C
    const float intervalPerTick;// 0x0020
    PAD(4);
    int       m_simticksthisframe;            // 0x0028
    float serverTime(UserCmd* = nullptr) const noexcept;
};
