#pragma once

#include "game/ai/match_bot.h"
#include "game/objects/paintable_object.h"
#include "game/simulation.h"
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

	// One exact fired angle: the straight launch must start above this top and
	// first reach it over the support. The jet is swept, so contact is where
	// its centred box's bottom crosses the top. The shot leaves after this
	// step's movement. Curved and contact tools are not judged here, and
	// intervening cover is not checked.
	inline bool top_launch_lands(const MatchBotObservation& view, const mattmath::RectangleF& support,
		float angle, const GameTuning& tuning)
	{
		const WeaponTuning& tool = tuning.weapon(view.weapon);
		if (tool.details.starting_vel_length <= 0.0f) { return true; }
		const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
		if (projectile.details.gravity != 0.0f) { return true; }
		// Weapon::nozzle_pos from the post-move centre; ProjectileJet centres
		// its collision box on that point.
		const bool left = angle > mattmath::PI * 0.5f || angle < -mattmath::PI * 0.5f;
		const mattmath::Vector2F tip = view.center + view.velocity / static_cast<float>(SIMULATION_HZ) +
			mattmath::Vector2F(left ? -tool.details.offset.x : tool.details.offset.x, tool.details.offset.y) +
			mattmath::Vector2F::rotate_vector(tool.details.nozzle_offset +
				mattmath::Vector2F(tool.details.size.x, 0.0f), angle);
		const mattmath::Vector2F half = projectile.details.col_rect_size * 0.5f;
		const float drop = support.top() - (tip.y + half.y);
		const float sine = std::sin(angle);
		if (drop < 0.0f || sine <= 0.0f) { return false; }
		const float x = tip.x + std::cos(angle) * (drop / sine);
		return x - half.x >= support.left() && x + half.x <= support.right();
	}

	struct SupportTop
	{
		mattmath::Vector2F point;
		mattmath::RectangleF bounds;
	};

	// One horizontal top tile on a support's top edge, as an aim point just
	// above the paint. Rotated tiles have a local face and are rejected.
	inline std::optional<mattmath::Vector2F> top_tile_point(const mattmath::RectangleF& bounds,
		const PaintSurfaceSample& surface, float thickness, float body_width)
	{
		constexpr float tolerance = 0.1f;
		if (surface.face != PaintSurfaceFace::top ||
			std::fabs(surface.corners[0].y - surface.corners[1].y) > tolerance) { return std::nullopt; }
		mattmath::Vector2F point;
		for (const mattmath::Vector2F& corner : surface.corners)
		{
			if (corner.x < bounds.left() - tolerance || corner.x > bounds.right() + tolerance ||
				std::fabs(corner.y - bounds.top()) > thickness + tolerance) { return std::nullopt; }
			point += corner;
		}
		point *= 0.25f;
		point.y -= thickness + body_width * 0.05f;
		return point;
	}

	// Geometry qualification only; the caller still checks ownership, range
	// and visibility before offering this aim-only point to the policy.
	inline std::optional<SupportTop> support_top_point(const MatchBotObservation& view,
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
			if (const std::optional<mattmath::Vector2F> point = top_tile_point(bounds, surface, thickness, view.size.x))
			{ return SupportTop{ *point, bounds }; }
		}
		return std::nullopt;
	}

	// Any scoring support's top, not only the one underfoot: a slow straight
	// tool can land its shot on a lower lip it has already climbed past.
	inline std::optional<SupportTop> scoring_top_point(const MatchBotObservation& view,
		const PaintSurfaceSample& surface, float thickness)
	{
		for (const MatchBotSupport& support : view.supports)
		{
			if (!support.paintable) { continue; }
			if (const std::optional<mattmath::Vector2F> point = top_tile_point(support.bounds, surface, thickness, view.size.x))
			{ return SupportTop{ *point, support.bounds }; }
		}
		return std::nullopt;
	}
}
