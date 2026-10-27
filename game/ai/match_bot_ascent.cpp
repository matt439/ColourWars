#include "game/pch.h"
#include "game/ai/match_bot.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	bool same_support(const RectangleF& a, const RectangleF& b)
	{
		return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
	}

	float steer(float target, const MatchBotObservation& view, const PlayerTuning& tuning)
	{
		const float dx = target - view.center.x;
		const float stopping = view.velocity.x * view.velocity.x / (2.0f * tuning.x_deacceleration);
		if (std::fabs(dx) < 2.0f || (dx * view.velocity.x > 0.0f && std::fabs(dx) <= stopping + 2.0f))
		{ return 0.0f; }
		return dx > 0.0f ? 1.0f : -1.0f;
	}
}

void MatchBot::choose_ascent(const MatchBotObservation& view)
{
	// Explore adjacent raised supports to open new sight lines, independently
	// of sampled paint. No arena names, authored routes or unseen enemies enter
	// this choice. Initial one-way spawns retain their ordinary descent policy.
	const PlayerTuning& movement = this->tuning_->player();
	const Vector2F half = view.size * 0.5f;
	float vy = movement.jump_launch_velocity, height = 0.0f, rise = 0.0f;
	float vx = movement.x_initial_velocity, reach = 0.0f;
	constexpr float dt = 1.0f / 60.0f;
	for (int step = 0; step < 120; ++step)
	{
		vy += movement.gravity * dt;
		if (static_cast<float>(step) * dt <= movement.jump_max_time) { vy += movement.jump_acceleration * dt; }
		height -= vy * dt; rise = std::max(rise, height);
		reach += vx * dt;
		vx = std::min(vx + movement.x_acceleration * dt, movement.max_velocity.x);
		if (height <= 0.0f) { break; }
	}
	float best = std::numeric_limits<float>::max();
	float best_climb = std::numeric_limits<float>::max();
	for (const MatchBotSupport& support : view.supports)
	{
		const RectangleF& bounds = support.bounds;
		const float climb = view.center.y + half.y - bounds.top();
		if (climb < half.y || climb > rise - 2.0f || bounds.width < view.size.x) { continue; }
		bool visited = false;
		for (const RectangleF& previous : this->visited_supports_)
		{ visited = visited || same_support(previous, bounds); }
		if (visited) { continue; }
		const Vector2F landing(std::clamp(view.center.x, bounds.left() + half.x,
			bounds.right() - half.x), bounds.top() - half.y);
		Vector2F takeoff(landing.x, view.center.y);
		if (!support.one_way)
		{
			// Rise beside a solid lip before crossing it, rather than sweeping a
			// diagonal through its underside. Retain room for the whole body.
			if (view.center.x < bounds.center().x) { takeoff.x = bounds.left() - half.x - 2.0f; }
			else { takeoff.x = bounds.right() + half.x + 2.0f; }
		}
		const float approach = std::fabs(takeoff.x - view.center.x);
		if (approach > std::min(reach, this->tuning_->bots().sight_distance) || !this->clear_standing(view, landing) ||
			!this->clear_route(view, takeoff)) { continue; }
		// Alignment must remain on a known floor; never walk into an unknown gap.
		bool floor = false;
		for (const MatchBotSupport& source : view.supports)
		{
			floor = floor || (std::fabs(source.bounds.top() - view.center.y - half.y) < 0.1f &&
				std::min(view.center.x, takeoff.x) - half.x >= source.bounds.left() &&
				std::max(view.center.x, takeoff.x) + half.x <= source.bounds.right());
		}
		if (!floor) { continue; }
		const Vector2F apex(takeoff.x, takeoff.y - rise);
		const Vector2F over_landing(landing.x, apex.y);
		if (!this->clear_route(view, takeoff, apex) || !this->clear_route(view, apex, over_landing) ||
			!this->clear_route(view, over_landing, landing)) { continue; }
		// Take adjacent steps in height order. Skipping a lower deck for a
		// taller overlapping bridge can strand the bot above that deck's lip.
		if (climb < best_climb || (climb == best_climb && approach < best))
		{
			best = approach; best_climb = climb;
			this->ascent_support_ = bounds; this->ascent_takeoff_ = takeoff;
			this->ascending_ = true; this->ascent_airborne_ = false;
			this->ascent_steps_left_ = this->tuning_->bots().progress_window_steps +
				this->tuning_->bots().recovery_steps;
		}
	}
	if (!this->ascending_) { this->continue_ascent_ = false; }
}

void MatchBot::remember_descent(const MatchBotObservation& view, Vector2F goal)
{
	this->seeking_after_descent_ = true;
	// A deliberate escape has already considered the one-way supports in its
	// drop column. Exploring straight back up would undo the chosen descent.
	for (const MatchBotSupport& support : view.supports)
	{
		const RectangleF& bounds = support.bounds;
		if (!support.one_way || bounds.top() < view.center.y + view.size.y * 0.5f - 0.1f ||
			bounds.top() > goal.y + view.size.y * 0.5f ||
			view.center.x < bounds.left() || view.center.x > bounds.right()) { continue; }
		bool visited = false;
		for (const RectangleF& previous : this->visited_supports_)
		{ visited = visited || same_support(previous, bounds); }
		if (!visited) { this->visited_supports_.push_back(bounds); }
	}
}

void MatchBot::steer_ascent(const MatchBotObservation& view, PlayerInputData& input)
{
	const PlayerTuning& movement = this->tuning_->player();
	const Vector2F half = view.size * 0.5f;
	const RectangleF& target = this->ascent_support_;
	bool present = false;
	for (const MatchBotSupport& support : view.supports)
	{ present = present || same_support(target, support.bounds); }
	const Vector2F landing(std::clamp(this->ascent_takeoff_.x, target.left() + half.x,
		target.right() - half.x), target.top() - half.y);
	if (!present || !this->clear_standing(view, landing) || --this->ascent_steps_left_ <= 0)
	{
		if (present) { this->visited_supports_.push_back(target); }
		this->ascending_ = this->continue_ascent_ = false;
		this->jump_left_ = 0;
		this->react_in_ = 1;
		input.x_movement = 0.0f;
		input.left_analog_stick = {};
		input.drop_through = false;
		return;
	}
	const float feet = view.center.y + half.y;
	input.drop_through = false;
	float destination = this->ascent_takeoff_.x;
	if (!view.grounded) { this->ascent_airborne_ = true; }
	if (this->ascent_airborne_ && view.grounded)
	{
		this->visited_supports_.push_back(target);
		this->continue_ascent_ = std::fabs(feet - target.top()) < 0.1f &&
			view.center.x + half.x > target.left() && view.center.x - half.x < target.right();
		this->ascending_ = false;
		this->jump_left_ = 0;
		this->react_in_ = 1;
		this->replan_in_ = 0;
		input.x_movement = 0.0f;
		input.left_analog_stick = {};
		return;
	}
	if (this->ascent_airborne_ && feet <= target.top())
	{
		destination = std::clamp(this->ascent_takeoff_.x, target.left() + half.x, target.right() - half.x);
	}
	input.x_movement = steer(destination, view, movement);
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	if (!this->ascent_airborne_ && std::fabs(view.center.x - destination) <= 3.0f &&
		std::fabs(view.velocity.x) <= movement.x_initial_velocity && !this->jump_was_held_)
	{
		const double steps = std::ceil(static_cast<double>(movement.jump_max_time) * 60.0);
		this->jump_left_ = std::max(1, static_cast<int>(std::min(steps,
			static_cast<double>((std::numeric_limits<int>::max)()))));
	}
}
