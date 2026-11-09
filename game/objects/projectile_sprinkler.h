#pragma once

#include "game/objects/projectile_planted.h"

// A sub's sprinkler (ROAD#11): thrown like the grenade, it sticks to the
// first cover it meets (ProjectilePlanted) and from there sprays a fan of
// drops over its top half every interval.
class ProjectileSprinkler final : public ProjectilePlanted
{
public:
	using ProjectilePlanted::ProjectilePlanted;

protected:
	// burst_count drops spread evenly from straight left to straight right
	// across the top, and every other release turned so its drops fill the
	// gaps between the last one's.
	std::vector<std::unique_ptr<Projectile>> spray(int release) const override;
};
