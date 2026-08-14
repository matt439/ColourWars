#include "game/pch.h"
#include "game/objects/level_end_info.h"
using namespace artattack;

namespace
{
	// to_wstring on a float gives six decimal places, and the screen wants one.
	// This was written out twice for the two teams; the one-team headline would
	// have made it three.
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
	// draw
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
	// The bar is a share of the paint on the ground, and with nobody on the
	// other team that share is 1.0 whatever happened: a player who painted one
	// tile in four minutes fills it exactly as far as one who painted the
	// arena. So the one-team bar shows coverage instead, which is the thing
	// that was actually being done, and the two-team bar is untouched.
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
std::wstring LevelEndInfo::team_a_percentage_string() const
{
	return percentage_string(this->team_a_percentage());
}
std::wstring LevelEndInfo::team_b_percentage_string() const
{
	return percentage_string(this->team_b_percentage());
}
std::wstring LevelEndInfo::winning_team_string() const
{
	// Nobody to beat, so nothing to win. Announcing "Team A Wins!" over an
	// empty arena is the game's least true sentence, and it is the one a
	// player is most likely to see first: the one-player row is the only
	// thing a stranger with one pad can reach.
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
