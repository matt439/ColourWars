#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include <cmath>

using namespace mattmath;

namespace
{
	// Steps (at 60 a second) a bot keeps walking before it may turn by itself.
	constexpr int min_walk_steps = 120;
	constexpr int max_walk_steps = 360;
	// Barely moving for this long while trying to walk: a wall.
	constexpr int wall_steps = 20;
	constexpr int stuck_jump_steps = 8;
	constexpr float still_speed = 1.0f;

	constexpr float chase_range = 900.0f;
	constexpr float paint_engage_range = 400.0f;
	constexpr float pi = 3.14159265f;
}

Bot::Bot(std::uint64_t seed) : random_(seed)
{
}

const std::vector<std::string>& Bot::behaviours()
{
	static const std::vector<std::string> names = { "idle", "walk", "jump", "chase", "paint", "skirmish" };
	return names;
}

bool Bot::is_behaviour(std::string_view name)
{
	for (const std::string& behaviour : behaviours())
	{
		if (behaviour == name)
		{
			return true;
		}
	}
	return false;
}

std::unique_ptr<Bot> Bot::make(std::string_view behaviour, std::uint64_t seed, int seat)
{
	const std::uint64_t mixed = BotRandom::seat_seed(seed, seat);
	if (behaviour == "walk") { return std::make_unique<WalkBot>(mixed); }
	if (behaviour == "jump") { return std::make_unique<JumpBot>(mixed); }
	if (behaviour == "chase") { return std::make_unique<ChaseBot>(mixed); }
	if (behaviour == "paint") { return std::make_unique<PaintBot>(mixed); }
	if (behaviour == "skirmish") { return std::make_unique<SkirmishBot>(mixed); }
	return nullptr;
}

PlayerInputData Bot::neutral()
{
	PlayerInputData input;
	input.connected = true;
	return input;
}

void Bot::hold_jump(PlayerInputData& input, bool held)
{
	input.jump_held = held;
	input.jump_pressed = held && !this->jump_was_held_;
	this->jump_was_held_ = held;
}

void Bot::aim(PlayerInputData& input, const Vector2F& direction)
{
	if (direction == Vector2F::ZERO)
	{
		return;
	}
	input.shoot_direction = direction.normalized();
	input.shoot_direction_requested = true;
	input.shoot_angle = input.shoot_direction.angle();
	input.right_analog_stick = input.shoot_direction;
}

// ---------------------------------------------------------------------------

WalkBot::WalkBot(std::uint64_t seed) : Bot(seed)
{
	this->heading_ = this->random_.unit() < 0.5f ? -1.0f : 1.0f;
	this->turn_in_ = this->random_.between(min_walk_steps, max_walk_steps);
}

PlayerInputData WalkBot::think(const BotSeatView& view)
{
	PlayerInputData input = neutral();
	if (!view.self.alive)
	{
		this->hold_jump(input, false);
		this->blocked_steps_ = 0;
		return input;
	}
	// The velocity is the result of the last step's walking.
	this->blocked_steps_ = std::fabs(view.self.velocity.x) < still_speed ?
		this->blocked_steps_ + 1 : 0;
	input.x_movement = this->walk_direction(view);
	input.left_analog_stick = Vector2F(input.x_movement, 0.0f);
	this->decorate(input, view);
	return input;
}

float WalkBot::walk_direction(const BotSeatView&)
{
	if (--this->turn_in_ <= 0 || this->blocked_steps_ >= wall_steps)
	{
		this->heading_ = -this->heading_;
		this->turn_in_ = this->random_.between(min_walk_steps, max_walk_steps);
		this->blocked_steps_ = 0;
	}
	return this->heading_;
}

void WalkBot::decorate(PlayerInputData& input, const BotSeatView&)
{
	this->hold_jump(input, false);
}

// ---------------------------------------------------------------------------

JumpBot::JumpBot(std::uint64_t seed) : WalkBot(seed)
{
	this->jump_in_ = this->random_.between(60, 180);
}

bool JumpBot::wants_jump(const BotSeatView&)
{
	if (this->blocked_steps() >= stuck_jump_steps)
	{
		return true;
	}
	if (--this->jump_in_ <= 0)
	{
		this->jump_in_ = this->random_.between(60, 180);
		return true;
	}
	return false;
}

