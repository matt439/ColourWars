#pragma once

#include "game/objects/projectile_planted.h"

// A sub's paint wall (ROAD#11): set down in front of the player, it falls
// until it stands on the first cover it meets (ProjectilePlanted), an
// upright barrier on its body record's collision size. Standing, it spends
// every enemy shot that reaches it, so it stops them, and each interval it
// drips a row of drops straight down from its foot, which paint the floor
// beneath it.
class ProjectileWall final : public ProjectilePlanted
{
public:
	using ProjectilePlanted::ProjectilePlanted;

	SubType sub_type() const override { return SubType::wall; }

	// Swept sideways from its owner at its own height, so cover between the
	// two stops it there, and a tall wall is not swept through their floor.
	labrador::Vector2F launch_origin(const labrador::Vector2F& owner_centre) const override;

protected:
	// burst_count drops spaced evenly across its foot, each falling straight
	// down at burst_speed.
	std::vector<std::unique_ptr<Projectile>> spray(int release) const override;
};
