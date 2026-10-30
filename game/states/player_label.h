#pragma once

#include <string>
#include "game/match_setup.h"

struct MenuText;

// Stable participant number, with a human device or explicit bot difficulty.
std::wstring player_label(const MatchSetup& setup, int player_index, const MenuText& text);