void JumpBot::decorate(PlayerInputData& input, const BotSeatView& view)
{
	bool hold = false;
	if (this->hold_left_ > 0)
	{
		--this->hold_left_;
		hold = true;
	}
	else if (!input.jump_held && view.self.on_ground && this->wants_jump(view))
	{
		// Held for a random time, so the jumps vary in height.
		this->hold_left_ = this->random_.between(6, 30) - 1;
		hold = true;
	}
	// Bot::hold_jump never repeats a press while the jump is held, so a
	// new jump always follows at least one released step.
	this->hold_jump(input, hold);
}

// ---------------------------------------------------------------------------

ChaseBot::ChaseBot(std::uint64_t seed) : JumpBot(seed)
{
}

const BotSeatView::Seat* ChaseBot::nearest_opponent(const BotSeatView& view)
{
	const BotSeatView::Seat* nearest = nullptr;
	float best = 0.0f;
	for (const BotSeatView::Seat& other : view.others)
	{
		if (!other.alive || other.team == view.self.team)
		{
			continue;
		}
		const float distance = Vector2F::distance_squared(other.center, view.self.center);
		if (nearest == nullptr || distance < best)
		{
			nearest = &other;
			best = distance;
		}
	}
	return nearest;
}

float ChaseBot::walk_direction(const BotSeatView& view)
{
	const BotSeatView::Seat* target = nearest_opponent(view);
	if (target == nullptr)
	{
		return JumpBot::walk_direction(view);
	}
	const float dx = target->center.x - view.self.center.x;
	if (std::fabs(dx) < 40.0f)
	{
		return 0.0f;
	}
	return dx > 0.0f ? 1.0f : -1.0f;
}

bool ChaseBot::wants_jump(const BotSeatView& view)
{
	const BotSeatView::Seat* target = nearest_opponent(view);
	if (target == nullptr)
	{
		return JumpBot::wants_jump(view);
	}
	return target->center.y < view.self.center.y - view.player_height ||
		this->blocked_steps() >= stuck_jump_steps;
}

void ChaseBot::decorate(PlayerInputData& input, const BotSeatView& view)
{
	JumpBot::decorate(input, view);
	const BotSeatView::Seat* target = nearest_opponent(view);
	if (target == nullptr)
	{
		return;
	}
	const Vector2F offset = target->center - view.self.center;
	if (offset.length() <= chase_range)
	{
		aim(input, offset);
		input.primary_shoot = view.self.ammo > 0.1f;
	}
}

// ---------------------------------------------------------------------------

PaintBot::PaintBot(std::uint64_t seed) : JumpBot(seed)
{
	this->sweep_ = this->random_.between(0.5f, 2.6f);
}

void PaintBot::decorate(PlayerInputData& input, const BotSeatView& view)
{
	JumpBot::decorate(input, view);

	// A random walk over the lower half-circle, 30 to 150 degrees below the
	// horizontal (y is down): the floor ahead, below and behind.
	this->sweep_ += this->random_.between(-0.06f, 0.06f);
	this->sweep_ = std::clamp(this->sweep_, pi / 6.0f, pi * 5.0f / 6.0f);
	Vector2F direction(std::cos(this->sweep_), std::sin(this->sweep_));

	for
 (const BotSeatView::Seat& other : view.others)
	{
		if (other.alive && other.team != view.self.team &&
			Vector2F::distance(other.center, view.self.center) <= paint_engage_range)
		{
			direction = other.center - view.self.center;
			break;
		}
	}
	aim(input, direction);

	// Fire down to a sliver of ammo, then let the tank refill.
	if (this->refilling_ && view.self.ammo >= 0.8f)
	{
		this->refilling_ = false;
	}
	else if (!this->refilling_ && view.self.ammo <= 0.15f)
	{
		this->refilling_ = true;
	}
	input.primary_shoot = !this->refilling_;
}

// ---------------------------------------------------------------------------

void JumpBot::reset_jump()
{
	PlayerInputData released = neutral();
	this->hold_jump(released, false);
	this->hold_left_ = 0;
	this->jump_in_ = this->random_.between(60, 180);
}

SkirmishBot::SkirmishBot(std::uint64_t seed) : JumpBot(seed)
{
	this->sweep_ = this->random_.between(pi / 6.0f, pi / 3.0f);
}

const BotSeatView::Seat* SkirmishBot::nearest_enemy(const BotSeatView& view)
{
	if (view.self.team != PlayerTeam::a && view.self.team != PlayerTeam::b)
	{
		return nullptr;
	}
	const PlayerTeam enemy = view.self.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
	const BotSeatView::Seat* nearest = nullptr;
	float best = 0.0f;
	for (const BotSeatView::Seat& other : view.others)
	{
		if (!other.alive || other.health <= 0.0f || other.team != enemy) { continue; }
		const float distance = Vector2F::distance_squared(other.center, view.self.center);
		// Strict comparison preserves view-order ties.
		if (nearest == nullptr || distance < best) { nearest = &other; best = distance; }
	}
	return nearest;
}

