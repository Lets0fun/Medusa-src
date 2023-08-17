#include "NadePredEXP.h"

#include "../SDK/Entity.h"
#include "../SDK/ClientClass.h"
#include "../SDK/EntityList.h"
#include "../SDK/ModelRender.h"
#include "../SDK/GlobalVars.h"
#include "../Memory.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/Engine.h"
#include "../SDK/Vector.h"
#include "../Interfaces.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/ITexture.h"
#include "../SDK/RenderContext.h"
#include "../SDK/RenderView.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ViewSetup.h"

#include "../Hooks.h"
#include "../GameData.h"
#include "../GUI.h"

#include <mutex>
#include "../fontawesome.h"
struct nadeDraw {
	std::vector<Vector> trail;
	std::vector<Vector> collision;
	Vector ground;
	bool drawGroundCircle = true;
	bool drawWarning = false;
	float timeToDetonate = 0.0f;
	float spawnTime = 0.0f;
	WeaponId type = WeaponId::None;
};

std::vector<nadeDraw> nadeDrawList;
std::mutex renderMutex;
namespace NadePrediction
{
	inline ITexture* nadeViewTexture;
	inline ViewSetup nadeViewSetup;
	inline Matrix4x4 nadeViewMatrix;
	inline Vector nadeViewOrigin;
	inline bool shouldRenderView = false;
}

struct GpConfig {
	bool enabled{ false };
	struct Warning {
		bool enabled = true;
		bool enemiesOnly = false;
		bool flash = true;
		bool HE = true;
		bool smoke = true;
		bool molotov = true;
		bool decoy = false;
	} warning;

	ColorToggleThickness trail;
	ColorToggle colision{ false, 1.0f,.0f, .0f, 1.0f };
	ColorToggle ground{ true, 0.895f, 0.209f, 0.642f, 0.216f };
	struct NadeView {
		bool enabled = true;
		ImVec2 nadeViewPosition{};
		bool noTitleBar{ false };
		float height{ 200.f };
		float FOV{ 100.f };
		float speed{ 32.f };
		bool pinpulledOnly = false;
	}nadeView;

}gpConfig;

static void TraceHull(Vector& src, Vector& end, Trace& tr) noexcept
{
	TraceFilterWorldAndPropsOnly customFilter;
	interfaces->engineTrace->traceRay1({ src, end, Vector{-2.0f, -2.0f, -2.0f}, Vector{2.0f, 2.0f, 2.0f} }, 0x200400B, customFilter, tr);
}

static void Setup(Vector& vecSrc, Vector& vecThrow, float throwStrength, const Vector& velocity, const Vector& viewangles) noexcept
{
	Vector angThrow = viewangles;
	float pitch = angThrow.x;

	if (pitch <= 90.0f)
	{
		if (pitch < -90.0f)
		{
			pitch += 360.0f;
		}
	}
	else
	{
		pitch -= 360.0f;
	}

	float a = pitch - (90.0f - fabs(pitch)) * 10.0f / 90.0f;
	angThrow.x = a;

	float flVel = 750.0f * 0.9f;

	float b = throwStrength;
	b = b * 0.7f;
	b = b + 0.3f;
	flVel *= b;

	Vector vForward = Vector::fromAngle(angThrow);

	float off = (throwStrength * 12.0f) - 12.0f;
	vecSrc.z += off;

	Trace tr;
	Vector vecDest = vecSrc;
	vecDest += vForward * 22.0f;
	TraceHull(vecSrc, vecDest, tr);
	Vector vecBack = vForward; vecBack *= 6.0f;
	vecSrc = tr.endpos;
	vecSrc -= vecBack;

	vecThrow = velocity; vecThrow *= 1.25f;
	vecThrow += vForward * flVel;
}

