#include "EnginePrediction.h"
#include "Fakelag.h"
#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Localplayer.h"
#include "../SDK/Vector.h"
#include "AntiAim.h"

void Fakelag::run(bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config->condAA.global)
        return;

    auto chokedPackets = 0;

    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;

    if (AntiAim::getDidShoot()) {
        sendPacket = true;
        return;
    }

    int tick_to_choke = 1;
        const float speed = EnginePrediction::getVelocity().length2D() >= 2.0f ? EnginePrediction::getVelocity().length2D() : 0.0f;
        switch (config->fakelag.mode) {
        case 0: //Static
            if (config->tickbase.doubletap.isActive() || config->tickbase.hideshots.isActive() && memory->globalVars->realtime - Tickbase::realTime > 0.24625f
                && localPlayer && localPlayer->isAlive()
                && localPlayer->getActiveWeapon()
                && localPlayer->getActiveWeapon()->nextPrimaryAttack() <= (localPlayer->tickBase() - Tickbase::getTargetTickShift()) * memory->globalVars->intervalPerTick
                && (config->misc.fakeduck && !config->misc.fakeduckKey.isActive()))
                chokedPackets = 1;
            else
            chokedPackets = config->fakelag.limit;
            break;
        case 1: //Adaptive
            if (config->tickbase.doubletap.isActive() || config->tickbase.hideshots.isActive() && memory->globalVars->realtime - Tickbase::realTime > 0.24625f
                && localPlayer && localPlayer->isAlive()
                && localPlayer->getActiveWeapon()
                && localPlayer->getActiveWeapon()->nextPrimaryAttack() <= (localPlayer->tickBase() - Tickbase::getTargetTickShift()) * memory->globalVars->intervalPerTick
                && (config->misc.fakeduck && !config->misc.fakeduckKey.isActive()))
                chokedPackets = 1;
            else
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (speed * memory->globalVars->intervalPerTick))), 1, config->fakelag.limit);
            break;
        case 2: // Random
            srand(static_cast<unsigned int>(time(NULL)));
            if (config->tickbase.doubletap.isActive() || config->tickbase.hideshots.isActive() && memory->globalVars->realtime - Tickbase::realTime > 0.24625f
                && localPlayer && localPlayer->isAlive()
                && localPlayer->getActiveWeapon()
                && localPlayer->getActiveWeapon()->nextPrimaryAttack() <= (localPlayer->tickBase() - Tickbase::getTargetTickShift()) * memory->globalVars->intervalPerTick
                && (config->misc.fakeduck && !config->misc.fakeduckKey.isActive()))
                chokedPackets = 1;
            else
            chokedPackets = rand() % config->fakelag.limit + 1;
            break;
        }

        chokedPackets = std::clamp(chokedPackets, 0, maxUserCmdProcessTicks - Tickbase::getTargetTickShift());

        sendPacket = netChannel->chokedPackets >= chokedPackets; 
}