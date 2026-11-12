#pragma once

#include <string>
#include "game/match/match_setup.h"

struct MenuText;
class SettingsSession;

// Stable participant number, with a human device or explicit bot difficulty.
std::wstring player_label(const MatchSetup& setup, int player_index, const MenuText& text);

// player_label with the name of the profile the seat's pad wears, through
// menu_text.labels.worn_profile ("P1 - Pad 1 (ALEX)"), or player_label alone
// for a seat wearing none: a bot, the keyboard or a bare pad (REV#35). A page
// that cannot fit it shows player_label instead.
std::wstring worn_player_label(const MatchSetup& setup, int player_index, const MenuText& text,
	const SettingsSession& session);
