#include "game/pch.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_ballistics.h"
#include <cmath>
#include <limits>

using namespace mattmath;

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
		const float cost = std::fabs(gap.x) + std::fabs(gap.y) * 2.0f;
		if (cost < best_aim && gap.length() <= view.engage_distance)
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
	if (!this->has_goal_ && view.on_platform && this->recovery_left_ == 0)
	{ this->choose_descent(view); }
	Vector2F goal = this->has_goal_ ? this->goal_ :
		view.center + Vector2F(this->heading_ * view.size.x * 4.0f, 0.0f);
	const bool engaging = enemy != nullptr && nearest <= view.engage_distance * view.engage_distance;
	// A visible opponent outside this tool's range still needs an approach.
	// Using the firing radius as the pursuit gate stranded short-range tools
	// on unrelated paint goals before they could ever enter combat.
	if (enemy != nullptr && !this->refilling_) { goal = enemy->center; }
	if (this->refilling_ && enemy != nullptr)
	{
		goal = view.center + Vector2F(enemy->center.x > view.center.x ? -view.size.x * 4.0f :
			view.size.x * 4.0f, 0.0f);
	}
	const BotTuning& policy = this->tuning_->bots();
	if (this->recovery_left_ == 0 && this->blocked_steps_ >= policy.stuck_turn_steps)
	{
		this->heading_ = -this->heading_;
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
	else if (!this->ascending_ && !this->climbing_ramp_ && !this->seeking_after_descent_ &&
		view.grounded && (!view.on_platform || this->continue_ascent_))
	{
		this->choose_ramp(view);
		if (!this->climbing_ramp_) { this->choose_ascent(view); }
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
	const bool jump = !this->ascending_ && !this->climbing_ramp_ && !dropping && view.grounded && movement != 0.0f &&
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
	if (engaging)
	{
		const WeaponTuning& tool = this->tuning_->weapon(view.weapon);
		aim = match_bot_ballistics::aim_direction(tool, this->tuning_->projectile(tool.details.proj_type),
			view.velocity, enemy->center - view.center, enemy->velocity);
	}
	else if (this->has_paint_goal_)
	{
		aim = this->paint_goal_ - view.center;
	}
	if (aim.length_squared() > 0.0f)
	{
		const float error = this->tuning_->bot(this->difficulty_).aim_error_radians;
		const float angle = aim.angle() + (this->random_unit() * 2.0f - 1.0f) * error;
		this->decision_.shoot_angle = angle;
		this->decision_.shoot_direction = Vector2F(std::cos(angle), std::sin(angle));
		this->decision_.shoot_direction_requested = true;
		this->decision_.right_analog_stick = this->decision_.shoot_direction;
	}
	this->decision_.primary_shoot = !this->refilling_ && !view.shot_empty;
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
		this->seeking_after_descent_ = false;
		this->visited_supports_.clear();
		this->decision_ = neutral;
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
	if (this->seeking_after_descent_ && view.grounded)
	{
		for (const MatchBotSupport& support : view.supports)
		{
			if (support.paintable && std::fabs(view.center.y + view.size.y * 0.5f - support.bounds.top()) < 0.1f &&
				view.center.x >= support.bounds.left() && view.center.x <= support.bounds.right())
			{ this->seeking_after_descent_ = false; }
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
	if (this->recovery_left_ == 0 && this->replan_in_-- <= 0) { this->choose_territory(view); }
	if (--this->react_in_ <= 0) { this->decide(view); }
	PlayerInputData input = this->decision_;
	if (this->ascending_) { this->steer_ascent(view, input); }
	if (this->climbing_ramp_) { this->steer_ramp(view, input); }
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
