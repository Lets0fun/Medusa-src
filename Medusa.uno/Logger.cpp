#include "Interfaces.h"
#include "Memory.h"

#include "Logger.h"

#include "Hooks.h"
#include "Hacks/Animations.h"
#include "Hacks/Resolver.h"
#include "../Medusa.uno/SDK/Surface.h"
#include "SDK/ConVar.h"
#include "SDK/GameEvent.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/LocalPlayer.h"
#include "GUI.h"
#include "GameData.h"
#include "../xor.h"
#include "../includes.hpp"
#include "SDK/PlayerResource.h"
#include "SDK/ItemSchema.h"
#include "../Localize.h"
#include "SDK/WeaponSystem.h"

std::string getStringFromHitgroup(int hitgroup) noexcept
{
    switch (hitgroup) {
    case HitGroup::Generic:
        return "generic";
    case HitGroup::Head:
        return "head";
    case HitGroup::Chest:
        return "chest";
    case HitGroup::Stomach:
        return "stomach";
    case HitGroup::LeftArm:
        return "left arm";
    case HitGroup::RightArm:
        return "right arm";
    case HitGroup::LeftLeg:
        return "left leg";
    case HitGroup::RightLeg:
        return "right leg";
    default:
        return "unknown";
    }
}

void Logger::reset() noexcept
{
    renderLogs.clear();
    logs.clear();
}

void Logger::getEvent(GameEvent* event) noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        renderLogs.clear();
        return;
    }

    if (!event || !localPlayer || interfaces->engine->isHLTV())
        return;

    static auto c4Timer = interfaces->cvar->findVar(skCrypt("mp_c4timer"));
    static std::mutex mtx;
    std::scoped_lock _{ mtx };

    struct PlayerPurchases {
        int totalCost;
        std::unordered_map<std::string, int> items;
    };

    static std::unordered_map<int, PlayerPurchases> playerPurchases;
    static std::unordered_map<std::string, int> purchaseTotal;
    static int totalCost;

    static auto freezeEnd = 0.0f;
    Log log;
    log.time = memory->globalVars->realtime;

    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("player_hurt"): {
        const int hurt = interfaces->engine->getPlayerFromUserID(event->getInt(skCrypt("userid")));
        const int attack = interfaces->engine->getPlayerFromUserID(event->getInt(skCrypt("attacker")));
        const auto damage = std::to_string(event->getInt(skCrypt("dmg_health")));
        const auto hitgroup = getStringFromHitgroup(event->getInt(skCrypt("hitgroup")));
        if (hurt != localPlayer->index() && attack == localPlayer->index())
        {
            if ((config->misc.loggerOptions.events & 1 << DamageDealt) != 1 << DamageDealt)
                break;

            const auto player = interfaces->entityList->getEntity(hurt);
            if (!player)
                break;

            std::string nothing;
            if ((player->health() - event->getInt(skCrypt("dmg_health"))) < 0)
                nothing = c_xor("0 hp remaining)");
            else
                nothing = std::to_string((player->health() - event->getInt(skCrypt("dmg_health")))) + c_xor(" hp remaining)");
            log.text = std::string(skCrypt("hit ")) + player->getPlayerName() + c_xor(" in ") + hitgroup + c_xor(" for ") + damage + " (" + nothing /* + "\n"*/;
        }
        else if (hurt == localPlayer->index() && attack != localPlayer->index())
        {
            if ((config->misc.loggerOptions.events & 1 << DamageReceived) != 1 << DamageReceived)
                break;

            const auto player = interfaces->entityList->getEntity(attack);
            if (!player)
                break;

            log.text = std::string(skCrypt("hurt by ")) + player->getPlayerName() + c_xor(" for ") + damage + c_xor(" in ") + hitgroup /* + "\n"*/;
        }
        break;
    }
    case fnv::hash("bomb_planted"): {
        if ((config->misc.loggerOptions.events & 1 << BombPlants) != 1 << BombPlants)
            break;

        const int idx = interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("userid")));
        if (idx == localPlayer->getUserId())
            break;

        const auto player = interfaces->entityList->getEntity(idx);
        if (!player)
            break;

        const std::string site = Misc::bombSiteCeva;

        log.text = std::string(skCrypt("bomb planted on ")) + site + " by " + player->getPlayerName() + std::string(skCrypt(", detonation in ")) + std::to_string(c4Timer->getInt()) + std::string(skCrypt(" seconds")) /* + "\n"*/;
        break;
    }
    case fnv::hash("hostage_follows"): {
        if ((config->misc.loggerOptions.events & 1 << HostageTaken) != 1 << HostageTaken)
            break;

        const int idx = interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("userid")));
        if (idx == localPlayer->getUserId())
            break;

        const auto player = interfaces->entityList->getEntity(idx);
        if (!player)
            break;

        log.text = std::string(skCrypt("hostage taken by ")) + player->getPlayerName() /* + "\n"*/;
        break;
    }
    case fnv::hash("item_purchase"): {
        if ((config->misc.loggerOptions.events & 1 << Purchases) != 1 << Purchases)
            break;

        auto userId = interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("userid")));
        if (!userId)
            break;

        if (!localPlayer)
            break;

        if (userId == localPlayer.get()->getUserId())
            break;

        const auto player = interfaces->entityList->getEntity(userId);
        if (!player)
            break;

        if (player->team() == localPlayer->team())
            break;

        std::string weapon = event->getString(c_xor("weapon"));

        std::ostringstream ss; ss << player->getPlayerName() << " bought " << weapon;
        log.text = ss.str().c_str();
        break;
    }  
    case fnv::hash("bomb_beginplant"): {
        if ((config->misc.loggerOptions.events & 1 << BombPlants) != 1 << BombPlants)
            break;

        auto userId = interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("userid")));
        if (!userId)
            break;

        if (!localPlayer)
            break;

        if (userId == localPlayer->getUserId())
            break;

        const auto player = interfaces->entityList->getEntity(userId);
        if (!player)
            break;

        log.text = player->getPlayerName() + std::string(skCrypt(" is planting the bomb"));
        break;
    }
    case fnv::hash("bomb_begindefuse"): {
        if ((config->misc.loggerOptions.events & 1 << BombPlants) != 1 << BombPlants)
            break;

        auto userId = interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("userid")));
        if (!userId)
            break;

        if (!localPlayer)
            break;

        if (userId == localPlayer->getUserId())
            break;

        const auto player = interfaces->entityList->getEntity(userId);
        if (!player)
            break;

        log.text = player->getPlayerName() + std::string(skCrypt(" is defusing the bomb"));
        break;
    }
    default:
        return;
    }
    
    if (log.text.empty())
        return;

    logs.push_front(log);
    renderLogs.push_front(log);
}

