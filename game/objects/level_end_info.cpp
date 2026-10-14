#include "game/pch.h"
#include "game/objects/level_end_info.h"
using namespace labrador;

namespace
{
	// Format percentages with one decimal place.
	std::wstring percentage_string(float percentage)
	{
		std::wstring result = std::to_wstring(percentage);
		size_t decimal_point = result.find(L'.');
		if (decimal_point != std::wstring::npos)
		{
			result = result.substr(0, decimal_point + 2);
		}
		return result + L"%";
	}
}

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
	return PlayerTeam::none;
}

bool LevelEndInfo::one_team_only() const
{
	return this->team_a_played != this->team_b_played;
}

float LevelEndInfo::coverage_ratio() const
{
	if (this->total_area <= 0.0f)
	{
		return 0.0f;
	}
	return (this->team_a_area + this->team_b_area) / this->total_area;
}

float LevelEndInfo::team_a_ratio() const
{
	// With one team, show arena coverage; a share of painted area alone
	// would be 100% regardless of how little was painted.
	if (this->one_team_only())
	{
		return this->team_a_played ? this->coverage_ratio() : 0.0f;
	}

	if (this->team_a_area == 0.0f && this->team_b_area == 0.0f)
	{
		return 0.5f;
	}
	return this->team_a_area / (this->team_a_area + this->team_b_area);
}
float LevelEndInfo::team_b_ratio() const
{
	if (this->one_team_only())
	{
		return this->team_b_played ? this->coverage_ratio() : 0.0f;
	}

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
// Label each team explicitly rather than relying on bar position or colour.
std::wstring LevelEndInfo::team_a_percentage_string() const
{
	return L"A " + percentage_string(this->team_a_percentage());
}
std::wstring LevelEndInfo::team_b_percentage_string() const
{
	return L"B " + percentage_string(this->team_b_percentage());
}
std::wstring LevelEndInfo::winning_team_string() const
{
	// A single-team match reports coverage without declaring a winner.
	if (this->one_team_only())
	{
		return percentage_string(100.0f * this->coverage_ratio()) + L" Painted";
	}

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

std::wstring LevelEndInfo::player_percentage_string(int slot) const
{
	// Invalid or unattributed owners have no per-player credit.
	if (slot < 0 || static_cast<size_t>(slot) >= this->player_areas.size())
	{
		return percentage_string(0.0f);
	}
	return percentage_string(this->total_area > 0.0f ?
		100.0f * this->player_areas.at(static_cast<size_t>(slot)) / this->total_area : 0.0f);
}
