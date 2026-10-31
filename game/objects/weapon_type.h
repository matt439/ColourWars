#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

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

// The one spelling of each selectable weapon: the key in tuning.json and
// presentation.json, the save's last_match and the Debug match plan. Indexed
// by WeaponType from sprayer to random_primary, which is "random" (menu
// descriptions, the weapon picker and the match plan; tuning has no entry).
inline constexpr size_t TUNED_WEAPON_COUNT = static_cast<size_t>(WeaponType::random_primary);
inline constexpr std::array<const char*, TUNED_WEAPON_COUNT + 1> WEAPON_NAMES{
    "sprayer", "sniper", "roller", "mister", "bucket", "random"
};

// The name of a tuned weapon or the Random choice; nullptr for the rest.
constexpr const char* weapon_name(WeaponType weapon)
{
    const size_t index = static_cast<size_t>(weapon);
    return index < WEAPON_NAMES.size() ? WEAPON_NAMES[index] : nullptr;
}

// The weapon a name spells, including "random"; empty for any other name.
constexpr std::optional<WeaponType> weapon_from_name(std::string_view name)
{
    for (size_t index = 0; index < WEAPON_NAMES.size(); ++index)
    {
        if (name == WEAPON_NAMES[index]) { return static_cast<WeaponType>(index); }
    }
    return std::nullopt;
}

// Calls visit(type, name) for each tuned weapon, sprayer to bucket, in enum
// order, so readers and validators meet the weapons in one order.
template <typename Visit>
constexpr void for_each_tuned_weapon(Visit&& visit)
{
    for (size_t index = 0; index < TUNED_WEAPON_COUNT; ++index)
    {
        visit(static_cast<WeaponType>(index), WEAPON_NAMES[index]);
    }
}
