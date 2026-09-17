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

	// Every paintable tile in the arena, painted or not. Only a match with one
	// team in it reads this: with two teams the question is who painted more of
	// what was painted, and the ground nobody reached is not part of it.
	float total_area = 0.0f;

	// Whether anybody was playing for each team. Both true is the normal match;
	// one false is the one-player path, which forces team A and skips team
	// select entirely.
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

	// True when one team was in the match and the other was not, which makes
	// every share-of-the-paint number below arithmetically 100% and
	// meaningless. The results screen asks this to know which half of itself
	// to leave out.
	bool one_team_only() const;

	// How much of the arena was painted at all, by anyone. What a match with
	// nobody to beat is actually a score of.
	float coverage_ratio() const;
};
