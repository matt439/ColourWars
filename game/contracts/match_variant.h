#pragma once

#include <array>
#include <stdexcept>
#include <string_view>

// Named match options preserve timed territory scoring. Values and behaviour
// belong to each option's C++ policy, never a free-form tuning override bag.
enum class MatchVariant
{
	standard,
	endless_paint
};

constexpr std::array<MatchVariant, 2> match_variants{
	MatchVariant::standard, MatchVariant::endless_paint };

constexpr bool valid_match_variant(MatchVariant variant)
{
	return variant == MatchVariant::standard || variant == MatchVariant::endless_paint;
}

constexpr const char* match_variant_key(MatchVariant variant)
{
	switch (variant)
	{
	case MatchVariant::standard: return "standard";
	case MatchVariant::endless_paint: return "endless_paint";
	}
	throw std::invalid_argument("Unknown match variant");
}

// Unknown spellings leave the caller's current value intact.
constexpr bool read_match_variant(std::string_view key, MatchVariant& variant)
{
	for (const MatchVariant known : match_variants)
	{
		if (key == match_variant_key(known)) { variant = known; return true; }
	}
	return false;
}
