#include "game/pch.h"
#include "game/ai/match_bot.h"
#include "game/simulation.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	bool same_bounds(const RectangleF& a, const RectangleF& b)
	{
		return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
	}

	float surface_y(const MatchBotRamp& ramp, float x)
	{
		const float along = std::clamp((x - ramp.bounds.left()) / ramp.bounds.width, 0.0f, 1.0f);
		return ramp.rises_right ? ramp.bounds.bottom() - along * ramp.bounds.height :
			ramp.bounds.top() + along * ramp.bounds.height;
	}

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

bool MatchBot::clear_ramp_path(const MatchBotObservation& view, Vector2F from,
	Vector2F to, const MatchBotRamp& ramp) const
{
	bool skipped_ramp = false;
	for (const RectangleF& obstacle : view.obstacles)
	{
		// Only a path constructed along this known hypotenuse can disregard
		// its AABB. All other ramp/slab bounds remain conservative barriers.
		if (!skipped_ramp && same_bounds(obstacle, ramp.bounds)) { skipped_ramp = true; continue; }
		bool one_way = false;
		for (const MatchBotSupport& support : view.supports)
		{ one_way = one_way || (support.one_way && same_bounds(obstacle, support.bounds)); }
		if (!one_way && crosses_interior(from, to, obstacle, view.size * 0.5f)) { return false; }
	}
	return true;
}

void MatchBot::choose_ramp(const MatchBotObservation& view)
{
	const Vector2F half = view.size * 0.5f;
	float nearest = std::numeric_limits<float>::max();
	for (const MatchBotRamp& ramp : view.ramps)
	{
		const RectangleF& bounds = ramp.bounds;
		if (!(bounds.width > 0.0f) || !(bounds.height > 0.0f)) { continue; }
		bool visited = false;
		for (const MatchBotSupport& previous : this->visited_supports_)
		{ visited = visited || same_bounds(previous.bounds, bounds); }
		if (visited) { continue; }
		const float direction = ramp.rises_right ? 1.0f : -1.0f;
		const float toe = ramp.rises_right ? bounds.left() : bounds.right();
		const float crest = ramp.rises_right ? bounds.right() : bounds.left();
		const float corner = view.center.x + direction * half.x;
		const float feet = view.center.y + half.y;
		const bool on_slope = corner >= bounds.left() && corner <= bounds.right() &&
			std::fabs(feet - surface_y(ramp, corner)) < 0.1f;
		const Vector2F toe_pose(toe - direction * half.x, bounds.bottom() - half.y);
		const Vector2F crest_pose(crest - direction * half.x, bounds.top() - half.y);
		const Vector2F exit(crest + direction * half.x, bounds.top() - half.y);
		bool source_known = on_slope;
		if (!on_slope && (view.center.x - toe_pose.x) * direction <= 0.1f &&
			std::fabs(feet - bounds.bottom()) < 0.1f)
		{
			for (const MatchBotSupport& source : view.supports)
			{
				if (std::fabs(source.bounds.top() - feet) < 0.1f &&
					view.center.x >= source.bounds.left() && view.center.x <= source.bounds.right() &&
					toe_pose.x >= source.bounds.left() && toe_pose.x <= source.bounds.right())
				{ source_known = true; }
			}
		}
		if (!source_known) { continue; }
		bool exit_known = false;
		for (const MatchBotSupport& support : view.supports)
		{
			if (std::fabs(support.bounds.top() - bounds.top()) < 0.1f &&
				exit.x - half.x >= support.bounds.left() && exit.x + half.x <= support.bounds.right())
			{ exit_known = true; }
		}
		const float approach = on_slope ? 0.0f : std::fabs(view.center.x - toe_pose.x);
		const Vector2F start = on_slope ? view.center : toe_pose;
		if (!exit_known || approach > this->tuning_->bots().sight_distance || approach >= nearest ||
			(!on_slope && !this->clear_ramp_path(view, view.center, toe_pose, ramp)) ||
			!this->clear_ramp_path(view, start, crest_pose, ramp) ||
			!this->clear_ramp_path(view, crest_pose, exit, ramp)) { continue; }
		nearest = approach;
		this->ramp_ = ramp; this->ramp_exit_ = exit;
		this->climbing_ramp_ = true;
		// Walking a long slope is different from a stalled jump alignment.
		// Derive a finite generous budget from ordinary minimum moving speed.
		const double travel = (static_cast<double>(approach) + bounds.width + view.size.x) /
			this->tuning_->player().x_initial_velocity * SIMULATION_HZ;
		const double budget = std::ceil(travel) + this->tuning_->bots().progress_window_steps +
			this->tuning_->bots().recovery_steps;
		this->ramp_steps_left_ = static_cast<int>(std::min(budget,
			static_cast<double>((std::numeric_limits<int>::max)())));
		this->jump_left_ = 0;
	}
}

