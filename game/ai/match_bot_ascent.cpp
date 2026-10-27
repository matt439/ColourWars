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
	const float inset = std::min(2.0f, half.x);
	float vy = movement.jump_launch_velocity, height = 0.0f, rise = 0.0f;
	float vx = movement.x_initial_velocity, reach = 0.0f;
	constexpr float dt = 1.0f / 60.0f;
	const int held_steps = std::max(1, static_cast<int>(std::min(
		std::ceil(static_cast<double>(movement.jump_max_time) * 60.0),
		static_cast<double>((std::numeric_limits<int>::max)()))));
	for (int step = 0; step < 120; ++step)
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
		height -= vy * dt; rise = std::max(rise, height);
		reach += vx * dt;
		vx = std::min(vx + movement.x_acceleration * dt, movement.max_velocity.x);
		if (height <= 0.0f) { break; }
	}
	// Reuse the same local geometry test for a candidate's next step. A
	// reachable dead-end shelf should not displace a continuing ascent chain.
	const auto route = [&](Vector2F center, const MatchBotSupport& support,
		Vector2F& takeoff, Vector2F& landing, float& cost, float& approach)
	{
		const RectangleF& bounds = support.bounds;
		const float climb = center.y + half.y - bounds.top();
		if (climb < 0.1f || climb > rise || bounds.width < view.size.x) { return false; }
		landing = Vector2F(std::clamp(center.x, bounds.left() + half.x,
			bounds.right() - half.x), bounds.top() - half.y);
		takeoff = Vector2F(landing.x, center.y);
		if (!support.one_way)
		{
			// Rise beside a solid lip before crossing it, rather than sweeping a
			// diagonal through its underside. Retain room for the whole body.
			if (center.x < bounds.center().x) { takeoff.x = bounds.left() - half.x - 2.0f; }
			else { takeoff.x = bounds.right() + half.x + 2.0f; }
		}
		// Align on the source support even when a gap separates it from the
		// target lip/landing. The crossing belongs to the jump, not the walk.
		bool floor = false;
		float alignment = std::numeric_limits<float>::max();
		float launch_x = takeoff.x;
		for (const MatchBotSupport& source : view.supports)
		{
			if (source.bounds.width < view.size.x ||
				std::fabs(source.bounds.top() - center.y - half.y) >= 0.1f ||
				center.x < source.bounds.left() || center.x > source.bounds.right()) { continue; }
			// A supported body may overhang a lip. Keep its center inside the
			// known floor, retaining room to brake before stepping off it.
			const float launch = std::clamp(takeoff.x, source.bounds.left() + inset,
				source.bounds.right() - inset);
			const float gap = std::fabs(launch - landing.x);
			if (gap < alignment) { alignment = gap; launch_x = launch; floor = true; }
		}
		if (!floor) { return false; }
		takeoff.x = launch_x;
		approach = std::fabs(takeoff.x - center.x);
		// The approach is a checked walk on the known source floor. Only the
		// airborne alignment consumes jump reach; a long supported walk must
		// not make an otherwise adjacent jump unreachable.
		if (approach > this->tuning_->bots().sight_distance ||
			alignment > std::min(reach, this->tuning_->bots().sight_distance) ||
			!this->clear_standing(view, landing) || !this->clear_route(view, center, takeoff)) { return false; }
		// Test a body-clear crossing just above the lip. Requiring the full
		// held-jump apex rejects usable low-ceiling routes: normal collision may
		// truncate their jump after it has already cleared the destination.
		const Vector2F apex(takeoff.x, takeoff.y - std::min(rise, climb + 2.0f));
		const Vector2F over_landing(landing.x, apex.y);
		if (!this->clear_route(view, takeoff, apex) || !this->clear_route(view, apex, over_landing) ||
			!this->clear_route(view, over_landing, landing)) { return false; }
		// Preserve the existing low-step ranking within one jump's locality.
		// Newly admitted longer walks pay their excess distance so a cheaper
		// remote shelf cannot displace useful nearby progression. A normal
		// solid lip already needs a body-width crossing; charge only the extra
		// airborne gap.
		cost = climb + std::max(0.0f, approach - reach) +
			std::max(0.0f, alignment - view.size.x);
		return true;
	};
	float best = std::numeric_limits<float>::max();
	float best_approach = std::numeric_limits<float>::max();
	bool best_continues = false;
	for (const MatchBotSupport& support : view.supports)
	{
		bool visited = false;
		for (const RectangleF& previous : this->visited_supports_)
		{ visited = visited || same_support(previous, support.bounds); }
		if (visited) { continue; }
		Vector2F takeoff, landing;
		float cost = 0.0f, approach = 0.0f;
		if (!route(view.center, support, takeoff, landing, cost, approach)) { continue; }
		bool continues = false;
		for (const MatchBotSupport& next : view.supports)
		{
			bool next_visited = false;
			for (const RectangleF& previous : this->visited_supports_)
			{ next_visited = next_visited || same_support(previous, next.bounds); }
			if (next_visited) { continue; }
			Vector2F next_takeoff, next_landing;
			float next_cost = 0.0f, next_approach = 0.0f;
			if (route(landing, next, next_takeoff, next_landing, next_cost, next_approach))
			{ continues = true; break; }
		}
		if ((continues && !best_continues) || (continues == best_continues &&
			(cost < best || (cost == best && approach < best_approach))))
		{
			best = cost; best_approach = approach; best_continues = continues;
			this->ascent_support_ = support.bounds; this->ascent_takeoff_ = takeoff;
			this->ascending_ = true; this->ascent_airborne_ = false;
			// Allow the supported approach at ordinary minimum moving speed,
			// then retain the existing finite jump/recovery allowance.
			const double travel = std::ceil(static_cast<double>(approach) /
				movement.x_initial_velocity * 60.0);
			const double budget = travel + this->tuning_->bots().progress_window_steps +
				this->tuning_->bots().recovery_steps;
			this->ascent_steps_left_ = static_cast<int>(std::min(budget,
				static_cast<double>((std::numeric_limits<int>::max)())));
		}
	}
	if (!this->ascending_) { this->continue_ascent_ = false; }
}

