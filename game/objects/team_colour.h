#pragma once

#include "game/objects/player_team.h"
#include "engine/render/colour.h"

namespace team_colour_consts
{
	// The game's team-colour palette.
	constexpr labrador::Colour TEAM_BLUE(6, 52, 204, 255);
	constexpr labrador::Colour TEAM_YELLOW(255, 255, 0, 255);
	constexpr labrador::Colour TEAM_ORANGE(255, 127, 0, 255);
	constexpr labrador::Colour TEAM_PINK(255, 0, 127, 255);
	constexpr labrador::Colour TEAM_GREEN(0, 255, 0, 255);
}

struct TeamColour
{
	labrador::Colour team_a = team_colour_consts::TEAM_BLUE;
	labrador::Colour team_b = team_colour_consts::TEAM_YELLOW;

	TeamColour() = default;
	TeamColour(const labrador::Colour& team_a, const labrador::Colour& team_b);
	const labrador::Colour& team_colour(PlayerTeam team) const;
};

enum class TeamColourPair
{
	// Random is a preference, never one of the fixed pairs in the draw.
	random = -1,
	blue_orange,
	pink_green,
	blue_yellow,
	// Count of fixed pairs; keep new pairs before this sentinel.
	count
};

namespace team_colour_consts
{
	const TeamColour BLUE_ORANGE = { TEAM_BLUE, TEAM_ORANGE };
	const TeamColour PINK_GREEN = { TEAM_PINK, TEAM_GREEN };
	const TeamColour BLUE_YELLOW = { TEAM_BLUE, TEAM_YELLOW };
}

class TeamColourTools
{
public:
	TeamColourTools() = default;

	static TeamColour generate_random_team_colour();

	static TeamColour team_colours(TeamColourPair team_colour);
	static const char* palette_key(TeamColourPair palette);
};