static int PhysicsClipVelocity(const Vector& in, const Vector& normal, Vector& out, float overbounce) noexcept
{
	static const float STOP_EPSILON = 0.1f;

	float    backoff;
	float    change;
	float    angle;
	int        i, blocked;

	blocked = 0;

	angle = normal[2];

	if (angle > 0)
	{
		blocked |= 1;        // floor
	}
	if (!angle)
	{
		blocked |= 2;        // step
	}

	backoff = in.dotProduct(normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
		{
			out[i] = 0;
		}
	}

	return blocked;
}

static void PushEntity(Vector& src, const Vector& move, Trace& tr) noexcept
{
	Vector vecAbsEnd = src;
	vecAbsEnd += move;
	TraceHull(src, vecAbsEnd, tr);
}

static void ResolveFlyCollisionCustom(Trace& tr, Vector& vecVelocity, float interval, std::vector<Entity*>& brokenEntities) noexcept
{
	// Calculate elasticity
	const float surfaceElasticity = 1.0;
	const float grenadeElasticity = 0.45f;
	float totalElasticity = grenadeElasticity * surfaceElasticity;
	if (totalElasticity > 0.9f) totalElasticity = 0.9f;
	if (totalElasticity < 0.0f) totalElasticity = 0.0f;

	// Calculate bounce
	Vector vecAbsVelocity;
	PhysicsClipVelocity(vecVelocity, tr.plane.normal, vecAbsVelocity, 2.0f);
	vecAbsVelocity *= totalElasticity;

	float speedSqr = vecAbsVelocity.squareLength();
	static const float minSpeedSqr = 20.0f * 20.0f;

	if (speedSqr < minSpeedSqr)
	{
		vecAbsVelocity.x = 0.0f;
		vecAbsVelocity.y = 0.0f;
		vecAbsVelocity.z = 0.0f;
	}

	if (tr.plane.normal.z > 0.7f)
	{
		vecVelocity = vecAbsVelocity;
		vecAbsVelocity *= ((1.0f - tr.fraction) * interval);
		PushEntity(tr.endpos, vecAbsVelocity, tr);
	}
	else
	{
		vecVelocity = vecAbsVelocity;
	}
}

static void AddGravityMove(Vector& move, Vector& vel, float frametime, bool onground) noexcept
{
	Vector basevel{ 0.0f, 0.0f, 0.0f };

	move.x = (vel.x + basevel.x) * frametime;
	move.y = (vel.y + basevel.y) * frametime;

	if (onground)
	{
		move.z = (vel.z + basevel.z) * frametime;
	}
	else
	{
		float gravity = 800.0f * 0.4f;
		float newZ = vel.z - (gravity * frametime);
		move.z = ((vel.z + newZ) / 2.0f + basevel.z) * frametime;
		vel.z = newZ;
	}
}

static bool checkDetonate(const Vector& vecThrow, const Trace& tr, int tick, WeaponId activeWeaponId, float time = 0.0f)
{
	switch (activeWeaponId)
	{
	case WeaponId::Flashbang:
	case WeaponId::HeGrenade:
		//return ticksToTime(tick) >= 1.5f && !(tick % timeToTicks(0.2f));
		return time >= 1.5f && !(tick % timeToTicks(0.2f));
		break;
	case WeaponId::SmokeGrenade:
	case WeaponId::Decoy:
		return vecThrow.length2D() <= 0.2f && !(tick % timeToTicks(0.2f));
		break;
	case WeaponId::Molotov:
	case WeaponId::IncGrenade:
		if (tr.fraction != 1.0f && tr.plane.normal.z > 0.7f)
			return true;
		//return ticksToTime(tick) >= 2.0f /*molotov_throw_detonate_time*/ && !(tick % timeToTicks(0.1f));
		return time >= 2.0f /*molotov_throw_detonate_time*/ && !(tick % timeToTicks(0.1f));
		break;
	default:
		break;
	}
	return false;
}

