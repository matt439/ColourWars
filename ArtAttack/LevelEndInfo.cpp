#include "pch.h"
#include "LevelEndInfo.h"

player_team LevelEndInfo::winning_team() const
{
	if (this->team_a_area > this->team_b_area)
	{
		return player_team::A;
	}
	else if (this->team_b_area > this->team_a_area)
	{
		return player_team::B;
	}
	// draw
	return player_team::NONE;
}

float LevelEndInfo::team_a_ratio() const
{
	if (this->team_a_area == 0.0f && this->team_b_area == 0.0f)
	{
		return 0.5f;
	}
	return this->team_a_area / (this->team_a_area + this->team_b_area);
}
float LevelEndInfo::team_b_ratio() const
{
	if (this->team_a_area == 0.0f && this->team_b_area == 0.0f)
	{
		return 0.5f;
	}
	return this->team_b_area / (this->team_a_area + this->team_b_area);
}
float LevelEndInfo::team_a_percentage() const
{
	return 100.0f * this->team_a_ratio();
}
float LevelEndInfo::team_b_percentage() const
{
	return 100.0f * this->team_b_ratio();
}
std::string LevelEndInfo::team_a_percentage_string() const
{
	std::string result = std::to_string(this->team_a_percentage());
	size_t decimal_point = result.find('.');
	if (decimal_point != std::string::npos)
	{
		result = result.substr(0, decimal_point + 2);
	}
	return result + "%";
}
std::string LevelEndInfo::team_b_percentage_string() const
{
	std::string result = std::to_string(this->team_b_percentage());
	size_t decimal_point = result.find('.');
	if (decimal_point != std::string::npos)
	{
		result = result.substr(0, decimal_point + 2);
	}
	return result + "%";
}
std::string LevelEndInfo::winning_team_string() const
{
	player_team winning = this->winning_team();
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
float LevelEndInfo::winning_score() const
{
	player_team winning = this->winning_team();
	if (winning == player_team::A)
	{
		return this->team_a_area;
	}
	if (winning == player_team::B)
	{
		return this->team_b_area;
	}
	if (winning == player_team::NONE)
	{
		return this->team_a_area;
	}
	throw std::exception("No winning team");
}
float LevelEndInfo::losing_score() const
{
	player_team winning = this->winning_team();
	if (winning == player_team::A)
	{
		return this->team_b_area;
	}
	if (winning == player_team::B)
	{
		return this->team_a_area;
	}
	if (winning == player_team::NONE)
	{
		return this->team_a_area;
	}
	throw std::exception("No winning team");
}
float LevelEndInfo::winning_ratio() const
{
	return this->winning_score() / this->losing_score();
}
float LevelEndInfo::losing_ratio() const
{
	return this->losing_score() / this->winning_score();
}
float LevelEndInfo::winning_percentage() const
{
	return 100.0f * this->winning_ratio();
}
float LevelEndInfo::losing_percentage() const
{
	return 100.0f - this->winning_percentage();
}