#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include "engine/render/screen_layout.h"
#include "game/input/input_slots.h"

// The roster and series rules every match obeys, in one place: how many seats
// a match has, which split screen a human count gets and which series lengths
// exist. MatchSetup, the remembered setup, the Debug replay file and plan, the
// menus and the level builder read them rather than spelling their own.

// A match seats 1-4 participants, humans and bots together.
constexpr int max_players = 4;
// A team game needs a player on each side: a series, and a team shuffle.
constexpr int min_versus_players = 2;
static_assert(max_players == static_cast<int>(labrador::ScreenLayout::four_player) + 1,
	"Every player count needs a split-screen layout");
static_assert(max_players <= input_slots::count, "Every human seat needs an input slot");

constexpr bool valid_player_count(int players)
{
	return players >= 1 && players <= max_players;
}

// "1-4", for messages that name the player-count range.
inline std::string player_count_range()
{
	return "1-" + std::to_string(max_players);
}

// One pane per human; bots have none, so the layout follows the humans alone.
// Throws std::invalid_argument outside 1..max_players.
inline labrador::ScreenLayout layout_for_humans(int humans)
{
	if (!valid_player_count(humans))
	{
		throw std::invalid_argument("A screen layout needs " + player_count_range() + " humans");
	}
	return static_cast<labrador::ScreenLayout>(humans - 1);
}

// The series lengths: a single match, best of 3 and best of 5. Draws award no
// win, so best-of counts decisive rounds (Session).
constexpr std::array<int, 3> best_of_choices{ 1, 3, 5 };

constexpr bool valid_best_of(int best_of)
{
	for (const int choice : best_of_choices)
	{
		if (choice == best_of) { return true; }
	}
	return false;
}
