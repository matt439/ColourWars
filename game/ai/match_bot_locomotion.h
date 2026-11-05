#pragma once

#include "game/config/tuning.h"

// What a bot's support top is to it this step: no paint (or airborne, or
// paint underfoot off), its own team's paint, or an enemy's.
enum class MatchBotFooting { none, own, enemy };

// One footing's horizontal ground rates, as Player::update_movement scales them.
struct MatchBotGroundRates
{
	float initial = 0.0f;
	float acceleration = 0.0f;
	float braking = 0.0f;
	float top = 0.0f;
};

// The bot's model of its own movement under paint underfoot. It mirrors
// the player: the underfoot multipliers scale the four ground rates only on a
// support's paint, and every airborne step uses the base rates. Every number comes from tuning; with paint underfoot off each
// footing answers the base rates.
class MatchBotLocomotion
{
public:
	explicit MatchBotLocomotion(const GameTuning& tuning);

	float speed_multiplier(MatchBotFooting footing) const;
	MatchBotGroundRates ground(MatchBotFooting footing) const;
	// The airborne rates: the base ground rates, unscaled.
	MatchBotGroundRates air() const;
	// The distance a body at vx covers while braking to a stop with no input.
	float stopping_distance(float vx, bool grounded, MatchBotFooting footing) const;

	// A held jump launched at launch_speed, accelerating toward its direction
	// with the airborne rates, sampled until it falls back to takeoff height.
	struct JumpArc
	{
		float rise = 0.0f;
		float reach = 0.0f;
	};
	JumpArc jump(float launch_speed) const;
	// The steps a bot holds jump for: jump_max_time in whole steps, at least
	// one. jump() models the same hold.
	int held_jump_steps() const;

private:
	const GameTuning& tuning_;
};
