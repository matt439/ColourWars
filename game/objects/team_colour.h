#pragma once

#include "game/objects/player_team.h"
#include "engine/render/colour.h"

// A default value is neutral; match builders always supply the authored pair.
struct TeamColour
{
	labrador::Colour team_a = labrador::Colour::gray;
	labrador::Colour team_b = labrador::Colour::gray;

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
	// Added by ROAD#8; each passes the colour-vision check in team_palette.
	purple_gold,
	red_azure,
	magenta_forest,
	// Count of fixed pairs; keep new pairs before this sentinel.
	count
};

struct TeamPalettePresentation;
class MatchRandom;

class TeamColourTools
{
public:
	TeamColourTools() = default;

	// One pair from the round's palette stream (HYG#39).
	static TeamColour generate_random_team_colour(const TeamPalettePresentation& palette,
		const MatchRandom& random);

	// A fixed pair's colours. Random has none until a round draws it, so it
	// is refused here; a match passes its MatchRandom to the form below.
	static TeamColour team_colours(TeamColourPair team_colour, const TeamPalettePresentation& palette);
	// A match's colours: the fixed pair, or Random drawn from `random`.
	static TeamColour team_colours(TeamColourPair team_colour, const TeamPalettePresentation& palette,
		const MatchRandom& random);
	static const char* palette_key(TeamColourPair palette);
};
