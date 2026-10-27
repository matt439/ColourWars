#pragma once

// The display copy read from presentation.json's menu_text section: the fixed
// heading, prompt and item roles, the per-screen text records, and MenuText.
// Text, not layout; the formatters are defined in presentation_menu_text.cpp.

#include "game/match_award.h"
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include "game/player_control.h"

enum class MenuHeading
{
	home, options, credits, controls_audio, pad_sticks, players, join_players,
	mode, teams, weapons, arena, count
};

enum class MenuPrompt
{
	title, select_back, options, controls, sticks, mode, join, team, team_shuffle,
	weapon, arena_browse, arena_locked, missing_player, pause, pause_missing,
	select, credits, bot_team, bot_weapon, count
};

enum class MenuItem
{
	play, options, credits, exit, resolution, fullscreen, team_palette,
	controls_audio, apply, back, single_match, best_of_three, best_of_five,
	match_length, one_player_practice, two_players, three_players, four_players,
	resume, restart, quit, yes, no, change_teams, change_weapons, change_level,
	exit_to_main_menu, pad_sticks, music_volume, sfx_volume, view_shake, aim_marker,
	reduce_effects, pad, move_deadzone, aim_deadzone, restore_defaults, keep, revert, keyboard_jump, pad_buttons, rumble, watch_replay, count
};

struct MenuTitleText
{
	std::wstring wordmark;
	std::wstring subtitle;
	std::wstring start;
	std::wstring no_controller;
};

struct MenuHelpText
{
	std::wstring controls;
	std::wstring pad_association;
	std::wstring aim_cutoff;
};

struct KeyboardJumpMenuText
{
	std::wstring heading, capture, reset, current, pending, help;
	std::wstring pending_status, saved_status, capture_wait_neutral, capture_ready, capture_release;
	std::wstring capture_rejected, capture_conflict, capture_cancelled, debug_collision;
	std::wstring current_key(std::wstring_view label) const;
	std::wstring pending_key(std::wstring_view label) const;
	std::wstring release_key(std::wstring_view label) const;
	std::wstring help_block(std::wstring_view current_label, std::wstring_view pending_label) const;
};

// The Pad Buttons page: one row per action entry, in PadAction order with the
// alternate after each primary. Control labels are the game's (input_bindings.h).
struct PadButtonsMenuText
{
	std::wstring heading, jump, jump_alternate, shoot, shoot_alternate, pause, pause_alternate;
	std::wstring reset, help, pending_status, saved_status, conflict_status;
	// Index 0-5: jump, jump_alternate, shoot, shoot_alternate, pause, pause_alternate.
	const std::wstring& row(size_t index) const;
};

struct MenuStatusText
{
	std::wstring options_pending;
	std::wstring options_saved;
	std::wstring options_save_failed;
	std::wstring controls_pending;
	std::wstring controls_saved;
	std::wstring active_save_failed;
	std::wstring sticks_pending;
	std::wstring sticks_saved;
	std::wstring options_reverted;
	std::wstring options_display_failed;
	std::wstring options_rollback_failed;
};

struct MenuValueText
{
	std::wstring on;
	std::wstring off;
	std::wstring random;
	std::wstring percentage;
	std::wstring duration_minutes;
	std::wstring duration_default;
	std::wstring resolution;
	std::wstring shared;
};

struct MenuConfirmationText
{
	std::wstring restart;
	std::wstring quit;
	std::wstring exit;
	std::wstring display;
};

struct MenuEndText
{
	std::wstring match_complete;
	std::wstring round_complete;
	std::wstring series_complete;
	std::wstring rematch;
	std::wstring play_series_again;
	std::wstring replay_draw;
	std::wstring next_round;
};

struct MenuResultsText
{
	std::wstring single_match_heading;
	std::wstring proceed;
	std::wstring contribution_heading;
	std::wstring series_team_a_wins;
	std::wstring series_team_b_wins;
	std::wstring draw_no_win;
	std::wstring series_continues;
	std::wstring series_score;
	std::wstring round_heading;
	std::wstring competitive_team_a_wins;
	std::wstring competitive_team_b_wins;
	std::wstring competitive_draw;
	std::wstring contribution_team_a;
	std::wstring contribution_team_b;
	std::wstring solo_coverage;
	std::wstring team_a_percentage;
	std::wstring team_b_percentage;
	// The call-outs at the right of the contribution rows, one per award.
	std::wstring award_paint_laid;
	std::wstring award_recaptured;
	std::wstring award_airtime;
	std::wstring award_splats;
	const std::wstring& award_line(MatchAward award) const;
	// The caller renders the percentage. These wrappers preserve that string,
	// including unusual numeric spellings, without imposing a numeric domain.
	std::wstring solo_coverage_line(const std::wstring& percentage) const;
	std::wstring team_a_percentage_line(const std::wstring& percentage) const;
	std::wstring team_b_percentage_line(const std::wstring& percentage) const;
	// Current Sessions are first to 1, 2 or 3; completion prevents both
	// teams reaching the target in the same ledger.
	std::wstring score_line(int team_a_wins, int team_b_wins, int target) const;
	// Draws can advance a series beyond its best-of length. Every positive
	// int is supported; resource validation requires finite line spacing,
	// glyph offsets/advances and resulting pen/extents. Spacing may be zero or negative.
	std::wstring round_title(int round) const;
};