void MatchBot::steer_ramp(const MatchBotObservation& view, PlayerInputData& input)
{
	bool present = false;
	for (const MatchBotRamp& ramp : view.ramps)
	{ present = present || (same_bounds(ramp.bounds, this->ramp_.bounds) && ramp.rises_right == this->ramp_.rises_right); }
	const float direction = this->ramp_.rises_right ? 1.0f : -1.0f;
	const float feet = view.center.y + view.size.y * 0.5f;
	const Vector2F half = view.size * 0.5f;
	const RectangleF& bounds = this->ramp_.bounds;
	const float toe = this->ramp_.rises_right ? bounds.left() : bounds.right();
	const float crest = this->ramp_.rises_right ? bounds.right() : bounds.left();
	const Vector2F toe_pose(toe - direction * half.x, bounds.bottom() - half.y);
	const Vector2F crest_pose(crest - direction * half.x, bounds.top() - half.y);
	const bool before_toe = (view.center.x - toe_pose.x) * direction < 0.0f;
	const bool past_crest = (view.center.x - crest_pose.x) * direction >= 0.0f;
	bool exit_known = false;
	for (const MatchBotSupport& support : view.supports)
	{
		if (std::fabs(support.bounds.top() - bounds.top()) < 0.1f &&
			this->ramp_exit_.x - half.x >= support.bounds.left() &&
			this->ramp_exit_.x + half.x <= support.bounds.right()) { exit_known = true; }
	}
	const bool clear = (before_toe ?
		this->clear_ramp_path(view, view.center, toe_pose, this->ramp_) &&
		this->clear_ramp_path(view, toe_pose, crest_pose, this->ramp_) :
		(past_crest || this->clear_ramp_path(view, view.center, crest_pose, this->ramp_))) &&
		this->clear_ramp_path(view, past_crest ? view.center : crest_pose, this->ramp_exit_, this->ramp_);
	const bool finished = view.grounded && std::fabs(feet - this->ramp_exit_.y - view.size.y * 0.5f) < 0.1f &&
		(view.center.x - this->ramp_exit_.x) * direction >= -2.0f;
	if (!present || !exit_known || !clear || finished || --this->ramp_steps_left_ <= 0 ||
		feet > bounds.bottom() + view.size.y)
	{
		// A ramp has no support identity; it is remembered by its bounds.
		if (present) { this->visited_supports_.push_back({ this->ramp_.bounds, false, false, 0 }); }
		this->climbing_ramp_ = false;
		this->continue_ascent_ = finished;
		this->react_in_ = 1; this->replan_in_ = 0;
		input.x_movement = 0.0f;
		input.left_analog_stick = {};
	}
	else
	{
		input.x_movement = direction;
		input.left_analog_stick = Vector2F(direction, 0.0f);
	}
	this->jump_left_ = 0;
	input.jump_pressed = input.jump_held = input.drop_through = false;
}
