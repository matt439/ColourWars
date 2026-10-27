#pragma once

// What the menu_text acceptance pieces share: the original includes, the
// default-text tables the checks compare against, the fixture helpers,
// World (the sheets, fonts and silent bank a Presentation is validated
// against), Variants (edited copies of presentation.json) and the checks
// main calls, declared in main's order.
//
// That order is load-bearing and only main sets it. Variants numbers its
// menu-text-N.json fixtures in call order, and some checks register fonts on
// World that later checks use. The core fonts cross files:
// check_inclusive_bounds registers menu_text_exact and check_text_and_fonts
// registers menu_text_limited and menu_text_tall, and checks in nearly every
// other file use them (menu_text_tall in check_controls_text and
// check_title_and_home_text), so main runs the core checks first. The other
// fonts a later check relies on stay within one file:
// menu_confirmation_width_overflow (confirmation bounds -> display trial),
// menu_end_heading_small_actions (End heading bounds -> End continuation
// bounds) and menu_controls_row_overflow (controls text -> controls option
// rows). A new check that needs a font from another file must run after it.
#include "game/presentation.h"
#include "game/controls_text.h"
#include "game/content_validation.h"
#include "game/play_settings.h"
#include "game/objects/level_end_info.h"
#include "game/states/option_row.h"
#include "game/states/main_menu.h"
#include "game/states/menu_level_settings.h"
#include "game/states/prompt_bar.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;

namespace menu_text_acceptance
{
    void require(bool condition, const std::string& message);
    rapidjson::Document read_document(const std::filesystem::path& path);
    UiText* row_text(OptionRow& row, const std::string& name);
    MenuPaperArtPresentation options_art(const MainMenuPresentation& main);

