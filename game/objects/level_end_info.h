#pragma once

#include "game/objects/team_colour.h"
#include "game/objects/player_team.h"

struct LevelEndInfo
{
	TeamColour team_colours = TeamColour();
	float team_a_area = 0.0f;
	float team_b_area = 0.0f;

	PlayerTeam winning_team() const;
	float team_a_ratio() const;
	float team_b_ratio() const;
	float team_a_percentage() const;
	float team_b_percentage() const;
	std::wstring team_a_percentage_string() const;
	std::wstring team_b_percentage_string() const;
	std::wstring winning_team_string() const;
};
