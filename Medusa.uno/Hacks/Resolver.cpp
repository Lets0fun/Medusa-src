#include "AimbotFunctions.h"
#include "Animations.h"
#include "Resolver.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"
#include "../Vector2D.hpp"
#include <DirectXMath.h>
#include <algorithm>
#include "../xor.h"
#include "Backtrack.h"
std::deque<Resolver::SnapShot> snapshots;
static std::array<Animations::Players, 65> players{};
UserCmd* cmd;
bool resolver = true;
bool occlusion = false;

void Resolver::CmdGrabber(UserCmd* cmd1)
{
    cmd = cmd1;
}

void Resolver::reset() noexcept
{
	snapshots.clear();
}

void Resolver::saveRecord(int playerIndex, float playerSimulationTime) noexcept
{
	const auto entity = interfaces->entityList->getEntity(playerIndex);
	const auto player = Animations::getPlayer(playerIndex);
	if (!player.gotMatrix || !entity)
		return;

	SnapShot snapshot;
	snapshot.player = player;
	snapshot.playerIndex = playerIndex;
	snapshot.eyePosition = localPlayer->getEyePosition();
	snapshot.model = entity->getModel();

    if (player.simulationTime >= playerSimulationTime - 0.001f && player.simulationTime <= playerSimulationTime + 0.001f)
    {
        snapshots.push_back(snapshot);
        return;
    }

    for (int i = 0; i < static_cast<int>(player.backtrackRecords.size()); i++)
    {
        if (player.backtrackRecords.at(i).simulationTime >= playerSimulationTime - 0.001f && player.backtrackRecords.at(i).simulationTime <= playerSimulationTime + 0.001f)
        {
            snapshot.backtrackRecord = i;
            snapshots.push_back(snapshot);
            return;
        }
    }
}

void Resolver::getEvent(GameEvent* event) noexcept
{
	if (!event || !localPlayer || interfaces->engine->isHLTV())
		return;

	switch (fnv::hashRuntime(event->getName())) {
	case fnv::hash("round_start"):
	{
		//Reset all
		auto players = Animations::setPlayers();
		if (players->empty())
			break;

		for (int i = 0; i < static_cast<int>(players->size()); i++)
		{
			players->at(i).misses = 0;
		}
		snapshots.clear();
		break;
	}	
	case fnv::hash("player_death"):
	{
		//Reset player
		const auto playerId = event->getInt(skCrypt("userid"));
		if (playerId == localPlayer->getUserId())
			break;

		const auto index = interfaces->engine->getPlayerFromUserID(playerId);
		Animations::setPlayer(index)->misses = 0;
		break;
	}
	case fnv::hash("player_hurt"):
	{
		if (snapshots.empty())
			break;

		if (event->getInt(skCrypt("attacker")) != localPlayer->getUserId())
			break;

		const auto hitgroup = event->getInt("hitgroup");
		if (hitgroup < HitGroup::Head || hitgroup > HitGroup::RightLeg)
			break;

		snapshots.pop_front(); //Hit somebody so dont calculate
		break;
	}
	case fnv::hash("bullet_impact"):
	{
		if (snapshots.empty())
			break;

		if (event->getInt("userid") != localPlayer->getUserId())
			break;

		auto& snapshot = snapshots.front();

		if (!snapshot.gotImpact)
		{
			snapshot.time = memory->globalVars->serverTime();
			snapshot.bulletImpact = Vector{ event->getFloat("x"), event->getFloat("y"), event->getFloat("z") };
			snapshot.gotImpact = true;
		}
		break;
	}
	default:
		break;
	}
	if (!resolver)
		snapshots.clear();
}

float get_backward_side(Entity* entity) {
    if (!entity->isAlive())
        return -1.f;
    const float result = Helpers::angleDiff(localPlayer->origin().y, entity->origin().y);
    return result;
}
float get_angle(Entity* entity) {
    return Helpers::angleNormalize(entity->eyeAngles().y);
}
float get_forward_yaw(Entity* entity) {
    return Helpers::angleNormalize(get_backward_side(entity) - 180.f);
}

Vector calcAngle(Vector source, Vector entityPos) {
	Vector delta = {};
	delta.x = source.x - entityPos.x;
	delta.y = source.y - entityPos.y;
	delta.z = source.z - entityPos.z;
	Vector angles = {};
	Vector viewangles = cmd->viewangles;
	angles.x = Helpers::rad2deg(atan(delta.z / hypot(delta.x, delta.y))) - viewangles.x;
	angles.y = Helpers::rad2deg(atan(delta.y / delta.x)) - viewangles.y;
	angles.z = 0;
	if (delta.x >= 0.f)
		angles.y += 180;

	return angles;
}

