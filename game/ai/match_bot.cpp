#include "game/pch.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_ballistics.h"
#include "game/ai/match_bot_territory.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	// The driver qualified a shot onto this support's own unpainted top.
	bool ride_target(const MatchBotObservation& view, const MatchBotSupport& support)
	{
		return view.support_open && view.support_paint && view.support_bounds == support.bounds;
	}
}

MatchBot::MatchBot(const GameTuning* tuning, BotDifficulty difficulty, std::uint64_t seed) :
	tuning_(tuning), difficulty_(difficulty), random_state_(seed)
{
	if (tuning == nullptr) { throw std::invalid_argument("A match bot requires tuning"); }
	(void)tuning->bot(difficulty);
	this->heading_ = this->random_unit() < 0.5f ? -1.0f : 1.0f;
}

std::uint64_t MatchBot::random()
{
	// splitmix64; no global random state or other participant's sequence.
	std::uint64_t value = (this->random_state_ += 0x9E3779B97F4A7C15ull);
	value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
	value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
	return value ^ (value >> 31);
}

float MatchBot::random_unit()
{
	return static_cast<float>(this->random() >> 40) / 16777216.0f;
}

bool MatchBot::needs_territory() const
{
	return this->replan_in_ <= 0 && this->recovery_left_ == 0;
}

bool MatchBot::deliberate(WeaponType weapon) const
{
	const float gate = this->tuning_->bots().deliberate_shot_interval;
	return gate > 0.0f && this->tuning_->weapon(weapon).details.shoot_interval >= gate;
}

bool MatchBot::finishing() const
{
	return this->finish_pending_ || this->finish_left_ > 0;
}

bool MatchBot::riding() const
{
	return this->riding_;
}

const MatchBotSupport* MatchBot::moving_support(const MatchBotObservation& view) const
{
	if (!view.grounded) { return nullptr; }
	const float feet = view.center.y + view.size.y * 0.5f;
	for (const MatchBotSupport& support : view.supports)
	{
		if (support.moving && support.paintable && std::fabs(feet - support.bounds.top()) < 0.1f &&
			view.center.x >= support.bounds.left() && view.center.x <= support.bounds.right()) { return &support; }
	}
	return nullptr;
}

void MatchBot::open_finish(const MatchBotObservation& view, const MatchBotSupport& support)
{
	// Long enough to recover the ordinary reserve hysteresis and then pass one
	// cooldown. Firing rules are unchanged; the bot only stays to use them.
	const BotTuning& policy = this->tuning_->bots();
	double refill = 0.0;
	if (this->refilling_)
	{
		const double per_step = static_cast<double>(this->tuning_->ammo_regen_rate()) *
			view.paint_refill / this->tuning_->starting_ammo() / 60.0;
		refill = std::ceil(static_cast<double>(this->tuning_->ammo_regen_delay()) * 60.0) + (per_step > 0.0 ?
			std::ceil(std::max(0.0, static_cast<double>(policy.reserve_resume - view.ammo)) / per_step) : 0.0);
	}
	const double budget = refill + std::ceil(static_cast<double>(
		this->tuning_->weapon(view.weapon).details.shoot_interval) * 60.0) + policy.finish_dwell_steps;
	this->finish_support_ = support;
	this->finish_left_ = static_cast<int>(std::min(budget, 3600.0));
	// A ride's hold is bounded, even while unpainted top remains in sight.
	if (support.moving && same_support(support, this->ride_support_))
	{ this->finish_left_ = std::min(this->finish_left_, this->ride_hold_left_); }
}

