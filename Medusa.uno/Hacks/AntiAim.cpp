#include "../Interfaces.h"
#include <random>
#include "AimbotFunctions.h"
#include "AntiAim.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"
#include "../GameData.h"
#include "../Memory.h"
#include "../SDK/Engine.h"
#include "../Netvars.h"
#include "../SDK/Entity.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
#include "Tickbase.h"
#include "../includes.hpp"
static bool isShooting{ false };
static bool didShoot{ false };
static float lastShotTime{ 0.f };

enum e_game_phase {
    GAMEPHASE_WARMUP_ROUND,
    GAMEPHASE_PLAYING_STANDARD,
    GAMEPHASE_PLAYING_FIRST_HALF,
    GAMEPHASE_PLAYING_SECOND_HALF,
    GAMEPHASE_HALFTIME,
    GAMEPHASE_MATCH_ENDED,
    GAMEPHASE_MAX
};

bool updateLby(bool update = false) noexcept
{
    static float timer = 0.f;
    static bool lastValue = false;

    if (!update)
        return lastValue;

    if (!(localPlayer->flags() & 1) || !localPlayer->getAnimstate())
    {
        lastValue = false;
        return false;
    }

    if (localPlayer->velocity().length2D() > 0.1f || fabsf(localPlayer->velocity().z) > 100.f)
        timer = memory->globalVars->serverTime() + 0.22f;

    if (timer < memory->globalVars->serverTime())
    {
        timer = memory->globalVars->serverTime() + 1.1f;
        lastValue = true;
        return true;
    }
    lastValue = false;
    return false;
}

bool autoDirection(Vector eyeAngle) noexcept
{
    constexpr float maxRange{ 8192.0f };

    Vector eye = eyeAngle;
    eye.x = 0.f;
    Vector eyeAnglesLeft45 = eye;
    Vector eyeAnglesRight45 = eye;
    eyeAnglesLeft45.y += 45.f;
    eyeAnglesRight45.y -= 45.f;


    eyeAnglesLeft45.toAngle();

    Vector viewAnglesLeft45 = {};
    viewAnglesLeft45 = viewAnglesLeft45.fromAngle(eyeAnglesLeft45) * maxRange;

    Vector viewAnglesRight45 = {};
    viewAnglesRight45 = viewAnglesRight45.fromAngle(eyeAnglesRight45) * maxRange;

    static Trace traceLeft45;
    static Trace traceRight45;

    Vector startPosition{ localPlayer->getEyePosition() };

    interfaces->engineTrace->traceRay({ startPosition, startPosition + viewAnglesLeft45 }, 0x4600400B, { localPlayer.get() }, traceLeft45);
    interfaces->engineTrace->traceRay({ startPosition, startPosition + viewAnglesRight45 }, 0x4600400B, { localPlayer.get() }, traceRight45);

    float distanceLeft45 = sqrtf(powf(startPosition.x - traceRight45.endpos.x, 2) + powf(startPosition.y - traceRight45.endpos.y, 2) + powf(startPosition.z - traceRight45.endpos.z, 2));
    float distanceRight45 = sqrtf(powf(startPosition.x - traceLeft45.endpos.x, 2) + powf(startPosition.y - traceLeft45.endpos.y, 2) + powf(startPosition.z - traceLeft45.endpos.z, 2));

    float mindistance = min(distanceLeft45, distanceRight45);

    if (distanceLeft45 == mindistance)
        return false;
    return true;
}

float RandomFloat(float a, float b, float multiplier) {
    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    float result = a + r;
    return result * multiplier;
}