float build_server_abs_yaw(Entity* entity, const float angle)
{
    Vector velocity = entity->velocity();
    const auto& anim_state = entity->getAnimstate();
    const float m_fl_eye_yaw = angle;
    float m_fl_goal_feet_yaw = 0.f;

    const float eye_feet_delta = Helpers::angleDiff(m_fl_eye_yaw, m_fl_goal_feet_yaw);

    static auto get_smoothed_velocity = [](const float min_delta, const Vector a, const Vector b) {
        const Vector delta = a - b;
        const float delta_length = delta.length();

        if (delta_length <= min_delta)
        {
            if (-min_delta <= delta_length)
                return a;
            const float i_radius = 1.0f / (delta_length + FLT_EPSILON);
            return b - delta * i_radius * min_delta;
        }
        const float i_radius = 1.0f / (delta_length + FLT_EPSILON);
        return b + delta * i_radius * min_delta;
    };

    if (const float spd = velocity.squareLength(); spd > std::powf(1.2f * 260.0f, 2.f))
    {
        const Vector velocity_normalized = velocity.normalized();
        velocity = velocity_normalized * (1.2f * 260.0f);
    }

    const float m_fl_choked_time = anim_state->lastUpdateTime;
    const float duck_additional = std::clamp(entity->duckAmount() + anim_state->duckAdditional, 0.0f, 1.0f);
    const float duck_amount = anim_state->animDuckAmount;
    const float choked_time = m_fl_choked_time * 6.0f;
    float v28;

    // clamp
    if (duck_additional - duck_amount <= choked_time)
        if (duck_additional - duck_amount >= -choked_time)
            v28 = duck_additional;
        else
            v28 = duck_amount - choked_time;
    else
        v28 = duck_amount + choked_time;

    const float fl_duck_amount = std::clamp(v28, 0.0f, 1.0f);

    const Vector animation_velocity = get_smoothed_velocity(m_fl_choked_time * 2000.0f, velocity, entity->velocity());
    const float speed = std::fminf(animation_velocity.length(), 260.0f);

    float fl_max_movement_speed = 260.0f;

    if (Entity* p_weapon = entity->getActiveWeapon(); p_weapon && p_weapon->getWeaponData())
        fl_max_movement_speed = std::fmaxf(p_weapon->getWeaponData()->maxSpeed, 0.001f);

    float fl_running_speed = speed / (fl_max_movement_speed * 0.520f);
    float fl_ducking_speed = speed / (fl_max_movement_speed * 0.340f);
    fl_running_speed = std::clamp(fl_running_speed, 0.0f, 1.0f);

    float fl_yaw_modifier = (anim_state->walkToRunTransition * -0.3f - 0.2f) * fl_running_speed + 1.0f;

    if (fl_duck_amount > 0.0f)
    {
        float fl_ducking_speed2 = std::clamp(fl_ducking_speed, 0.0f, 1.0f);
        fl_yaw_modifier += (fl_ducking_speed2 * fl_duck_amount) * (0.5f - fl_yaw_modifier);
    }

    constexpr float v60 = -58.f;
    constexpr float v61 = 58.f;

    const float fl_min_yaw_modifier = v60 * fl_yaw_modifier;

    if (const float fl_max_yaw_modifier = v61 * fl_yaw_modifier; eye_feet_delta <= fl_max_yaw_modifier)
    {
        if (fl_min_yaw_modifier > eye_feet_delta)
            m_fl_goal_feet_yaw = fabs(fl_min_yaw_modifier) + m_fl_eye_yaw;
    }
    else
    {
        m_fl_goal_feet_yaw = m_fl_eye_yaw - fabs(fl_max_yaw_modifier);
    }

    Helpers::normalizeYaw(m_fl_goal_feet_yaw);

    if (speed > 0.1f || fabs(velocity.z) > 100.0f)
    {
        m_fl_goal_feet_yaw = Helpers::approachAngle(
            m_fl_eye_yaw,
            m_fl_goal_feet_yaw,
            (anim_state->walkToRunTransition * 20.0f + 30.0f)
            * m_fl_choked_time);
    }
    else
    {
        m_fl_goal_feet_yaw = Helpers::approachAngle(
            entity->lby(),
            m_fl_goal_feet_yaw,
            m_fl_choked_time * 100.0f);
    }

    return m_fl_goal_feet_yaw;
}

