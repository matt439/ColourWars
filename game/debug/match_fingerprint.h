#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Level;

// A match's observable state at one update, for replay checks: every seat's
// position, velocity, health, ammo, respawn timer and move state as exact
// bit patterns, the paint areas, and the level state. Public state only:
// hidden state (air time, a jump buffer, projectiles in flight, which tiles
// are painted) shows up when it changes something here, a little later.
// Debug-only.
struct MatchFingerprint
{
	struct Seat
	{
		std::uint32_t x = 0;
		std::uint32_t y = 0;
		std::uint32_t vx = 0;
		std::uint32_t vy = 0;
		std::uint32_t health = 0;
		std::uint32_t ammo = 0;
		std::uint32_t respawn = 0;
		int move_state = 0;
		int alive = 0;
	};

	int updates = 0;
	std::uint64_t hash = 0;
	std::vector<Seat> seats;
	std::uint32_t team_a = 0;
	std::uint32_t team_b = 0;
	std::uint32_t total = 0;
	int level_state = 0;

	static MatchFingerprint of(const Level& level, int updates);

	// "" when equal; otherwise the first difference, field by field
	// ("P2 x 1532.5 != 1530.25").
	static std::string difference(const MatchFingerprint& expected,
		const MatchFingerprint& actual);

	// "A 1234.5 B 987.25 of 4000" for logs.
	std::string paint_text() const;
};

float float_from_bits(std::uint32_t bits);
std::uint32_t bits_from_float(float value);