static nadeDraw simulate(Vector vecSrc, Vector vecThrow, WeaponId type, int spawnTick = 0, int simulationOffset = 0, bool self = false)
{
	nadeDraw newNade;
	float interval = memory->globalVars->intervalPerTick;
	int logstep = timeToTicks(0.05f); //1.f
	int logtimer = 0;
	//int spawnTick = 0;
	float time = 0.0f;
	if (spawnTick > 0)
	{
		spawnTick += simulationOffset;
		time = memory->globalVars->currenttime - ticksToTime(spawnTick);
		spawnTick = memory->globalVars->tickCount - spawnTick;
	}
	std::vector<Entity*> brokenEntities;
	for (int tick = spawnTick; tick < timeToTicks(60.0f); ++tick) // maximum time for simulation is 60 seconds
	{
		if (!logtimer)
			newNade.trail.emplace_back(vecSrc);

		Vector move;
		AddGravityMove(move, vecThrow, interval, false);

		// Push entity
		Trace tr;
		PushEntity(vecSrc, move, tr);

		int result = 0;
		if (checkDetonate(vecThrow, tr, tick, type, time))
			result |= 1;

		if (tr.fraction != 1.0f && tr.didHit())
		{
			result |= 2; // Collision!
			ResolveFlyCollisionCustom(tr, vecThrow, interval, brokenEntities);
			newNade.collision.emplace_back(tr.endpos);
		}
		vecSrc = tr.endpos;

		if (result & 1)
			break;

		if ((result & 2) || logtimer >= logstep)
			logtimer = 0;
		else
			++logtimer;
		time += ticksToTime(1);
	}

	newNade.trail.emplace_back(vecSrc);
	newNade.ground = vecSrc;
	newNade.type = type;
	if (spawnTick)
	{
		newNade.timeToDetonate = time;
		newNade.spawnTime = ticksToTime(spawnTick);
	}
	/*if (self)
	{
		auto delta = newNade.ground - NadePrediction::nadeViewOrigin;
		delta /= gpConfig.nadeView.speed;
		NadePrediction::nadeViewOrigin += delta;
	}*/
	return newNade;
}

static nadeDraw simulateSelf(Vector eyePosition, Vector eyeAngle, Vector velocity, float throwStrength, WeaponId type)
{
	Setup(eyePosition, eyeAngle, throwStrength, velocity, eyeAngle);
	return simulate(eyePosition, eyeAngle, type, 0, 0, true);
}
static bool isGrenade(WeaponId id)
{
	switch (id)
	{
	case WeaponId::Flashbang:
	case WeaponId::HeGrenade:
	case WeaponId::SmokeGrenade:
	case WeaponId::Molotov:
	case WeaponId::Decoy:
	case WeaponId::IncGrenade:
		return true;
	default:
		return false;
		break;
	}
}
static bool isGrenadeWarningEnabled(WeaponId id)
{
	switch (id)
	{
	case WeaponId::Flashbang:
		return gpConfig.warning.flash;
		break;
	case WeaponId::HeGrenade:
		return gpConfig.warning.HE;
		break;
	case WeaponId::SmokeGrenade:
		return gpConfig.warning.smoke;
		break;
	case WeaponId::Molotov:
	case WeaponId::IncGrenade:
		return gpConfig.warning.molotov;
		break;
	case WeaponId::Decoy:
		return gpConfig.warning.decoy;
		break;
	default:
		return false;
		break;
	}
}
void NadePrediction::run() noexcept
{
	std::lock_guard<std::mutex> renderLock(renderMutex);
	nadeDrawList.clear();

	if (!gpConfig.enabled)
		return;
	GameData::Lock lock;
	const auto& local = GameData::local();
	if (!gpConfig.warning.enabled)
		return;

	for (const auto& projectile : GameData::projectiles())
	{
		if (projectile.trajectory.empty() || projectile.exploded || !projectile.Velocity.notNull())
			continue;
		if (gpConfig.warning.enemiesOnly && !projectile.thrownByEnemy)
			continue;
		if (!isGrenadeWarningEnabled(projectile.type))
			continue;
		auto& newNade = nadeDrawList.emplace_back(simulate(projectile.trajectory.back().second, projectile.Velocity, projectile.type, projectile.spawnTick, projectile.simulationOffset));
		newNade.drawGroundCircle = false;
		newNade.drawWarning = true;
	}
}