void Resolver::detect_side(Entity* entity, int* side) {
    /* externals */
    Vector forward{};
    Vector right{};
    Vector up{};
    Trace tr;
    Helpers::AngleVectors(Vector(0, get_backward_side(entity), 0), &forward, &right, &up);
    /* filtering */

    const Vector src_3d = entity->getEyePosition();
    const Vector dst_3d = src_3d + forward * 384;

    /* back engine tracers */
    interfaces->engineTrace->traceRay({ src_3d, dst_3d }, MASK_SHOT, { entity }, tr);
    float back_two = (tr.endpos - tr.startpos).length();

    /* right engine tracers */
    interfaces->engineTrace->traceRay(Ray(src_3d + right * 35, dst_3d + right * 35), MASK_SHOT, { entity }, tr);
    float right_two = (tr.endpos - tr.startpos).length();

    /* left engine tracers */
    interfaces->engineTrace->traceRay(Ray(src_3d - right * 35, dst_3d - right * 35), MASK_SHOT, { entity }, tr);
    float left_two = (tr.endpos - tr.startpos).length();
    /* fix side */
    if (left_two > right_two) {
        *side = -1;
    }
    else if (right_two > left_two) {
        *side = 1;
    }
    else
        *side = 0;
}

bool DesyncDetect(Entity* entity)
{
    if (!localPlayer)
        return false;
    if (!entity || !entity->isAlive())
        return false;
    if (entity->isBot())
        return false;
    if (entity->team() == localPlayer->team())
        return false;
    if (entity->moveType() == MoveType::NOCLIP || entity->moveType() == MoveType::LADDER)
        return false;
    return true;
}

bool isSlowWalking(Entity* entity)
{
    float velocity_2D[64]{}, old_velocity_2D[64]{};
    if (entity->velocity().length2D() != velocity_2D[entity->index()] && entity->velocity().length2D() != NULL) {
        old_velocity_2D[entity->index()] = velocity_2D[entity->index()];
        velocity_2D[entity->index()] = entity->velocity().length2D();
    }
    Vector velocity = entity->velocity();
    Vector direction = entity->eyeAngles();

    float speed = velocity.length();
    direction.y = entity->eyeAngles().y - direction.y;
    //method 1
    if (velocity_2D[entity->index()] > 1) {
        int tick_counter[64]{};
        if (velocity_2D[entity->index()] == old_velocity_2D[entity->index()])
            tick_counter[entity->index()] += 1;
        else
            tick_counter[entity->index()] = 0;

        while (tick_counter[entity->index()] > (1 / memory->globalVars->intervalPerTick * fabsf(0.1f)))//should give use 100ms in ticks if their speed stays the same for that long they are definetely up to something..
            return true;
    }


    return false;
}

int GetChokedPackets(Entity* entity) {
    int last_ticks[65]{};
    auto ticks = timeToTicks(entity->simulationTime() - entity->oldSimulationTime());
    if (ticks == 0 && last_ticks[entity->index()] > 0) {
        return last_ticks[entity->index()] - 1;
    }
    else {
        last_ticks[entity->index()] = ticks;
        return ticks;
    }
}
void Resolver::resolve_entity(const Animations::Players& player, Entity* entity) {
    // get the players max rotation.
    if (DesyncDetect(entity) == false)
        return;
    float max_rotation = entity->getMaxDesyncAngle();
    int index = 0;
    const float eye_yaw = entity->getAnimstate()->eyeYaw;
    if (!player.extended && fabs(max_rotation) > 60.f)
    {
        max_rotation = max_rotation / 1.8f;
    }

    // resolve shooting players separately.
    if (player.shot) {
        entity->getAnimstate()->footYaw = eye_yaw + resolve_shot(player, entity);
        return;
    }
    if (entity->velocity().length2D() <= 2.5f) {
        const float angle_difference = Helpers::angleDiff(eye_yaw, entity->getAnimstate()->footYaw);
        index = 2 * angle_difference <= 0.0f ? 1 : -1;
    }
    else
    {
        if (!static_cast<int>(player.layers[12].weight * 1000.f) && entity->velocity().length2D() > 3.f) {
            const auto m_layer_delta1 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);
            const auto m_layer_delta2 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);

            if (const auto m_layer_delta3 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate); m_layer_delta1 < m_layer_delta2
                || m_layer_delta3 <= m_layer_delta2
                || static_cast<signed int>((m_layer_delta2 * 1000.0f)))
            {
                if (m_layer_delta1 >= m_layer_delta3
                    && m_layer_delta2 > m_layer_delta3
                    && !static_cast<signed int>((m_layer_delta3 * 1000.0f)))
                {
                    index = 1;
                }
            }
            else
            {
                index = -1;
            }
        }
    }

    switch (player.misses % 3) {
    case 0: //default
        entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y + max_rotation * static_cast<float>(index));
        break;
    case 1: //reverse
        entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y + max_rotation * static_cast<float>(-index));
        break;
    case 2: //middle
        entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y);
        break;
    default: break;
    }

}