struct MenuStageText
{
	std::wstring browse_status;
	std::wstring locked_status;
	std::wstring briefing_label;
	std::wstring goal;
	std::wstring solo_objective;
	std::wstring versus_objective;
	std::wstring random_navigation;
	std::wstring locked_hint;
	std::wstring browse_hint;
	std::wstring solo_mode;
	std::wstring single_match_mode;
	std::wstring best_of_mode;
	std::wstring solo_roster;
	std::wstring versus_roster;
	std::wstring numbered_navigation;
	std::wstring random_hint;
};

// Labels for the per-stage records. The numbers and the joins between the
// labels stay in code, as the other composed lines do.
struct MenuRecordText
{
	std::wstring new_best;
	std::wstring stage_best;
	std::wstring best_win;
	std::wstring best_coverage;
	std::wstring played;
};

// Menu-only device names and roster rows. The player token retains the
// canonical P1-P4 badge identity; hardware slots remain separate from seats.
struct MenuLabelText
{
	std::wstring pad;
	std::wstring keyboard_mouse;
	std::wstring waiting;
	std::wstring roster_row;
	std::wstring bot;
	std::wstring bot_easy;
	std::wstring bot_normal;
	std::wstring bot_hard;
	// A seat's handicap on Weapon Select, one per level from none upward.
	std::wstring handicap_none;
	std::wstring handicap_plus_one;
	std::wstring handicap_plus_two;
};

// Authored connection copy; the missing roster is assembled by the menu.
// Only pause_player_title has a numeric {player} token.
struct MenuConnectionText
{
	std::wstring join_reconnect_suffix;
	std::wstring join_ready;
	std::wstring join_waiting;
	std::wstring pause_title;
	std::wstring pause_player_title;
	std::wstring controller_lost;
	std::wstring all_connected;
	std::wstring teams_required;
};

// Pad Sticks preview copy. Percent signs and authored line breaks are literal;
// the menu supplies rounded integers rather than formatting raw input here.
struct MenuStickReadoutsText
{
	std::wstring connected;
	std::wstring disconnected;
	std::wstring raw_movement;
	std::wstring movement_response;
	std::wstring aim_inactive;
	std::wstring aim_active;
};

// Fixed roles select display text; navigation and accepted inputs remain code.
struct MenuText
{
	std::wstring brand;
	std::wstring solo_practice_hint;
	// Untimed practice's pause row (POL#19), with one {tool} token.
	std::wstring practice_tool;
	MenuTitleText title;
	std::wstring home_subtitle;
	std::wstring display_trial_countdown;
	std::wstring options_palette_timing;
	MenuHelpText help;
	KeyboardJumpMenuText keyboard_jump;
	PadButtonsMenuText pad_buttons;
	MenuStatusText status;
	MenuValueText values;
	MenuConfirmationText confirmations;
	MenuEndText end;
	MenuResultsText results;
	MenuStageText stage;
	MenuRecordText records;
	MenuLabelText labels;
	MenuConnectionText connection;
	MenuStickReadoutsText stick_readouts;
	std::array<std::wstring, static_cast<size_t>(MenuHeading::count)> headings;
	std::array<std::wstring, static_cast<size_t>(MenuPrompt::count)> prompts;
	std::array<std::wstring, static_cast<size_t>(MenuItem::count)> items;
	const std::wstring& heading(MenuHeading heading) const;
	const std::wstring& prompt(MenuPrompt prompt) const;
	const std::wstring& item(MenuItem item) const;
	std::wstring display_countdown(int seconds) const;
	// Integer volume and rounded effective-deadzone values share the 0-100 domain.
	std::wstring setting_percentage(int value) const;
	// A per-pad switch that follows the shared one, showing what it resolves to.
	std::wstring setting_shared(bool value) const;
	// Positive finite seconds retain the Mode row's float division and default
	// stream precision, including scientific notation and minutes underflowing to 0.
	std::wstring setting_duration(float seconds, bool use_default) const;
	// Window presets and live fullscreen sizes share the positive int domain.
	std::wstring setting_resolution(int width, int height) const;
	// Stage summary domains follow the existing choices: best-of 3/5, and
	// one solo player or two through four versus players.
	std::wstring stage_best_of_mode(int best_of) const;
	std::wstring stage_roster(int players) const;
	// Concrete Arena indices are one-based; Random needs at least one eligible stage.
	std::wstring stage_navigation(size_t index, size_t total) const;
	std::wstring stage_random_hint(size_t count) const;
	// -1 is unclaimed, 0-3 are pads and 4 is keyboard/mouse. Roster indices
	// are 0-3 and substitute the canonical badge label, never a device number.
	std::wstring device_label(int slot) const;
	std::wstring roster_label(int roster_index, int slot) const;
	std::wstring bot_label(BotDifficulty difficulty) const;
	std::wstring bot_roster_label(int roster_index, BotDifficulty difficulty) const;
	// Throws invalid_argument outside handicaps::valid.
	std::wstring handicap_label(int level) const;
	// Player numbers are the visible roster numbers 1-4, never device slots.
	std::wstring pause_title_for_player(int number) const;
	std::wstring practice_tool_label(std::wstring_view tool) const;
	// Pad slots are 0-3; rounded percentages are -100 through 100 inclusive.
	std::wstring stick_connection(int pad_slot, bool connected) const;
	std::wstring stick_raw_movement(int value) const;
	std::wstring stick_movement_response(int value) const;
	std::wstring stick_aim_inactive(int raw_x, int raw_y) const;
	std::wstring stick_aim_active(int raw_x, int raw_y, int aim_x, int aim_y) const;
};
