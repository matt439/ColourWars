#pragma once

#include "game/objects/player_team.h"
#include "engine/math/colour.h"

struct TeamColour
{
	mattmath::Colour team_a = colour_consts::TEAM_BLUE;
	mattmath::Colour team_b = colour_consts::TEAM_YELLOW;

	TeamColour() = default;
	TeamColour(const mattmath::Colour& team_a, const mattmath::Colour& team_b);
	const mattmath::Colour& get_team_colour(PlayerTeam team) const;
};

enum class TeamColourPair
{
	blue_orange,
	pink_green,
	blue_yellow,
	max = blue_yellow
};

namespace team_colour_consts
{
	const TeamColour BLUE_ORANGE =
	{
		colour_consts::TEAM_BLUE,
		colour_consts::TEAM_ORANGE
	};
	const TeamColour PINK_GREEN =
	{
		colour_consts::TEAM_PINK,
		colour_consts::TEAM_GREEN
	};
	const TeamColour BLUE_YELLOW =
	{
		colour_consts::TEAM_BLUE,
		colour_consts::TEAM_YELLOW
	};
}

class TeamColourTools
{
public:
	TeamColourTools() = default;

	static TeamColour generate_random_team_colour();

	static TeamColour get_team_colours(TeamColourPair team_colour);

	static mattmath::Colour get_team_colour(TeamColour team_colours,
		PlayerTeam team);
};