bool IsAdjustingBalance(const Animations::Players& player, Entity* entity)
{
    for (int i = 0; i < 15; i++)
    {
        const int activity = entity->sequence();
        if (activity == 979)
        {
            return true;
        }
    }
    return false;
}

bool is_breaking_lby(const Animations::Players& player, Entity* entity, AnimationLayer cur_layer, AnimationLayer prev_layer)
{
    if (IsAdjustingBalance(player, entity))
    {
            if ((prev_layer.cycle != cur_layer.cycle) || cur_layer.weight == 1.f)
            {
                return true;
            }
            else if (cur_layer.cycle == 0.f && (prev_layer.cycle > 0.92f && cur_layer.cycle > 0.92f))
            {
                return true;
            }
    }

    return false;
}
float Resolver::resolve_shot(const Animations::Players& player, Entity* entity) {
    /* fix unrestricted shot */
    if (DesyncDetect(entity) == false)
        return 0;

    const float fl_pseudo_fire_yaw = Helpers::angleNormalize(Helpers::angleDiff(localPlayer->origin().y, player.matrix[8].origin().y));
    if (is_breaking_lby(player, entity, player.layers[3], player.oldlayers[3])) {
        const float fl_left_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y + 58.f)));
        const float fl_right_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y - 58.f)));
        return fl_left_fire_yaw_delta > fl_right_fire_yaw_delta ? -58.f : 58.f;
    }
    const float fl_left_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y + 28.f)));
    const float fl_right_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y - 28.f)));

    return fl_left_fire_yaw_delta > fl_right_fire_yaw_delta ? -28.f : 28.f;
}

void Resolver::setup_detect(Animations::Players& player, Entity* entity) {

    // detect if player is using maximum desync.
    if (is_breaking_lby(player, entity, player.layers[3], player.oldlayers[3]))
    {
        player.extended = true;
    }
    /* calling detect side */
    detect_side(entity, &player.side);
    const int side = player.side;
    /* brute-forcing vars */
    float resolve_value = 50.f;
    static float brute = 0.f;
    const float fl_max_rotation = entity->getMaxDesyncAngle();
    const float fl_eye_yaw = entity->getAnimstate()->eyeYaw;
    const bool fl_forward = fabsf(Helpers::angleNormalize(get_angle(entity) - get_forward_yaw(entity))) < 90.f;
    const int fl_shots = player.misses;

    /* clamp angle */
    if (fl_max_rotation < resolve_value) {
        resolve_value = fl_max_rotation;
    }

    /* detect if entity is using max desync angle */
    if (player.extended) {
        resolve_value = fl_max_rotation;
    }

    const float perfect_resolve_yaw = resolve_value;

    /* setup brute-forcing */
    if (fl_shots == 0) {
        brute = perfect_resolve_yaw * static_cast<float>(fl_forward ? -side : side);
    }
    else {
        switch (fl_shots % 3) {
        case 0: {
            brute = perfect_resolve_yaw * static_cast<float>(fl_forward ? -side : side);
        } break;
        case 1: {
            brute = perfect_resolve_yaw * static_cast<float>(fl_forward ? side : -side);
        } break;
        case 2: {
            brute = 0;
        } break;
        default: break;
        }
    }

    /* fix goal feet yaw */
    entity->getAnimstate()->footYaw = fl_eye_yaw + brute;
}

Vector calc_angle(const Vector source, const Vector entity_pos) {
    const Vector delta{ source.x - entity_pos.x, source.y - entity_pos.y, source.z - entity_pos.z };
    const auto& [x, y, z] = cmd->viewangles;
    Vector angles{ Helpers::rad2deg(atan(delta.z / hypot(delta.x, delta.y))) - x, Helpers::rad2deg(atan(delta.y / delta.x)) - y, 0.f };
    if (delta.x >= 0.f)
        angles.y += 180;
    return angles;
}