void Logger::process() noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        renderLogs.clear();
        return;
    }

    console();
    render();
}

void Logger::console() noexcept
{
    if (logs.empty())
        return;

    if ((config->misc.loggerOptions.modes & 1 << Console) != 1 << Console)
    {
        logs.clear();
        return;
    }

    std::array<std::uint8_t, 4> color;
    if (!config->misc.logger.rainbow)
    {
        color.at(0) = static_cast<uint8_t>(config->misc.logger.color.at(0) * 255.0f);
        color.at(1) = static_cast<uint8_t>(config->misc.logger.color.at(1) * 255.0f);
        color.at(2) = static_cast<uint8_t>(config->misc.logger.color.at(2) * 255.0f);
    }
    else
    {
        const auto [colorR, colorG, colorB] { rainbowColor(config->misc.logger.rainbowSpeed) };
        color.at(0) = static_cast<uint8_t>(colorR * 255.0f);
        color.at(1) = static_cast<uint8_t>(colorG * 255.0f);
        color.at(2) = static_cast<uint8_t>(colorB * 255.0f);
    }
    color.at(3) = static_cast<uint8_t>(255.0f);

    for (auto log : logs)
    {
        Helpers::logConsole(c_xor("[Medusa.uno] "), color);
        Helpers::logConsole(log.text + "\n");
    }

    logs.clear();
}

void Logger::render() noexcept
{
    if ((config->misc.loggerOptions.modes & 1 << EventLog) != 1 << EventLog)
    {
        renderLogs.clear();
        return;
    }

    if (renderLogs.empty())
        return;

    while (renderLogs.size() > 12)
        renderLogs.pop_back();

    for (int i = renderLogs.size() - 1; i >= 0; i--)
    {
        if (renderLogs[i].time + 5.0f <= memory->globalVars->realtime && renderLogs[i].alpha != 0.f)
            renderLogs[i].alpha -= 2.f;

        interfaces->surface->setTextColor(static_cast<int>(config->misc.logger.color[0] * 255.f), static_cast<int>(config->misc.logger.color[1] * 255.f), static_cast<int>(config->misc.logger.color[2] * 255.f), static_cast<int>(renderLogs[i].alpha));
        interfaces->surface->setTextFont(hooks->tahomaAA);
        if (config->misc.loggerOptions.position == 0)
            interfaces->surface->setTextPosition(8, 5 + (17 * i) + 1);
        else if(config->misc.loggerOptions.position == 1)
            interfaces->surface->setTextPosition(interfaces->surface->getScreenSize().first / 2 - interfaces->surface->getTextSize(hooks->tahomaAA, std::wstring(renderLogs[i].text.begin(), renderLogs[i].text.end()).c_str()).first / 2, interfaces->surface->getScreenSize().second / 2 + config->misc.loggerOptions.offset + (17 * i) + 1);
        interfaces->surface->printText(std::wstring(renderLogs[i].text.begin(), renderLogs[i].text.end()));
    }

    for (int i = renderLogs.size() - 1; i >= 0; i--) {
        if (renderLogs[i].alpha <= 2.0f) {
            renderLogs.erase(renderLogs.begin() + i);
            break;
        }
    }
}

void Logger::addLog(std::string logText) noexcept
{
    Log log;
    log.time = memory->globalVars->realtime;
    log.text = logText;

    logs.push_front(log);
    renderLogs.push_front(log);
}

void Logger::addConsoleLog(std::string logText) noexcept
{
    Log log;
    log.time = memory->globalVars->realtime;
    log.text = logText;

    logs.push_front(log);
}