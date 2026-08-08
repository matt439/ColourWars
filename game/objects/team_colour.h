#pragma once

#include "game/objects/player_team.h"
#include "engine/render/colour.h"

namespace team_colour_consts
{
	// The five team colours. They were colour_consts::TEAM_* in the engine's
	// palette, sitting among the CSS names in engine/math/colour.h - a game
	// noun inside a library that is not allowed to know this game exists, and
	// nothing in the engine ever read them. A paint match's team colours are
	// as game-specific as a paint match, so they live in the game.
	constexpr artattack::Colour TEAM_BLUE(6, 52, 204, 255);
	constexpr artattack::Colour TEAM_YELLOW(255, 255, 0, 255);
	constexpr artattack::Colour TEAM_ORANGE(255, 127, 0, 255);
	constexpr artattack::Colour TEAM_PINK(255, 0, 127, 255);
	constexpr artattack::Colour TEAM_GREEN(0, 255, 0, 255);
}

struct TeamColour
{
	artattack::Colour team_a = team_colour_consts::TEAM_BLUE;
	artattack::Colour team_b = team_colour_consts::TEAM_YELLOW;

	TeamColour() = default;
	TeamColour(const artattack::Colour& team_a, const artattack::Colour& team_b);
	const artattack::Colour& team_colour(PlayerTeam team) const;
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

	static artattack::Colour team_colour(TeamColour team_colours,
		PlayerTeam team);
};
