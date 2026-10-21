#pragma once

#include "engine/math/vector2f.h"
#include "game/objects/player_input_data.h"
#include "game/objects/player_team.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// A bot's own random numbers: splitmix64, seeded from the match seed and the
// seat, so a bot never touches rand() and two bots never share a sequence.
class BotRandom
{
public:
	explicit BotRandom(std::uint64_t seed = 0);

	std::uint64_t next();
	// [0, 1).
	float unit();
	// [low, high], inclusive.
	int between(int low, int high);
	float between(float low, float high);

private:
	std::uint64_t state_ = 0;
};

// What one bot sees on one logic step: public state only, built by
// BotDriver from the running Level.
struct BotSeatView
{
	struct Seat
	{
		mattmath::Vector2F center = mattmath::Vector2F::ZERO;
		mattmath::Vector2F velocity = mattmath::Vector2F::ZERO;
		PlayerTeam team = PlayerTeam::none;
		bool alive = true;
		bool on_ground = false;
		bool facing_right = true;
		float health = 1.0f; // fraction of starting health
		float ammo = 1.0f;   // fraction of starting ammo
	};

	int step = 0;
	float player_height = 120.0f;
	Seat self;
	std::vector<Seat> others; // every other seat, in view order
};

// A scripted seat: turns what it sees into one PlayerInputData per logic
// step. Deterministic: the output depends only on the view sequence and the
// bot's BotRandom. Every output is connected, never opens a menu or the F1
// overlay. Debug-only.
//
// Deliberately a hierarchy (AGENTS.md, "deliberately OOP-heavy"): each
// behaviour refines the one it extends through a virtual hook.
class Bot
{
public:
	virtual ~Bot() = default;

	Bot(const Bot&) = delete;
	Bot& operator=(const Bot&) = delete;

	virtual PlayerInputData think(const BotSeatView& view) = 0;
	virtual std::string_view name() const = 0;

	// idle, walk, jump, chase, paint. `idle` is neutral connected input and
	// has no class; make returns nullptr for it.
	static const std::vector<std::string>& behaviours();
	static bool is_behaviour(std::string_view name);
	static std::unique_ptr<Bot> make(std::string_view behaviour, std::uint64_t seed, int seat);

	// The neutral input a seat with no bot (or an idle one) is given.
	static PlayerInputData neutral();

protected:
	explicit Bot(std::uint64_t seed);

	BotRandom random_;

	// Jump held this step: sets jump_held, and jump_pressed only on the step
	// the hold begins, as a real button does.
	void hold_jump(PlayerInputData& input, bool held);

	// Aim along `direction` (normalised here; zero aims nowhere).
	static void aim(PlayerInputData& input, const mattmath::Vector2F& direction);

private:
	bool jump_was_held_ = false;
};

// Walks one way, turns at a wall (barely moving for a while) or at random.
class WalkBot : public Bot
{
public:
	explicit WalkBot(std::uint64_t seed);

	PlayerInputData think(const BotSeatView& view) override;
	std::string_view name() const override { return "walk"; }

protected:
	// -1 or +1: which way to walk this step. The default keeps a heading.
	virtual float walk_direction(const BotSeatView& view);
	// Called with the walking input; derived bots add to it.
	virtual void decorate(PlayerInputData& input, const BotSeatView& view);

	// How many steps the bot has been trying to walk without moving.
	int blocked_steps() const { return this->blocked_steps_; }

private:
	float heading_ = 1.0f;
	int turn_in_ = 0;
	int blocked_steps_ = 0;
};

// Walks, and jumps when blocked or now and then, holding for a random
// number of steps (so the height varies).
class JumpBot : public WalkBot
{
public:
	explicit JumpBot(std::uint64_t seed);

	std::string_view name() const override { return "jump"; }

protected:
	void decorate(PlayerInputData& input, const BotSeatView& view) override;
	virtual bool wants_jump(const BotSeatView& view);

private:
	int jump_in_ = 0;
	int hold_left_ = 0;
};

// Walks towards the nearest living opponent, jumps when it is above, and
// shoots at it within range.
class ChaseBot : public JumpBot
{
public:
	explicit ChaseBot(std::uint64_t seed);

	std::string_view name() const override { return "chase"; }

protected:
	float walk_direction(const BotSeatView& view) override;
	bool wants_jump(const BotSeatView& view) override;
	void decorate(PlayerInputData& input, const BotSeatView& view) override;

	// The nearest living opponent, or null.
	static const BotSeatView::Seat* nearest_opponent(const BotSeatView& view);
};

// Roams like JumpBot and paints the floor ahead of it: the aim sweeps down
// and forward, it fires while it has ammo and waits for the tank to refill;
// an opponent close by is shot at instead.
class PaintBot : public JumpBot
{
public:
	explicit PaintBot(std::uint64_t seed);

	std::string_view name() const override { return "paint"; }

protected:
	void decorate(PlayerInputData& input, const BotSeatView& view) override;

private:
	float sweep_ = 0.6f; // radians below the horizontal
	bool refilling_ = false;
};
