#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

// The sub weapons (ROAD#11): a second, paint-hungry tool every player carries
// beside the one they chose, fired with its own button. A separate list from
// WeaponType, so the five tools and the subs never stand in for each other.
// docs/feature-roadmap.md#sub-weapons-road11 plans the sprinkler and the
// paint wall; each joins this enum before count.
enum class SubType
{
	grenade,
	count
};

inline constexpr size_t sub_count = static_cast<size_t>(SubType::count);

// The one spelling of each sub: its key under "subs" in tuning.json.
inline constexpr std::array<const char*, sub_count> sub_names{ "grenade" };

constexpr const char* sub_name(SubType sub)
{
	const size_t index = static_cast<size_t>(sub);
	return index < sub_names.size() ? sub_names[index] : nullptr;
}

// The sub a name spells; empty for any other name.
constexpr std::optional<SubType> sub_from_name(std::string_view name)
{
	for (size_t index = 0; index < sub_names.size(); ++index)
	{
		if (name == sub_names[index]) { return static_cast<SubType>(index); }
	}
	return std::nullopt;
}

// Calls visit(type, name) for each sub, in enum order.
template <typename Visit>
constexpr void for_each_sub(Visit&& visit)
{
	for (size_t index = 0; index < sub_names.size(); ++index)
	{
		visit(static_cast<SubType>(index), sub_names[index]);
	}
}
