#include "game/pch.h"
#include "game/objects/team_colour.h"

#include "game/presentation_match.h"
#include "game/objects/match_random.h"
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

TeamColour TeamColourTools::generate_random_team_colour(const TeamPalettePresentation& palette,
	const MatchRandom& random)
{
	if (palette.pairs.empty()) { throw std::invalid_argument("No team palette pairs to draw"); }
	BotRandom draw = random.palette();
	// Reduced in 64 bits before narrowing, so a 32-bit size_t draws the same.
	const std::uint64_t pairs = static_cast<std::uint64_t>(palette.pairs.size());
	return palette.random_pair(static_cast<size_t>(draw.next() % pairs)).colours;
}

TeamColour TeamColourTools::team_colours(TeamColourPair team_colour,
	const TeamPalettePresentation& palette)
{
	if (team_colour == TeamColourPair::random)
	{
		throw std::invalid_argument("The Random team palette is drawn from a round's MatchRandom");
	}
	return palette.fixed_pair(team_colour).colours;
}

TeamColour TeamColourTools::team_colours(TeamColourPair team_colour,
	const TeamPalettePresentation& palette, const MatchRandom& random)
{
	if (team_colour == TeamColourPair::random)
	{
		return generate_random_team_colour(palette, random);
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
	case TeamColourPair::purple_gold: return "purple_gold";
	case TeamColourPair::red_azure: return "red_azure";
	case TeamColourPair::magenta_forest: return "magenta_forest";
	default: throw std::invalid_argument("Invalid team palette");
	}
}