static bool invert = true;
void AntiAim::rage(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    static bool willgetstabbed = false;
    if (!config->condAA.global)
    {
        return;
    }
    /*breakers*/
    if ((config->condAA.animBreakers & 1 << 0) == 1 << 0)
        localPlayer->setPoseParameter(1, 6);
    else
        localPlayer->setPoseParameter(0, 6);
    /*if ((config->condAA.animBreakers & 1 << 1) == 1 << 1)
    {
        localPlayer->setPoseParameter(1, 14);
        localPlayer->setPoseParameter(1, 15);
    }
    if ((config->condAA.animBreakers & 1 << 2) == 1 << 2)
    {
        localPlayer->poseParameters().data()[8] = 0;
        localPlayer->poseParameters().data()[9] = 0;
        localPlayer->poseParameters().data()[10] = 0;
    }*/
    auto moving_flag1{ get_moving_flag(cmd) };
    if ((cmd->viewangles.x == currentViewAngles.x || Tickbase::isShifting()))
    {
        if (willgetstabbed)
            cmd->viewangles.x = 0.f;
        else
        {
            switch (config->rageAntiAim[static_cast<int>(moving_flag1)].pitch)
            {
            case 0: //None
                break;
            case 1: //Down
                cmd->viewangles.x = 89.f;
                break;
            case 2: //Zero
                cmd->viewangles.x = 0.f;
                break;
            case 3: //Up
                cmd->viewangles.x = -89.f;
                break;
            case 4: //Fake pitch
            {
                float pitch = 89.f;
                if (memory->globalVars->tickCount % 20 == 0)
                {
                    pitch = -89.f;
                }
                else
                    pitch = 89.f;
                cmd->viewangles.x = pitch;
                break;
            }
            case 5: //random
            {
                float pitch = round(RandomFloat(-89, 89, 1.f));
                cmd->viewangles.x = pitch;
                break;
            }
            default:
                break;
            }
        }
    }
    if (cmd->viewangles.y == currentViewAngles.y || Tickbase::isShifting())
    {
        if (config->rageAntiAim[static_cast<int>(moving_flag1)].yawBase != Yaw::off)   //AntiAim
        {
            const bool forward = config->manualForward.isActive();
            const bool back = config->manualBackward.isActive();
            const bool right = config->manualRight.isActive();
            const bool left = config->manualLeft.isActive();
            const bool isManualSet = forward || back || right || left;
            float yaw = 0.f;
            static float staticYaw = 0.f;
            static bool flipJitter = false;
            bool isFreestanding{ false };
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].atTargets && localPlayer->moveType() != MoveType::LADDER && !willgetstabbed)
            {
                Vector localPlayerEyePosition = localPlayer->getEyePosition();
                const auto aimPunch = localPlayer->getAimPunch();
                float bestFov = 255.f;
                float yawAngle = 0.f;
                for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                    const auto entity{ interfaces->entityList->getEntity(i) };
                    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                        || !entity->isOtherEnemy(localPlayer.get()))
                        continue;

                    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, entity->getAbsOrigin(), cmd->viewangles + aimPunch) };
                    const auto fov{ angle.length2D() };
                    if (fov < bestFov)
                    {
                        yawAngle = angle.y;
                        bestFov = fov;
                    }
                }
                yaw = yawAngle;
            }
            if (localPlayer->moveType() != MoveType::LADDER || localPlayer->moveType() != MoveType::NOCLIP)
            {
                Vector localPlayerEyePosition = localPlayer->getEyePosition();
                const auto aimPunch = localPlayer->getAimPunch();
                float bestFov = 255.f;
                float yawAngle = 0.f;
                for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                    const auto entity{ interfaces->entityList->getEntity(i) };
                    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                        || !entity->isOtherEnemy(localPlayer.get()))
                    {
                        willgetstabbed = false;
                        continue;
                    }

                    auto enemyDist = entity->getAbsOrigin().distTo(localPlayer->getAbsOrigin());
                    if (enemyDist > 469)
                    {
                        willgetstabbed = false;
                        continue;
                    }

                    if (!entity->getActiveWeapon())
                    {
                        willgetstabbed = false;
                        continue;
                    }

                    if (entity->getActiveWeapon()->isKnife())
                    {
                        const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, entity->getAbsOrigin(), cmd->viewangles) };
                        const auto fov{ angle.length2D() };
                        if (fov < bestFov)
                        {
                            yawAngle = angle.y;
                            bestFov = fov;
                        }
                        yaw = yawAngle;
                        switch (config->rageAntiAim[static_cast<int>(moving_flag1)].yawBase)
                        {
                        case Yaw::forward:
                            yaw += 0.f;
                            break;
                        case Yaw::backward:
                            yaw -= 180.f;
                            break;
                        case Yaw::right:
                            yaw -= -90.f;
                            break;
                        case Yaw::left:
                            yaw -= 90.f;
                            break;
                        default:
                            break;
                        }
                        willgetstabbed = true;
                    }
                    else
                        willgetstabbed = false;
                }
            }
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].yawModifier != 6)
                staticYaw = 0.f;

            switch (config->rageAntiAim[static_cast<int>(moving_flag1)].yawBase)
            {
            case Yaw::forward:
                yaw += 0.f;
                break;
            case Yaw::backward:
                yaw += 180.f;
                break;
            case Yaw::right:
                yaw += -90.f;
                break;
            case Yaw::left:
                yaw += 90.f;
                break;
            default:
                break;
            }

            if (!willgetstabbed)
            {
                if (back) {
                    yaw = -180.f;
                    if (left)
                        yaw -= 45.f;
                    else if (right)
                        yaw += 45.f;
                }
                else if (left) {
                    yaw = 90.f;
                    if (back)
                        yaw += 45.f;
                    else if (forward)
                        yaw -= 45.f;
                }
                else if (right) {
                    yaw = -90.f;
                    if (back)
                        yaw -= 45.f;
                    else if (forward)
                        yaw += 45.f;
                }
                else if (forward) {
                    yaw = 0.f;
                }
            }

            if (config->rageAntiAim[static_cast<int>(moving_flag1)].fakeFlick && config->fakeFlickOnKey.isActive() && !willgetstabbed)
            {
                auto rate = config->rageAntiAim[static_cast<int>(moving_flag1)].fakeFlickRate;
                auto tick = (int)memory->globalVars->tickCount % rate;
                if (tick == 0 && !config->flipFlick.isActive())
                    yaw += 90;
                else if (tick == 0 && config->flipFlick.isActive())
                    yaw -= 90;
                else
                    yaw = yaw;
            }
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].freestand && config->freestandKey.isActive() && !willgetstabbed)
            {
                constexpr std::array positions = { -30.0f, 0.0f, 30.0f};
                std::array active = { false, false, false };
                const auto fwd = Vector::fromAngle2D(cmd->viewangles.y);
                const auto side = fwd.crossProduct(Vector::up());

                for (std::size_t i{}; i < positions.size(); i++)
                {
                    const auto start = localPlayer->getEyePosition() + side * positions[i];
                    const auto end = start + fwd * 100.0f;

                    Trace trace{};
                    interfaces->engineTrace->traceRay({ start, end }, 0x1 | 0x2, nullptr, trace);

                    if (trace.fraction != 1.0f)
                        active[i] = true;
                }

                if (active[0] && active[1] && !active[2])
                {
                    yaw = 90.f;
                    auto_direction_yaw = -1;
                    isFreestanding = true;
                }
                else if (!active[0] && active[1] && active[2])
                {
                    yaw = -90.f;
                    auto_direction_yaw = 1;
                    isFreestanding = true;
                }
                else
                {
                    auto_direction_yaw = 0;
                    isFreestanding = false;
                }
            }
            if (sendPacket && !AntiAim::getDidShoot())
                flipJitter ^= 1;
            switch (config->rageAntiAim[static_cast<int>(moving_flag1)].yawModifier)
            {
            case 0:
                break;
            case 1: //Jitter centered
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
                {
                    if (config->rageAntiAim[static_cast<int>(moving_flag1)].desync && config->rageAntiAim[static_cast<int>(moving_flag1)].peekMode == 3)
                        yaw -= invert ? config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange : -config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                    else
                        yaw -= flipJitter ? config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange : -config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                }
                break;
            case 2 : // jitter offset
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
                {
                    yaw += flipJitter ? config->invert.isActive() ? config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange : -config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange : 0;
                }
                break;
            case 3:
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
                {                
                    yaw += round(RandomFloat(-config->rageAntiAim[moving_flag1].randomRange, config->rageAntiAim[moving_flag1].randomRange, 1.f));
                }
                break;
            case 4:
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
            {
                static int stage = 0;
                if (stage == 0)
                {
                    yaw -= config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                    stage = 1;
                }
                else if (stage == 1)
                {
                    yaw += 0;
                    stage = 2;
                }
                else if (stage == 2)
                {
                    yaw += config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                    stage = 0;
                }
            }
                break;
            case 5:
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
            {
                static int stage = 0;
                if (stage == 0)
                {
                    yaw -= config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                    stage = 1;
                }
                else if (stage == 1)
                {
                    yaw -= config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange / 2;
                    stage = 2;
                }
                else if (stage == 2)
                {
                    yaw += 0;
                    stage = 3;
                }
                else if (stage == 3)
                {
                    yaw += config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange / 2;
                    stage = 4;
                }
                else if (stage == 4)
                {
                    yaw += config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                    stage = 0;
                }
            }
            break;
            case 6: //spin           
                if (!isManualSet && isFreestanding == false && !willgetstabbed) 
                {
                    staticYaw += config->rageAntiAim[static_cast<int>(moving_flag1)].spinBase;
                    yaw += staticYaw;
                }
                break;
            case 7:
                if (!isManualSet && isFreestanding == false && !willgetstabbed)
                {
                    yaw -= memory->globalVars->tickCount % config->rageAntiAim[static_cast<int>(moving_flag1)].tickDelays - 1 == 0 ? config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange : -config->rageAntiAim[static_cast<int>(moving_flag1)].jitterRange;
                }
                break;
            default:
                break;
            }
            if (!isManualSet && isFreestanding == false && !willgetstabbed)
                yaw += static_cast<float>(config->rageAntiAim[static_cast<int>(moving_flag1)].yawAdd);
            cmd->viewangles.y += yaw;
        }
        if (config->rageAntiAim[static_cast<int>(moving_flag1)].desync && !Tickbase::isShifting())
        {
            float rollOffsetAngle = config->rageAntiAim[static_cast<int>(moving_flag1)].roll.offset;
            float PitchAngle = config->rageAntiAim[static_cast<int>(moving_flag1)].roll.pitch;
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].roll.exploitPitch)
                auto PitchAngle = config->rageAntiAim[static_cast<int>(moving_flag1)].roll.epAmnt + 41225040 * 129600;
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].roll.enabled && localPlayer->velocity().length2D() < 100.f) {
                cmd->viewangles.z = invert ? config->rageAntiAim[static_cast<int>(moving_flag1)].roll.add : (config->rageAntiAim[static_cast<int>(moving_flag1)].roll.add) * -1.f;
            }
            else
                cmd->viewangles.z = 0.f;
            bool isInvertToggled = config->invert.isActive();
            if (config->rageAntiAim[static_cast<int>(moving_flag1)].peekMode != 3)
                invert = isInvertToggled;
            float leftDesyncAngle = config->rageAntiAim[static_cast<int>(moving_flag1)].leftLimit * 2.f;
            float rightDesyncAngle = config->rageAntiAim[static_cast<int>(moving_flag1)].rightLimit * -2.f;
            switch (config->rageAntiAim[static_cast<int>(moving_flag1)].peekMode)
            {
            case 0:
                break;
            case 1: // Peek real
                if(!isInvertToggled)
                    invert = !autoDirection(cmd->viewangles);
                else
                    invert = autoDirection(cmd->viewangles);
                break;
            case 2: // Peek fake
                if (isInvertToggled)
                    invert = !autoDirection(cmd->viewangles);
                else
                    invert = autoDirection(cmd->viewangles);
                break;
            case 3: // Jitter
                if (sendPacket && config->rageAntiAim[static_cast<int>(moving_flag1)].yawModifier != 7)
                    invert = !invert;
                else if (sendPacket && config->rageAntiAim[static_cast<int>(moving_flag1)].yawModifier == 7)
                    memory->globalVars->tickCount% config->rageAntiAim[static_cast<int>(moving_flag1)].tickDelays - 1 == 0 ? invert = !invert : invert = !invert;
                break;
            default:
                break;
            }

            switch (config->rageAntiAim[static_cast<int>(moving_flag1)].lbyMode)
            {
            case 0: // Normal(sidemove)
                if (fabsf(cmd->sidemove) < 5.0f)
                {
                    if (cmd->buttons & UserCmd::IN_DUCK)
                        cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
                    else
                        cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
                }
                break;
            case 1: // Opposite (Lby break)
                if (updateLby())
                {
                    cmd->viewangles.y += !invert ? leftDesyncAngle : rightDesyncAngle;
                    sendPacket = false;
                    return;
                }
                break;
            case 2: //Sway (flip every lby update)
                static bool flip = false;
                if (updateLby())
                {
                    cmd->viewangles.y += !flip ? leftDesyncAngle : rightDesyncAngle;
                    sendPacket = false;
                    flip = !flip;
                    return;
                }
                if (!sendPacket)
                    cmd->viewangles.y += flip ? leftDesyncAngle : rightDesyncAngle;
                break;
            }

            if (sendPacket)
                return;

            cmd->viewangles.y += invert ? leftDesyncAngle : rightDesyncAngle;
        }
    }
}

