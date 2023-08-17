#include <atomic>
#include <cstring>
#include <list>
#include <mutex>
#include <thread>
#include "Config.h"
#include "fnv.h"
#include "GameData.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Radar.h"
#include "Resources/avatar_ct.h"
#include "Resources/avatar_tt.h"
#include "Resources/skillgroups.h"

#include "stb_image.h"

#include "SDK/ClientClass.h"
#include "SDK/Engine.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/GlobalVars.h"
#include "../Localize.h"
#include "SDK/LocalPlayer.h"
#include "SDK/ModelInfo.h"
#include "SDK/NetworkChannel.h"
#include "SDK/PlayerResource.h"
#include "SDK/Sound.h"
#include "SDK/Steam.h"
#include "SDK/WeaponId.h"
#include "SDK/WeaponData.h"
#include "xor.h"
static Matrix4x4 viewMatrix;
static LocalPlayerData localPlayerData;
static std::vector<PlayerData> playerData;
static std::vector<ObserverData> observerData;
static std::vector<WeaponData> weaponData;
static std::vector<EntityData> entityData;
static std::vector<LootCrateData> lootCrateData;
static std::forward_list<ProjectileData> projectileData;
static BombData bombData;
static std::vector<InfernoData> infernoData;
static std::vector<SmokeData> smokeData;
static std::atomic_int netOutgoingLatency;
static std::string gameModeName;
static std::array<std::string, 19> skillGroupNames;
static std::array<std::string, 16> skillGroupNamesDangerzone;


static auto playerByHandleWritable(int handle) noexcept
{
    const auto it = std::ranges::find(playerData, handle, &PlayerData::handle);
    return it != playerData.end() ? &(*it) : nullptr;
}

static void updateNetLatency() noexcept
{
    if (const auto networkChannel = interfaces->engine->getNetworkChannel())
        netOutgoingLatency = (std::max)(static_cast<int>(networkChannel->getLatency(0) * 1000.0f), 0);
    else
        netOutgoingLatency = 0;
}

constexpr auto playerVisibilityUpdateDelay = 0.1f;
static float nextPlayerVisibilityUpdateTime = 0.0f;

static bool shouldUpdatePlayerVisibility() noexcept
{
    return nextPlayerVisibilityUpdateTime <= memory->globalVars->realtime;
}

void GameData::update() noexcept
{
    static int lastFrame;
    if (lastFrame == memory->globalVars->framecount)
        return;
    lastFrame = memory->globalVars->framecount;

    updateNetLatency();

    Lock lock;
    observerData.clear();
    weaponData.clear();
    entityData.clear();
    lootCrateData.clear();
    infernoData.clear();
    smokeData.clear();

    localPlayerData.update();
    bombData.update();

    if (static bool skillgroupNamesInitialized = false; !skillgroupNamesInitialized) {
        for (std::size_t i = 0; i < skillGroupNames.size(); ++i)
        {
            const auto rank = interfaces->localize->findAsUTF8(("RankName_" + std::to_string(i)).c_str());
            skillGroupNames[i] = std::string(rank);
        }

        for (std::size_t i = 0; i < skillGroupNamesDangerzone.size(); ++i)
        {
            const auto rank = interfaces->localize->findAsUTF8(("skillgroup_" + std::to_string(i) + "dangerzone").c_str());
            skillGroupNamesDangerzone[i] = std::string(rank);
        }

        skillgroupNamesInitialized = true;
    }

    if (!localPlayer) {
        playerData.clear();
        projectileData.clear();
        gameModeName.clear();
        return;
    }

    gameModeName = memory->getGameModeName(false);
    viewMatrix = interfaces->engine->worldToScreenMatrix();

    const auto observerTarget = localPlayer->getObserverMode() == ObsMode::InEye ? localPlayer->getObserverTarget() : nullptr;

    const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
    for (int i = 1; i <= highestEntityIndex; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity)
            continue;

        if (entity->isPlayer()) {
            if (entity == localPlayer.get() || entity == observerTarget)
                continue;

            if (const auto player = playerByHandleWritable(entity->handle())) {
                player->update(entity);
            } else {
                playerData.emplace_back(entity);
            }

            if (!entity->isDormant() && !entity->isAlive()) {
                if (const auto obs = entity->getObserverTarget())
                    observerData.emplace_back(entity, obs, obs == localPlayer.get());
            }
        } else {
            if (entity->isDormant())
                continue;

            if (entity->isWeapon()) {
                if (entity->ownerEntity() == -1)
                    weaponData.emplace_back(entity);
            } else {
                const auto classId = entity->getClientClass()->classId;
                const int classIdInt = (int)classId;
                switch (classId) {
                case ClassId::BaseCSGrenadeProjectile:
                    if (!entity->shouldDraw()) {
                        if (const auto it = std::find(projectileData.begin(), projectileData.end(), entity->handle()); it != projectileData.end())
                            it->exploded = true;
                        break;
                    }
                    [[fallthrough]];
                case ClassId::BreachChargeProjectile:
                case ClassId::BumpMineProjectile:
                case ClassId::DecoyProjectile:
                case ClassId::MolotovProjectile:
                case ClassId::SensorGrenadeProjectile:
                case ClassId::SmokeGrenadeProjectile:
                case ClassId::SnowballProjectile:
                    if (const auto it = std::find(projectileData.begin(), projectileData.end(), entity->handle()); it != projectileData.end())
                        it->update(entity);
                    else
                        projectileData.emplace_front(entity);
                    break;
                case ClassId::DynamicProp:
                    if (const auto model = entity->getModel(); !model || !std::strstr(model->name, "challenge_coin"))
                        break;
                    [[fallthrough]];
                case ClassId::EconEntity:
                case ClassId::Chicken:
                case ClassId::PlantedC4:
                case ClassId::Hostage:
                case ClassId::Dronegun:
                case ClassId::Cash:
                case ClassId::AmmoBox:
                case ClassId::RadarJammer:
                case ClassId::SnowballPile:
                    entityData.emplace_back(entity);
                    break;
                case ClassId::LootCrate:
                    lootCrateData.emplace_back(entity);
                    break;
                case ClassId::Inferno:
                    infernoData.emplace_back(entity);
                    break;
                }

                if (classIdInt == dynamicClassId->fogController && !config->visuals.noFog)
                {
                    const auto fog = reinterpret_cast<FogController*>(entity);

                    unsigned char _color[3];

                    if (config->visuals.fog.rainbow)
                    {
                        const auto [colorR, colorG, colorB] { rainbowColor(config->visuals.fog.rainbowSpeed) };
                        _color[0] = std::clamp(static_cast<int>(colorR * 255.0f), 0, 255);
                        _color[1] = std::clamp(static_cast<int>(colorG * 255.0f), 0, 255);
                        _color[2] = std::clamp(static_cast<int>(colorB * 255.0f), 0, 255);
                    }
                    else
                    {
                        _color[0] = std::clamp(static_cast<int>(config->visuals.fog.color[0] * 255.0f), 0, 255);
                        _color[1] = std::clamp(static_cast<int>(config->visuals.fog.color[1] * 255.0f), 0, 255);
                        _color[2] = std::clamp(static_cast<int>(config->visuals.fog.color[2] * 255.0f), 0, 255);
                    }

                    const unsigned long color = *(unsigned long*)_color;

                    fog->enable() = config->visuals.fog.enabled ? 1 : 0;
                    fog->start() = config->visuals.fogOptions.start;
                    fog->end() = config->visuals.fogOptions.end;
                    fog->density() = config->visuals.fogOptions.density;
                    fog->color() = color;
                }

                if (classId == ClassId::SmokeGrenadeProjectile && entity->didSmokeEffect())
                    smokeData.emplace_back(entity);
            }
        }
    }

    std::sort(playerData.begin(), playerData.end());
    std::sort(weaponData.begin(), weaponData.end());
    std::sort(entityData.begin(), entityData.end());
    std::sort(lootCrateData.begin(), lootCrateData.end());

    std::for_each(projectileData.begin(), projectileData.end(), [](auto& projectile) {
        if (interfaces->entityList->getEntityFromHandle(projectile.handle) == nullptr)
            projectile.exploded = true;
    });

    std::erase_if(projectileData, [](const auto& projectile) { return interfaces->entityList->getEntityFromHandle(projectile.handle) == nullptr
        && (projectile.trajectory.size() < 1 || projectile.trajectory[projectile.trajectory.size() - 1].first + 60.0f < memory->globalVars->realtime); });

    std::erase_if(playerData, [](const auto& player) { return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr; });

    if (shouldUpdatePlayerVisibility())
        nextPlayerVisibilityUpdateTime = memory->globalVars->realtime + playerVisibilityUpdateDelay;
}