void MatchBot::choose_territory(const MatchBotObservation& view)
{
	float best = std::numeric_limits<float>::max();
	float best_aim = std::numeric_limits<float>::max();
	this->has_goal_ = false;
	this->has_paint_goal_ = false;
	for (const Vector2F& candidate : view.territory)
	{
		const Vector2F aim_gap = candidate - view.center;
		const float aim_cost = std::fabs(aim_gap.x) + std::fabs(aim_gap.y) * 2.0f;
		if (aim_cost < best_aim && aim_gap.length() <= view.engage_distance)
		{
			best_aim = aim_cost; this->paint_goal_ = candidate; this->has_paint_goal_ = true;
		}
		Vector2F standing;
		if (!this->standing_goal(view, candidate, standing)) { continue; }
		const Vector2F gap = standing - view.center;
		// Favour nearby paint, with a cost for changing floor. Ignore the tile
		// already directly under our feet so a single target cannot pin us there.
		if (std::fabs(gap.x) < view.size.x && std::fabs(gap.y) < view.size.y) { continue; }
		const float cost = std::fabs(gap.x) + std::fabs(gap.y) * 2.0f;
		if (cost < best)
		{
			best = cost; this->goal_ = standing; this->has_goal_ = true;
		}
	}
	if (view.support_paint)
	{
		const Vector2F gap = *view.support_paint - view.center;
		// Prefer the checked local launch over a random nearer tile that can
		// lie behind this tool's muzzle. This remains an aim-only choice.
		if (gap.length() <= view.engage_distance)
		{ this->paint_goal_ = *view.support_paint; this->has_paint_goal_ = true; }
	}
	// Occluded scoring faces must not leave a bot retrying a spawn baffle.
	// A one-way support can offer an ordinary downward route to new sight lines.
	if (!this->has_goal_ && view.on_platform) { this->choose_descent(view); }
	if (!this->has_goal_ && this->random_unit() < 0.25f) { this->heading_ = -this->heading_; }
	this->replan_in_ = this->tuning_->bot(this->difficulty_).replan_steps;
}

bool MatchBot::obstacle_ahead(const MatchBotObservation& view) const
{
	const float front = view.center.x + this->heading_ * view.size.x;
	const float head = view.center.y - view.size.y * 0.4f;
	const float feet = view.center.y + view.size.y * 0.4f;
	for (const RectangleF& obstacle : view.obstacles)
	{
		if (front >= obstacle.left() && front <= obstacle.right() &&
			obstacle.top() < feet && obstacle.bottom() > head) { return true; }
	}
	return false;
}

bool MatchBot::ledge_ahead(const MatchBotObservation& view) const
{
	const float ahead = view.center.x + this->heading_ * view.size.x * 1.5f;
	const float feet = view.center.y + view.size.y * 0.5f;
	for (const RectangleF& obstacle : view.obstacles)
	{
		if (ahead >= obstacle.left() && ahead <= obstacle.right() &&
			obstacle.top() >= feet - view.size.y * 0.25f &&
			obstacle.top() <= feet + view.size.y) { return false; }
	}
	return true;
}

bool MatchBot::unscored_floor(const MatchBotObservation& view) const
{
	if (!view.grounded) { return false; }
	const float feet = view.center.y + view.size.y * 0.5f;
	bool standing = false;
	for (const MatchBotSupport& support : view.supports)
	{
		if (std::fabs(feet - support.bounds.top()) >= 0.1f ||
			view.center.x < support.bounds.left() || view.center.x > support.bounds.right()) { continue; }
		if (support.paintable) { return false; }
		standing = true;
	}
	return standing;
}

