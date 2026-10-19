#pragma once

#include "game/paint_owners.h"
#include <array>

#include "game/objects/team_colour.h"
#include "game/objects/player_team.h"

struct MenuResultsText;

// A percentage as the results show it: one decimal place, truncated, then "%".
std::wstring percentage_text(float percentage);
// The number percentage_text shows, in tenths (68.4% is 684), so a comparison
// agrees with what a player can read.
int percentage_tenths(float percentage);

struct LevelEndInfo
{
	TeamColour team_colours = TeamColour();
	float team_a_area = 0.0f;
	float team_b_area = 0.0f;
	// Surface still held at the finish, indexed by paint owner, not seat.
	std::array<float, paint_owners::count> player_areas{};
	std::wstring player_percentage_string(int slot) const;

	// Total paintable area, including unpainted tiles, for the coverage score.
	float total_area = 0.0f;

	// Whether each team has a player in the match.
	bool team_a_played = true;
	bool team_b_played = true;

	PlayerTeam winning_team() const;
	float team_a_ratio() const;
	float team_b_ratio() const;
	float team_a_percentage() const;
	float team_b_percentage() const;
	std::wstring team_a_percentage_string(const MenuResultsText& text) const;
	std::wstring team_b_percentage_string(const MenuResultsText& text) const;
	std::wstring winning_team_string(const MenuResultsText& text) const;

	// Whether only one team participated; results then show coverage instead
	// of a competitive paint share.
	bool one_team_only() const;

	// Fraction of the arena painted by either team.
	float coverage_ratio() const;
};
