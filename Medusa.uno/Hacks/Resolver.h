#pragma once

#include "Animations.h"

#include "../SDK/GameEvent.h"
#include "../SDK/Entity.h"

namespace Resolver
{
	void reset() noexcept;

	void resolve_entity(const Animations::Players& player, Entity* entity);

	float resolve_shot(const Animations::Players& player, Entity* entity);

	void setup_detect(Animations::Players& player, Entity* entity);

	void processMissedShots() noexcept;
	void CmdGrabber(UserCmd* cmd1);
	void saveRecord(int playerIndex, float playerSimulationTime) noexcept;
	void getEvent(GameEvent* event) noexcept;

	void runPreUpdate(Animations::Players player, Entity* entity) noexcept;
	void runPostUpdate(Animations::Players player, Entity* entity) noexcept;

	void detect_side(Entity* entity, int* side);

	void updateEventListeners(bool forceRemove = false) noexcept;

	struct SnapShot
	{
		Animations::Players player;
		const Model* model{ };
		Vector eyePosition{};
		Vector bulletImpact{};
		bool gotImpact{ false };
		float time{ -1 };
		int playerIndex{ -1 };
		int backtrackRecord{ -1 };
	};
}