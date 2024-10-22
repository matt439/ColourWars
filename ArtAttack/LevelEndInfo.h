#ifndef LEVEL_END_INFO_H
#define LEVEL_END_INFO_H

#include "TeamColour.h"
#include "player_team.h"

struct LevelEndInfo
{
	team_colour team_colours = team_colour();
	float team_a_area = 0.0f;
	float team_b_area = 0.0f;

	player_team winning_team() const
	{
		if (team_a_area > team_b_area)
		{
			return player_team::A;
		}
		else if (team_b_area > team_a_area)
		{
			return player_team::B;
		}
		// draw
		return player_team::NONE;
	}
	float team_a_ratio() const
	{
		if (team_a_area == 0.0f && team_b_area == 0.0f)
		{
			return 0.5f;
		}
		return team_a_area / (team_a_area + team_b_area);
	}
	float team_b_ratio() const
	{
		if (team_a_area == 0.0f && team_b_area == 0.0f)
		{
			return 0.5f;
		}
		return team_b_area / (team_a_area + team_b_area);
	}
	float team_a_percentage() const
	{
		return 100.0f * team_a_ratio();
	}
	float team_b_percentage() const
	{
		return 100.0f * team_b_ratio();
	}
	std::string team_a_percentage_string() const
	{
		std::string result = std::to_string(team_a_percentage());
		size_t decimal_point = result.find('.');
		if (decimal_point != std::string::npos)
		{
			result = result.substr(0, decimal_point + 2);
		}
		return result + "%";
	}
	std::string team_b_percentage_string() const
	{
		std::string result = std::to_string(team_b_percentage());
		size_t decimal_point = result.find('.');
		if (decimal_point != std::string::npos)
		{
			result = result.substr(0, decimal_point + 2);
		}
		return result + "%";
	}
	std::string winning_team_string() const
	{
		player_team winning = winning_team();
		if (winning == player_team::A)
		{
			return "Team A Wins!";
		}
		else if (winning == player_team::B)
		{
			return "Team B Wins!";
		}
		else if (winning == player_team::NONE)
		{
			return "Draw!";
		}
		throw std::exception("No winning team");
	}
	float winning_score() const
	{
		player_team winning = winning_team();
		if (winning == player_team::A)
		{
			return team_a_area;
		}
		if (winning == player_team::B)
		{
			return team_b_area;
		}
		if (winning == player_team::NONE)
		{
			return team_a_area;
		}
		throw std::exception("No winning team");
	}
	float losing_score() const
	{
		player_team winning = winning_team();
		if (winning == player_team::A)
		{
			return team_b_area;
		}
		if (winning == player_team::B)
		{
			return team_a_area;
		}
		if (winning == player_team::NONE)
		{
			return team_a_area;
		}
		throw std::exception("No winning team");
	}
	float winning_ratio() const
	{
		return this->winning_score() / this->losing_score();
	}
	float losing_ratio() const
	{
		return this->losing_score() / this->winning_score();
	}
	float winning_percentage() const
	{
		return 100.0f * this->winning_ratio();
	}
	float losing_percentage() const
	{
		return 100.0f - this->winning_percentage();
	}
};

#endif // !LEVEL_END_INFO_H
