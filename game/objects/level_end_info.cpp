#include "game/pch.h"
#include "game/objects/level_end_info.h"

PlayerTeam LevelEndInfo::winning_team() const
{
	if (this->team_a_area > this->team_b_area)
	{
		return PlayerTeam::a;
	}
	else if (this->team_b_area > this->team_a_area)
	{
		return PlayerTeam::b;
	}
	// draw
	return PlayerTeam::none;
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
	PlayerTeam winning = this->winning_team();
	if (winning == PlayerTeam::a)
	{
		return "Team A Wins!";
	}
	else if (winning == PlayerTeam::b)
	{
		return "Team B Wins!";
	}
	else if (winning == PlayerTeam::none)
	{
		return "Draw!";
	}
	throw std::exception("No winning team");
}
float LevelEndInfo::winning_score() const
{
	PlayerTeam winning = this->winning_team();
	if (winning == PlayerTeam::a)
	{
		return this->team_a_area;
	}
	if (winning == PlayerTeam::b)
	{
		return this->team_b_area;
	}
	if (winning == PlayerTeam::none)
	{
		return this->team_a_area;
	}
	throw std::exception("No winning team");
}
float LevelEndInfo::losing_score() const
{
	PlayerTeam winning = this->winning_team();
	if (winning == PlayerTeam::a)
	{
		return this->team_b_area;
	}
	if (winning == PlayerTeam::b)
	{
		return this->team_a_area;
	}
	if (winning == PlayerTeam::none)
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