void AntiAim::updateInput() noexcept
{
    config->freestandKey.handleToggle();
    config->invert.handleToggle();
    config->fakeFlickOnKey.handleToggle();
    config->flipFlick.handleToggle();
    config->manualForward.handleToggle();
    config->manualBackward.handleToggle();
    config->manualRight.handleToggle();
    config->manualLeft.handleToggle();
}

void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    auto moving_flag2{ get_moving_flag(cmd) };
    if (cmd->buttons & (UserCmd::IN_USE))
        return;

    if (localPlayer->moveType() == MoveType::LADDER || localPlayer->moveType() == MoveType::NOCLIP)
        return;

    else if (config->condAA.global || config->rageAntiAim[static_cast<int>(moving_flag2)].desync)
        AntiAim::rage(cmd, previousViewAngles, currentViewAngles, sendPacket);
}

bool AntiAim::canRun(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;
    
    updateLby(true); //Update lby timer

    if ((*memory->gameRules)->freezePeriod())
        return false;

    if (localPlayer->flags() & (1 << 6))
        return false;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return true;

    if (activeWeapon->isThrowing())
        return false;

    if (activeWeapon->isGrenade())
        return true;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto() || localPlayer->waitForNoAttack())
        return true;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime())
        return true;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return true;

    if (activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return true;

    if (localPlayer->nextAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return false;

    if (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return false;
    
    if (activeWeapon->isKnife())
    {
        if (activeWeapon->nextSecondaryAttack() <= memory->globalVars->serverTime() && cmd->buttons & (UserCmd::IN_ATTACK2))
            return false;
    }

    const auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return true;

    return true;
}

float AntiAim::getLastShotTime()
{
    return lastShotTime;
}

bool AntiAim::getIsShooting()
{
    return isShooting;
}

bool AntiAim::getDidShoot()
{
    return didShoot;
}

void AntiAim::setLastShotTime(float shotTime)
{
    lastShotTime = shotTime;
}

void AntiAim::setIsShooting(bool shooting)
{
    isShooting = shooting;
}

void AntiAim::setDidShoot(bool shot)
{
    didShoot = shot;
}