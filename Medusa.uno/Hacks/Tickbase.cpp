
#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../includes.hpp"
#include "Tickbase.h"
#include "AntiAim.h"
#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../Hacks/EnginePrediction.h"
#include "../SDK/Input.h"
#include "../SDK/Prediction.h"
UserCmd* command;
static bool doDefensive{ false };
void Tickbase::getCmd(UserCmd* cmd)
{
    command = cmd;
}

void Tickbase::start(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
    {
        hasHadTickbaseActive = false;
        return;
    }

    if (const auto netChannel = interfaces->engine->getNetworkChannel(); netChannel)
        if (netChannel->chokedPackets > chokedPackets)
            chokedPackets = netChannel->chokedPackets;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
    {
        hasHadTickbaseActive = false;
        return;
    }

    if (!config->tickbase.doubletap.isActive() && !config->tickbase.hideshots.isActive())
    {
        if (hasHadTickbaseActive)
            shiftOffensive(cmd, ticksAllowedForProcessing, true);
        hasHadTickbaseActive = false;
        return;
    }

    if (config->tickbase.doubletap.isActive() && activeWeapon->isKnife())
        targetTickShift = 12;
    else if (config->tickbase.doubletap.isActive())
        targetTickShift = 13;
    else if (config->tickbase.hideshots.isActive() && !config->tickbase.doubletap.isActive())
        targetTickShift = ((*memory->gameRules)->isValveDS()) ? 6 : 9;

    targetTickShift = std::clamp(targetTickShift, 0, maxUserCmdProcessTicks - 1);
    hasHadTickbaseActive = true;
}

void Tickbase::end(UserCmd* cmd, bool sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    auto weapon = localPlayer->getActiveWeapon();
    if (!weapon)
        return;

    if (!config->tickbase.doubletap.isActive() && !config->tickbase.hideshots.isActive())
    {
        targetTickShift = 0;
        return;
    }

    if (weapon->isKnife() && cmd->buttons & UserCmd::IN_ATTACK2 && config->tickbase.doubletap.isActive())
        shiftOffensive(cmd, targetTickShift);
    if (cmd->buttons & UserCmd::IN_ATTACK && config->tickbase.doubletap.isActive())
        shiftOffensive(cmd, targetTickShift);
    else if (cmd->buttons & UserCmd::IN_ATTACK && config->tickbase.hideshots.isActive())
        shiftOffensive(cmd, targetTickShift);
}

bool Tickbase::shiftOffensive(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShiftDT(shiftAmount, forceShift))
        return false;

    auto weapon = localPlayer->getActiveWeapon();
    if (weapon->itemDefinitionIndex2() == WeaponId::Revolver)
        return false;

    realTime = memory->globalVars->realtime;
    shiftedTickbase = shiftAmount;
    shiftCommand = cmd->commandNumber;
    tickShift = shiftAmount;
    return true;
}

bool Tickbase::shiftDefensive(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShiftDT(shiftAmount, forceShift))
        return false;

    auto weapon = localPlayer->getActiveWeapon();
    if (weapon->itemDefinitionIndex2() == WeaponId::Revolver)
        return false;

    realTime = memory->globalVars->realtime;
    shiftedTickbase = shiftAmount;
    shiftCommand = cmd->commandNumber;
    tickShift = shiftAmount;
    for (int i = 0; i < shiftAmount; i++)
    {
        ++memory->clientState->netChannel->chokedPackets;
    }
    return true;
}

bool Tickbase::shiftHideShots(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShiftHS(shiftAmount, forceShift))
        return false;

    //tickShift = shiftAmount;
    shiftedTickbase = shiftAmount;
    return true;
}