static void drawTrail(ImDrawList* drawList, const std::vector<Vector>& worldPoints, bool nadeView = false)
{
	if (!gpConfig.trail.enabled)
		return;

	ImVec2 currentPoint{};
	ImVec2 previousPoint{};
	const auto color = Helpers::calculateColor(gpConfig.trail);
	for (auto& point : worldPoints)
	{
		if (Helpers::worldToScreenPixelAligned(point, currentPoint))
		{
			if (previousPoint == ImVec2(0.0f, 0.0f))
			{
				previousPoint = currentPoint;
				continue;
			}
			drawList->AddLine(previousPoint, currentPoint, color, 2.5);
			previousPoint = currentPoint;

		}
	}


	return;
	// TODO: add 3d depth
	//ImVec2 currentPoint;
	//ImVec2 prevPoint;
	//const auto color = Helpers::calculateColor(gpConfig.trail);
	//for (auto& nade : worldPoints)
	//{
	//	if (Helpers::worldToScreenPixelAligned(nade, currentPoint))
	//	{
	//		if (prevPoint == ImVec2(0.0f, 0.0f)) {
	//			prevPoint = currentPoint;
	//		}
	//		else {
	//			drawList->AddLine(prevPoint, currentPoint, color, gpConfig.trailThickness);
	//			prevPoint = currentPoint;
	//		}
	//	}
	//}
}

static void drawCollision(ImDrawList* drawList, const std::vector<Vector>& worldPoints, bool inNadeView = false)
{
	if (!gpConfig.colision.enabled)
		return;
	// TODO: add 3d box
	ImVec2 currentPoint;
	const auto color = Helpers::calculateColor(gpConfig.colision);
	for (auto& nade : worldPoints)
	{
		if (!inNadeView ? Helpers::worldToScreenPixelAligned(nade, currentPoint) : Helpers::worldToWindow(NadePrediction::nadeViewMatrix, nade, currentPoint))
		{
			drawList->AddCircleFilled(currentPoint, 3.f, color);
		}
	}
}

static void drawWarning(ImDrawList* drawList, const Vector& worldPoint, WeaponId type, float timeToDetonate, float spawnTime)
{
	if (!gpConfig.warning.enabled)
		return;
	ImVec2 screenPosition{ 0.f,0.f };
	constexpr auto radius = 20.f;
	if (Helpers::worldToScreen(worldPoint, screenPosition))
	{
		drawList->AddCircleFilled(screenPosition, radius, ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, .4f)), 40);
		std::string icon;
		switch (type)
		{
		case WeaponId::HeGrenade:
			icon = "j";
			break;
		case WeaponId::Molotov:
		case WeaponId::IncGrenade:
			icon = "l";
			break;
		case WeaponId::Decoy:
			icon = "m";
			break;
		case WeaponId::Flashbang:
			icon = "i";
			break;
		case WeaponId::SmokeGrenade:
			icon = "k";
			break;
		default:
			break;
		}
		auto textSize = gui->fonts.weaponIcons->CalcTextSizeA(20.f, FLT_MAX, -1, icon.c_str());
		drawList->AddText(gui->fonts.weaponIcons, 20.0f, { screenPosition.x - (textSize.x / 2), screenPosition.y - (textSize.y / 2) }, IM_COL32_WHITE, icon.c_str());

		constexpr float pi = std::numbers::pi_v<float>;
		constexpr float min = -pi / 2;
		const float max = std::clamp(pi * 2 * (0.75f - spawnTime / timeToDetonate), -pi / 2, -pi / 2 + pi * 2);

		drawList->PathArcTo(screenPosition, radius, min, max, 40);
		drawList->PathStroke(IM_COL32_WHITE, false, 2.45f);
	}
}

