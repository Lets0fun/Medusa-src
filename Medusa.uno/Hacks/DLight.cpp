#include "DLight.h"
#include "../GameData.h"
#include "../Config.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/LocalPlayer.h"
#include "../Memory.h"
#include "../Interfaces.h"
#include "../GUI.h"

struct PDlight {
    bool hasBeenCreated = false;
    dlight_t* dlight = nullptr;
    dlight_t* elight = nullptr;
    dlight_t dlight_settings;
};

static std::vector<PDlight> playerLights(65);

void applyLight(size_t i, int handle)
{
    dlight_t* dLight = interfaces->effects->CL_AllocDlight(handle);
    dlight_t* eLight = interfaces->effects->CL_AllocElight(handle);

    *eLight = *dLight;
    playerLights.at(i).hasBeenCreated = true;
    playerLights.at(i).dlight = dLight;
    playerLights.at(i).elight = eLight;
    playerLights.at(i).dlight_settings = *dLight;
}

void Dlights::setupLights() noexcept
{
    for (const auto& e : GameData::players())
    {
        if (!e.alive || e.dormant)
        {
            if (playerLights.at(e.index).hasBeenCreated) {
                playerLights.at(e.index).dlight->die = memory->globalVars->currenttime;
                playerLights.at(e.index).elight->die = memory->globalVars->currenttime;
            }
            playerLights.at(e.index).hasBeenCreated = false;

            continue;
        }
        float r;
        float g;
        float b;
        int exponent;
        float radius;
        bool enabled;

        if (!e.enemy)
        {
             r = config->dlightConfig.teammate.asColor3.color[0];
            g = config->dlightConfig.teammate.asColor3.color[1];
            b = config->dlightConfig.teammate.asColor3.color[2];
            exponent = config->dlightConfig.teammate.exponent;
            radius = config->dlightConfig.teammate.raduis;
            enabled = config->dlightConfig.teammate.enabled;
        }
        else if (e.enemy)
        {
            r = config->dlightConfig.enemy.asColor3.color[0];
            g = config->dlightConfig.enemy.asColor3.color[1];
            b = config->dlightConfig.enemy.asColor3.color[2];
            exponent = config->dlightConfig.enemy.exponent;
            radius = config->dlightConfig.enemy.raduis;
            enabled = config->dlightConfig.enemy.enabled;
        }
        if (!enabled)
        {
            if (playerLights.at(e.index).hasBeenCreated) {
                playerLights.at(e.index).dlight->die = memory->globalVars->currenttime;
                playerLights.at(e.index).elight->die = memory->globalVars->currenttime;
            }
            playerLights.at(e.index).hasBeenCreated = false;
            continue;
        }
        if (!playerLights.at(e.index).hasBeenCreated) {
            applyLight(e.index, e.handle);
        }
        else
        {
            playerLights.at(e.index).dlight_settings.origin = e.origin;
            playerLights.at(e.index).dlight_settings.die = memory->globalVars->currenttime + 2.0f;

            playerLights.at(e.index).dlight_settings.m_Direction = e.origin;
            playerLights.at(e.index).dlight_settings.color.r = (std::byte)(r * 255);
            playerLights.at(e.index).dlight_settings.color.g = (std::byte)(g * 255);
            playerLights.at(e.index).dlight_settings.color.b = (std::byte)(b * 255);
            playerLights.at(e.index).dlight_settings.radius = radius;
            playerLights.at(e.index).dlight_settings.color.exponent = exponent;
            *(playerLights.at(e.index).dlight) = playerLights.at(e.index).dlight_settings;
            *(playerLights.at(e.index).elight) = playerLights.at(e.index).dlight_settings;
        }
    }
    
    if (localPlayer)
    {
        if (!localPlayer->isAlive())
            return;

        float r;
        float g;
        float b;
        int exponent;
        float radius;
        bool enabled;
        r = config->dlightConfig.local.asColor3.color[0];
        g = config->dlightConfig.local.asColor3.color[1];
        b = config->dlightConfig.local.asColor3.color[2];
        exponent = config->dlightConfig.local.exponent;
        radius = config->dlightConfig.local.raduis;
        enabled = config->dlightConfig.local.enabled;
        if (!enabled)
            return;
        if (!playerLights.at(localPlayer->index()).hasBeenCreated) {
            applyLight(localPlayer->index(), localPlayer->handle());
        }
        else
        {
            playerLights.at(localPlayer->index()).dlight_settings.origin = localPlayer->getAbsOrigin();
            playerLights.at(localPlayer->index()).dlight_settings.die = memory->globalVars->currenttime + 2.0f;

            playerLights.at(localPlayer->index()).dlight_settings.m_Direction = localPlayer->getAbsOrigin();
            playerLights.at(localPlayer->index()).dlight_settings.color.r = (std::byte)(r * 255);
            playerLights.at(localPlayer->index()).dlight_settings.color.g = (std::byte)(g * 255);
            playerLights.at(localPlayer->index()).dlight_settings.color.b = (std::byte)(b * 255);
            playerLights.at(localPlayer->index()).dlight_settings.radius = radius;
            playerLights.at(localPlayer->index()).dlight_settings.color.exponent = exponent;
            *(playerLights.at(localPlayer->index()).dlight) = playerLights.at(localPlayer->index()).dlight_settings;
            *(playerLights.at(localPlayer->index()).elight) = playerLights.at(localPlayer->index()).dlight_settings;
        }
    }
    /*for (int i = 0; i <= maxClients; i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity->isDormant() || !entity->isAlive() || !entity->isPlayer()) {

            if (playerLights.at(i).hasBeenCreated) {
                playerLights.at(i).dlight->die = memory->globalVars->currenttime;
                playerLights.at(i).elight->die = memory->globalVars->currenttime;
            }
            playerLights.at(i).hasBeenCreated = false;

            continue;
        }
        float r;
        float g;
        float b;
        int exponent;
        float radius;
        bool enabled;
        if (localPlayer)
        {
            if (entity == localPlayer.get() && localPlayer->isAlive())
            {
                r = config->dlightConfig.local.asColor3.color[0];
                g = config->dlightConfig.local.asColor3.color[1];
                b = config->dlightConfig.local.asColor3.color[2];
                exponent = config->dlightConfig.local.exponent;
                radius = config->dlightConfig.local.raduis;
                enabled = config->dlightConfig.local.enabled;
            }
            else if (!localPlayer->isOtherEnemy(entity))
            {
                r = config->dlightConfig.teammate.asColor3.color[0];
                g = config->dlightConfig.teammate.asColor3.color[1];
                b = config->dlightConfig.teammate.asColor3.color[2];
                exponent = config->dlightConfig.teammate.exponent;
                radius = config->dlightConfig.teammate.raduis;
                enabled = config->dlightConfig.teammate.enabled;
            }
            else if (localPlayer->isOtherEnemy(entity))
            {
                r = config->dlightConfig.enemy.asColor3.color[0];
                g = config->dlightConfig.enemy.asColor3.color[1];
                b = config->dlightConfig.enemy.asColor3.color[2];
                exponent = config->dlightConfig.enemy.exponent;
                radius = config->dlightConfig.enemy.raduis;
                enabled = config->dlightConfig.enemy.enabled;
            }
        }
        if (!enabled)
            continue;
        if (!playerLights.at(i).hasBeenCreated) {
            applyLight(i, entity->handle());
        }
        else
        {
            playerLights.at(i).dlight_settings.origin = entity->getAbsOrigin();
            playerLights.at(i).dlight_settings.die = memory->globalVars->currenttime + 2.0f;

            playerLights.at(i).dlight_settings.m_Direction = entity->getAbsOrigin();
            playerLights.at(i).dlight_settings.color.r = (std::byte)(r * 255);
            playerLights.at(i).dlight_settings.color.g = (std::byte)(g * 255);
            playerLights.at(i).dlight_settings.color.b = (std::byte)(b * 255);
            playerLights.at(i).dlight_settings.radius = radius;
            playerLights.at(i).dlight_settings.color.exponent = exponent;
            *(playerLights.at(i).dlight) = playerLights.at(i).dlight_settings;
            *(playerLights.at(i).elight) = playerLights.at(i).dlight_settings;
        }
    }*/
}
