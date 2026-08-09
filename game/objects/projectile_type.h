#pragma once

// What a weapon fires. Its own header, like WeaponType, so that a file naming
// the enum does not have to include Projectile - weapon_details.h did, and
// Projectile includes the weapon tuning that names it back.
enum projectile_type
{
	SPRAY,
	JET,
	ROLLING,
	MIST,
	BALL
};
