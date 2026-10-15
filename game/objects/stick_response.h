#pragma once

#include "game/play_settings.h"
#include "engine/input/gamepad.h"

struct StickResponse
{
	float movement = 0.0f;
	mattmath::Vector2F aim = mattmath::Vector2F::ZERO;
};

// Shared by gameplay and the live settings preview. Movement uses horizontal
// travel; aiming uses a radial cutoff and preserves the stick's direction.
inline StickResponse stick_response(const mattmath::Vector2F& left,
	const mattmath::Vector2F& right, float authored_movement,
	const PlaySettings& preferences, int slot)
{
	return {
		labrador::apply_deadzone(left.x, preferences.movement_deadzone(slot, authored_movement)),
		labrador::apply_deadzone(right, preferences.aim_deadzone(slot))
	};
}