void MatchBot::decide(const MatchBotObservation& view)
{
	this->decision_ = {};
	this->decision_.connected = true;
	const MatchBotObservation::Enemy* enemy = nullptr;
	float nearest = std::numeric_limits<float>::max();
	for (const MatchBotObservation::Enemy& candidate : view.enemies)
	{
		const float distance = Vector2F::distance_squared(candidate.center, view.center);
		if (distance < nearest) { nearest = distance; enemy = &candidate; }
	}
	// Landing can happen between territory replans. Check the newly available
	// one-way escape before a fallback obstacle jump sends us back into a loop.
	const bool was_finishing = this->finish_left_ > 0;
	if (!was_finishing && !this->has_goal_ && view.on_platform && this->recovery_left_ == 0)
	{ this->choose_descent(view); }
	Vector2F goal = this->has_goal_ ? this->goal_ :
		view.center + Vector2F(this->heading_ * view.size.x * 4.0f, 0.0f);
	const bool engaging = enemy != nullptr && nearest <= view.engage_distance * view.engage_distance;
	// A visible opponent outside this tool's range still needs an approach.
	// Using the firing radius as the pursuit gate stranded short-range tools
	// on unrelated paint goals before they could ever enter combat.
	if (enemy != nullptr && !this->refilling_) { goal = enemy->center; }
	// Back away only from an enemy that could reach us soon. Retreating from
	// one far outside its own range walked a refill back into home ground.
	const BotTuning& policy = this->tuning_->bots();
	const float threat = enemy == nullptr ? 0.0f : enemy->engage_distance + policy.refill_retreat_margin;
	const bool retreating = this->refilling_ && enemy != nullptr && nearest <= threat * threat;
	if (retreating)
	{
		goal = view.center + Vector2F(enemy->center.x > view.center.x ? -view.size.x * 4.0f :
			view.size.x * 4.0f, 0.0f);
	}
	// Enemy paint stops the refill under the underfoot trial, so walking on to
	// the next paint goal could leave the bot empty. Go back to the last ground
	// that wasn't enemy paint, and hold still there until the reserve resumes.
	else if (this->refilling_ && policy.refill_off_enemy_paint && view.underfoot)
	{
		if (!view.on_enemy_paint) { goal = view.center; }
		// That ground may have been painted since; then walk on as before.
		else if (this->has_off_enemy_ && std::fabs(this->off_enemy_center_.x - view.center.x) > view.size.x)
		{ goal = this->off_enemy_center_; }
	}
	if (this->recovery_left_ == 0 && this->blocked_steps_ >= policy.stuck_turn_steps)
	{
		this->heading_ = -this->heading_;
		// A completed descent onto non-scoring ground keeps the latch while
		// the bot can walk on. A second stuck turn there, blocked both ways,
		// means the floor's only exit may be a climb, so ascents become
		// eligible once recovery ends. One dead end still walks out.
		if (this->seeking_after_descent_ && this->descent_airborne_ && ++this->descent_turns_ >= 2)
		{ this->seeking_after_descent_ = this->descent_airborne_ = false; }
		this->recovery_left_ = policy.recovery_steps;
		this->blocked_steps_ = this->progress_steps_ = 0;
		this->progress_origin_ = view.center;
		this->has_goal_ = false;
		this->has_paint_goal_ = false;
		this->replan_in_ = 0;
	}
	// Commit to leaving the failed goal before asking the sampler for another.
	// Otherwise a fast replan can turn back into the same obstruction at once.
	const bool recovering = this->recovery_left_ > 0;
	if (recovering || enemy != nullptr)
	{
		this->ascending_ = this->continue_ascent_ = false;
		this->climbing_ramp_ = false;
	}
	else if (!was_finishing && !this->ascending_ && !this->climbing_ramp_ && !this->seeking_after_descent_ &&
		view.grounded && (!view.on_platform || this->continue_ascent_))
	{
		this->choose_ramp(view);
		if (!this->climbing_ramp_) { this->choose_ascent(view); }
	}
	// Only a chain that ends on this landing opens the window; a landing with
	// a further ascent or ramp has already chosen it above.
	if (this->finish_pending_)
	{
		this->finish_pending_ = false;
		if (!recovering && enemy == nullptr && !this->ascending_ && !this->climbing_ramp_ &&
			view.grounded && view.support_open) { this->open_finish(view, this->ascent_support_); }
	}
	// A bot riding a moving paintable support stays and paints its visible
	// unpainted top, however it got there, for a bounded hold per ride.
	// Only a qualified shot onto that top holds it: a sliver that no shot
	// reaches does not.
	if (this->finish_left_ == 0 && !recovering && enemy == nullptr && !this->ascending_ &&
		!this->climbing_ramp_ && this->ride_hold_left_ > 0)
	{
		const MatchBotSupport* mover = this->moving_support(view);
		if (mover != nullptr && ride_target(view, *mover)) { this->open_finish(view, *mover); }
	}
	const bool finishing = this->finish_left_ > 0;
	if (finishing)
	{
		// Stay on the fully supported span, keeping a local goal when it lies
		// on this support; otherwise explore along it, turned inward.
		for (const MatchBotSupport& support : view.supports)
		{ if (same_support(this->finish_support_, support)) { this->finish_support_ = support; } }
		const RectangleF& span = this->finish_support_.bounds;
		this->finish_anchor_ = span.left();
		const float half = view.size.x * 0.5f, inset = std::min(2.0f, half);
		const float low = span.left() + half + inset;
		const float high = span.right() - half - inset;
		if (view.center.x < low) { this->heading_ = 1.0f; }
		else if (view.center.x > high) { this->heading_ = -1.0f; }
		const bool local_goal = this->has_goal_ && std::fabs(this->goal_.y - view.center.y) < 0.1f;
		const float wanted = local_goal ? this->goal_.x : view.center.x + this->heading_ * view.size.x * 4.0f;
		this->finish_x_ = low <= high ? std::clamp(wanted, low, high) : span.center().x;
		goal = Vector2F(this->finish_x_, view.center.y);
	}
	if (recovering) { goal = view.center + Vector2F(this->heading_ * view.size.x * 4.0f, 0.0f); }
	const float dx = goal.x - view.center.x;
	float movement = std::fabs(dx) > view.size.x * 0.4f ? (dx > 0.0f ? 1.0f : -1.0f) : 0.0f;
	if (engaging && !this->refilling_ && !recovering && std::fabs(dx) <= view.engage_distance * 0.55f)
	{
		movement = 0.0f;
	}
	if (movement != 0.0f) { this->heading_ = movement; }
	this->decision_.x_movement = movement;
	this->decision_.left_analog_stick = Vector2F(movement, 0.0f);
	const bool above = goal.y < view.center.y - view.size.y * 0.5f;
	const bool dropping = view.on_platform && goal.y > view.center.y + view.size.y * 0.5f;
	if (dropping && !this->ascending_ && !this->climbing_ramp_) { this->remember_descent(view, goal); }
	if (dropping) { this->jump_left_ = 0; }
	const bool jump = !finishing && !this->ascending_ && !this->climbing_ramp_ && !dropping && view.grounded && movement != 0.0f &&
		(above || this->blocked_steps_ >= policy.stuck_jump_steps ||
			this->obstacle_ahead(view) || (this->ledge_ahead(view) && goal.y <= view.center.y));
	if (jump && !this->jump_was_held_)
	{
		const double steps = std::ceil(static_cast<double>(this->tuning_->player().jump_max_time) * 60.0);
		this->jump_left_ = std::max(1, static_cast<int>(std::min(steps,
			static_cast<double>((std::numeric_limits<int>::max)()))));
	}
	this->decision_.drop_through = dropping;
	Vector2F aim(this->heading_, 0.7f);
	bool targeted = true;
	if (engaging)
	{
		const WeaponTuning& tool = this->tuning_->weapon(view.weapon);
		aim = match_bot_ballistics::aim_direction(tool, this->tuning_->projectile(tool.details.proj_type),
			view.velocity, enemy->center - view.center, enemy->velocity);
	}
	else if (view.support_paint && (finishing || this->deliberate(view.weapon)))
	{
		// A finishing or slow tool requalifies its support target from the
		// current pose rather than walking into a point fixed at the last replan.
		this->paint_goal_ = *view.support_paint; this->has_paint_goal_ = true;
		aim = this->paint_goal_ - view.center;
	}
	else if (this->has_paint_goal_)
	{
		aim = this->paint_goal_ - view.center;
	}
	else { targeted = false; }
	if (aim.length_squared() > 0.0f)
	{
		const float error = this->tuning_->bot(this->difficulty_).aim_error_radians;
		this->aim_error_ = (this->random_unit() * 2.0f - 1.0f) * error;
		const float angle = aim.angle() + this->aim_error_;
		this->decision_.shoot_angle = angle;
		this->decision_.shoot_direction = Vector2F(std::cos(angle), std::sin(angle));
		this->decision_.shoot_direction_requested = true;
		this->decision_.right_analog_stick = this->decision_.shoot_direction;
	}
	// Untargeted fire paints the floor ahead. On a known non-scoring floor it
	// only drains the reserve the bot needs on reaching scoring ground.
	this->decision_.primary_shoot = !this->refilling_ && !view.shot_empty &&
		(targeted || !this->unscored_floor(view));
	// The window steers each step; a held decision would count as blocked.
	if (finishing) { this->decision_.x_movement = 0.0f; this->decision_.left_analog_stick = {}; }
	this->react_in_ = this->tuning_->bot(this->difficulty_).reaction_steps;
}