void MatchBot::remember_descent(const MatchBotObservation& view, Vector2F goal)
{
	// A fresh drop must leave the ground before its landing ends the latch.
	if (!this->seeking_after_descent_) { this->descent_airborne_ = false; }
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

void MatchBot::steer_finish(const MatchBotObservation& view, PlayerInputData& input)
{
	input.x_movement = steer(this->finish_x_, view, this->tuning_->player());
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	input.drop_through = false;
}

void MatchBot::steer_ascent(const MatchBotObservation& view, PlayerInputData& input)
{
	const PlayerTuning& movement = this->tuning_->player();
	const Vector2F half = view.size * 0.5f;
	const RectangleF& target = this->ascent_support_;
	bool present = false;
	bool one_way = false;
	bool paintable = false;
	for (const MatchBotSupport& support : view.supports)
	{
		if (same_support(target, support.bounds))
		{ present = true; one_way = support.one_way; paintable = support.paintable; }
	}
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
	const bool crossing = !one_way && std::fabs(landing.x - this->ascent_takeoff_.x) > view.size.x;
	input.drop_through = false;
	float destination = this->ascent_takeoff_.x;
	if (!view.grounded) { this->ascent_airborne_ = true; }
	if (this->ascent_airborne_ && view.grounded)
	{
		this->visited_supports_.push_back(target);
		this->continue_ascent_ = std::fabs(feet - target.top()) < 0.1f &&
			view.center.x + half.x > target.left() && view.center.x - half.x < target.right();
		// The next decision opens a finishing window if the chain ends here.
		this->finish_pending_ = this->continue_ascent_ && paintable;
		this->ascending_ = false;
		this->jump_left_ = 0;
		this->react_in_ = 1;
		this->replan_in_ = 0;
		input.x_movement = 0.0f;
		input.left_analog_stick = {};
		return;
	}
	if (this->ascent_airborne_)
	{
		if (one_way || crossing || feet <= target.top()) { destination = landing.x; }
		else if (this->ascent_takeoff_.x < target.center().x)
		{ destination = target.left() - half.x - 2.0f; }
		else { destination = target.right() + half.x + 2.0f; }
	}
	input.x_movement = steer(destination, view, movement);
	// Across a gap, braking to a standing takeoff wastes the short interval
	// above a solid lip. Carry the ordinary run-up into the jump instead.
	if (crossing && !this->ascent_airborne_)
	{ input.x_movement = landing.x > view.center.x ? 1.0f : -1.0f; }
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	const float launch_margin = crossing ? std::max(3.0f, std::fabs(view.velocity.x) / 60.0f + 2.0f) : 3.0f;
	if (!this->ascent_airborne_ && std::fabs(view.center.x - destination) <= launch_margin &&
		(crossing || std::fabs(view.velocity.x) <= movement.x_initial_velocity) && !this->jump_was_held_)
	{
		const double steps = std::ceil(static_cast<double>(movement.jump_max_time) * 60.0);
		this->jump_left_ = std::max(1, static_cast<int>(std::min(steps,
			static_cast<double>((std::numeric_limits<int>::max)()))));
	}
}
