#pragma once

#include <cstdint>

class Projectile;

// ROAD#44's contact tally: how round shots (every ProjectileType::ball but
// the planted subs, the sprinkler and the paint wall) left play. A grenade body counts apart from the balls, which are the bucket's
// shots and every sub's drops. Read only; nothing in a match reads it back.
struct ShotRetirementCounts
{
	std::uint64_t cover = 0;
	std::uint64_t player = 0;
	std::uint64_t breakable = 0;
	// Retired with no contact: it ran out its time.
	std::uint64_t expired = 0;
	std::uint64_t out_of_bounds = 0;
	std::uint64_t total() const { return cover + player + breakable + expired + out_of_bounds; }
	ShotRetirementCounts& operator+=(const ShotRetirementCounts& other);
};

struct ShotRetirements
{
	ShotRetirementCounts ball;
	ShotRetirementCounts grenade;
	// Counts projectile if it is round; outside is that it left the scene
	// still live, which wins over any cause.
	void count(const Projectile& projectile, bool outside);
	ShotRetirements& operator+=(const ShotRetirements& other);
};
