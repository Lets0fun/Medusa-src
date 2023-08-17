#include "AimbotFunctions.h"
#include "Animations.h"
#include "Legitbot.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/ModelInfo.h"

void Legitbot::updateInput() noexcept
{
    config->legitbotKey.handleToggle();
}

void Legitbot::run(UserCmd* cmd) noexcept
{
    if (!config->lgb.enabled)
        return;

    if (!config->legitbotKey.isActive())
        return;

    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    const auto& cfg = config->legitbob;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!cfg[weaponIndex].override)
        weaponIndex = weaponClass;

    if (!cfg[weaponIndex].override)
        weaponIndex = 0;

    const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };

    if (config->recoilControlSystem[weaponIndex].enabled && (config->recoilControlSystem[weaponIndex].horizontal || config->recoilControlSystem[weaponIndex].vertical) && aimPunch.notNull())
    {
        static Vector lastAimPunch{ };
        if (localPlayer->shotsFired() > config->recoilControlSystem[weaponIndex].shotsFired)
        {
            if (cmd->buttons & UserCmd::IN_ATTACK)
            {
                Vector currentPunch = aimPunch;

                currentPunch.x *= config->recoilControlSystem[weaponIndex].vertical;
                currentPunch.y *= config->recoilControlSystem[weaponIndex].horizontal;

                if (!config->recoilControlSystem[weaponIndex].silent)
                {
                    cmd->viewangles.y += lastAimPunch.y - currentPunch.y;
                    cmd->viewangles.x += lastAimPunch.x - currentPunch.x;
                    lastAimPunch.y = currentPunch.y;
                    lastAimPunch.x = currentPunch.x;
                }
                else
                {
                    cmd->viewangles.y -= currentPunch.y;
                    cmd->viewangles.x -= currentPunch.x;
                    lastAimPunch = Vector{ };
                }
            }
        }
        else
        {
            lastAimPunch = Vector{ };
        }

        if (!config->recoilControlSystem[weaponIndex].silent)
            interfaces->engine->setViewAngles(cmd->viewangles);
    }

    if (!cfg[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->lgb.ignoreFlash && localPlayer->isFlashed())
        return;

    if (config->lgb.enabled && (cmd->buttons & UserCmd::IN_ATTACK || config->lgb.aimlock)) {

        auto bestFov = config->legitbob[weaponIndex].fov;
        Vector bestTarget{ };
        const auto localPlayerEyePosition = localPlayer->getEyePosition();

        std::array<bool, Hitboxes::Max> hitbox{ false };

        // Head
        hitbox[Hitboxes::Head] = (cfg[weaponIndex].hitboxes & 1 << 0) == 1 << 0;
        // Chest
        hitbox[Hitboxes::UpperChest] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
        hitbox[Hitboxes::Thorax] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
        hitbox[Hitboxes::LowerChest] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
        //Stomach
        hitbox[Hitboxes::Belly] = (cfg[weaponIndex].hitboxes & 1 << 2) == 1 << 2;
        hitbox[Hitboxes::Pelvis] = (cfg[weaponIndex].hitboxes & 1 << 2) == 1 << 2;
        //Arms
        hitbox[Hitboxes::RightUpperArm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
        hitbox[Hitboxes::RightForearm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
        hitbox[Hitboxes::LeftUpperArm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
        hitbox[Hitboxes::LeftForearm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
        //Legs
        hitbox[Hitboxes::RightCalf] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
        hitbox[Hitboxes::RightThigh] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
        hitbox[Hitboxes::LeftCalf] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
        hitbox[Hitboxes::LeftThigh] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                || !entity->isOtherEnemy(localPlayer.get()) && !config->lgb.friendlyFire || entity->gunGameImmunity())
                continue;

            const Model* model = entity->getModel();
            if (!model)
                continue;

            StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
            if (!hdr)
                continue;

            StudioHitboxSet* set = hdr->getHitboxSet(0);
            if (!set)
                continue;

            const auto player = Animations::getPlayer(i);
            if (!player.gotMatrix)
                continue;

            for (size_t j = 0; j < hitbox.size(); j++)
            {
                if (!hitbox[j])
                    continue;

                StudioBbox* hitbox = set->getHitbox(j);
                if (!hitbox)
                    continue;

                for (auto& bonePosition : AimbotFunction::multiPoint(entity, player.matrix.data(), hitbox, localPlayerEyePosition, j, 0, 0))
                {
                    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch) };
                    const auto fov{ angle.length2D() };
                    if (fov > bestFov)
                        continue;

                    if (!config->lgb.ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, bonePosition, 1))
                        continue;

                    if (!entity->isVisible(bonePosition) && (config->lgb.visibleOnly || !AimbotFunction::canScan(entity, bonePosition, activeWeapon->getWeaponData(), config->lgb.killshot ? entity->health() : cfg[weaponIndex].minDamage, config->lgb.friendlyFire)))
                        continue;

                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTarget = bonePosition;
                    }
                }
            }
        }

        static float lastTime = 0.f;
        if (bestTarget.notNull())
        {
            if (memory->globalVars->realtime - lastTime <= static_cast<float>(cfg[weaponIndex].reactionTime) / 1000.f)
                return;

            static Vector lastAngles{ cmd->viewangles };
            static int lastCommand{ };

            auto angle = AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles + aimPunch);
            bool clamped{ false };

            if (std::abs(angle.x) > 255.f || std::abs(angle.y) > 255.f) {
                angle.x = std::clamp(angle.x, -255.f, 255.f);
                angle.y = std::clamp(angle.y, -255.f, 255.f);
                clamped = true;
            }

            if (config->lgb.scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                return;

            angle /= cfg[weaponIndex].smooth;
            cmd->viewangles += angle;
            interfaces->engine->setViewAngles(cmd->viewangles);
            if (clamped || cfg[weaponIndex].smooth > 1.0f) lastAngles = cmd->viewangles;
            else lastAngles = Vector{ };

            lastCommand = cmd->commandNumber;
        }
        else
            lastTime = memory->globalVars->realtime;
    }
}