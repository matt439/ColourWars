#include "game/pch.h"
#include "game/objects/level_end_info.h"
using namespace artattack;

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
std::wstring LevelEndInfo::team_a_percentage_string() const
{
	std::wstring result = std::to_wstring(this->team_a_percentage());
	size_t decimal_point = result.find(L'.');
	if (decimal_point != std::wstring::npos)
	{
		result = result.substr(0, decimal_point + 2);
	}
	return result + L"%";
}
std::wstring LevelEndInfo::team_b_percentage_string() const
{
	std::wstring result = std::to_wstring(this->team_b_percentage());
	size_t decimal_point = result.find(L'.');
	if (decimal_point != std::wstring::npos)
	{
		result = result.substr(0, decimal_point + 2);
	}
	return result + L"%";
}
std::wstring LevelEndInfo::winning_team_string() const
{
	PlayerTeam winning = this->winning_team();
	if (winning == PlayerTeam::a)
	{
		return L"Team A Wins!";
	}
	else if (winning == PlayerTeam::b)
	{
		return L"Team B Wins!";
	}
	else if (winning == PlayerTeam::none)
	{
		return L"Draw!";
	}
	throw std::exception("No winning team");
}
