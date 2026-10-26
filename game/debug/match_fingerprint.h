#pragma once

#include <array>
#include <cstdint>
#include <optional>
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
        // Optional active-prototype state. Off records/hashes retain the legacy bytes.
        bool paint_underfoot = false;
        int support_paint_team = 0;
        int step_paint_team = 0;
	};

    // Unconditional value layout; the optional domain is populated only for
    // admitted movers, at the completed Level-update boundary. No object borrows.
    struct HorizontalTilePrint
    {
        int face = 0;
        int team = 0;
        int owner = -1;
        std::uint32_t area = 0;
        std::array<std::uint32_t, 8> corners{};
    };
    struct HorizontalStructurePrint
    {
        std::uint32_t id = 0;
        std::array<std::uint32_t, 7> spec{};
        std::uint64_t phase = 0;
        int direction = 1;
        std::array<std::uint32_t, 4> offsets{};
        std::array<std::uint32_t, 8> pose{};
        std::vector<HorizontalTilePrint> tiles;
    };
    struct HorizontalTerrainPrint
    {
        std::vector<HorizontalStructurePrint> structures;
        std::vector<std::uint32_t> carriers;
    };
    std::optional<HorizontalTerrainPrint> horizontal_motion;

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
