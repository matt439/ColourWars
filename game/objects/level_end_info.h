#pragma once

#include "game/input_slots.h"
#include <array>

#include "game/objects/team_colour.h"
#include "game/objects/player_team.h"

struct LevelEndInfo
{
	TeamColour team_colours = TeamColour();
	float team_a_area = 0.0f;
	float team_b_area = 0.0f;
	// Surface still held at the finish, indexed by device slot, not seat.
	std::array<float, input_slots::count> player_areas{};
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
	std::wstring team_a_percentage_string() const;
	std::wstring team_b_percentage_string() const;
	std::wstring winning_team_string() const;

	// Whether only one team participated; results then show coverage instead
	// of a competitive paint share.
	bool one_team_only() const;

	// Fraction of the arena painted by either team.
	float coverage_ratio() const;
};
