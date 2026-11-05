#include "game/pch.h"
#include "game/ai/match_bot.h"
#include "game/contracts/simulation.h"
#include <cmath>
#include <limits>

using namespace labrador;

namespace
{
	float steer(float target, const MatchBotObservation& view, const MatchBotLocomotion& locomotion)
	{
		const float dx = target - view.center.x;
		const float stopping = locomotion.stopping_distance(view.velocity.x, view.grounded, view.footing);
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
	const Vector2F half = view.size * 0.5f;
	const float inset = std::min(2.0f, half.x);
	// A standing launch starts at its footing's starting speed. The takeoff
	// may lie away from this footing, so plan the first step at the slower of
	// this footing and unpainted ground: own paint here is no promise of own
	// paint at the lip. A next step from a landing whose paint is not yet
	// known assumes unpainted ground.
	const MatchBotLocomotion::JumpArc here = this->locomotion_.jump(std::min(
		this->locomotion_.ground(view.footing).initial,
		this->locomotion_.ground(MatchBotFooting::none).initial));
	const MatchBotLocomotion::JumpArc onward = this->locomotion_.jump(
		this->locomotion_.ground(MatchBotFooting::none).initial);
	float rise = here.rise, reach = here.reach;
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
		for (const MatchBotSupport& previous : this->visited_supports_)
		{ visited = visited || same_support(previous, support); }
		if (visited) { continue; }
		Vector2F takeoff, landing;
		float cost = 0.0f, approach = 0.0f;
		rise = here.rise; reach = here.reach;
		if (!route(view.center, support, takeoff, landing, cost, approach)) { continue; }
		rise = onward.rise; reach = onward.reach;
		bool continues = false;
		for (const MatchBotSupport& next : view.supports)
		{
			bool next_visited = false;
			for (const MatchBotSupport& previous : this->visited_supports_)
			{ next_visited = next_visited || same_support(previous, next); }
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
			this->ascent_support_ = support; this->ascent_takeoff_ = takeoff;
			this->ascending_ = true; this->ascent_airborne_ = false;
			// Allow the supported approach at ordinary minimum moving speed,
			// then retain the existing finite jump/recovery allowance.
			const double travel = std::ceil(static_cast<double>(approach) /
				this->tuning_->player().x_initial_velocity * SIMULATION_HZ);
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
	// A fresh drop must leave the ground before stuck turns end the latch.
	if (!this->seeking_after_descent_) { this->descent_airborne_ = false; this->descent_turns_ = 0; }
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
		for (const MatchBotSupport& previous : this->visited_supports_)
		{ visited = visited || same_support(previous, support); }
		if (!visited) { this->visited_supports_.push_back(support); }
	}
}

void MatchBot::steer_finish(const MatchBotObservation& view, PlayerInputData& input)
{
	// Hold the chosen spot on a moving support, not where it was in the world.
	float target = this->finish_x_;
	for (const MatchBotSupport& support : view.supports)
	{
		if (same_support(this->finish_support_, support) && support.bounds.left() != this->finish_anchor_)
		{ target += support.bounds.left() - this->finish_anchor_; }
	}
	input.x_movement = steer(target, view, this->locomotion_);
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	input.drop_through = false;
}

void MatchBot::steer_ascent(const MatchBotObservation& view, PlayerInputData& input)
{
	const PlayerTuning& movement = this->tuning_->player();
	const Vector2F half = view.size * 0.5f;
	bool present = false;
	bool one_way = false;
	bool paintable = false;
	for (const MatchBotSupport& support : view.supports)
	{
		if (same_support(this->ascent_support_, support))
		{ present = true; one_way = support.one_way; paintable = support.paintable; this->ascent_support_ = support; }
	}
	const RectangleF& target = this->ascent_support_.bounds;
	const Vector2F landing(std::clamp(this->ascent_takeoff_.x, target.left() + half.x,
		target.right() - half.x), target.top() - half.y);
	if (!present || !this->clear_standing(view, landing) || --this->ascent_steps_left_ <= 0)
	{
		if (present) { this->visited_supports_.push_back(this->ascent_support_); }
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
		this->visited_supports_.push_back(this->ascent_support_);
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
		// A crossing still rising with its feet below a solid lip holds beside
		// the lip's face too: a fast run-up otherwise meets that face low
		// enough to catch its corner and lose the jump. A falling one has no
		// height left to gain and keeps its line.
		if (one_way || feet <= target.top() || (crossing && view.velocity.y >= 0.0f)) { destination = landing.x; }
		else if (this->ascent_takeoff_.x < target.center().x)
		{ destination = target.left() - half.x - 2.0f; }
		else { destination = target.right() + half.x + 2.0f; }
	}
	input.x_movement = steer(destination, view, this->locomotion_);
	// Across a gap, braking to a standing takeoff wastes the short interval
	// above a solid lip. Carry the ordinary run-up into the jump instead.
	if (crossing && !this->ascent_airborne_)
	{ input.x_movement = landing.x > view.center.x ? 1.0f : -1.0f; }
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	const float launch_margin = crossing ? std::max(3.0f, std::fabs(view.velocity.x) / static_cast<float>(SIMULATION_HZ) + 2.0f) : 3.0f;
	if (!this->ascent_airborne_ && std::fabs(view.center.x - destination) <= launch_margin &&
		(crossing || std::fabs(view.velocity.x) <= this->locomotion_.ground(view.footing).initial) && !this->jump_was_held_)
	{
		const double steps = std::ceil(static_cast<double>(movement.jump_max_time) * SIMULATION_HZ);
		this->jump_left_ = std::max(1, static_cast<int>(std::min(steps,
			static_cast<double>((std::numeric_limits<int>::max)()))));
	}
}