PlayerInputData MatchBot::think(const MatchBotObservation& view)
{
	PlayerInputData neutral;
	neutral.connected = true;
	if (!view.alive)
	{
		this->had_alive_step_ = false;
		this->react_in_ = this->replan_in_ = this->blocked_steps_ = this->jump_left_ = 0;
		this->recovery_left_ = this->progress_steps_ = 0;
		this->jump_was_held_ = this->refilling_ = this->has_goal_ = false;
		this->has_paint_goal_ = false;
		this->ascending_ = this->ascent_airborne_ = this->continue_ascent_ = false;
		this->climbing_ramp_ = false;
		this->seeking_after_descent_ = this->descent_airborne_ = false;
		this->descent_turns_ = 0;
		this->finish_pending_ = false;
		this->finish_left_ = 0;
		this->riding_ = false;
		this->ride_hold_left_ = 0;
		this->has_off_enemy_ = false;
		this->visited_supports_.clear();
		this->decision_ = neutral;
		this->aim_error_ = 0.0f;
		return neutral;
	}
	if (!this->had_alive_step_)
	{
		this->had_alive_step_ = true;
		this->react_in_ = this->tuning_->bot(this->difficulty_).reaction_steps;
		this->progress_origin_ = view.center;
	}
	const BotTuning& policy = this->tuning_->bots();
	if (view.shot_empty || view.ammo <= policy.reserve_low) { this->refilling_ = true; }
	else if (this->refilling_ && view.ammo >= policy.reserve_resume) { this->refilling_ = false; }
	if (view.grounded && !view.on_enemy_paint) { this->has_off_enemy_ = true; this->off_enemy_center_ = view.center; }
	if (this->seeking_after_descent_ && !view.grounded) { this->descent_airborne_ = true; }
	if (this->seeking_after_descent_ && view.grounded)
	{
		for (const MatchBotSupport& support : view.supports)
		{
			if (support.paintable && std::fabs(view.center.y + view.size.y * 0.5f - support.bounds.top()) < 0.1f &&
				view.center.x >= support.bounds.left() && view.center.x <= support.bounds.right())
			{ this->seeking_after_descent_ = this->descent_airborne_ = false; }
		}
	}
	if (!this->ascending_ && !this->climbing_ramp_ && this->decision_.x_movement != 0.0f && std::fabs(view.velocity.x) < 1.0f)
	{
		++this->blocked_steps_;
	}
	else { this->blocked_steps_ = 0; }
	if (!this->ascending_ && !this->climbing_ramp_ && this->decision_.x_movement != 0.0f && this->recovery_left_ == 0)
	{
		if (Vector2F::distance_squared(view.center, this->progress_origin_) >=
			policy.progress_distance * policy.progress_distance)
		{
			this->progress_origin_ = view.center;
			this->progress_steps_ = 0;
		}
		else if (++this->progress_steps_ >= policy.progress_window_steps)
		{
			// Velocity alone misses pacing or a moving support carrying us back
			// and forth without making progress toward any new territory.
			this->blocked_steps_ = std::max(this->blocked_steps_, policy.stuck_turn_steps);
		}
	}
	else
	{
		this->progress_origin_ = view.center;
		this->progress_steps_ = 0;
	}
	// A visible enemy ends the window and keeps the ordinary reaction delay.
	if (this->finish_left_ > 0 && !view.enemies.empty()) { this->finish_left_ = 0; }
	else if (this->finish_left_ > 0 && (!view.grounded || this->recovery_left_ > 0))
	{ this->finish_left_ = 0; this->react_in_ = 1; }
	// A new ride starts a fresh hold; the hold counts down while riding.
	const MatchBotSupport* mover = this->moving_support(view);
	if (mover == nullptr) { this->ride_hold_left_ = 0; }
	else if (!this->riding_ || !same_support(*mover, this->ride_support_))
	{ this->ride_support_ = *mover; this->ride_hold_left_ = policy.mover_hold_steps; }
	else if (this->ride_hold_left_ > 0) { --this->ride_hold_left_; }
	this->riding_ = mover != nullptr;
	// Decide now rather than inheriting a drop through the support.
	if (mover != nullptr && this->finish_left_ == 0 && this->ride_hold_left_ > 0 && ride_target(view, *mover) &&
		view.enemies.empty() && this->recovery_left_ == 0 && !this->ascending_ && !this->climbing_ramp_)
	{ this->react_in_ = std::min(this->react_in_, 1); }
	if (this->recovery_left_ == 0 && this->replan_in_-- <= 0) { this->choose_territory(view); }
	if (--this->react_in_ <= 0) { this->decide(view); }
	PlayerInputData input = this->decision_;
	if (this->ascending_) { this->steer_ascent(view, input); }
	if (this->climbing_ramp_) { this->steer_ramp(view, input); }
	if (this->finish_left_ > 0)
	{
		this->steer_finish(view, input);
		this->jump_left_ = 0;
		if (--this->finish_left_ == 0) { this->react_in_ = 1; }
	}
	// One deliberate shot outlasts its decision. With a checked top target and
	// no visible opponent, re-aim each step with the decision's drawn error and
	// spend it only on an angle that lands. A climb's pose changes faster than
	// that shot recovers, so mid-chain it is spent on nothing else.
	// Before this life's first decision there is no drawn error to reuse.
	const bool slow = view.enemies.empty() && this->deliberate(view.weapon) &&
		this->decision_.shoot_direction_requested;
	if (slow && view.support_paint)
	{
		const float angle = (*view.support_paint - view.center).angle() + this->aim_error_;
		input.shoot_angle = angle;
		input.shoot_direction = Vector2F(std::cos(angle), std::sin(angle));
		input.shoot_direction_requested = true;
		input.right_analog_stick = input.shoot_direction;
		input.primary_shoot = match_bot_territory::top_launch_lands(view, view.support_bounds, angle, *this->tuning_);
	}
	else if (slow && (this->ascending_ || this->climbing_ramp_ || this->continue_ascent_)) { input.primary_shoot = false; }
	input.connected = true;
	// The reserve guard is immediate, even between policy reactions. It cannot
	// buy an unaffordable shot or inherit firing from a depleted decision.
	if (this->refilling_ || view.shot_empty) { input.primary_shoot = false; }
	input.jump_held = this->jump_left_ > 0;
	input.jump_pressed = input.jump_held && !this->jump_was_held_;
	this->jump_was_held_ = input.jump_held;
	if (this->jump_left_ > 0) { --this->jump_left_; }
	if (this->recovery_left_ > 0) { --this->recovery_left_; }
	return input;
}
