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

BotRandom::BotRandom(std::uint64_t seed) : state_(seed)
{
}

std::uint64_t BotRandom::next()
{
	std::uint64_t z = (this->state_ += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

float BotRandom::unit()
{
	// 24 bits: every value is exact in a float.
	return static_cast<float>(this->next() >> 40) / 16777216.0f;
}

int BotRandom::between(int low, int high)
{
	const std::uint64_t span = static_cast<std::uint64_t>(high - low) + 1;
	return low + static_cast<int>(this->next() % span);
}

float BotRandom::between(float low, float high)
{
	return low + (high - low) * this->unit();
}

// ---------------------------------------------------------------------------

Bot::Bot(std::uint64_t seed) : random_(seed)
{
}

const std::vector<std::string>& Bot::behaviours()
{
	static const std::vector<std::string> names = { "idle", "walk", "jump", "chase", "paint" };
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
	const std::uint64_t mixed = seed * 0x9E3779B97F4A7C15ull + static_cast<std::uint64_t>(seat) + 1;
	if (behaviour == "walk") { return std::make_unique<WalkBot>(mixed); }
	if (behaviour == "jump") { return std::make_unique<JumpBot>(mixed); }
	if (behaviour == "chase") { return std::make_unique<ChaseBot>(mixed); }
	if (behaviour == "paint") { return std::make_unique<PaintBot>(mixed); }
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
#endif
