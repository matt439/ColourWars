#pragma once

#include "game/ai/match_bot.h"
#include "game/objects/paintable_object.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace match_bot_territory
{
	// A nearby floor tile can lie behind a long muzzle. Reject that aim rather
	// than repeatedly launching below the scoring top. Include the projectile
	// half-height and the full authored aim-error envelope; this is launch
	// clearance only, not a trajectory or future-motion guarantee.
	inline bool top_launch_clear(const MatchBotObservation& view, mattmath::Vector2F point,
		const GameTuning& tuning, BotDifficulty difficulty)
	{
		const WeaponTuning& tool = tuning.weapon(view.weapon);
		// The Roller places contact paint with its own constrained tool pose.
		if (tool.details.starting_vel_length <= 0.0f) { return true; }
		const mattmath::Vector2F nozzle = tool.details.nozzle_offset +
			mattmath::Vector2F(tool.details.size.x, 0.0f);
		const float angle = (point - view.center).angle();
		const float error = tuning.bot(difficulty).aim_error_radians;
		const float phase = angle + std::atan2(nozzle.y, nozzle.x);
		float lowest = std::max(std::sin(phase - error), std::sin(phase + error));
		const float peak_gap = std::remainder(mattmath::PI * 0.5f - phase, mattmath::PI * 2.0f);
		if (std::fabs(peak_gap) <= error) { lowest = 1.0f; }
		const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
		const float bottom = tool.details.offset.y + nozzle.length() * lowest +
			projectile.details.col_rect_size.y * 0.5f;
		return bottom < view.size.y * 0.5f;
	}

	// Geometry qualification only; the caller still checks ownership, range
	// and visibility before offering this aim-only point to the policy.
	inline std::optional<mattmath::Vector2F> support_top_point(const MatchBotObservation& view,
		const PaintSurfaceSample& surface, float thickness)
	{
		if (!view.grounded || surface.face != PaintSurfaceFace::top) { return std::nullopt; }
		constexpr float tolerance = 0.1f;
		const float feet = view.center.y + view.size.y * 0.5f;
		for (const MatchBotSupport& support : view.supports)
		{
			const mattmath::RectangleF& bounds = support.bounds;
			if (!support.paintable || std::fabs(feet - bounds.top()) > tolerance ||
				view.center.x + view.size.x * 0.5f <= bounds.left() ||
				view.center.x - view.size.x * 0.5f >= bounds.right()) { continue; }
			bool belongs = true;
			mattmath::Vector2F point;
			for (const mattmath::Vector2F& corner : surface.corners)
			{
				belongs = belongs && corner.x >= bounds.left() - tolerance &&
					corner.x <= bounds.right() + tolerance &&
					std::fabs(corner.y - bounds.top()) <= thickness + tolerance;
				point += corner;
			}
			// A rotated tile's face is local, so require a horizontal top edge.
			if (!belongs || std::fabs(surface.corners[0].y - surface.corners[1].y) > tolerance)
			{ continue; }
			point *= 0.25f;
			point.y -= thickness + view.size.x * 0.05f;
			return point;
		}
		return std::nullopt;
	}
}