void GameData::clearProjectileList() noexcept
{
    Lock lock;
    projectileData.clear();
}

static void clearSkillgroupTextures() noexcept;
static void clearAvatarTextures() noexcept;

struct PlayerAvatar {
    mutable Texture texture;
    std::unique_ptr<std::uint8_t[]> rgba;
};

static std::unordered_map<int, PlayerAvatar> playerAvatars;

void GameData::clearTextures() noexcept
{
    Lock lock;

    clearSkillgroupTextures();
    clearAvatarTextures();
    for (const auto& [handle, avatar] : playerAvatars)
        avatar.texture.clear();
}

void GameData::clearUnusedAvatars() noexcept
{
    Lock lock;
    std::erase_if(playerAvatars, [](const auto& pair) { return std::ranges::find(std::as_const(playerData), pair.first, &PlayerData::handle) == playerData.cend(); });
}

int GameData::getNetOutgoingLatency() noexcept
{
    return netOutgoingLatency;
}

const Matrix4x4& GameData::toScreenMatrix() noexcept
{
    return viewMatrix;
}

const LocalPlayerData& GameData::local() noexcept
{
    return localPlayerData;
}

const std::vector<PlayerData>& GameData::players() noexcept
{
    return playerData;
}

const PlayerData* GameData::playerByHandle(int handle) noexcept
{
    return playerByHandleWritable(handle);
}

const std::vector<ObserverData>& GameData::observers() noexcept
{
    return observerData;
}

const std::vector<WeaponData>& GameData::weapons() noexcept
{
    return weaponData;
}

const std::vector<EntityData>& GameData::entities() noexcept
{
    return entityData;
}

const std::vector<LootCrateData>& GameData::lootCrates() noexcept
{
    return lootCrateData;
}

const std::forward_list<ProjectileData>& GameData::projectiles() noexcept
{
    return projectileData;
}

const BombData& GameData::plantedC4() noexcept
{
    return bombData;
}

const std::string& GameData::gameMode() noexcept
{
    return gameModeName;
}

const std::vector<InfernoData>& GameData::infernos() noexcept
{
    return infernoData;
}

const std::vector<SmokeData>& GameData::smokes() noexcept
{
    return smokeData;
}

void LocalPlayerData::update() noexcept
{
    if (!localPlayer) {
        exists = false;
        return;
    }

    exists = true;
    alive = localPlayer->isAlive();
    inaccuracy = Vector{};
    team = localPlayer->getTeamNumber();
    velocityModifier = localPlayer->velocityModifier();

    if (const auto activeWeapon = localPlayer->getActiveWeapon()) {
        inaccuracy = localPlayer->getEyePosition() + Vector::fromAngle(interfaces->engine->getViewAngles() + Vector{ Helpers::rad2deg(activeWeapon->getInaccuracy() + activeWeapon->getSpread()), 0.0f, 0.0f }) * 1000.0f;
        inReload = activeWeapon->isInReload();
        noScope = activeWeapon->isSniperRifle() && !localPlayer->isScoped();
        nextWeaponAttack = activeWeapon->nextPrimaryAttack();
        shooting = activeWeapon->isPistol() ? !inReload && nextWeaponAttack > memory->globalVars->serverTime() : localPlayer->shotsFired() > 1;
    }
    fov = localPlayer->fov() ? localPlayer->fov() : localPlayer->defaultFov();
    handle = localPlayer->handle();
    flashDuration = localPlayer->flashDuration();

    aimPunch = localPlayer->getEyePosition() + Vector::fromAngle(interfaces->engine->getViewAngles() + localPlayer->getAimPunch()) * 1000.0f;

    const auto obsMode = localPlayer->getObserverMode();
    if (const auto obs = localPlayer->getObserverTarget(); obs && obsMode != ObsMode::Roaming && obsMode != ObsMode::Deathcam)
        origin = obs->getAbsOrigin();
    else
        origin = localPlayer->getAbsOrigin();
}

