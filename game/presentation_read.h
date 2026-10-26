#pragma once

// Private to the presentation_*.cpp files: the key tables, label templates,
// colour readers and section readers they share. Nothing else includes it.

#include "game/presentation.h"
#include "engine/assets/json.h"
#include "engine/render/colour.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

namespace presentation_read
{
	// JsonValue provides source context for missing keys and wrong types.
	inline constexpr std::array<const char*, WeaponSelectArtworkPresentation::CHOICES> weapon_choice_keys{
		"sprayer", "sniper", "roller", "mister", "bucket", "random"
	};
	inline constexpr std::array<const char*, static_cast<size_t>(MenuHeading::count)> menu_heading_keys{
		"home", "options", "credits", "controls_audio", "pad_sticks", "players", "join_players",
		"mode", "teams", "weapons", "arena"
	};
	inline constexpr std::array<const char*, static_cast<size_t>(MenuPrompt::count)> menu_prompt_keys{
		"title", "select_back", "options", "controls", "sticks", "mode", "join", "team", "team_shuffle",
		"weapon", "arena_browse", "arena_locked", "missing_player", "pause", "pause_missing", "select", "credits"
	};
	inline constexpr std::array<const char*, static_cast<size_t>(MenuItem::count)> menu_item_keys{
		"play", "options", "credits", "exit", "resolution", "fullscreen", "team_palette",
		"controls_audio", "apply", "back", "single_match", "best_of_three", "best_of_five",
		"match_length", "one_player_practice", "two_players", "three_players", "four_players",
		"resume", "restart", "quit", "yes", "no", "change_teams", "change_weapons", "change_level",
		"exit_to_main_menu", "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
		"reduce_effects", "pad", "move_deadzone", "aim_deadzone", "restore_defaults", "keep", "revert", "keyboard_jump", "pad_jump", "rumble"
	};

	struct LabelToken
	{
		std::wstring_view name;
		std::wstring value;
	};

	std::wstring format_label_template(const std::wstring& text,
		std::initializer_list<LabelToken> tokens);

	labrador::Colour read_colour(const labrador::JsonValue& parent, const char* key);
	labrador::Colour colour_from_text(const labrador::JsonValue& where, const std::string& text);

	MenuText read_menu_text(const labrador::JsonValue& root);
	TeamPalettePresentation read_team_palette(const labrador::JsonValue& root);
	MenuStyle read_menu_style(const labrador::JsonValue& root);
	MainMenuPresentation read_main_menu(const labrador::JsonValue& root, float content_height);
	ControlsSettingsPresentation read_controls_settings(const labrador::JsonValue& root, float content_height);
	PauseMenuPresentation read_pause_menu(const labrador::JsonValue& root);
	EndMenuPresentation read_end_menu(const labrador::JsonValue& root);
	ResultsMenuPresentation read_results_menu(const labrador::JsonValue& root);
	PromptBarPresentation read_prompt_bar(const labrador::JsonValue& root);
	HudPresentation read_hud(const labrador::JsonValue& root);
	PlayerIdentityPresentation read_player_identity(const labrador::JsonValue& root);
	std::array<IdleDripPresentation, 5> read_idle_drips(const labrador::JsonValue& root);
	WeaponFeedbackPresentation read_weapon_feedback(const labrador::JsonValue& root);
	BlockedShotPresentation read_blocked_shot(const labrador::JsonValue& root);
	ToolMotionPresentation read_tool_motion(const labrador::JsonValue& root);
	FigurePresentation read_figure(const labrador::JsonValue& root);
	RespawnRedrawPresentation read_respawn_redraw(const labrador::JsonValue& root);
	OpeningPresentation read_opening(const labrador::JsonValue& root);
	DebugOverlayPresentation read_debug_overlay(const labrador::JsonValue& root);
}