    struct DefaultText { const char* key; const wchar_t* text; };
    constexpr DefaultText headings[]{
        { "home", L"Make your mark." }, { "options", L"Options" }, { "credits", L"Credits" },
        { "controls_audio", L"Controls / audio" }, { "pad_sticks", L"Pad sticks" },
        { "players", L"Number of Players" }, { "join_players", L"Join Players" },
        { "mode", L"Mode Select" }, { "teams", L"Team Select" }, { "weapons", L"Weapon Select" },
        { "arena", L"Choose your arena" }
    };
    constexpr DefaultText prompts[]{
        { "title", L"A/Enter Start" }, { "select_back", L"A/Enter Select    B/Esc Back" },
        { "options", L"Left/Right Change    A/Enter Select    B/Esc Back" },
        { "controls", L"Left/Right Change    A/Enter Select    B/Esc Back (discard unapplied changes)" },
        { "sticks", L"D-pad/arrows Navigate / change    A/Enter Select    B/Esc Back (discard unapplied changes)" },
        { "mode", L"A/Enter Select    Left/Right Match length    B/Esc Back" },
        { "join", L"A/Enter Join / P1 continue    B/Esc Leave / P1 back" },
        { "team", L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel" },
        { "team_shuffle", L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams" },
        { "weapon", L"Left/Right Change weapon    A/Enter Confirm    B/Esc Cancel" },
        { "arena_browse", L"Left/Right Browse arenas    A/Enter Lock arena    B/Esc Back" },
        { "arena_locked", L"A/Enter Start match    B/Esc Unlock arena" },
        { "missing_player", L" Reconnect controller    B/Esc Change players" },
        { "pause", L"A/Enter Select    B/Start/Esc Resume" },
        { "pause_missing", L" Reconnect pads to resume    A/Enter Select" },
        { "select", L"A/Enter Select" }, { "credits", L"A/Enter Back    B/Esc Back" }
    };
    constexpr wchar_t brand[] = L"C O L O U R W A R S   /   LOCAL PAINT BATTLES";
    constexpr wchar_t practice_hint[] = L"Solo practice: choose Single Match";
    constexpr wchar_t home_subtitle[] = L"Paint the arena. Claim the win.";
    constexpr wchar_t palette_timing[] = L"Palette applies next round or restart.";
    struct TitleText { const char* key; const wchar_t* text; std::wstring MenuTitleText::* field; };
    constexpr TitleText title_text[]{
        { "wordmark", L"COLOUR\nWARS", &MenuTitleText::wordmark },
        { "subtitle", L"MAKE A MESS. MAKE IT YOURS.", &MenuTitleText::subtitle },
        { "start", L"LET'S PLAY", &MenuTitleText::start },
        { "no_controller", L"Connect a controller", &MenuTitleText::no_controller }
    };
    struct HelpText { const char* key; const wchar_t* text; std::wstring MenuHelpText::* field; bool multiline; };
    constexpr HelpText help_text[]{
        { "controls", L"CONTROLLER\nMove / drop: left stick\nAim: right stick    Shoot: RT\n"
            L"Jump: Pad Jump choice / LT (hold for height)\nPause: Start    Debug: Back / View\nKEYBOARD + MOUSE\n"
            L"Move: A / D    Drop: S / Down\nAim: mouse cursor    Shoot: left click\n"
            L"Jump: {jump} (hold for height)\nPause: Esc    Debug: F1\nMENUS\n"
            L"Navigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc\nAIM / READINESS\n"
            L"Direction only; roller aim is fixed.\nReady: chevron / Cooldown: bars\nNo paint: cross\n"
            L"Pad sticks: movement / aim deadzones.\nREDUCE EFFECTS\nLess shake, flashes and idle detail.",
            &MenuHelpText::controls, true },
        { "pad_association", L"Profiles follow Pad 1-4 slots,\nnot the player number or device.\n"
            L"After reconnecting, check the slot.\nKeyboard/mouse is unchanged.", &MenuHelpText::pad_association, true },
        { "aim_cutoff", L"Right stick: radial cutoff", &MenuHelpText::aim_cutoff, false }
    };
    struct KeyboardJumpText {
        const char* key; const wchar_t* text; std::wstring KeyboardJumpMenuText::* field;
        bool multiline = false; bool key_template = false;
    };
    constexpr KeyboardJumpText keyboard_jump_text[]{
        { "heading", L"Keyboard Jump", &KeyboardJumpMenuText::heading },
        { "capture", L"Choose Jump key", &KeyboardJumpMenuText::capture },
        { "reset", L"Reset to Space", &KeyboardJumpMenuText::reset },
        { "current", L"Current: {key}", &KeyboardJumpMenuText::current, false, true },
        { "pending", L"Pending: {key}", &KeyboardJumpMenuText::pending, false, true },
        { "help", L"Choose one A-Z key or Space.\nA / D / S already control movement or drop.\n"
            L"Apply activates and saves your choice.\nBack discards unapplied changes.\nMenus and intro skip keep their fixed keys.",
            &KeyboardJumpMenuText::help, true },
        { "pending_status", L"Apply to activate this Jump key.", &KeyboardJumpMenuText::pending_status },
        { "saved_status", L"Jump key applied and saved.", &KeyboardJumpMenuText::saved_status },
        { "capture_wait_neutral", L"Release all keys before choosing.", &KeyboardJumpMenuText::capture_wait_neutral },
        { "capture_ready", L"Press one key. Esc / pad Back cancels.", &KeyboardJumpMenuText::capture_ready },
        { "capture_release", L"Release {key} to keep this choice.", &KeyboardJumpMenuText::capture_release, false, true },
        { "capture_rejected", L"Use one A-Z key or Space. Try again.", &KeyboardJumpMenuText::capture_rejected },
        { "capture_conflict", L"Key already used for movement or drop.", &KeyboardJumpMenuText::capture_conflict },
        { "capture_cancelled", L"Gesture cancelled; pending key unchanged.", &KeyboardJumpMenuText::capture_cancelled },
        { "debug_collision", L"Jump key also has a developer bind.", &KeyboardJumpMenuText::debug_collision }
    };
    struct PadJumpText {
        const char* key; const wchar_t* text; std::wstring PadJumpMenuText::* field;
        bool multiline = false; bool button_template = false;
    };
    constexpr PadJumpText pad_jump_text[]{
        { "heading", L"Pad Jump", &PadJumpMenuText::heading },
        { "button", L"Jump button", &PadJumpMenuText::button },
        { "reset", L"Reset this pad to A", &PadJumpMenuText::reset },
        { "current", L"Current: {button} / LT", &PadJumpMenuText::current, false, true },
        { "pending", L"Pending: {button} / LT", &PadJumpMenuText::pending, false, true },
        { "help", L"Choose A, B, X or Y for this pad.\nLT also jumps; hold for height.\n"
            L"Apply saves edited pads.\nBack discards unapplied changes.\nMenus: A selects; B goes back.\n"
            L"Intro skip keeps A / B / Start.\nProfiles follow Pad 1-4 slots.", &PadJumpMenuText::help, true },
        { "pending_status", L"Apply to activate edited pads' Jump buttons.", &PadJumpMenuText::pending_status },
        { "saved_status", L"Edited pads' Jump buttons applied and saved.", &PadJumpMenuText::saved_status }
    };
    struct StatusText { const char* key; const wchar_t* text; std::wstring MenuStatusText::* field; bool options; };
    constexpr StatusText status_text[]{
        { "options_pending", L"Apply to save changes.", &MenuStatusText::options_pending, true },
        { "options_saved", L"Settings applied and saved.", &MenuStatusText::options_saved, true },
        { "options_save_failed", L"Settings active for this run. Could not save for next time.",
            &MenuStatusText::options_save_failed, true },
        { "controls_pending", L"Apply to save changes for all players.", &MenuStatusText::controls_pending, false },
        { "controls_saved", L"Applied and saved for all players.", &MenuStatusText::controls_saved, false },
        { "active_save_failed", L"Active for this run. Could not save.", &MenuStatusText::active_save_failed, false },
        { "sticks_pending", L"Apply to save pad settings.", &MenuStatusText::sticks_pending, false },
        { "sticks_saved", L"Pad settings applied and saved.", &MenuStatusText::sticks_saved, false },
        { "options_reverted", L"Display settings reverted.", &MenuStatusText::options_reverted, true },
        { "options_display_failed", L"Could not change display settings. Previous settings restored.",
            &MenuStatusText::options_display_failed, true },
        { "options_rollback_failed", L"Could not restore display settings. Try Apply again.",
            &MenuStatusText::options_rollback_failed, true }
    };
    struct ConnectionText { const char* key; const wchar_t* text; std::wstring MenuConnectionText::* field; };
    constexpr ConnectionText connection_text[]{
        { "join_reconnect_suffix", L" - Reconnect", &MenuConnectionText::join_reconnect_suffix },
        { "join_ready", L"Everyone joined. Player 1 can continue.", &MenuConnectionText::join_ready },
        { "join_waiting", L"Press A / Enter on each device to join. Reconnect reserved pads.",
            &MenuConnectionText::join_waiting },
        { "pause_title", L"Pause Menu", &MenuConnectionText::pause_title },
        { "pause_player_title", L"Pause Menu P{player}", &MenuConnectionText::pause_player_title },
        { "controller_lost", L"Controller lost", &MenuConnectionText::controller_lost },
        { "all_connected", L"All players connected.", &MenuConnectionText::all_connected },
        { "teams_required", L"Both teams need a player", &MenuConnectionText::teams_required }
    };
    struct LabelText { const char* key; const wchar_t* text; std::wstring MenuLabelText::* field; };
    constexpr LabelText label_text[]{
        { "pad", L"Pad {pad}", &MenuLabelText::pad },
        { "keyboard_mouse", L"Keyboard/mouse", &MenuLabelText::keyboard_mouse },
        { "waiting", L"Waiting to join", &MenuLabelText::waiting },
        { "roster_row", L"{player} - {device}", &MenuLabelText::roster_row }
    };
    struct StickReadoutText
    {
        const char* key;
        const wchar_t* text;
        std::wstring MenuStickReadoutsText::* field;
        bool multiline;
        std::initializer_list<const char*> tokens;
    };
    const StickReadoutText stick_readout_text[]{
        {"connected", L"{device} - connected", &MenuStickReadoutsText::connected, false, {"{device}"}},
        {"disconnected", L"{device} - disconnected", &MenuStickReadoutsText::disconnected, false, {"{device}"}},
        {"raw_movement", L"Left stick raw X: {value}%", &MenuStickReadoutsText::raw_movement, false, {"{value}"}},
        {"movement_response", L"Move response: {value}%", &MenuStickReadoutsText::movement_response, false, {"{value}"}},
        {"aim_inactive", L"Raw X: {raw_x}%\nRaw Y: {raw_y}%\nNo new aim\nRaw: hollow\nAim: solid",
            &MenuStickReadoutsText::aim_inactive, true, {"{raw_x}", "{raw_y}"}},
        {"aim_active", L"Raw X: {raw_x}%\nRaw Y: {raw_y}%\nAim X: {aim_x}%\nAim Y: {aim_y}%\nRaw: hollow\nAim: solid",
            &MenuStickReadoutsText::aim_active, true, {"{raw_x}", "{raw_y}", "{aim_x}", "{aim_y}"}}
    };
    struct ValueText { const char* key; const wchar_t* text; std::wstring MenuValueText::* field; };
    constexpr ValueText value_text[]{
        { "on", L"On", &MenuValueText::on }, { "off", L"Off", &MenuValueText::off },
        { "random", L"Random", &MenuValueText::random }
    };
    struct ConfirmationText { const char* key; const wchar_t* text; std::wstring MenuConfirmationText::* field; };
    constexpr ConfirmationText confirmation_text[]{
        { "restart", L"Are you sure you want to restart?", &MenuConfirmationText::restart },
        { "quit", L"Are you sure you want to quit?", &MenuConfirmationText::quit },
        { "exit", L"Exit ColourWars?", &MenuConfirmationText::exit },
        { "display", L"Keep these display settings?", &MenuConfirmationText::display }
    };
    struct EndText { const char* key; const wchar_t* text; std::wstring MenuEndText::* field; };
    constexpr EndText end_text[]{
        { "match_complete", L"Match Complete", &MenuEndText::match_complete },
        { "round_complete", L"Round Complete", &MenuEndText::round_complete },
        { "series_complete", L"Series Complete", &MenuEndText::series_complete }
    };
    constexpr EndText end_continuation_text[]{
        { "rematch", L"Rematch", &MenuEndText::rematch },
        { "play_series_again", L"Play Series Again", &MenuEndText::play_series_again },
        { "replay_draw", L"Replay Draw", &MenuEndText::replay_draw },
        { "next_round", L"Next Round", &MenuEndText::next_round }
    };
    struct ResultsText { const char* key; const wchar_t* text; std::wstring MenuResultsText::* field; };
    constexpr ResultsText results_text[]{
        { "single_match_heading", L"Results", &MenuResultsText::single_match_heading },
        { "proceed", L"Press A/Enter to proceed", &MenuResultsText::proceed },
        { "contribution_heading", L"Surface held (% of arena)", &MenuResultsText::contribution_heading }
    };
    constexpr ResultsText session_result_text[]{
        { "series_team_a_wins", L"Team A wins the series!", &MenuResultsText::series_team_a_wins },
        { "series_team_b_wins", L"Team B wins the series!", &MenuResultsText::series_team_b_wins },
        { "draw_no_win", L"Draw - no win awarded", &MenuResultsText::draw_no_win },
        { "series_continues", L"Series continues", &MenuResultsText::series_continues }
    };
    constexpr ResultsText competitive_winner_text[]{
        { "competitive_team_a_wins", L"Team A Wins!", &MenuResultsText::competitive_team_a_wins },
        { "competitive_team_b_wins", L"Team B Wins!", &MenuResultsText::competitive_team_b_wins },
        { "competitive_draw", L"Draw!", &MenuResultsText::competitive_draw }
    };
    constexpr ResultsText contribution_team_text[]{
        { "contribution_team_a", L" | Team A | ", &MenuResultsText::contribution_team_a },
        { "contribution_team_b", L" | Team B | ", &MenuResultsText::contribution_team_b }
    };
    struct ResultPercentageText
    {
        const char* key; const wchar_t* text; std::wstring MenuResultsText::* field;
        std::wstring (MenuResultsText::* line)(const std::wstring&) const;
        const wchar_t* example;
    };
    constexpr ResultPercentageText result_percentage_text[]{
        { "solo_coverage", L"{percentage} Painted", &MenuResultsText::solo_coverage, &MenuResultsText::solo_coverage_line, L"12.5% Painted" },
        { "team_a_percentage", L"A {percentage}", &MenuResultsText::team_a_percentage, &MenuResultsText::team_a_percentage_line, L"A 12.5%" },
        { "team_b_percentage", L"B {percentage}", &MenuResultsText::team_b_percentage, &MenuResultsText::team_b_percentage_line, L"B 12.5%" }
    };
    struct SeriesScoreExample { int team_a; int team_b; int target; };
    constexpr SeriesScoreExample series_score_examples[]{
        { 0, 0, 1 }, { 0, 1, 1 }, { 1, 0, 1 },
        { 0, 0, 2 }, { 0, 1, 2 }, { 0, 2, 2 }, { 1, 0, 2 },
        { 1, 1, 2 }, { 1, 2, 2 }, { 2, 0, 2 }, { 2, 1, 2 },
        { 0, 0, 3 }, { 0, 1, 3 }, { 0, 2, 3 }, { 0, 3, 3 },
        { 1, 0, 3 }, { 1, 1, 3 }, { 1, 2, 3 }, { 1, 3, 3 },
        { 2, 0, 3 }, { 2, 1, 3 }, { 2, 2, 3 }, { 2, 3, 3 },
        { 3, 0, 3 }, { 3, 1, 3 }, { 3, 2, 3 }
    };
    struct RecordText { const char* key; const wchar_t* text; std::wstring MenuRecordText::* field; };
    constexpr RecordText record_text[]{
        { "new_best", L"NEW BEST", &MenuRecordText::new_best },
        { "stage_best", L"STAGE BEST", &MenuRecordText::stage_best },
        { "best_win", L"BEST WIN", &MenuRecordText::best_win },
        { "best_coverage", L"BEST COVER", &MenuRecordText::best_coverage },
        { "played", L"PLAYED", &MenuRecordText::played }
    };
    struct StageText { const char* key; const wchar_t* text; std::wstring MenuStageText::* field; };
    constexpr StageText stage_text[]{
        { "browse_status", L"FIND YOUR ARENA", &MenuStageText::browse_status },
        { "locked_status", L"ARENA LOCKED", &MenuStageText::locked_status }
    };
    struct StageCountText
    {
        const char* key;
        const wchar_t* text;
        std::wstring MenuStageText::* field;
        bool multiline;
    };
    constexpr StageCountText stage_count_text[]{
        { "numbered_navigation", L"<   ARENA {index} / {total}   >", &MenuStageText::numbered_navigation, false },
        { "random_hint", L"Random: {count} available arenas.\nConfirm to lock your choice.", &MenuStageText::random_hint, true }
    };
    struct StageSummaryText
    {
        const char* key;
        const wchar_t* text;
        std::wstring MenuStageText::* field;
        const char* token;
        int row;
        int value;
        bool detail;
    };
    constexpr StageSummaryText stage_summary_text[]{
        { "best_of_mode", L"BEST OF {best_of}", &MenuStageText::best_of_mode, "{best_of}", 1, 5, true },
        { "solo_roster", L"{players} PLAYER", &MenuStageText::solo_roster, "{players}", 2, 1, false },
        { "versus_roster", L"{players} PLAYERS / 2 TEAMS", &MenuStageText::versus_roster, "{players}", 2, 4, false }
    };
    struct StageBriefingText {
        const char* key; const wchar_t* text; std::wstring MenuStageText::* field; int row; int next_row; bool multiline;
        bool detail = false;
    };
    constexpr StageBriefingText stage_briefing_text[]{
        { "briefing_label", L"MATCH SETUP", &MenuStageText::briefing_label, 0, 1, false },
        { "goal", L"PAINT TO WIN", &MenuStageText::goal, 3, 4, false },
        { "solo_objective", L"Make the arena your canvas.\nTry weapons. Cover more ground.",
            &MenuStageText::solo_objective, 4, 6, true },
        { "versus_objective", L"Cover the arena in your colour.\nMost painted surface wins.",
            &MenuStageText::versus_objective, 4, 6, true },
        { "solo_mode", L"SOLO PRACTICE", &MenuStageText::solo_mode, 1, 2, false, true },
        { "single_match_mode", L"SINGLE MATCH", &MenuStageText::single_match_mode, 1, 2, false, true }
    };
    struct StageCueText { const char* key; const wchar_t* text; std::wstring MenuStageText::* field; bool hint; };
    constexpr StageCueText stage_cue_text[]{
        { "random_navigation", L"<   SURPRISE ME   >", &MenuStageText::random_navigation, false },
        { "locked_hint", L"Confirm again to start.\nBack to change your choice.", &MenuStageText::locked_hint, true },
        { "browse_hint", L"Left/Right to browse.\nConfirm to lock your choice.", &MenuStageText::browse_hint, true }
    };
    constexpr DefaultText items[]{
        { "play", L"Play" }, { "options", L"Options" }, { "credits", L"Credits" }, { "exit", L"Exit" },
        { "resolution", L"Resolution" }, { "fullscreen", L"Fullscreen" }, { "team_palette", L"Team palette" },
        { "controls_audio", L"Controls / audio" }, { "apply", L"Apply" }, { "back", L"Back" },
        { "single_match", L"Single Match" }, { "best_of_three", L"Best of 3" }, { "best_of_five", L"Best of 5" },
        { "match_length", L"Match length" }, { "one_player_practice", L"1 Player (practice)" },
        { "two_players", L"2 Players" }, { "three_players", L"3 Players" }, { "four_players", L"4 Players" },
        { "resume", L"Resume" }, { "restart", L"Restart" }, { "quit", L"Quit" },
        { "yes", L"Yes" }, { "no", L"No" },
        { "change_teams", L"Change Teams" }, { "change_weapons", L"Change Weapons" },
        { "change_level", L"Change Level" }, { "exit_to_main_menu", L"Exit to Main Menu" },
        { "pad_sticks", L"Pad sticks" }, { "music_volume", L"Music volume" }, { "sfx_volume", L"SFX volume" },
        { "view_shake", L"View shake" }, { "aim_marker", L"Aim marker" }, { "reduce_effects", L"Reduce effects" },
        { "pad", L"Pad" }, { "move_deadzone", L"Move deadzone" }, { "aim_deadzone", L"Aim deadzone" },
        { "restore_defaults", L"Restore defaults" }, { "keep", L"Keep" }, { "revert", L"Revert" },
        { "keyboard_jump", L"Keyboard Jump" }, { "pad_jump", L"Pad Jump" }, { "rumble", L"Rumble" }
    };

    struct World
    {
        RenderResources render;
        AudioResources audio;

        explicit World(const std::filesystem::path& content)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(
                    (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* name : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(name) + ".spritefont")).string());
                auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                render.add_font(name, std::move(font));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    struct Variants
    {
        rapidjson::Document source;
        std::filesystem::path scratch;
        size_t next = 0;

        Variants(const std::filesystem::path& content, const std::filesystem::path& folder) :
            source(read_document(content / "presentation.json")), scratch(folder)
        {
            std::filesystem::create_directories(scratch);
        }

        template<class Edit>
        std::filesystem::path write(Edit edit)
        {
            rapidjson::Document document;
            document.CopyFrom(source, document.GetAllocator());
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode menu text fixture");
            const auto path = scratch / ("menu-text-" + std::to_string(next++) + ".json");
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close();
            require(output.good(), "Cannot write menu text fixture");
            return path;
        }

        template<class Edit>
        Presentation load(Edit edit)
        {
            const auto path = write(edit);
            return Presentation(path.string().c_str());
        }

        template<class Edit>
        void rejects(const std::string& field, Edit edit, World* resources = nullptr)
        {
            const auto path = write(edit);
            bool contextual = false;
            std::string diagnostic = "accepted without exception";
            try
            {
                const Presentation invalid(path.string().c_str());
                if (resources != nullptr) { invalid.validate_resources(resources->render, resources->audio); }
            }
            catch (const std::runtime_error& error)
            {
                diagnostic = error.what();
                contextual = diagnostic.find(path.filename().string()) != std::string::npos &&
                    diagnostic.find("menu_text") != std::string::npos;
                size_t begin = 0;
                while (begin < field.size())
                {
                    const size_t end = field.find('.', begin);
                    contextual = contextual && diagnostic.find(field.substr(begin, end - begin)) != std::string::npos;
                    if (end == std::string::npos) { break; }
                    begin = end + 1;
                }
            }
            require(contextual, "Invalid menu text " + field + " lost source/key context: " + diagnostic);
        }
    };

    void check_defaults(const Presentation& presentation);
    void check_keyed_content(Variants& variants);
    void check_text_and_fonts(Variants& variants, World& world, const std::filesystem::path& content);
    void check_inclusive_bounds(Variants& variants, World& world);
    void check_item_bounds(Variants& variants, World& world);
    void check_restore_defaults_text(Variants& variants, World& world);
    void check_options_palette_timing(Variants& variants, World& world);
    void check_setup_text(Variants& variants, World& world);
    void check_pause_text(Variants& variants, World& world);
    void check_connection_text(Variants& variants, World& world);
    void check_connection_bounds(Variants& variants, World& world);
    void check_label_text(Variants& variants, World& world, const MenuText& defaults);
    void check_bot_label_text(Variants& variants, World& world, const MenuText& defaults);
    void check_label_resources(Variants& variants, World& world);
    void check_label_results_extrema(Variants& variants, World& world);
    void check_confirmation_text(Variants& variants, World& world);
    void check_confirmation_bounds(Variants& variants, World& world);
    void check_display_trial_text(Variants& variants, World& world);
    void check_end_text(Variants& variants, World& world);
    void check_end_headings(Variants& variants, World& world);
    void check_end_heading_bounds(Variants& variants, World& world);
    void check_controls_text(Variants& variants, World& world);
    void check_controls_option_rows(Variants& variants, World& world);
    void check_pad_jump_text(Variants& variants, World& world);
    void check_setting_percentage_text(Variants& variants, World& world, const MenuText& defaults);
    void check_setting_percentage_rows(Variants& variants, World& world);
    void check_value_text(Variants& variants, World& world);
    void check_value_fitting(Variants& variants, World& world);
    void check_stage_random_fitting(Variants& variants, World& world);
    void check_stage_random_bounds(Variants& variants, World& world);
    void check_stage_count_text(Variants& variants, World& world, const MenuText& defaults);
    void check_stage_count_bounds(Variants& variants, World& world);
    void check_stage_summary_text(Variants& variants, World& world, const MenuText& defaults);
    void check_stage_summary_bounds(Variants& variants, World& world);
    void check_stage_briefing_text(Variants& variants, World& world);
    void check_stage_briefing_bounds(Variants& variants, World& world);
    void check_stage_status_text(Variants& variants, World& world);
    void check_stage_status_bounds(Variants& variants, World& world);
    void check_stage_cue_text(Variants& variants, World& world);
    void check_stage_cue_bounds(Variants& variants, World& world);
    void check_title_and_home_text(Variants& variants, World& world, const Presentation& defaults);
    void check_title_and_home_bounds(Variants& variants, World& world);
    void check_help_text(Variants& variants, World& world);
    void check_help_fitting(Variants& variants, World& world);
    void check_help_bounds(Variants& variants, World& world);
    void check_stick_readout_text(Variants& variants, World& world, const MenuText& defaults);
    void check_stick_readout_bounds(Variants& variants, World& world);
    void check_stick_numeric_extrema(Variants& variants, World& world);
    void check_status_text(Variants& variants, World& world);
    void check_status_fitting(Variants& variants, World& world);
    void check_status_bounds(Variants& variants, World& world);
    void check_option_row_fitting(Variants& variants, World& world);
    void check_prompt_bar(World& world, const Presentation& look);
    void check_setting_duration_text(Variants& variants, const MenuText& defaults);
    void check_setting_duration_content(Variants& variants, World& world, const std::filesystem::path& content);
    void check_setting_resolution_text(Variants& variants, const MenuText& defaults);
    void check_setting_resolution_resources(Variants& variants, World& world);
    void check_end_continuation_text(Variants& variants, World& world);
    void check_end_continuation_bounds(Variants& variants, World& world);
    void check_results_plain_text(Variants& variants, World& world);
    void check_results_plain_bounds(Variants& variants, World& world);
    void check_session_result_text(Variants& variants, World& world);
    void check_session_result_bounds(Variants& variants, World& world);
    void check_series_score_text(Variants& variants, const MenuText& defaults);
    void check_series_score_resources(Variants& variants, World& world, const Presentation& defaults);
    void check_round_heading_text(Variants& variants, const MenuText& defaults);
    void check_round_heading_resources(Variants& variants, World& world, const Presentation& defaults);
    void check_competitive_winner_text(Variants& variants, World& world);
    void check_competitive_winner_selection(const MenuText& defaults);
    void check_competitive_winner_bounds(Variants& variants, World& world);
    void check_contribution_team_text(Variants& variants, World& world);
    void check_contribution_team_roster(World& world, const Presentation& defaults);
    void check_contribution_team_bounds(Variants& variants, World& world);
    void check_result_percentage_text(Variants& variants, const MenuText& defaults);
    void check_result_percentage_numbers(const MenuText& defaults);
    void check_result_percentage_resources(Variants& variants, World& world, const Presentation& defaults);
}
