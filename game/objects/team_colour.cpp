#include "game/pch.h"
#include "game/objects/team_colour.h"

using namespace team_colour_consts;
using namespace labrador;

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
	int enum_count = static_cast<int>(TeamColourPair::count);
	int random = rand() % enum_count;
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
	case TeamColourPair::random:
		return generate_random_team_colour();
	case TeamColourPair::blue_orange:
		return BLUE_ORANGE;
	case TeamColourPair::pink_green:
		return PINK_GREEN;
	case TeamColourPair::blue_yellow:
		return BLUE_YELLOW;
	default:
		throw std::invalid_argument("Invalid team palette");
	}
}

const char* TeamColourTools::palette_key(TeamColourPair palette)
{
	switch (palette)
	{
	case TeamColourPair::random: return "random";
	case TeamColourPair::blue_orange: return "blue_orange";
	case TeamColourPair::pink_green: return "pink_green";
	case TeamColourPair::blue_yellow: return "blue_yellow";
	default: throw std::invalid_argument("Invalid team palette");
	}
}
