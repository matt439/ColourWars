#include "game/pch.h"
#include "game/states/player_label.h"
#include "game/config/presentation/presentation_menu_text.h"
#include "game/config/settings_session.h"

std::wstring player_label(const MatchSetup& setup, int player_index, const MenuText& text)
{
	const SeatSetup& player = setup.player_settings().at(static_cast<size_t>(player_index));
	if (player.controller == PlayerController::bot)
	{
		return text.bot_roster_label(player_index, player.difficulty);
	}
	return text.roster_label(player_index, setup.device_slot(player.num));
}

std::wstring worn_player_label(const MatchSetup& setup, int player_index, const MenuText& text,
	const SettingsSession& session)
{
	const SeatSetup& player = setup.player_settings().at(static_cast<size_t>(player_index));
	const std::wstring label = player_label(setup, player_index, text);
	return player.controller == PlayerController::human ?
		text.worn_profile_label(label, session.worn_profile_name(player.num)) : label;
}
