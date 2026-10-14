#pragma once

enum class WeaponType
{
    sprayer,
    sniper,
    roller,  // Fixed downward aim; fires a short-lived contact volume.
    mister,
    bucket,
    random_primary,
    grenade_std,
    none,
    max_prim_wep = random_primary
};

// Selection casts indices in [sprayer, random_primary) to concrete primaries.
// Tuning ends at bucket; menu descriptions include the Random entry as well.
// Keep these checks with the enum when adding or reordering primary weapons.
static_assert(static_cast<int>(WeaponType::sprayer) == 0);
static_assert(static_cast<int>(WeaponType::sniper) == static_cast<int>(WeaponType::sprayer) + 1);
static_assert(static_cast<int>(WeaponType::roller) == static_cast<int>(WeaponType::sniper) + 1);
static_assert(static_cast<int>(WeaponType::mister) == static_cast<int>(WeaponType::roller) + 1);
static_assert(static_cast<int>(WeaponType::bucket) == static_cast<int>(WeaponType::mister) + 1);
static_assert(static_cast<int>(WeaponType::random_primary) == static_cast<int>(WeaponType::bucket) + 1);
static_assert(WeaponType::max_prim_wep == WeaponType::random_primary);
