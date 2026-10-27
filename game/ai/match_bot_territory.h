#pragma once

#include "game/ai/match_bot.h"
#include "game/objects/paintable_object.h"
#include <cmath>
#include <optional>

namespace match_bot_territory
{
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