void NadePrediction::draw(ImDrawList* drawList, bool nadeView) noexcept
{
	if (!gpConfig.enabled)
		return;

	std::lock_guard<std::mutex> lock(renderMutex);
	for (const auto& nadeDrawItem : nadeDrawList)
	{
		drawTrail(drawList, nadeDrawItem.trail, nadeView);
		if (nadeDrawItem.drawWarning)
			drawWarning(drawList, nadeDrawItem.ground, nadeDrawItem.type, nadeDrawItem.timeToDetonate, nadeDrawItem.spawnTime);
	}
}

void NadePrediction::drawGUI() noexcept
{
	bool resetConfig = false;
	ImGui::BeginChild(ImGui::GetID("nadePredictionTab"), { ImGui::GetContentRegionAvail().x * 0.5f ,-1 });
	ImGui::Checkbox("Enable grenade prediction", &gpConfig.enabled);
	ImGuiCustom::colorPicker("Trail", gpConfig.trail);
	ImGuiCustom::colorPicker("Colision box", gpConfig.colision);
	ImGuiCustom::colorPicker("Ground radius", gpConfig.ground);
	ImGui::Checkbox("Enable Grenade Warning", &gpConfig.warning.enabled);
	ImGui::Checkbox("Enemies only", &gpConfig.warning.enemiesOnly);
	ImGui::Checkbox("Flashbangs", &gpConfig.warning.flash);
	ImGui::Checkbox("HE grenade", &gpConfig.warning.HE);
	ImGui::Checkbox("Smoke grenade", &gpConfig.warning.smoke);
	ImGui::Checkbox("Molotov/incendiary grenade", &gpConfig.warning.molotov);
	ImGui::Checkbox("Decoy", &gpConfig.warning.decoy);
	ImGui::Checkbox("Enable grenade view", &gpConfig.nadeView.enabled);
	ImGui::Checkbox("When grenade Pinpulled only", &gpConfig.nadeView.pinpulledOnly);
	ImGui::Checkbox("Hide title bar", &gpConfig.nadeView.noTitleBar);
	ImGui::InputFloat("Camera Height", &gpConfig.nadeView.height, 10.f, 20.f, "%.0f");
	gpConfig.nadeView.height = max(gpConfig.nadeView.height, 50.0f);
	ImGui::InputFloat("Camera FOV", &gpConfig.nadeView.FOV, 10.f, 20.f, "%.0f");
	gpConfig.nadeView.FOV = std::clamp(gpConfig.nadeView.FOV, 40.0f, 150.f);

	ImGui::InputFloat("Camera Speed", &gpConfig.nadeView.speed, 10.f, 20.f, "%.0f");
}
//void computeCustomViewmatrix()
//{
//	//view.ComputeViewMatrices(pWorldToView, pViewToProjection, pWorldToProjection);
//
//	ComputeViewMatrix(pWorldToView, origin, angles);
//	//void ComputeViewMatrix(VMatrix * pViewMatrix, const Vector & origin, const QAngle & angles)
//	{
//		static VMatrix baseRotation;
//		static bool bDidInit;
//
//		if (!bDidInit)
//		{
//			MatrixBuildRotationAboutAxis(baseRotation, Vector(1, 0, 0), -90);
//			MatrixRotate(baseRotation, Vector(0, 0, 1), 90);
//			bDidInit = true;
//		}
//
//		*pViewMatrix = baseRotation;
//		MatrixRotate(*pViewMatrix, Vector(1, 0, 0), -angles[2]);
//		MatrixRotate(*pViewMatrix, Vector(0, 1, 0), -angles[0]);
//		MatrixRotate(*pViewMatrix, Vector(0, 0, 1), -angles[1]);
//
//		MatrixTranslate(*pViewMatrix, -origin);
//	}
//
//
//
//
//	MatrixBuildPerspectiveX(*pViewToProjection, fov, flAspectRatio, zNear, zFar);
//
//	MatrixMultiply(*pViewToProjection, *pWorldToView, *pWorldToProjection);
//
//
//
//	//2nd part
//	ComputeWorldToScreenMatrix(pWorldToPixels, *pWorldToProjection, view);
//
//}