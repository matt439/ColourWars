#ifndef LEVEL_END_INFO_H
#define LEVEL_END_INFO_H

#include "TeamColour.h"
#include "player_team.h"

struct LevelEndInfo
{
	TeamColour team_colours = TeamColour();
	float team_a_area = 0.0f;
	float team_b_area = 0.0f;

	player_team winning_team() const;
	float team_a_ratio() const;
	float team_b_ratio() const;
	float team_a_percentage() const;
	float team_b_percentage() const;
	std::string team_a_percentage_string() const;
	std::string team_b_percentage_string() const;
	std::string winning_team_string() const;
	float winning_score() const;
	float losing_score() const;
	float winning_ratio() const;
	float losing_ratio() const;
	float winning_percentage() const;
	float losing_percentage() const;
};

#endif // !LEVEL_END_INFO_H