BaseData::BaseData(Entity* entity) noexcept
{
    distanceToLocal = entity->getAbsOrigin().distTo(localPlayerData.origin);
 
    if (entity->isPlayer()) {
        const auto collideable = entity->getCollideable();
        obbMins = collideable->obbMins();
        obbMaxs = collideable->obbMaxs();
    } else if (const auto model = entity->getModel()) {
        obbMins = model->mins;
        obbMaxs = model->maxs;
    }

    coordinateFrame = entity->toWorldTransform();
}

EntityData::EntityData(Entity* entity) noexcept : BaseData{ entity }
{
    name = [](Entity* entity) {
        switch (entity->getClientClass()->classId) {
        case ClassId::EconEntity: return "Defuse Kit";
        case ClassId::Chicken: return "Chicken";
        case ClassId::PlantedC4: return "Planted C4";
        case ClassId::Hostage: return "Hostage";
        case ClassId::Dronegun: return "Sentry";
        case ClassId::Cash: return "Cash";
        case ClassId::AmmoBox: return "Ammo Box";
        case ClassId::RadarJammer: return "Radar Jammer";
        case ClassId::SnowballPile: return "Snowball Pile";
        case ClassId::DynamicProp: return "Collectable Coin";
        default: assert(false); return "unknown";
        }
    }(entity);
}

ProjectileData::ProjectileData(Entity* projectile) noexcept : BaseData { projectile }
{
    name = [](Entity* projectile) {
        switch (projectile->getClientClass()->classId) {
        case ClassId::BaseCSGrenadeProjectile:
            if (const auto model = projectile->getModel(); model && strstr(model->name, "flashbang"))
                return "FLASHBANG";
            else
                return "HE GRENADE";
        case ClassId::BreachChargeProjectile: return "BREACH CHARGE";
        case ClassId::BumpMineProjectile: return "BUMP MINE";
        case ClassId::DecoyProjectile: return "DECOY";
        case ClassId::MolotovProjectile: return "MOLOTOV";
        case ClassId::SensorGrenadeProjectile: return "TA GRENADE";
        case ClassId::SmokeGrenadeProjectile: return "SMOKE";
        case ClassId::SnowballProjectile: return "SNOWBALL";
        default: assert(false); return "?";
        }
    }(projectile);
    type = [](Entity* projectile) {
        switch (projectile->getClientClass()->classId) {
        case ClassId::BaseCSGrenadeProjectile:
            if (const auto model = projectile->getModel(); model && strstr(model->name, "flashbang"))
                return WeaponId::Flashbang;
            else
                return WeaponId::HeGrenade;
        case ClassId::BumpMineProjectile: return WeaponId::BumpMine;
        case ClassId::DecoyProjectile: return WeaponId::Decoy;
        case ClassId::MolotovProjectile: return WeaponId::Molotov;
        case ClassId::SensorGrenadeProjectile: return WeaponId::TaGrenade;
        case ClassId::SmokeGrenadeProjectile: return WeaponId::SmokeGrenade;
        case ClassId::SnowballProjectile: return WeaponId::Snowball;
        default: return WeaponId::None;
        }
    }(projectile);
    if (const auto thrower = interfaces->entityList->getEntityFromHandle(projectile->thrower()); thrower && localPlayer) {
        if (thrower == localPlayer.get())
            thrownByLocalPlayer = true;
        else
            thrownByEnemy = memory->isOtherEnemy(localPlayer.get(), thrower);
    }
    spawnTick = static_cast<int>(0.5f + projectile->projectileSpawnTime() / memory->globalVars->intervalPerTick);
    handle = projectile->handle();
}

void ProjectileData::update(Entity* projectile) noexcept
{
    static_cast<BaseData&>(*this) = { projectile };
    simulationOffset = static_cast<int>(0.5f + (memory->globalVars->currenttime - projectile->simulationTime()) / memory->globalVars->intervalPerTick);
    if (const auto& pos = projectile->getAbsOrigin(); trajectory.size() < 1 || trajectory[trajectory.size() - 1].second != pos)
    {
        Velocity = projectile->velocity();
        trajectory.emplace_back(memory->globalVars->realtime, pos);
    }    
}

PlayerData::PlayerData(Entity* entity) noexcept : BaseData{ entity }, userId{ entity->getUserId() }, steamID{ entity->getSteamId() }, handle{ entity->handle() }, money{ entity->money() }
{
    if (steamID) {
        const auto ctx = interfaces->engine->getSteamAPIContext();
        const auto avatar = ctx->steamFriends->getSmallFriendAvatar(steamID);
        constexpr auto rgbaDataSize = 4 * 32 * 32;

        PlayerAvatar playerAvatar;
        playerAvatar.rgba = std::make_unique<std::uint8_t[]>(rgbaDataSize);
        if (ctx->steamUtils->getImageRGBA(avatar, playerAvatar.rgba.get(), rgbaDataSize))
            playerAvatars[handle] = std::move(playerAvatar);
    }

    update(entity);
}


