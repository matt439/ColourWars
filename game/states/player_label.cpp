#include "game/pch.h"
#include "game/states/player_label.h"
#include "game/presentation_menu_text.h"

std::wstring player_label(const MatchSetup& setup, int player_index, const MenuText& text)
{
	const SeatSetup& player = setup.player_settings().at(static_cast<size_t>(player_index));
	if (player.controller == PlayerController::bot)
	{
		return text.bot_roster_label(player_index, player.difficulty);
	}
	return text.roster_label(player_index, setup.device_slot(player.num));
}