bool Tickbase::canRun() noexcept
{
    static float spawnTime = 0.f;
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
    {
        ticksAllowedForProcessing = 0;
        chokedPackets = 0;
        pauseTicks = 0;
        return true;
    }

    if (!localPlayer || !localPlayer->isAlive() || !targetTickShift)
    {
        ticksAllowedForProcessing = 0;
        return true;
    }

    if ((*memory->gameRules)->freezePeriod())
    {
        realTime = memory->globalVars->realtime;
        return true;
    }

    if (spawnTime != localPlayer->spawnTime())
    {
        spawnTime = localPlayer->spawnTime();
        ticksAllowedForProcessing = 0;
        pauseTicks = 0;
    }

    if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
    {
        realTime = memory->globalVars->realtime;
        shiftedTickbase = 0;
        shiftCommand = 0;
        tickShift = 0;
        return true;
    }
    auto data = localPlayer.get()->getActiveWeapon();
    float recharge_time = 0;
    recharge_time = 0.24825f;

    if ((ticksAllowedForProcessing < targetTickShift || chokedPackets > maxUserCmdProcessTicks - targetTickShift) && memory->globalVars->realtime - realTime > recharge_time)
    {       
        ticksAllowedForProcessing = min(ticksAllowedForProcessing++, maxUserCmdProcessTicks);
        chokedPackets = max(chokedPackets--, 0);
        pauseTicks++;
        return false;
    }
    return true;
}

bool Tickbase::canShiftDT(int shiftAmount, bool forceShift) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;

    if (!shiftAmount || shiftAmount > ticksAllowedForProcessing || memory->globalVars->realtime - realTime <= 0.5f)
        return false;

    if (forceShift)
        return true;

    if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
        return false;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->isGrenade() || activeWeapon->isBomb()
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Healthshot)
        return false;

    const float shiftTime = (localPlayer->tickBase() - shiftAmount) * memory->globalVars->intervalPerTick;
    if (localPlayer->nextAttack() > shiftTime)
        return false;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return false;

    return activeWeapon->nextPrimaryAttack() <= shiftTime;
}

bool Tickbase::canShiftHS(int shiftAmount, bool forceShift) noexcept
{
    if (config->tickbase.doubletap.isActive())
        return false;

    if (!localPlayer || !localPlayer->isAlive())
        return false;

    if (!shiftAmount || shiftAmount > ticksAllowedForProcessing)
        return false;

    if (forceShift)
        return true;

    if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
        return false;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->isKnife() ||
        activeWeapon->isGrenade() || activeWeapon->isBomb()
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Healthshot)
        return false;

    const float shiftTime = (localPlayer->tickBase() - shiftAmount) * memory->globalVars->intervalPerTick;
    if (localPlayer->nextAttack() > shiftTime)
        return false;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return false;

    return activeWeapon->nextPrimaryAttack() <= shiftTime;
}

int Tickbase::getCorrectTickbase(int commandNumber) noexcept
{
    const int tickBase = localPlayer->tickBase();

    if (commandNumber == shiftCommand)
        return tickBase - shiftedTickbase;
    else if (commandNumber == shiftCommand + 1)
    {
        if (!config->tickbase.teleport)
            return tickBase + shiftedTickbase;
        return tickBase;
    }
    if (pauseTicks)
        return tickBase + pauseTicks;
    return tickBase;
}


//If you have dt enabled, you need to shift 13 ticks, so it will return 13 ticks
//If you have hs enabled, you need to shift 9 ticks, so it will return 7 ticks
int Tickbase::getTargetTickShift() noexcept
{
    return targetTickShift;
}

int Tickbase::getTickshift() noexcept
{
    return tickShift;
}

void Tickbase::resetTickshift() noexcept
{
    shiftedTickbase = tickShift;
    if (config->tickbase.teleport && config->tickbase.doubletap.isActive())
    {
        ticksAllowedForProcessing = max(ticksAllowedForProcessing - tickShift, 0);
    }
    tickShift = 0;
}

int& Tickbase::pausedTicks() noexcept
{
    return pauseTicks;
}

bool& Tickbase::isFinalTick() noexcept
{
    return finalTick;
}

bool& Tickbase::isShifting() noexcept
{
    return shifting;
}

void Tickbase::updateInput() noexcept
{
    config->tickbase.doubletap.handleToggle();
    config->tickbase.hideshots.handleToggle();
}

void Tickbase::reset() noexcept
{
    hasHadTickbaseActive = false;
    pauseTicks = 0;
    chokedPackets = 0;
    tickShift = 0;
    shiftCommand = 0;
    shiftedTickbase = 0;
    ticksAllowedForProcessing = 0;
    realTime = 0.0f;
}