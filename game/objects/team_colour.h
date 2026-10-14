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
	// Count of fixed pairs; keep new pairs before this sentinel.
	count
};

struct TeamPalettePresentation;

class TeamColourTools
{
public:
	TeamColourTools() = default;

	static TeamColour generate_random_team_colour(const TeamPalettePresentation& palette);

	static TeamColour team_colours(TeamColourPair team_colour, const TeamPalettePresentation& palette);
	static const char* palette_key(TeamColourPair palette);
};
