#include "pch.h"
#include "TeamColour.h"

using namespace MattMath;
using namespace team_colour_consts;

TeamColour::TeamColour(const Colour& team_a, const Colour& team_b)
	: team_a(team_a), team_b(team_b)
{

}

const Colour& TeamColour::get_team_colour(player_team team) const
{
	switch (team)
	{
	case player_team::A:
		return this->team_a;
	case player_team::B:
		return this->team_b;
	default:
		throw std::exception("Invalid team.");
	}
}

TeamColour TeamColourTools::generate_random_team_colour()
{
	int enum_max = static_cast<int>(team_colour_enum::MAX);
	int random = rand() % enum_max;
	switch (random)
	{
	case 0:
		return BLUE_ORANGE;
	case 1:
		return PINK_GREEN;
	case 2:
		return BLUE_YELLOW;
	default:
		return BLUE_ORANGE;
	}
}

TeamColour TeamColourTools::get_team_colours(team_colour_enum team_colour)
{
	switch (team_colour)
	{
	case team_colour_enum::BLUE_ORANGE:
		return BLUE_ORANGE;
	case team_colour_enum::PINK_GREEN:
		return PINK_GREEN;
	case team_colour_enum::BLUE_YELLOW:
		return BLUE_YELLOW;
	default:
		return BLUE_ORANGE;
	}
}

Colour TeamColourTools::get_team_colour(TeamColour team_colours,
	player_team team)
{
	switch (team)
	{
	case player_team::A:
		return team_colours.team_a;
	case player_team::B:
		return team_colours.team_b;
	default:
		return team_colours.team_a;
	}
}