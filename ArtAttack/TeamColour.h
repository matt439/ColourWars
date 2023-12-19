#ifndef TEAMCOLOUR_H
#define TEAMCOLOUR_H
//#include <glm/glm.hpp>
//#include "colour.h"
#include "player_team.h"
#include "colour.h"

struct team_colour
{
	MattMath::Colour team_a = colour_consts::TEAM_BLUE;
	MattMath::Colour team_b = colour_consts::TEAM_YELLOW;
	
	inline const MattMath::Colour& get_team_colour(player_team team)
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
	const team_colour BLUE_ORANGE =
	{
		colour_consts::TEAM_BLUE,
		colour_consts::TEAM_ORANGE
	};
	const team_colour PINK_GREEN =
	{
		colour_consts::TEAM_PINK,
		colour_consts::TEAM_GREEN
	};
	const team_colour BLUE_YELLOW =
	{
		colour_consts::TEAM_BLUE,
		colour_consts::TEAM_YELLOW
	};
}

class TeamColour
{
public:
	TeamColour() {}
	inline team_colour generate_random_team_colour()
	{
		int enum_max = static_cast<int>(team_colour_enum::MAX);
		int random = rand() % enum_max;
		switch (random)
		{
		case 0:
			return team_colour_consts::BLUE_ORANGE;
		case 1:
			return team_colour_consts::PINK_GREEN;
		case 2:
			return team_colour_consts::BLUE_YELLOW;
		default:
			return team_colour_consts::BLUE_ORANGE;
		}
	}
	inline team_colour get_team_colours(team_colour_enum team_colour)
	{
		switch (team_colour)
		{
		case team_colour_enum::BLUE_ORANGE:
			return team_colour_consts::BLUE_ORANGE;
		case team_colour_enum::PINK_GREEN:
			return team_colour_consts::PINK_GREEN;
		case team_colour_enum::BLUE_YELLOW:
			return team_colour_consts::BLUE_YELLOW;
		default:
			return team_colour_consts::BLUE_ORANGE;
		}
	}
	inline MattMath::Colour get_team_colour(team_colour team_colours,
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
};

#endif // !TEAM_COLOURS_H
