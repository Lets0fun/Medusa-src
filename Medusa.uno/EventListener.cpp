#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "GameData.h"

#include "Interfaces.h"
#include "Memory.h"
#include "Logger.h"
#include "xor.h"
#include "Hacks/Misc.h"
#include "Hacks/Resolver.h"
#include "SkinChanger.h"
#include "Hacks/Visuals.h"
#include "Hacks/ESP.h"
EventListener::EventListener() noexcept
{
    assert(interfaces);
    interfaces->gameEventManager->addListener(this, skCrypt("round_start"));
    interfaces->gameEventManager->addListener(this, skCrypt("round_freeze_end"));
    interfaces->gameEventManager->addListener(this, skCrypt("player_hurt"));
    interfaces->gameEventManager->addListener(this, skCrypt("bomb_planted"));
    interfaces->gameEventManager->addListener(this, skCrypt("hostage_follows"));

    interfaces->gameEventManager->addListener(this, skCrypt("weapon_fire"));
    interfaces->gameEventManager->addListener(this, skCrypt("item_purchase"));

    interfaces->gameEventManager->addListener(this, skCrypt("smokegrenade_detonate"));
    interfaces->gameEventManager->addListener(this, skCrypt("molotov_detonate"));
    interfaces->gameEventManager->addListener(this, skCrypt("inferno_expire"));
    interfaces->gameEventManager->addListener(this, skCrypt("bullet_impact"));
    interfaces->gameEventManager->addListener(this, skCrypt("player_death"));
    interfaces->gameEventManager->addListener(this, skCrypt("vote_cast"));
    interfaces->gameEventManager->addListener(this, skCrypt("bomb_begindefuse"));
    interfaces->gameEventManager->addListener(this, skCrypt("bomb_beginplant"));
    interfaces->gameEventManager->addListener(this, skCrypt("bomb_defused"));
    interfaces->gameEventManager->addListener(this, skCrypt("player_say"));
    if (const auto desc = memory->getEventDescriptor(interfaces->gameEventManager, skCrypt("player_death"), nullptr))
        std::swap(desc->listeners[0], desc->listeners[desc->listeners.size - 1]);
    else
        assert(false);
}

void EventListener::remove() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->removeListener(this);
}

void EventListener::fireGameEvent(GameEvent* event)
{
    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
        GameData::clearProjectileList();
        Misc::preserveKillfeed(true);
        Misc::autoBuy(event);
        Resolver::getEvent(event);
        Visuals::bulletTracer(event);
        [[fallthrough]];
    case fnv::hash("round_freeze_end"):
        //Misc::purchaseList(event);
        break;
    case fnv::hash("player_death"):
        SkinChanger::updateStatTrak(*event);
        SkinChanger::overrideHudIcon(*event);
        Misc::killfeedChanger(*event);
        Misc::killMessage(*event);
        Misc::killSound(*event);
        Resolver::getEvent(event);
        break;
    case fnv::hash("player_hurt"):
        Misc::playHitSound(*event);
        Visuals::drawHitboxMatrix(event);
        Visuals::hitEffect(event);
        Visuals::hitMarker(event);
        Visuals::dmgMarker(event);
        Logger::getEvent(event);
        Resolver::getEvent(event);
        break;
    case fnv::hash("player_say"):
        Misc::chatRevealer(*event, event);
        break;
    case fnv::hash("weapon_fire"):
        Visuals::bulletTracer(event);
        break;
    case fnv::hash("vote_cast"):
        Misc::voteRevealer(*event);
        break;
    case fnv::hash("bomb_planted"):
        Logger::getEvent(event);
        break;
    case fnv::hash("hostage_follows"):
        Logger::getEvent(event);
        break;
    case fnv::hash("smokegrenade_detonate"):
        Visuals::drawSmokeTimerEvent(event);
        break;
    case fnv::hash("molotov_detonate"):
        Visuals::drawMolotovTimerEvent(event);
        break;
    case fnv::hash("inferno_expire"):
        Visuals::molotovExtinguishEvent(event);
        break;
    case fnv::hash("bomb_defused"):
        Logger::getEvent(event);
        break;
    case fnv::hash("bomb_beginplant"):
        Logger::getEvent(event);
        break;
    case fnv::hash("bomb_begindefuse"):
        Logger::getEvent(event);
        break;
    case fnv::hash("item_purchase"):
        Logger::getEvent(event);
        break;
    }
}