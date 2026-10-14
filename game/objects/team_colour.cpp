#include "game/pch.h"
#include "game/objects/team_colour.h"

#include "game/presentation.h"
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

TeamColour TeamColourTools::generate_random_team_colour(const TeamPalettePresentation& palette)
{
	return palette.random_pair(static_cast<size_t>(rand())).colours;
}

TeamColour TeamColourTools::team_colours(TeamColourPair team_colour,
	const TeamPalettePresentation& palette)
{
	if (team_colour == TeamColourPair::random)
	{
		return generate_random_team_colour(palette);
	}
	return palette.fixed_pair(team_colour).colours;
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
