#include "game/pch.h"
#include "game/objects/shot_retirements.h"
#include "game/objects/projectile_grenade.h"
#include "game/objects/projectile_planted.h"

ShotRetirementCounts& ShotRetirementCounts::operator+=(const ShotRetirementCounts& other)
{
	this->cover += other.cover;
	this->player += other.player;
	this->breakable += other.breakable;
	this->expired += other.expired;
	this->out_of_bounds += other.out_of_bounds;
	return *this;
}

void ShotRetirements::count(const Projectile& projectile, bool outside)
{
	// A sprinkler or paint wall is a ball by kind but no round shot: its body
	// keeps the box in both shapes, and planted it retires by expiring or
	// breaking, which would read as balls running out of time.
	if (projectile.projectile_type() != ProjectileType::ball ||
		dynamic_cast<const ProjectilePlanted*>(&projectile) != nullptr) { return; }
	ShotRetirementCounts& counts = dynamic_cast<const ProjectileGrenade*>(&projectile) != nullptr ?
		this->grenade : this->ball;
	if (outside) { ++counts.out_of_bounds; return; }
	switch (projectile.retire_cause())
	{
	case ShotRetireCause::cover: ++counts.cover; break;
	case ShotRetireCause::player: ++counts.player; break;
	case ShotRetireCause::breakable: ++counts.breakable; break;
	case ShotRetireCause::none: ++counts.expired; break;
	}
}

ShotRetirements& ShotRetirements::operator+=(const ShotRetirements& other)
{
	this->ball += other.ball;
	this->grenade += other.grenade;
	return *this;
}
