#pragma once

#include "../ConfigStructs.h"

struct UserCmd;
struct Vector;

namespace AntiAim
{
    bool r8Working = false;
    enum moving_flag
    {
        freestanding = 0,
        moving = 1,
        jumping = 2,
        ducking = 3,
        duck_jumping = 4,
        slow_walking = 5,
        on_use = 6
    };
    inline moving_flag latest_moving_flag{};
    void rage(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
    void run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
    void updateInput() noexcept;
    bool canRun(UserCmd* cmd) noexcept;
    inline int auto_direction_yaw{};

    float getLastShotTime();
    bool getIsShooting();
    bool getDidShoot();
    void setLastShotTime(float shotTime);
    void setIsShooting(bool shooting);
    void setDidShoot(bool shot);
}
