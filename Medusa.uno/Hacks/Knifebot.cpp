#include "AimbotFunctions.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Knifebot.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"
#include "Resolver.h"

bool InRange(float x, float one, float two)
{
    return x >= one && x <= two;
}

inline float get_distance(const Vector& start, const Vector& end)
{
    float distance = sqrt((start - end).length());

    if (distance < 1.0f)
        distance = 1.0f;

    return distance;
}

void runKnifebot(UserCmd* cmd) noexcept
{
    if (!config->ragebot.knifebot)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->isKnife())
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return;

    const auto& localPlayerOrigin = localPlayer->origin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    const auto& aimPunch = localPlayer->getAimPunch();

    auto bestDistance{ FLT_MAX };
    Entity* bestTarget{ };
    float bestSimulationTime = 0;
    Vector absAngle{ };
    Vector origin{ };
    Vector bestTargetPosition{ };

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
            || !entity->isOtherEnemy(localPlayer.get()) || entity->gunGameImmunity())
            continue;

        const auto player = Animations::getPlayer(i);
        if (!player.gotMatrix)
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupMins = entity->getCollideable()->obbMins();
        Vector backupMaxs = entity->getCollideable()->obbMaxs();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        auto distance{ localPlayerOrigin.distTo(player.origin) };

        if (distance < bestDistance) {
            bestDistance = distance;
            bestTarget = entity;
            absAngle = player.absAngle;
            origin = player.origin;
            bestSimulationTime = player.simulationTime;
            bestTargetPosition = player.matrix[7].origin();
        }

        if (!config->backtrack.enabled)
            continue;

        const auto records = Animations::getBacktrackRecords(entity->index());
        if (!records || records->empty())
            continue;

        int lastTick = -1;

        for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
        {
            if (Backtrack::valid(records->at(i).simulationTime))
            {
                lastTick = i;
                break;
            }
            else {
                lastTick = -1;
                break;
            }
        }

        if (lastTick <= -1)
            continue;

        const auto record = records->at(lastTick);

        distance = localPlayerOrigin.distTo(record.origin);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestTarget = entity;
            absAngle = record.absAngle;
            origin = record.origin;
            bestSimulationTime = record.simulationTime;
            bestTargetPosition = record.matrix[7].origin();
        }
    }

    if (!bestTarget || bestDistance >= 78.69420f)
        return;

    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bestTarget->getBonePosition(7), cmd->viewangles) };
    const bool firstSwing = (localPlayer->nextPrimaryAttack() + 0.4) < memory->globalVars->serverTime();

    Vector targetForward = Vector::fromAngle(absAngle);
    targetForward.z = 0;

    Vector vecLOS = (origin - localPlayer->origin());
    vecLOS.z = 0;
    vecLOS.normalize();

    float dot = vecLOS.dotProduct(targetForward);

    auto hp = bestTarget->health();
    auto armor = bestTarget->armor() > 1;

    int minDmgSol;
    int minDmgSag;

    if (armor)
    {
        minDmgSag = 55;
        minDmgSol = 30;
    }
    else
    {
        minDmgSag = 65;
        minDmgSol = 35;
    }
    if (hp <= minDmgSol)
    {
        cmd->buttons |= UserCmd::IN_ATTACK;
    }
    else if (hp <= minDmgSag)
    {
        if (bestDistance <= 64.f)
            cmd->buttons |= UserCmd::IN_ATTACK2;
    }
    else
    {
        cmd->buttons |= UserCmd::IN_ATTACK;
    }

    cmd->viewangles += angle;
    cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());
}

void Knifebot::run(UserCmd* cmd) noexcept
{
    if(config->ragebot.knifebot)
        runKnifebot(cmd);
}