void PlayerData::update(Entity* entity) noexcept
{
    name = entity->getPlayerName();
    const auto idx = entity->index();
    index = entity->index();
    ducking = entity->flags() & PlayerFlag_Crouched;
    {
        const Vector start = entity->getEyePosition();
        const Vector end = start + Vector::fromAngle(entity->eyeAngles()) * 1000.0f;

        Trace trace;
        interfaces->engineTrace->traceRay({ start, end }, ALL_VISIBLE_CONTENTS | CONTENTS_MOVEABLE | CONTENTS_DETAIL, entity, trace);
        lookingAt = trace.endpos;
    }
    if (const auto pr = *memory->playerResource) {
        armor = pr->armor()[idx];
        skillgroup = pr->competitiveRanking()[idx];
        competitiveWins = pr->competitiveWins()[idx];
        hasBomb = idx == pr->playerC4Index();
        if (const auto clantag = pr->getClan(idx);
            clantag && clantag[0] != '\0')
        {
            clanTag = std::string(clantag);
        }
        else
            clanTag = "";
    }
    dormant = entity->isDormant();
    if (dormant) {
        if (const auto pr = *memory->playerResource) {
            alive = pr->getIPlayerResource()->isAlive(idx);
            if (!alive)
                lastContactTime = 0.0f;
            health = pr->getIPlayerResource()->getPlayerHealth(idx);
        }
        return;
    }
    observerMode = entity->getObserverMode();
    hasDefuser = entity->hasDefuser();
    money = entity->money();
    team = entity->getTeamNumber();
    static_cast<BaseData&>(*this) = { entity };
    origin = entity->getAbsOrigin();
    inViewFrustum = !interfaces->engine->cullBox(obbMins + origin, obbMaxs + origin);
    alive = entity->isAlive();
    lastContactTime = alive ? memory->globalVars->realtime : 0.0f;

    const Vector start = entity->getEyePosition();
    const Vector end = start + Vector::fromAngle(entity->eyeAngles()) * 1000.0f;

    Trace trace;
    interfaces->engineTrace->traceRay({ start, end }, 0x80040FF, entity, trace);
    lookingAt = trace.endpos;

    if (localPlayer) {
        enemy = memory->isOtherEnemy(entity, localPlayer.get());

        if (!inViewFrustum || !alive)
            visible = false;
        else if (shouldUpdatePlayerVisibility())
            visible = entity->visibleTo(localPlayer.get());
    }

    constexpr auto isEntityAudible = [](int entityIndex) noexcept {
        for (int i = 0; i < memory->activeChannels->count; ++i)
            if (memory->channels[memory->activeChannels->list[i]].soundSource == entityIndex)
                return true;
        return false;
    };
    audible = isEntityAudible(entity->index());
    spotted = entity->spotted();
    health = entity->health();
    immune = entity->gunGameImmunity();
    flashDuration = entity->flashDuration();
    hasHelmet = entity->hasHelmet();
    {
        auto animstate = entity->getAnimstate();
        if (animstate)
        {
            auto fakeducking = [&]() -> bool
            {
                static auto storedticks = 0;
                static int crouched_ticks[65];
                if (animstate->animDuckAmount)
                {
                    if (animstate->animDuckAmount <= 0.9f && animstate->animDuckAmount >= 0.3f)
                    {
                        if (storedticks != memory->globalVars->tickCount)
                        {
                            crouched_ticks[entity->index()]++;
                            storedticks = memory->globalVars->tickCount;
                        }
                        return crouched_ticks[entity->index()] > 16;
                    }
                    else
                        crouched_ticks[entity->index()] = 0;
                }
                return false;
            };
            if (fakeducking())
                isFakeDucking = true;
            else
                isFakeDucking = false;
        }
    }
    if (const auto weapon = entity->getActiveWeapon()) {
        audible = audible || isEntityAudible(weapon->index());
        if (const auto weaponInfo = weapon->getWeaponData())
        {
            isKnife = weapon->isKnife();
            isGrenade = weapon->isGrenade();
            if (weapon->isGrenade())
                pinPulled = entity->pinPulled();
            isBomb = weapon->isBomb();
            if (weapon->itemDefinitionIndex2() == WeaponId::Awp || weapon->itemDefinitionIndex2() == WeaponId::Ssg08 || weapon->itemDefinitionIndex2() == WeaponId::Scar20 || weapon->itemDefinitionIndex2() == WeaponId::G3SG1)
                isScoped = entity->isScoped();
            else
                isScoped = false;
            isHealthshot = weapon->itemDefinitionIndex2() == WeaponId::Healthshot;
            clip2 = weapon->clip();
            maxClip = weaponInfo->maxClip;
            switch (weapon->itemDefinitionIndex2())
            {
                case WeaponId::None:
                    activeWeaponIcon = "";
                    activeWeapon = "";
                    break;
                case WeaponId::Awp:
                    activeWeaponIcon = skCrypt("Z");
                    activeWeapon = skCrypt("AWP");
                    break;
                case WeaponId::Ak47:
                    activeWeaponIcon = skCrypt("W");
                    activeWeapon = skCrypt("AK-47");
                    break;
                case WeaponId::Ssg08:
                    activeWeaponIcon = skCrypt("a");
                    activeWeapon = skCrypt("SSG-08");
                    break;
                case WeaponId::Deagle:
                    activeWeaponIcon = skCrypt("A");
                    activeWeapon = skCrypt("DESERT EAGLE");
                    break;
                case WeaponId::Elite:
                    activeWeaponIcon = skCrypt("B");
                    activeWeapon = skCrypt("DUAL BERETTAS");
                    break;
                case WeaponId::Fiveseven:
                    activeWeaponIcon = skCrypt("C");
                    activeWeapon = skCrypt("FIVE-SEVEN");
                    break;
                case WeaponId::Glock:
                    activeWeaponIcon = skCrypt("D");
                    activeWeapon = skCrypt("GLOCK-18");
                    break;
                case WeaponId::P250:
                    activeWeaponIcon = skCrypt("E");
                    activeWeapon = skCrypt("P250");
                    break;
                case WeaponId::Hkp2000:
                    activeWeaponIcon = skCrypt("F");
                    activeWeapon = skCrypt("P2000");
                    break;
                case WeaponId::Usp_s:
                    activeWeaponIcon = skCrypt("G");
                    activeWeapon = skCrypt("USP-S");
                    break;
                case WeaponId::Tec9:
                    activeWeaponIcon = skCrypt("H");
                    activeWeapon = skCrypt("TEC-9");
                    break;
                case WeaponId::Taser:
                    activeWeaponIcon = skCrypt("h");
                    activeWeapon = skCrypt("ZEUS x27");
                    break;
                case WeaponId::Cz75a:
                    activeWeaponIcon = skCrypt("I");
                    activeWeapon = skCrypt("CZ75");
                    break;
                case WeaponId::Revolver:
                    activeWeaponIcon = skCrypt("J");
                    activeWeapon = skCrypt("R8 REVOLVER");
                    break;
                case WeaponId::Xm1014:
                    activeWeaponIcon = skCrypt("b");
                    activeWeapon = skCrypt("XM1014");
                    break;
                case WeaponId::Sawedoff:
                    activeWeaponIcon = skCrypt("c");
                    activeWeapon = skCrypt("SAWED-OFF");
                    break;
                case WeaponId::Mag7:
                    activeWeaponIcon = skCrypt("d");
                    activeWeapon = skCrypt("MAG-7");
                    break;
                case WeaponId::Nova:
                    activeWeaponIcon = skCrypt("e");
                    activeWeapon = skCrypt("NOVA");
                    break;
                case WeaponId::Negev:
                    activeWeaponIcon = skCrypt("f");
                    activeWeapon = skCrypt("NEGEV");
                    break;
                case WeaponId::M249:
                    activeWeaponIcon = skCrypt("g");
                    activeWeapon = skCrypt("M249");
                    break;
                case WeaponId::Flashbang:
                    activeWeaponIcon = skCrypt("i");
                    activeWeapon = skCrypt("FLASHBANG");
                    break;
                case WeaponId::HeGrenade:
                    activeWeaponIcon = skCrypt("j");
                    activeWeapon = skCrypt("HE GRENADE");
                    break;
                case WeaponId::Mac10:
                    activeWeaponIcon = skCrypt("K");
                    activeWeapon = skCrypt("MAC-10");
                    break;
                case WeaponId::SmokeGrenade:
                    activeWeaponIcon = skCrypt("k");
                    activeWeapon = skCrypt("SMOKE GRENADE");
                    break;
                case WeaponId::Ump45:
                    activeWeaponIcon = skCrypt("L");
                    activeWeapon = skCrypt("UMP-45");
                    break;
                case WeaponId::Molotov:
                    activeWeaponIcon = skCrypt("l");
                    activeWeapon = skCrypt("MOLOTOV");
                    break;
                case WeaponId::Bizon:
                    activeWeaponIcon = skCrypt("M");
                    activeWeapon = skCrypt("PP-BIZON");
                    break;
                case WeaponId::Decoy:
                    activeWeaponIcon = skCrypt("m");
                    activeWeapon = skCrypt("DECOY");
                    break;
                case WeaponId::Mp7:
                    activeWeaponIcon = skCrypt("N");
                    activeWeapon = skCrypt("MP7");
                    break;
                case WeaponId::Mp5sd:
                    activeWeaponIcon = skCrypt("N");
                    activeWeapon = skCrypt("MP5-SD");
                    break;
                case WeaponId::IncGrenade:
                    activeWeaponIcon = skCrypt("n");
                    activeWeapon = skCrypt("INCENDIARY");
                    break;
                case WeaponId::C4:
                    activeWeaponIcon = skCrypt("o");
                    activeWeapon = skCrypt("C4 BOMB");
                    break;
                case WeaponId::Mp9:
                    activeWeaponIcon = skCrypt("O");
                    activeWeapon = skCrypt("MP9");
                    break;
                case WeaponId::P90:
                    activeWeaponIcon = skCrypt("P");
                    activeWeapon = skCrypt("P90");
                    break;
                case WeaponId::GalilAr:
                    activeWeaponIcon = skCrypt("Q");
                    activeWeapon = skCrypt("GALIL");
                    break;
                case WeaponId::Famas:
                    activeWeaponIcon = skCrypt("R");
                    activeWeapon = skCrypt("FAMAS");
                    break;
                case WeaponId::M4A1:
                    activeWeaponIcon = skCrypt("S");
                    activeWeapon = skCrypt("M4A4");
                    break;
                case WeaponId::M4a1_s:
                    activeWeaponIcon = skCrypt("T");
                    activeWeapon = skCrypt("M4A1-S");
                    break;
                case WeaponId::Aug:
                    activeWeaponIcon = skCrypt("U");
                    activeWeapon = skCrypt("AUG");
                    break;
                case WeaponId::Sg553:
                    activeWeaponIcon = skCrypt("V");
                    activeWeapon = skCrypt("SG-553");
                    break;
                case WeaponId::G3SG1:
                    activeWeaponIcon = skCrypt("X");
                    activeWeapon = skCrypt("G3SG1");
                    break;
                case WeaponId::Scar20:
                    activeWeaponIcon = skCrypt("Y");
                    activeWeapon = skCrypt("SCAR-20");
                    break;
                case WeaponId::KnifeT:
                    activeWeaponIcon = skCrypt("[");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Knife:
                    activeWeaponIcon = skCrypt("]");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::GoldenKnife:
                    activeWeaponIcon = skCrypt("]");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Falchion:
                    activeWeaponIcon = skCrypt("0");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Huntsman:
                    activeWeaponIcon = skCrypt("6");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Butterfly:
                    activeWeaponIcon = skCrypt("8");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Daggers:
                    activeWeaponIcon = skCrypt("9");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Karambit:
                    activeWeaponIcon = skCrypt("4");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Gut:
                    activeWeaponIcon = skCrypt("3");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Flip:
                    activeWeaponIcon = skCrypt("2");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::M9Bayonet:
                    activeWeaponIcon = skCrypt("5");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Bayonet:
                    activeWeaponIcon = "1";
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::Bowie:
                    activeWeaponIcon = skCrypt("7");
                    activeWeapon = skCrypt("KNIFE");
                    break;
                case WeaponId::TaGrenade:activeWeaponIcon = ""; activeWeapon = "TA Grenade"; break;
                case WeaponId::Firebomb:activeWeaponIcon = ""; activeWeapon = "FIRE BOMB"; break;
                case WeaponId::Diversion:activeWeaponIcon = ""; activeWeapon = "DIVERSION"; break;
                case WeaponId::FragGrenade:activeWeaponIcon = ""; activeWeapon = "FRAG GRENADE"; break;
                case WeaponId::Snowball:activeWeaponIcon = ""; activeWeapon = "SNOWBALL"; break;
                case WeaponId::Axe:activeWeaponIcon = ""; activeWeapon = "AXE"; break;
                case WeaponId::Hammer:activeWeaponIcon = ""; activeWeapon = "HAMMER"; break;
                case WeaponId::Spanner:activeWeaponIcon = ""; activeWeapon = "WRENCH"; break;
                case WeaponId::Healthshot:activeWeaponIcon = ""; activeWeapon = "HEALTHSHOT"; break;
                case WeaponId::BumpMine:activeWeaponIcon = ""; activeWeapon = "BUMP MINE"; break;
                case WeaponId::ZoneRepulsor:activeWeaponIcon = ""; activeWeapon = "ZONE REPULSOR"; break;
                case WeaponId::Shield:activeWeaponIcon = ""; activeWeapon = "SHIELD"; break;
            }     
        }
    }

    if (!alive || !inViewFrustum)
        return;

    const auto model = entity->getModel();
    if (!model)
        return;

    const auto studioModel = interfaces->modelInfo->getStudioModel(model);
    if (!studioModel)
        return;

    if (!entity->getBoneCache().memory)
        return;

    matrix3x4 boneMatrices[MAXSTUDIOBONES];
    memcpy(boneMatrices, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
    bones.clear();
    bones.reserve(20);

    for (int i = 0; i < studioModel->numBones; ++i) {
        const auto bone = studioModel->getBone(i);

        if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
            continue;

        bones.emplace_back(boneMatrices[i].origin(), boneMatrices[bone->parent].origin());
    }

    const auto set = studioModel->getHitboxSet(entity->hitboxSet());
    if (!set)
        return;

    const auto headBox = set->getHitbox(0);

    headMins = headBox->bbMin.transform(boneMatrices[headBox->bone]);
    headMaxs = headBox->bbMax.transform(boneMatrices[headBox->bone]);

    if (headBox->capsuleRadius > 0.0f) {
        headMins -= headBox->capsuleRadius;
        headMaxs += headBox->capsuleRadius;
    }
}

const std::string PlayerData::getRankName() const noexcept
{
    if (gameModeName == "survival")
        return skillGroupNamesDangerzone[std::size_t(skillgroup) < skillGroupNamesDangerzone.size() ? skillgroup : 0];
    else
        return skillGroupNames[std::size_t(skillgroup) < skillGroupNames.size() ? skillgroup : 0];
}

struct PNGTexture {
    template <std::size_t N>
    PNGTexture(const std::array<char, N>& png) noexcept : pngData{ png.data() }, pngDataSize{ png.size() } {}

    ImTextureID getTexture() const noexcept
    {
        if (!texture.get()) {
            int width, height;
            stbi_set_flip_vertically_on_load_thread(false);

            if (const auto data = stbi_load_from_memory((const stbi_uc*)pngData, pngDataSize, &width, &height, nullptr, STBI_rgb_alpha)) {
                texture.init(width, height, data);
                stbi_image_free(data);
            } else {
                assert(false);
            }
        }

        return texture.get();
    }

    void clearTexture() const noexcept { texture.clear(); }

private:
    const char* pngData;
    std::size_t pngDataSize;

    mutable Texture texture;
};

static const auto skillgroupImages = std::array<PNGTexture, 19>({
Resource::skillgroup0, Resource::skillgroup1, Resource::skillgroup2, Resource::skillgroup3, Resource::skillgroup4, Resource::skillgroup5, Resource::skillgroup6, Resource::skillgroup7,
Resource::skillgroup8, Resource::skillgroup9, Resource::skillgroup10, Resource::skillgroup11, Resource::skillgroup12, Resource::skillgroup13, Resource::skillgroup14, Resource::skillgroup15,
Resource::skillgroup16, Resource::skillgroup17, Resource::skillgroup18 });

static const auto dangerZoneImages = std::array<PNGTexture, 16>({
Resource::dangerzone0, Resource::dangerzone1, Resource::dangerzone2, Resource::dangerzone3, Resource::dangerzone4, Resource::dangerzone5, Resource::dangerzone6, Resource::dangerzone7,
Resource::dangerzone8, Resource::dangerzone9, Resource::dangerzone10, Resource::dangerzone11, Resource::dangerzone12, Resource::dangerzone13, Resource::dangerzone14, Resource::dangerzone15 });


static const PNGTexture avatarTT{ Resource::avatar_tt };
static const PNGTexture avatarCT{ Resource::avatar_ct };

static void clearAvatarTextures() noexcept
{
    avatarTT.clearTexture();
    avatarCT.clearTexture();
}

ImTextureID PlayerData::getAvatarTexture() const noexcept
{
    const auto it = std::as_const(playerAvatars).find(handle);
    if (it == playerAvatars.cend())
        return team == Team::TT ? avatarTT.getTexture() : avatarCT.getTexture();

    const auto& avatar = it->second;
    if (!avatar.texture.get())
        avatar.texture.init(32, 32, avatar.rgba.get());
    return avatar.texture.get();
}

static void clearSkillgroupTextures() noexcept
{
    for (const auto& img : skillgroupImages)
        img.clearTexture();
    for (const auto& img : dangerZoneImages)
        img.clearTexture();
}

ImTextureID PlayerData::getRankTexture() const noexcept
{
    if (gameModeName == "survival")
        return dangerZoneImages[std::size_t(skillgroup) < dangerZoneImages.size() ? skillgroup : 0].getTexture();
    else
        return skillgroupImages[std::size_t(skillgroup) < skillgroupImages.size() ? skillgroup : 0].getTexture();
}

WeaponData::WeaponData(Entity* entity) noexcept : BaseData{ entity }
{
    clip = entity->clip();
    reserveAmmo = entity->reserveAmmoCount();
    if (const auto weaponInfo = entity->getWeaponData()) {
        maxClip = weaponInfo->maxClip;
        group = [](WeaponType type, WeaponId weaponId) {
            switch (type) {
            case WeaponType::Pistol: return "Pistols";
            case WeaponType::SubMachinegun: return "SMGs";
            case WeaponType::Rifle: return "Rifles";
            case WeaponType::SniperRifle: return "Sniper Rifles";
            case WeaponType::Shotgun: return "Shotguns";
            case WeaponType::Machinegun: return "Machineguns";
            case WeaponType::Grenade: return "Grenades";
            case WeaponType::Melee: return "Melee";
            default:
                switch (weaponId) {
                case WeaponId::C4:
                case WeaponId::Healthshot:
                case WeaponId::BumpMine:
                case WeaponId::ZoneRepulsor:
                case WeaponId::Shield:
                    return "Other";
                default: return "All";
                }
            }
        }(weaponInfo->type, entity->itemDefinitionIndex2());
        switch (entity->itemDefinitionIndex2())
        {
        case WeaponId::None:
            icon = "";
            break;
        case WeaponId::Awp:
            icon = "Z";
            break;
        case WeaponId::Ak47:
            icon = "W";
            break;
        case WeaponId::Ssg08:
            icon = "a";
            break;
        case WeaponId::Deagle:
            icon = "A";
            break;
        case WeaponId::Elite:
            icon = "B";
            break;
        case WeaponId::Fiveseven:
            icon = "C";
            break;
        case WeaponId::Glock:
            icon = "D";
            break;
        case WeaponId::P250:
            icon = "E";
            break;
        case WeaponId::Hkp2000:
            icon = "F";
            break;
        case WeaponId::Usp_s:
            icon = "G";
            break;
        case WeaponId::Tec9:
            icon = "H";
            break;
        case WeaponId::Taser:
            icon = "h";
            break;
        case WeaponId::Cz75a:
            icon = "I";
            break;
        case WeaponId::Revolver:
            icon = "J";
            break;
        case WeaponId::Xm1014:
            icon = "b";
            break;
        case WeaponId::Sawedoff:
            icon = "c";
            break;
        case WeaponId::Mag7:
            icon = "d";
            break;
        case WeaponId::Nova:
            icon = "e";
            break;
        case WeaponId::Negev:
            icon = "f";
            break;
        case WeaponId::M249:
            icon = "g";
            break;
        case WeaponId::Flashbang:
            icon = "i";
            break;
        case WeaponId::HeGrenade:
            icon = "j";
            break;
        case WeaponId::Mac10:
            icon = "K";
            break;
        case WeaponId::SmokeGrenade:
            icon = "k";
            break;
        case WeaponId::Ump45:
            icon = "L";
            break;
        case WeaponId::Molotov:
            icon = "l";
            break;
        case WeaponId::Bizon:
            icon = "M";
            break;
        case WeaponId::Decoy:
            icon = "m";
            break;
        case WeaponId::Mp7:
            icon = "N";
            break;
        case WeaponId::IncGrenade:
            icon = "n";
            break;
        case WeaponId::C4:
            icon = "o";
            break;
        case WeaponId::Mp9:
            icon = "O";
            break;
        case WeaponId::P90:
            icon = "P";
            break;
        case WeaponId::GalilAr:
            icon = "Q";
            break;
        case WeaponId::Famas:
            icon = "R";
            break;
        case WeaponId::M4A1:
            icon = "S";
            break;
        case WeaponId::M4a1_s:
            icon = "T";
            break;
        case WeaponId::Aug:
            icon = "U";
            break;
        case WeaponId::Sg553:
            icon = "V";
            break;
        case WeaponId::G3SG1:
            icon = "X";
            break;
        case WeaponId::Scar20:
            icon = "Y";
            break;
        case WeaponId::KnifeT:
            icon = "[";
            break;
        case WeaponId::Knife:
            icon = "]";
            break;
        case WeaponId::GoldenKnife:
            icon = "]";
            break;
        case WeaponId::Falchion:
            icon = "0";
            break;
        case WeaponId::Huntsman:
            icon = "6";
            break;
        case WeaponId::Butterfly:
            icon = "8";
            break;
        case WeaponId::Daggers:
            icon = "9";
            break;
        case WeaponId::Karambit:
            icon = "4";
            break;
        case WeaponId::Gut:
            icon = "3";
            break;
        case WeaponId::Flip:
            icon = "2";
            break;
        case WeaponId::M9Bayonet:
            icon = "5";
            break;
        case WeaponId::Bayonet:
            icon = "1";
            break;
        case WeaponId::Bowie:
            icon = "7";
            break;
        }
        name = [](WeaponId weaponId) {
                switch (weaponId) {
                default: return "All";

                case WeaponId::Glock: return "GLOCK-18";
                case WeaponId::Hkp2000: return "P2000";
                case WeaponId::Usp_s: return "USP-S";
                case WeaponId::Elite: return "DUAL BERETTAS";
                case WeaponId::P250: return "P250";
                case WeaponId::Tec9: return "TEC-9";
                case WeaponId::Fiveseven: return "FIVE-SEVEN";
                case WeaponId::Cz75a: return "CZ75";
                case WeaponId::Deagle: return "DESERT EAGLE";
                case WeaponId::Revolver: return "R8 REVOLVER";

                case WeaponId::Mac10: return "MAC-10";
                case WeaponId::Mp9: return "MP9";
                case WeaponId::Mp7: return "MP7";
                case WeaponId::Mp5sd: return "MP5-SD";
                case WeaponId::Ump45: return "UMP-45";
                case WeaponId::P90: return "P90";
                case WeaponId::Bizon: return "PP-BIZON";

                case WeaponId::GalilAr: return "GALIL";
                case WeaponId::Famas: return "FAMAS";
                case WeaponId::Ak47: return "AK-47";
                case WeaponId::M4A1: return "M4A4";
                case WeaponId::M4a1_s: return "M4A1-S";
                case WeaponId::Sg553: return "SG-553";
                case WeaponId::Aug: return "AUG";

                case WeaponId::Ssg08: return "SSG 08";
                case WeaponId::Awp: return "AWP";
                case WeaponId::G3SG1: return "G3SG1";
                case WeaponId::Scar20: return "SCAR-20";

                case WeaponId::Nova: return "NOVA";
                case WeaponId::Xm1014: return "XM1014";
                case WeaponId::Sawedoff: return "SAWED-Off";
                case WeaponId::Mag7: return "MAG-7";

                case WeaponId::M249: return "M249";
                case WeaponId::Negev: return "NEGEV";

                case WeaponId::Flashbang: return "FLASHBANG";
                case WeaponId::HeGrenade: return "HE GRENADE";
                case WeaponId::SmokeGrenade: return "SMOKE GRENADE";
                case WeaponId::Molotov: return "MOLOTOV";
                case WeaponId::Decoy: return "DECOY";
                case WeaponId::IncGrenade: return "INCENDIARY";
                case WeaponId::TaGrenade: return "TA Grenade";
                case WeaponId::Firebomb: return "FIRE BOMB";
                case WeaponId::Diversion: return "DIVERSION";
                case WeaponId::FragGrenade: return "FRAG GRENADE";
                case WeaponId::Snowball: return "SNOWBALL";
                case WeaponId::Taser: return "ZEUS x27";
                case WeaponId::Axe: return "AXE";
                case WeaponId::Hammer: return "HAMMER";
                case WeaponId::Spanner: return "WRENCH";

                case WeaponId::C4: return "C4";
                case WeaponId::Healthshot: return "HEALTHSHOT";
                case WeaponId::BumpMine: return "BUMP MINE";
                case WeaponId::ZoneRepulsor: return "ZONE REPULSOR";
                case WeaponId::Shield: return "SHIELD";
                }            
        }(entity->itemDefinitionIndex2());

        displayName = interfaces->localize->findAsUTF8(weaponInfo->name);
    }
}

float PlayerData::fadingAlpha() const noexcept
{
    float fadeTime = enemy ? config->esp.enemy.dormantTime : config->esp.allies.dormantTime;
    return std::clamp(1.0f - (memory->globalVars->realtime - lastContactTime - 0.25f) / fadeTime, 0.0f, 1.0f);
}

LootCrateData::LootCrateData(Entity* entity) noexcept : BaseData{ entity }
{
    const auto model = entity->getModel();
    if (!model)
        return;

    name = [](const char* modelName) -> const char* {
        switch (fnv::hashRuntime(modelName)) {
        case fnv::hash("models/props_survival/cases/case_pistol.mdl"): return "Pistol Case";
        case fnv::hash("models/props_survival/cases/case_light_weapon.mdl"): return "Light Case";
        case fnv::hash("models/props_survival/cases/case_heavy_weapon.mdl"): return "Heavy Case";
        case fnv::hash("models/props_survival/cases/case_explosive.mdl"): return "Explosive Case";
        case fnv::hash("models/props_survival/cases/case_tools.mdl"): return "Tools Case";
        case fnv::hash("models/props_survival/cash/dufflebag.mdl"): return "Cash Dufflebag";
        default: return nullptr;
        }
    }(model->name);
}

ObserverData::ObserverData(Entity* entity, Entity* obs, bool targetIsLocalPlayer) noexcept : playerHandle{ entity->handle() }, targetHandle{ obs->handle() }, targetIsLocalPlayer{ targetIsLocalPlayer } {}

void BombData::update() noexcept
{
    if (memory->plantedC4s->size > 0 && (!*memory->gameRules || (*memory->gameRules)->mapHasBombTarget())) {
        if (const auto bomb = (*memory->plantedC4s)[0]; bomb && bomb->c4Ticking()) {
            blowTime = bomb->c4BlowTime();
            timerLength = bomb->c4TimerLength();
            defuserHandle = bomb->c4Defuser();
            if (defuserHandle != -1) {
                defuseCountDown = bomb->c4DefuseCountDown();
                defuseLength = bomb->c4DefuseLength();
            }

            if (*memory->playerResource) {
                const auto& bombOrigin = bomb->origin();
                bombsite = bombOrigin.distTo((*memory->playerResource)->bombsiteCenterA()) > bombOrigin.distTo((*memory->playerResource)->bombsiteCenterB());
            }
            return;
        }
    }
    blowTime = 0.0f;
}

InfernoData::InfernoData(Entity* inferno) noexcept
{
    origin = inferno->getAbsOrigin();

    points.reserve(inferno->fireCount());
    for (int i = 0; i < inferno->fireCount(); ++i) {
        if (inferno->fireIsBurning()[i])
            points.emplace_back(inferno->fireXDelta()[i] + origin.x, inferno->fireYDelta()[i] + origin.y, inferno->fireZDelta()[i] + origin.z);
    }
}

SmokeData::SmokeData(Entity* smoke) noexcept
{
    origin = smoke->getAbsOrigin();
}
