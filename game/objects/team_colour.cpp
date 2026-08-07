#include "game/pch.h"
#include "game/objects/team_colour.h"

using namespace mattmath;
using namespace team_colour_consts;

TeamColour::TeamColour(const Colour& team_a, const Colour& team_b)
	: team_a(team_a), team_b(team_b)
{

}

const Colour& TeamColour::team_colour(PlayerTeam team) const
{
	switch (team)
	{
	case PlayerTeam::a:
		return this->team_a;
	case PlayerTeam::b:
		return this->team_b;
	default:
		throw std::exception("Invalid team.");
	}
}

TeamColour TeamColourTools::generate_random_team_colour()
{
	int enum_max = static_cast<int>(TeamColourPair::max);
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

TeamColour TeamColourTools::team_colours(TeamColourPair team_colour)
{
	switch (team_colour)
	{
	case TeamColourPair::blue_orange:
		return BLUE_ORANGE;
	case TeamColourPair::pink_green:
		return PINK_GREEN;
	case TeamColourPair::blue_yellow:
		return BLUE_YELLOW;
	default:
		return BLUE_ORANGE;
	}
}

Colour TeamColourTools::team_colour(TeamColour team_colours,
	PlayerTeam team)
{
	switch (team)
	{
	case PlayerTeam::a:
		return team_colours.team_a;
	case PlayerTeam::b:
		return team_colours.team_b;
	default:
		return team_colours.team_a;
	}
}