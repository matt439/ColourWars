#include "game/pch.h"
#include "game/ai/match_bot_ballistics.h"
#include "engine/core/step_timer.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	Vector2F inherited_velocity(const RelativeWeaponDetails& relative, Vector2F velocity)
	{
		switch (relative.add_vel)
		{
		case AddPlayerVelocity::x_and_y: return velocity * relative.player_vel_amount;
		case AddPlayerVelocity::x_only: return { velocity.x * relative.player_vel_amount, 0.0f };
		case AddPlayerVelocity::y_only: return { 0.0f, velocity.y * relative.player_vel_amount };
		case AddPlayerVelocity::none: return {};
		}
		return {};
	}

	float miss_squared(float angle, const WeaponTuning& tool, const ProjectileTuning& projectile,
		Vector2F inherited, Vector2F target, Vector2F target_velocity)
	{
		const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(
			labrador::StepTimer::TicksPerSecond / 60));
		const Vector2F direction(std::cos(angle), std::sin(angle));
		Vector2F position = Vector2F::rotate_vector(tool.details.nozzle_offset +
			Vector2F(tool.details.size.x, 0.0f), angle);
		position += Vector2F(direction.x < 0.0f ? -tool.details.offset.x : tool.details.offset.x,
			tool.details.offset.y);
		Vector2F velocity = direction * tool.details.starting_vel_length + inherited;
		Vector2F previous = position - target;
		float best = (std::numeric_limits<float>::max)();
		float elapsed = 0.0f;
		// One second, including the first sweep of a shorter-lived projectile.
		// Use relative swept distance so a fast shot need not land on a tick endpoint.
		for (int step = 0; step < 60; ++step)
		{
			elapsed += dt;
			if (step > 0 && elapsed > projectile.details.delete_timer) { break; }
			velocity.y = std::clamp(velocity.y + projectile.details.gravity * dt,
				-projectile.max_velocity.y, projectile.max_velocity.y);
			if (std::fabs(velocity.x) > projectile.details.wind_resistance * projectile.wind_stop_factor)
			{
				velocity.x += (velocity.x > 0.0f ? -1.0f : 1.0f) * projectile.details.wind_resistance * dt;
			}
			else { velocity.x = 0.0f; }
			velocity.x = std::clamp(velocity.x, -projectile.max_velocity.x, projectile.max_velocity.x);
			position += velocity * dt;
			const Vector2F current = position - target - target_velocity * elapsed;
			const Vector2F segment = current - previous;
			const float length = segment.length_squared();
			const float along = length > 0.0f ? std::clamp(-Vector2F::dot(previous, segment) / length, 0.0f, 1.0f) : 0.0f;
			best = std::min(best, (previous + segment * along).length_squared());
			previous = current;
		}
		return best;
	}
}

Vector2F match_bot_ballistics::aim_direction(const WeaponTuning& tool, const ProjectileTuning& projectile,
	Vector2F shooter_velocity, Vector2F target_offset, Vector2F target_velocity)
{
	if (target_offset.length_squared() == 0.0f) { return { 1.0f, 0.0f }; }
	const float direct = target_offset.angle();
	if (tool.details.starting_vel_length <= 0.0f) { return { std::cos(direct), std::sin(direct) }; }
	const Vector2F inherited = inherited_velocity(tool.relative, shooter_velocity);
	float best_angle = direct;
	float best = miss_squared(direct, tool, projectile, inherited, target_offset, target_velocity);
	// A fixed coarse ring and eight local refinements bound the work to 32
	// trajectories (1920 integration steps) per reaction, independent of arena size.
	constexpr int directions = 16;
	const float increment = 2.0f * PI / static_cast<float>(directions);
	for (int index = 1; index < directions; ++index)
	{
		const float angle = direct + increment * static_cast<float>(index);
		const float miss = miss_squared(angle, tool, projectile, inherited, target_offset, target_velocity);
		if (miss < best) { best = miss; best_angle = angle; }
	}
	float radius = increment;
	for (int refinement = 0; refinement < 8; ++refinement)
	{
		radius *= 0.5f;
		const float center = best_angle;
		for (const float angle : { center - radius, center + radius })
		{
			const float miss = miss_squared(angle, tool, projectile, inherited, target_offset, target_velocity);
			if (miss < best) { best = miss; best_angle = angle; }
		}
	}
	return { std::cos(best_angle), std::sin(best_angle) };
}