PlayerInputData SkirmishBot::think(const BotSeatView& view)
{
	if (!view.self.alive || view.self.health <= 0.0f)
	{
		if (this->was_alive_) { this->reset_jump(); }
		this->was_alive_ = false;
		this->mode_ = Mode::paint;
		this->target_near_ = false;
		this->target_offset_ = Vector2F::ZERO;
		this->return_point_ = Vector2F::ZERO;
		BotSeatView inactive = view;
		inactive.self.alive = false;
		return JumpBot::think(inactive);
	}
	if (!this->was_alive_)
	{
		this->was_alive_ = true;
		this->mode_ = Mode::paint;
		this->return_point_ = view.self.center;
	}
	const BotSeatView::Seat* target = nearest_enemy(view);
	this->target_offset_ = target != nullptr ? target->center - view.self.center : Vector2F::ZERO;
	this->target_near_ = target != nullptr && view.engage_distance > 0.0f &&
		this->target_offset_.length() <= view.engage_distance;

	// A tank can be positive yet unable to buy the next shot (notably Sniper).
	// Recovery requires both the larger tank threshold and full-shot readiness.
	if (this->mode_ != Mode::refill && (view.shot_empty || view.self.ammo <= 0.15f))
	{
		this->mode_ = Mode::refill;
		this->return_point_ = view.self.center;
		this->retreat_heading_ = target != nullptr && this->target_offset_.x != 0.0f ?
			(this->target_offset_.x > 0.0f ? -1.0f : 1.0f) :
			(view.self.facing_right ? -1.0f : 1.0f);
	}
	if (this->mode_ == Mode::refill)
	{
		if (view.self.ammo >= 0.8f && !view.shot_empty) { this->mode_ = Mode::return_to_paint; }
	}
	if (this->mode_ != Mode::refill)
	{
		if (this->target_near_) { this->mode_ = Mode::engage; }
		else if (this->mode_ == Mode::engage) { this->mode_ = Mode::paint; }
		if (this->mode_ == Mode::return_to_paint &&
			std::fabs(this->return_point_.x - view.self.center.x) <= view.player_height * 0.15f)
		{
			this->mode_ = Mode::paint;
		}
	}
	return JumpBot::think(view);
}

float SkirmishBot::walk_direction(const BotSeatView& view)
{
	if (this->mode_ == Mode::refill) { return this->retreat_heading_; }
	if (this->mode_ == Mode::return_to_paint)
	{
		return this->return_point_.x > view.self.center.x ? 1.0f : -1.0f;
	}
	if (this->mode_ == Mode::engage)
	{
		// A simple stopping distance, not cover selection or tool balance.
		const float gap = view.engage_distance * 0.6f;
		if (std::fabs(this->target_offset_.x) <= gap) { return 0.0f; }
		return this->target_offset_.x > 0.0f ? 1.0f : -1.0f;
	}
	return JumpBot::walk_direction(view);
}

bool SkirmishBot::wants_jump(const BotSeatView& view)
{
	const bool above = this->mode_ == Mode::engage ?
		this->target_offset_.y < -view.player_height :
		this->mode_ == Mode::return_to_paint &&
		this->return_point_.y < view.self.center.y - view.player_height;
	return above || JumpBot::wants_jump(view);
}

void SkirmishBot::decorate(PlayerInputData& input, const BotSeatView& view)
{
	JumpBot::decorate(input, view);
	if (this->mode_ == Mode::engage)
	{
		aim(input, this->target_offset_);
		input.primary_shoot = !view.shot_empty;
		return;
	}
	if (this->mode_ == Mode::paint)
	{
		this->sweep_ += this->random_.between(-0.04f, 0.04f);
		this->sweep_ = std::clamp(this->sweep_, pi / 6.0f, pi * 5.0f / 12.0f);
	}
	const float facing = input.x_movement != 0.0f ? input.x_movement :
		(view.self.facing_right ? 1.0f : -1.0f);
	aim(input, Vector2F(facing * std::cos(this->sweep_), std::sin(this->sweep_)));
	// Returning conserves the refill until the old location or a new fight.
	input.primary_shoot = this->mode_ == Mode::paint && !view.shot_empty;
}
#endif
