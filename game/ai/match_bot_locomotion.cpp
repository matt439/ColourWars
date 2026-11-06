#include "game/pch.h"
#include "game/ai/match_bot_locomotion.h"
#include "game/contracts/simulation.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	// Steps a held jump the way Player::start_jump and its held extension do,
	// holding for held_steps, calling visit(height, travelled) after each step
	// until it lands.
	template <class Visit>
	void held_jump(const PlayerTuning& movement, int held_steps, float launch_speed, Visit visit)
	{
		float vy = movement.jump_launch_velocity, height = 0.0f;
		float vx = launch_speed, travelled = 0.0f;
		constexpr float dt = simulation::seconds_per_step;
		for (int step = 0; step < 2 * simulation::simulation_hz; ++step)
		{
			// Player::start_jump overwrites gravity on the takeoff tick. Held
			// extension begins on the following tick and includes only held outputs.
			if (step == 0) { vy = std::max(vy, -movement.max_velocity.y); }
			else
			{
				vy = std::min(vy + movement.gravity * dt, movement.max_velocity.y);
				if (step < held_steps && static_cast<float>(step - 1) * dt <= movement.jump_max_time)
				{ vy += movement.jump_acceleration * dt; }
				vy = std::max(vy, -movement.max_velocity.y);
			}
			height -= vy * dt;
			travelled += vx * dt;
			vx = std::min(vx + movement.x_acceleration * dt, movement.max_velocity.x);
			if (!visit(height, travelled) || height <= 0.0f) { break; }
		}
	}
}

MatchBotLocomotion::MatchBotLocomotion(const GameTuning& tuning) : tuning_(tuning)
{
}

float MatchBotLocomotion::speed_multiplier(MatchBotFooting footing) const
{
	const PaintUnderfootTuning& effect = this->tuning_.player().paint_underfoot;
	if (!effect.enabled || footing == MatchBotFooting::none) { return 1.0f; }
	return footing == MatchBotFooting::own ? effect.own_speed_multiplier : effect.enemy_speed_multiplier;
}

MatchBotGroundRates MatchBotLocomotion::ground(MatchBotFooting footing) const
{
	const PlayerTuning& base = this->tuning_.player();
	const float multiplier = this->speed_multiplier(footing);
	return { base.x_initial_velocity * multiplier, base.x_acceleration * multiplier,
		base.x_deacceleration * multiplier, base.max_velocity.x * multiplier };
}

MatchBotGroundRates MatchBotLocomotion::air() const
{
	return this->ground(MatchBotFooting::none);
}

float MatchBotLocomotion::stopping_distance(float vx, bool grounded, MatchBotFooting footing) const
{
	const float braking = (grounded ? this->ground(footing) : this->air()).braking;
	return braking > 0.0f ? vx * vx / (2.0f * braking) : std::numeric_limits<float>::max();
}

int MatchBotLocomotion::held_jump_steps() const
{
	const double steps = std::ceil(static_cast<double>(this->tuning_.player().jump_max_time) * simulation::simulation_hz);
	return std::max(1, static_cast<int>(std::min(steps, static_cast<double>((std::numeric_limits<int>::max)()))));
}

MatchBotLocomotion::JumpArc MatchBotLocomotion::jump(float launch_speed) const
{
	JumpArc arc;
	held_jump(this->tuning_.player(), this->held_jump_steps(), launch_speed, [&](float height, float travelled)
	{
		arc.rise = std::max(arc.rise, height);
		arc.reach = travelled;
		return true;
	});
	return arc;
}
