#include "game/pch.h"
#include "game/ai/match_bot.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	bool crosses_interior(Vector2F from, Vector2F to, const RectangleF& bounds, Vector2F half)
	{
		double enter = 0.0, leave = 1.0;
		const auto axis = [&](double start, double finish, double low, double high)
		{
			const double delta = finish - start;
			if (delta == 0.0) { return start > low && start < high; }
			double first = (low - start) / delta, last = (high - start) / delta;
			if (first > last) { std::swap(first, last); }
			enter = std::max(enter, first); leave = std::min(leave, last);
			return enter < leave;
		};
		return axis(from.x, to.x, static_cast<double>(bounds.left()) - half.x,
			static_cast<double>(bounds.right()) + half.x) &&
			axis(from.y, to.y, static_cast<double>(bounds.top()) - half.y,
				static_cast<double>(bounds.bottom()) + half.y);
	}
}

bool MatchBot::clear_route(const MatchBotObservation& view, Vector2F center) const
{
	return this->clear_route(view, view.center, center);
}

bool MatchBot::clear_route(const MatchBotObservation& view, Vector2F from, Vector2F to) const
{
	const Vector2F half = view.size * 0.5f;
	for (const RectangleF& barrier : view.obstacles)
	{
		bool one_way = false;
		for (const MatchBotSupport& platform : view.supports)
		{
			const RectangleF& bounds = platform.bounds;
			one_way = one_way || (platform.one_way && barrier.x == bounds.x && barrier.y == bounds.y &&
				barrier.width == bounds.width && barrier.height == bounds.height);
		}
		if (one_way) { continue; }
		// Sweep the body, retaining ramp/slab AABBs as conservative barriers.
		// Positive interiors only: feet moving along a support top or ending
		// exactly on it remain clear. Clip in double rather than perturbing a
		// float bound, which can lose the perturbation when recentered.
		if (crosses_interior(from, to, barrier, half)) { return false; }
	}
	return true;
}

bool MatchBot::clear_standing(const MatchBotObservation& view, Vector2F center) const
{
	const Vector2F half = view.size * 0.5f;
	for (const RectangleF& obstacle : view.obstacles)
	{
		// Positive interiors only: feet touching the support top are valid.
		if (center.x + half.x > obstacle.left() && center.x - half.x < obstacle.right() &&
			center.y + half.y > obstacle.top() && center.y - half.y < obstacle.bottom()) { return false; }
	}
	return true;
}

bool MatchBot::standing_goal(const MatchBotObservation& view, Vector2F aim, Vector2F& standing) const
{
	const PlayerTuning& movement = this->tuning_->player();
	const float gravity = std::max(movement.gravity, 1.0f);
	const float rise = movement.jump_launch_velocity * movement.jump_launch_velocity / (2.0f * gravity) +
		std::fabs(movement.jump_acceleration) * movement.jump_max_time * movement.jump_max_time;
	float best = std::numeric_limits<float>::max();
	bool found = false;
	for (const MatchBotSupport& support : view.supports)
	{
		const RectangleF& bounds = support.bounds;
		if (bounds.width < view.size.x) { continue; }
		const Vector2F candidate(std::clamp(aim.x, bounds.left() + view.size.x * 0.5f,
			bounds.right() - view.size.x * 0.5f), bounds.top() - view.size.y * 0.5f);
		const Vector2F gap = candidate - view.center;
		if (gap.y < -rise || gap.length() > this->tuning_->bots().sight_distance ||
			!this->clear_standing(view, candidate) || !this->clear_route(view, candidate)) { continue; }
		// A paint face is an aim point. Choose a nearby body-sized standing pose
		// separately, including for side/bottom faces and narrow or covered tops.
		const float cost = Vector2F::distance_squared(candidate, aim);
		if (cost < best) { best = cost; standing = candidate; found = true; }
	}
	return found;
}

void MatchBot::choose_descent(const MatchBotObservation& view)
{
	float nearest = std::numeric_limits<float>::max();
	const Vector2F half = view.size * 0.5f;
	for (const MatchBotSupport& support : view.supports)
	{
		const RectangleF& bounds = support.bounds;
		// Start over the landing support, so a drop never invents an airborne
		// horizontal crossing or sends the bot into a gap with no known landing.
		if (bounds.width < view.size.x || view.center.x - half.x < bounds.left() ||
			view.center.x + half.x > bounds.right() || bounds.top() <= view.center.y + half.y) { continue; }
		const float room = this->heading_ < 0.0f ? view.center.x - bounds.left() - half.x :
			bounds.right() - half.x - view.center.x;
		const float direction = room < view.size.x ? -this->heading_ : this->heading_;
		const Vector2F candidate(std::clamp(view.center.x + direction * view.size.x * 4.0f,
			bounds.left() + half.x, bounds.right() - half.x), bounds.top() - half.y);
		const float distance = Vector2F::distance(candidate, view.center);
		if (distance >= nearest || distance > this->tuning_->bots().sight_distance ||
			!this->clear_standing(view, candidate)) { continue; }
		if (this->clear_route(view, candidate))
		{ nearest = distance; this->goal_ = candidate; this->has_goal_ = true; }
	}
}