void Resolver::processMissedShots() noexcept
{
	if (!resolver)
	{
		snapshots.clear();
		return;
	}

	if (!localPlayer)
	{
		snapshots.clear();
		return;
	}

	if (snapshots.empty())
		return;

	if (snapshots.front().time == -1) //Didnt get data yet
		return;

	auto snapshot = snapshots.front();
	snapshots.pop_front(); //got the info no need for this
    const auto& time = localPlayer->isAlive() ? localPlayer->tickBase() * memory->globalVars->intervalPerTick : memory->globalVars->currenttime;
    if (fabs(time - snapshot.time) > 1.f)
    {
        if (snapshot.gotImpact)
            Logger::addLog(c_xor("missed shot due to ping"));
        else
            Logger::addLog(c_xor("missed shot due to server rejection"));
        snapshots.clear();
        return;
    }
	if (!snapshot.player.gotMatrix)
		return;

	const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);
	if (!entity)
		return;

	const Model* model = snapshot.model;
	if (!model)
		return;

	StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
	if (!hdr)
		return;

	StudioHitboxSet* set = hdr->getHitboxSet(0);
	if (!set)
		return;

	const auto angle = AimbotFunction::calculateRelativeAngle(snapshot.eyePosition, snapshot.bulletImpact, Vector{ });
	const auto end = snapshot.bulletImpact + Vector::fromAngle(angle) * 2000.f;

	const auto matrix = snapshot.backtrackRecord <= -1 ? snapshot.player.matrix.data() : snapshot.player.backtrackRecords.at(snapshot.backtrackRecord).matrix;

	bool resolverMissed = false;    
	for (int hitbox = 0; hitbox < Hitboxes::Max; hitbox++)
	{
		if (AimbotFunction::hitboxIntersection(matrix, hitbox, set, snapshot.eyePosition, end))
		{
			resolverMissed = true;
			std::string missed = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to resolver"));
			std::string missedBT = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to invalid backtrack tick [")) + std::to_string(snapshot.backtrackRecord) + "]";
			std::string missedPred = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to prediction error"));
			std::string missedJitter = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to jitter"));
            if (snapshot.backtrackRecord == 1 && config->backtrack.enabled)
                Logger::addLog(missedJitter);
            else if (snapshot.backtrackRecord > 1 && config->backtrack.enabled)
                Logger::addLog(missedBT);
			else
				Logger::addLog(missed);
			Animations::setPlayer(snapshot.playerIndex)->misses++;
			break;
		}
	}
	if (!resolverMissed)
		Logger::addLog(std::string(skCrypt("missed shot due to spread")));
}

void Resolver::runPreUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

    if (!entity->isDormant())
        return;

	if (player.chokedPackets <= 0)
		return;

	if (snapshots.empty())
		return;

    //auto& [snapshot_player, model, eyePosition, bulletImpact, gotImpact, time, playerIndex, backtrackRecord] = snapshots.front();
    setup_detect(player, entity);
    resolve_entity(player, entity);
}

void Resolver::runPostUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

    if (!entity->isDormant())
        return;

	if (player.chokedPackets <= 0)
		return;

	if (snapshots.empty())
		return;

    //auto& [snapshot_player, model, eyePosition, bulletImpact, gotImpact, time, playerIndex, backtrackRecord] = snapshots.front();
    setup_detect(player, entity);
    resolve_entity(player, entity);
}

void Resolver::updateEventListeners(bool forceRemove) noexcept
{
	class ImpactEventListener : public GameEventListener {
	public:
		void fireGameEvent(GameEvent* event) {
			getEvent(event);
		}
	};

	static ImpactEventListener listener[4];
	static bool listenerRegistered = false;

	if (resolver && !listenerRegistered) {
		interfaces->gameEventManager->addListener(&listener[0], skCrypt("bullet_impact"));
		interfaces->gameEventManager->addListener(&listener[1], skCrypt("player_hurt"));
		interfaces->gameEventManager->addListener(&listener[2], skCrypt("round_start"));
		interfaces->gameEventManager->addListener(&listener[3], skCrypt("weapon_fire"));
		listenerRegistered = true;
	}

	else if ((!resolver || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener[0]);
		interfaces->gameEventManager->removeListener(&listener[1]);
		interfaces->gameEventManager->removeListener(&listener[2]);
		interfaces->gameEventManager->removeListener(&listener[3]);
		listenerRegistered = false;
	}
}
