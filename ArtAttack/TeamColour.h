#ifndef TEAMCOLOUR_H
#define TEAMCOLOUR_H

#include "player_team.h"
#include "colour.h"

struct TeamColour
{
	MattMath::Colour team_a = colour_consts::TEAM_BLUE;
	MattMath::Colour team_b = colour_consts::TEAM_YELLOW;

	TeamColour() = default;
	TeamColour(const MattMath::Colour& team_a, const MattMath::Colour& team_b);
	const MattMath::Colour& get_team_colour(player_team team) const;
};

enum class team_colour_enum
{
	BLUE_ORANGE,
	PINK_GREEN,
	BLUE_YELLOW,
	MAX = BLUE_YELLOW
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

	static TeamColour get_team_colours(team_colour_enum team_colour);

	static MattMath::Colour get_team_colour(TeamColour team_colours,
		player_team team);
};

#endif // !TEAM_COLOURS_H
