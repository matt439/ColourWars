#include "game/pch.h"
#include "game/presentation.h"
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

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

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
            L"Jump: A / LT (hold for height)\nPause: Start    Debug: Back / View\nKEYBOARD + MOUSE\n"
            L"Move: A / D    Drop: S / Down\nAim: mouse cursor    Shoot: left click\n"
            L"Jump: Space (hold for height)\nPause: Esc    Debug: F1\nMENUS\n"
            L"Navigate: stick / D-pad / arrows\nSelect: A / Enter    Back: B / Esc\nAIM / READINESS\n"
            L"Direction only; roller aim is fixed.\nReady: chevron / Cooldown: bars\nNo paint: cross\n"
            L"Pad sticks: movement / aim deadzones.\nREDUCE EFFECTS\nLess shake, flashes and idle detail.",
            &MenuHelpText::controls, true },
        { "pad_association", L"Profiles follow Pad 1-4 slots,\nnot the player number or device.\n"
            L"After reconnecting, check the slot.\nKeyboard/mouse is unchanged.", &MenuHelpText::pad_association, true },
        { "aim_cutoff", L"Right stick: radial cutoff", &MenuHelpText::aim_cutoff, false }
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
        { "sticks_pending", L"Apply to save pad deadzones.", &MenuStatusText::sticks_pending, false },
        { "sticks_saved", L"Pad deadzones applied and saved.", &MenuStatusText::sticks_saved, false },
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
    std::wstring stage_summary(const MenuText& text, const StageSummaryText& entry, int value)
    {
        return entry.detail ? text.stage_best_of_mode(value) : text.stage_roster(value);
    }
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
    void select_stage_briefing_font(rapidjson::Document& d, const StageBriefingText& entry, const char* name)
    {
        if (entry.detail) { d["menu_style"]["fonts"]["detail"].SetString(name, d.GetAllocator()); }
        else { d["menu_style"]["chrome"]["brand_font"].SetString(name, d.GetAllocator()); }
    }
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
        { "restore_defaults", L"Restore defaults" }, { "keep", L"Keep" }, { "revert", L"Revert" }
    };

    rapidjson::Document read_document(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read " + path.string());
        const std::string text{ std::istreambuf_iterator<char>(input), {} };
        rapidjson::Document result;
        result.Parse(text.c_str());
        require(!result.HasParseError(), "Cannot parse " + path.string());
        return result;
    }

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
            for (const char* name : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
                "gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
                "gill_sans_mt_bold_144", "courier_new_bold_16" })
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

    rapidjson::Value& text_field(rapidjson::Document& d, const char* group, const char* key)
    {
        return group == nullptr ? d["menu_text"][key] : d["menu_text"][group][key];
    }

    void check_defaults(const Presentation& presentation)
    {
        const MenuText& text = presentation.menu_text();
        require(text.values.percentage == L"{value}%", "Default setting percentage template changed");
        require(text.values.duration_minutes == L"{value} min" && text.values.duration_default == L"Default ({value})",
            "Default match-duration templates changed");
        require(text.values.resolution == L"{width}x{height}", "Default resolution template changed");
        require(text.brand == brand && text.solo_practice_hint == practice_hint && text.home_subtitle == home_subtitle &&
            text.options_palette_timing == palette_timing &&
            text.display_trial_countdown == L"Reverting in {seconds}s" &&
            text.headings.size() == std::size(headings) &&
            text.prompts.size() == std::size(prompts) && text.items.size() == std::size(items),
            "Shared menu text changed its brand or fixed role count");
        for (const auto& entry : title_text)
        {
            require(text.title.*entry.field == entry.text, std::string("Title default changed: ") + entry.key);
        }
        for (const auto& entry : help_text)
        {
            require(text.help.*entry.field == entry.text, std::string("Controls/Pad Sticks help default changed: ") + entry.key);
        }
        for (const auto& entry : status_text)
        {
            require(text.status.*entry.field == entry.text, std::string("Save-status default changed: ") + entry.key);
        }
        for (const auto& entry : connection_text)
        {
            require(text.connection.*entry.field == entry.text,
                std::string("Connection default changed: ") + entry.key);
        }
        for (const auto& entry : label_text)
        {
            require(text.labels.*entry.field == entry.text, std::string("Device/roster default changed: ") + entry.key);
        }
        for (const auto& entry : stick_readout_text)
        {
            require(text.stick_readouts.*entry.field == entry.text,
                std::string("Pad Sticks readout default changed: ") + entry.key);
        }
        for (int player = 1; player <= 4; ++player)
        {
            require(text.pause_title_for_player(player) == L"Pause Menu P" + std::to_wstring(player),
                "Default Pause title changed its roster player number");
        }
        for (const auto& entry : value_text)
        {
            require(text.values.*entry.field == entry.text, std::string("Fixed setting value default changed: ") + entry.key);
        }
        for (const auto& entry : confirmation_text)
        {
            require(text.confirmations.*entry.field == entry.text,
                std::string("Confirmation default changed: ") + entry.key);
        }
        for (const auto& entry : end_text)
        {
            require(text.end.*entry.field == entry.text, std::string("End heading default changed: ") + entry.key);
        }
        for (const auto& entry : end_continuation_text)
        {
            require(text.end.*entry.field == entry.text, std::string("End continuation default changed: ") + entry.key);
        }
        for (const auto& entry : results_text)
        {
            require(text.results.*entry.field == entry.text, std::string("Plain Results default changed: ") + entry.key);
        }
        for (const auto& entry : session_result_text)
        {
            require(text.results.*entry.field == entry.text, std::string("Session result default changed: ") + entry.key);
        }
        for (const auto& entry : competitive_winner_text)
            require(text.results.*entry.field == entry.text, std::string("Competitive winner default changed: ") + entry.key);
        for (const auto& entry : contribution_team_text)
            require(text.results.*entry.field == entry.text, std::string("Contribution team default or its outer spaces changed: ") + entry.key);
        require(text.results.series_score == L"Wins: A {team_a_wins} - B {team_b_wins}  |  First to {target}",
            "Default series-score template changed its literal labels, token names or double spaces");
        require(text.results.round_heading == L"Round {round}",
            "Default numbered Results heading changed its exact literal prefix or token");
        for (const auto& entry : stage_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena status default changed: ") + entry.key);
        }
        for (const auto& entry : stage_count_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena count default changed: ") + entry.key);
        }
        for (const auto& entry : stage_summary_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena summary default changed: ") + entry.key);
        }
        for (const auto& entry : record_text)
        {
            require(text.records.*entry.field == entry.text, std::string("Stage record default changed: ") + entry.key);
        }
        for (const auto& entry : stage_briefing_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena briefing default changed: ") + entry.key);
        }
        for (const auto& entry : stage_cue_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena cue default changed: ") + entry.key);
        }
        for (size_t index = 0; index < std::size(headings); ++index)
        {
            const auto& actual = text.heading(static_cast<MenuHeading>(index));
            require(actual == headings[index].text && &actual == &text.headings[index],
                std::string("Heading default changed: ") + headings[index].key);
        }
        for (size_t index = 0; index < std::size(prompts); ++index)
        {
            const auto& actual = text.prompt(static_cast<MenuPrompt>(index));
            require(actual == prompts[index].text && &actual == &text.prompts[index],
                std::string("Complete keyboard-expanded prompt default changed: ") + prompts[index].key);
        }
        for (const int invalid : { -1, static_cast<int>(MenuHeading::count), 999 })
        {
            bool rejected = false;
            try { text.heading(static_cast<MenuHeading>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared heading role resolved silently");
        }
        for (const int invalid : { -1, static_cast<int>(MenuPrompt::count), 999 })
        {
            bool rejected = false;
            try { text.prompt(static_cast<MenuPrompt>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared prompt role resolved silently");
        }
        for (size_t index = 0; index < std::size(items); ++index)
        {
            const auto& actual = text.item(static_cast<MenuItem>(index));
            require(actual == items[index].text && &actual == &text.items[index],
                std::string("Shared menu item default changed: ") + items[index].key);
        }
        for (const int invalid : { -1, static_cast<int>(MenuItem::count), 999 })
        {
            bool rejected = false;
            try { text.item(static_cast<MenuItem>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared item role resolved silently");
        }
    }

    void check_keyed_content(Variants& variants)
    {
        const auto reversed = variants.load([](rapidjson::Document& d)
        {
            for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end", "stage", "records", "connection", "stick_readouts", "labels" })
            {
                auto& object = d["menu_text"][group];
                rapidjson::Value reordered(rapidjson::kObjectType);
                auto member = object.MemberEnd();
                while (member != object.MemberBegin())
                {
                    --member;
                    rapidjson::Value name;
                    name.CopyFrom(member->name, d.GetAllocator());
                    rapidjson::Value value;
                    value.CopyFrom(member->value, d.GetAllocator());
                    reordered.AddMember(name, value, d.GetAllocator());
                }
                object.Swap(reordered);
            }
        });
        check_defaults(reversed);
        variants.rejects("", [](rapidjson::Document& d) { d.RemoveMember("menu_text"); });
        for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end", "stage", "records", "connection", "stick_readouts", "labels" })
        {
            variants.rejects(group, [group](rapidjson::Document& d) { d["menu_text"].RemoveMember(group); });
            variants.rejects(group, [group](rapidjson::Document& d) { d["menu_text"][group].SetArray(); });
        }
        const auto required = [&](const char* group, const char* key)
        {
            const std::string field = group == nullptr ? key : std::string(group) + '.' + key;
            variants.rejects(field, [group, key](rapidjson::Document& d)
                { (group == nullptr ? d["menu_text"] : d["menu_text"][group]).RemoveMember(key); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetBool(true); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetNull(); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetString(""); });
            variants.rejects(field, [group, key](rapidjson::Document& d)
                { text_field(d, group, key).SetString("bad\0text", 8, d.GetAllocator()); });
        };
        required(nullptr, "brand");
        required(nullptr, "solo_practice_hint");
        required(nullptr, "home_subtitle");
        required(nullptr, "display_trial_countdown");
        required(nullptr, "options_palette_timing");
        for (const auto& entry : title_text) { required("title", entry.key); }
        for (const auto& entry : help_text) { required("help", entry.key); }
        for (const auto& entry : status_text) { required("status", entry.key); }
        for (const auto& entry : connection_text) { required("connection", entry.key); }
        for (const auto& entry : label_text) { required("labels", entry.key); }
        for (const auto& entry : stick_readout_text) { required("stick_readouts", entry.key); }
        for (const auto& entry : value_text) { required("values", entry.key); }
        required("values", "percentage");
        required("values", "duration_minutes");
        required("values", "duration_default");
        required("values", "resolution");
        for (const auto& entry : confirmation_text) { required("confirmations", entry.key); }
        for (const auto& entry : end_text) { required("end", entry.key); }
        for (const auto& entry : end_continuation_text) { required("end", entry.key); }
        for (const auto& entry : record_text) { required("records", entry.key); }
        for (const auto& entry : stage_text) { required("stage", entry.key); }
        for (const auto& entry : stage_count_text) { required("stage", entry.key); }
        for (const auto& entry : stage_summary_text) { required("stage", entry.key); }
        for (const auto& entry : stage_briefing_text) { required("stage", entry.key); }
        for (const auto& entry : stage_cue_text) { required("stage", entry.key); }
        for (const auto& entry : headings) { required("headings", entry.key); }
        for (const auto& entry : prompts) { required("prompts", entry.key); }
        for (const auto& entry : items) { required("items", entry.key); }
    }

    struct TextRole { const char* group; const char* key; };
    constexpr TextRole categories[]{ {nullptr, "brand"}, {"headings", "home"}, {"prompts", "title"},
        {"items", "play"}, {nullptr, "solo_practice_hint"} };

    void select_font(rapidjson::Document& d, const TextRole& role, const char* font)
    {
        if (role.group == nullptr && std::string(role.key) == "solo_practice_hint")
        {
            d["menu_style"]["fonts"]["detail"].SetString(font, d.GetAllocator());
        }
        else if (role.group == nullptr) { d["menu_style"]["chrome"]["brand_font"].SetString(font, d.GetAllocator()); }
        else if (std::string(role.group) == "headings")
        {
            d["menu_style"]["fonts"]["heading"].SetString(font, d.GetAllocator());
        }
        else if (std::string(role.group) == "items")
        {
            d["menu_style"]["fonts"]["item"].SetString(font, d.GetAllocator());
        }
        else { d["prompt_bar"]["font"].SetString(font, d.GetAllocator()); }
    }

    void check_text_and_fonts(Variants& variants, World& world, const std::filesystem::path& content)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& role : categories)
            {
                text_field(d, role.group, role.key).SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        const std::wstring expected = L"  A \"B\"-C...  ";
        require(normalized.menu_text().brand == expected && normalized.menu_text().heading(MenuHeading::home) == expected &&
            normalized.menu_text().prompt(MenuPrompt::title) == expected && normalized.menu_text().item(MenuItem::play) == expected &&
            normalized.menu_text().solo_practice_hint == expected,
            "Shared menu text did not normalize punctuation/NBSP while preserving authored outer spaces");
        normalized.validate_resources(world.render, world.audio);
        const SpriteFontFile font = read_sprite_font_file((content / "fonts/gill_sans_mt_bold_24.spritefont").string());
        auto glyphs = font.glyphs;
        const auto removed = std::remove_if(glyphs.begin(), glyphs.end(),
            [](const Glyph& glyph) { return glyph.character == U'~'; });
        require(removed != glyphs.end(), "Missing-glyph fixture requires the shipped tilde glyph");
        glyphs.erase(removed, glyphs.end());
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), font.line_spacing);
        limited->set_stand_in(U'?');
        world.render.add_font("menu_text_limited", std::move(limited));
        world.render.add_font("menu_text_tall", std::make_unique<Font>(TextureHandle{}, font.glyphs, 1081.0f));
        for (const auto& role : categories)
        {
            const std::string field = role.group == nullptr ? role.key : std::string(role.group) + '.' + role.key;
            for (const char* text : { "   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            {
                variants.rejects(field, [role, text](rapidjson::Document& d)
                    { text_field(d, role.group, role.key).SetString(text, d.GetAllocator()); });
            }
            variants.rejects(field, [role](rapidjson::Document& d)
                { text_field(d, role.group, role.key).SetString(std::string(200, 'W').c_str(), d.GetAllocator()); }, &world);
            variants.rejects(field, [role](rapidjson::Document& d)
            {
                select_font(d, role, "menu_text_limited");
                text_field(d, role.group, role.key).SetString("Glyph~");
            }, &world);
            variants.rejects(field, [role](rapidjson::Document& d)
                { select_font(d, role, "menu_text_tall"); }, &world);
        }
    }

    void check_prompt_bar(World& world, const Presentation& look)
    {
        const auto& style = look.prompt_bar();
        for (size_t index = 0; index < std::size(prompts); ++index)
        {
            const std::wstring& legend = look.menu_text().prompt(static_cast<MenuPrompt>(index));
            PromptBar bar("authored_prompt", legend, style, &world.render);
            UiTextDropShadow reference("measured_prompt", legend, style.font, style.position, &world.render,
                style.colour, style.shadow_colour, style.shadow_offset);
            require(bar.text() == legend && bar.text_bounds() == reference.text_bounds() && bar.bounds() == reference.bounds(),
                "Actual authored PromptBar changed the complete legend, font or measured geometry");
            bar.set_hidden(true);
            DrawList no_device;
            bar.draw(no_device);
        }
        std::wstring source = L"  X/R Authored legend  ";
        PromptBar first("first", source, style, &world.render);
        source = L"A/Enter New legend";
        PromptBar next("next", source, style, &world.render);
        require(first.text() == L"  X/R Authored legend  " && next.text() == source,
            "PromptBar retained a borrowed legend or discarded complete authored spacing");
        const std::vector<PromptBar::Prompt> legacy{ {L"A", L"Select"}, {L"B", L"Back"} };
        PromptBar old("legacy", legacy, style, &world.render);
        require(old.text() == L"A Select    B Back", "Legacy PromptBar vector formatting changed");
    }

    void check_inclusive_bounds(Variants& variants, World& world)
    {
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_text_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_text_exact"), L"Fit") ==
            mattmath::Vector2F(24.0f, 20.0f), "Exact-boundary fixture font has unexpected metrics");
        const auto edge = [](rapidjson::Document& d)
        {
            d["prompt_bar"]["font"].SetString("menu_text_exact");
            for (const auto& entry : prompts) { d["menu_text"]["prompts"][entry.key].SetString("Fit"); }
            // 24x20 text plus a one-unit shadow, with the original 150-unit
            // right margin, exactly touches both inclusive design boundaries.
            d["prompt_bar"]["position"]["x"].SetInt(1745);
            d["prompt_bar"]["position"]["y"].SetInt(1059);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(1);
        };
        const auto fits = variants.load(edge);
        fits.validate_resources(world.render, world.audio);
        const auto reversed_shadow = variants.load([&](rapidjson::Document& d)
        {
            edge(d);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(-1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(-1);
        });
        reversed_shadow.validate_resources(world.render, world.audio);
        const auto top_left = [&](rapidjson::Document& d)
        {
            edge(d);
            d["prompt_bar"]["position"]["x"].SetInt(1);
            d["prompt_bar"]["position"]["y"].SetInt(1);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(-1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(-1);
        };
        const auto shadow_touches_edges = variants.load(top_left);
        shadow_touches_edges.validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("prompts.title", [&](rapidjson::Document& d)
            {
                top_left(d);
                d["prompt_bar"]["position"][axis].SetDouble(0.75);
            }, &world);
            variants.rejects("prompts.title", [&](rapidjson::Document& d)
            {
                edge(d);
                d["prompt_bar"]["position"][axis].SetDouble(std::string(axis) == "x" ? 1745.25 : 1059.25);
            }, &world);
            for (const double invalid : { -1.0, 1e100 })
            {
                variants.rejects("prompts.title", [&](rapidjson::Document& d)
                {
                    edge(d);
                    d["prompt_bar"]["position"][axis].SetDouble(invalid);
                }, &world);
            }
        }
    }

    UiText* row_text(OptionRow& row, const std::string& name)
    {
        for (const auto& child : row.children())
        {
            if (child.first == name) { return dynamic_cast<UiText*>(child.second); }
        }
        return nullptr;
    }

    MenuPaperArtPresentation options_art(const MainMenuPresentation& main)
    {
        MenuPaperArtPresentation art = main.art;
        art.focus_padding = main.setup_focus_padding;
        art.shadow_offset = main.setup_shadow_offset;
        return art;
    }

    void check_option_row_fitting(Variants& variants, World& world)
    {
        const std::string authored(200, 'W');
        const auto look = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : { "resolution", "fullscreen", "team_palette" })
            {
                d["menu_text"]["items"][key].SetString(authored.c_str(), d.GetAllocator());
            }
        });
        // These labels are not plain text actions. Resource validation checks
        // their glyphs and fitted geometry, preserving the actual OptionRow's
        // ability to fit a long, otherwise valid label.
        look.validate_resources(world.render, world.audio);
        for (const char* key : { "resolution", "fullscreen", "team_palette" })
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        const MenuStyle& style = look.menu();
        const auto art = options_art(look.main_menu());
        const mattmath::Vector2F label_position(100.0f, 100.0f);
        const mattmath::Vector2F value_position(420.0f, 100.0f);
        const float label_column = value_position.x - label_position.x - art.focus_padding.x;
        const auto font = world.render.resolve_sprite_font(style.item_font);
        for (const MenuItem item : { MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette })
        {
            int adjustments = 0;
            const std::wstring& text = look.menu_text().item(item);
            const auto original = world.render.measure_text(font, text);
            require(original.x > label_column, "Long OptionRow fixture already fits without scaling");
            OptionRow row("fitted", text, label_position, value_position, style, art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; },
                &art, {160.0f, 1000.0f});
            UiText* label = row_text(row, "fitted_label");
            UiText* value = row_text(row, "fitted_value");
            require(label != nullptr && value != nullptr && label->text() == text &&
                std::isfinite(label->bounds().width) && label->bounds().width > 0.0f &&
                label->bounds().width <= label_column + 0.01f && label->bounds().width < original.x,
                "Actual OptionRow truncated or failed to fit its long authored label to its column");
            const auto retained_label = label->bounds();
            row.set_value(L"Current value");
            require(value->text() == L"Current value" && value->bounds().width <= 160.01f &&
                label->bounds() == retained_label && label->text() == text,
                "OptionRow value updates changed its fitted label or escaped the supplied value width");
            row.adjust(Direction::right);
            require(adjustments == 1, "Fitted OptionRow lost its right adjustment action");
            row.adjust(Direction::left);
            require(adjustments == 0, "Fitting an authored OptionRow label changed its adjustment actions");
        }

        const std::wstring short_label = L"Height constrained";
        const auto original = world.render.measure_text(font, short_label);
        require(original.y > 10.0f, "OptionRow height fixture already fits without scaling");
        OptionRow height_row("height", short_label, label_position, {10000.0f, 100.0f}, style,
            art.ink, &world.render, {}, &art, {160.0f, 10.0f});
        const UiText* fitted = row_text(height_row, "height_label");
        require(fitted != nullptr && fitted->text() == short_label && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 10.01f && fitted->bounds().height < original.y &&
            fitted->bounds().width < original.x,
            "Actual OptionRow did not preserve and shrink a height-constrained authored label");
    }

    void check_item_bounds(Variants& variants, World& world)
    {
        for (const char* key : { "play", "options", "credits", "exit", "controls_audio", "apply", "back",
            "single_match", "best_of_three", "best_of_five", "match_length", "one_player_practice",
            "two_players", "three_players", "four_players", "resume", "restart", "quit", "yes", "no",
            "change_teams", "change_weapons", "change_level", "exit_to_main_menu", "restore_defaults", "keep", "revert" })
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
                { d["menu_text"]["items"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        const auto original = variants.load([](rapidjson::Document&) {});
        const float row_height = 20.0f + original.main_menu().art.focus_padding.y * 2.0f;
        const float paper_right = (1920.0f + original.main_menu().home_box_size.x) / 2.0f;
        const float right_position = paper_right - 24.0f - original.main_menu().art.focus_padding.x;
        const auto exact_items = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
        };
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            d["menu_text"]["home_subtitle"].SetString("Fit");
            d["main_menu"]["home_spacing"]["y"].SetFloat(row_height);
            d["main_menu"]["home_subtitle_position"]["y"].SetFloat(
                original.main_menu().home_position.y + row_height / 2.0f);
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.play", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["main_menu"]["home_spacing"]["y"].SetFloat(row_height - 0.25f);
        }, &world);

        const auto paper_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            d["main_menu"]["home_position"]["x"].SetFloat(right_position);
        };
        const auto fits_paper = variants.load(paper_edge);
        fits_paper.validate_resources(world.render, world.audio);
        variants.rejects("items.play", [&](rapidjson::Document& d)
        {
            paper_edge(d);
            d["main_menu"]["home_position"]["x"].SetFloat(right_position + 0.25f);
        }, &world);
    }

    void check_restore_defaults_text(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& main = original.main_menu();
        const auto art = options_art(main);
        require(main.options_spacing == mattmath::Vector2F(540.0f, 100.0f) &&
            art.focus_padding == mattmath::Vector2F(28.0f, 7.0f) && art.shadow_offset == 2.0f &&
            original.menu().item_font == "gill_sans_mt_bold_48",
            "Seven-row Options changed its reviewed compact geometry or existing item font");
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["items"]["restore_defaults"].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().item(MenuItem::restore_defaults) == L"  A \"B\"-C...  ",
            "Restore defaults lost normalized punctuation or authored spaces");
        for (const char* invalid : { " \xC2\xA0 ", "line\nbreak", "line\rbreak", "tab\ttext", "control\x01", "caf\xC3\xA9" })
        {
            variants.rejects("items.restore_defaults", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["items"]["restore_defaults"].SetString(invalid, d.GetAllocator());
            });
        }
        variants.rejects("items.restore_defaults", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["items"]["restore_defaults"].SetString("Glyph~");
        }, &world);
        for (const int width : { 1472, 1473 })
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? width : 8, 16),
                    0.0f, 0.0f, 0.0f });
            }
            const std::string name = "restore_defaults_width_" + std::to_string(width);
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
            const auto edit = [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString(name.c_str(), d.GetAllocator());
                d["menu_text"]["items"]["restore_defaults"].SetString("~");
            };
            if (width == 1473) { variants.rejects("items.restore_defaults", edit, &world); continue; }
            const auto edge = variants.load(edit);
            edge.validate_resources(world.render, world.audio);
            std::wstring source = edge.menu_text().item(MenuItem::restore_defaults);
            UiText text("restore_defaults", source, name,
                main.options_position + mattmath::Vector2F(0.0f, main.options_spacing.y * 4.0f), &world.render, art.ink);
            source = L"Changed after construction";
            auto card = text.bounds(); card.inflate(art.focus_padding);
            require(text.text() == L"~" && text.bounds().width == 1472.0f && text.bounds().y == 620.0f &&
                card.right() == (1920.0f + main.options_box_size.x) / 2.0f,
                "Plain Restore defaults borrowed its copy, fitted its width or missed row4's inclusive card/shade edge");
            text.set_hidden(true); DrawList no_device; text.draw(no_device);
        }
        // Only row6 Back reaches the paper bottom; a stale six-row mapping
        // would accept the overflow variant below.
        const auto back_bottom = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            d["prompt_bar"]["font"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
            d["main_menu"]["options_position"]["y"].SetFloat(313.0f);
            d["main_menu"]["options_status_position"]["y"].SetFloat(914.0f);
        };
        variants.load(back_bottom).validate_resources(world.render, world.audio);
        variants.rejects("items.back", [&](rapidjson::Document& d)
        {
            back_bottom(d);
            d["main_menu"]["options_position"]["y"].SetFloat(313.25f);
        }, &world);
    }

    void check_setup_text(Variants& variants, World& world)
    {
        constexpr const char* setup_keys[]{ "single_match", "best_of_three", "best_of_five", "match_length",
            "one_player_practice", "two_players", "three_players", "four_players" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : setup_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(setup_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::single_match) + index)) == L"A -B",
                "Mode/player item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : setup_keys)
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        const auto exact_setup = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(1.5f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(1.5f);
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& look = original.main_menu();
        const float row_height = 20.0f + 1.5f + look.setup_focus_padding.y * 2.0f + look.setup_shadow_offset;
        for (const char* spacing : { "mode_select_spacing", "player_count_spacing" })
        {
            const auto row_edge = [&](rapidjson::Document& d)
            {
                exact_setup(d);
                d["main_menu"][spacing]["y"].SetFloat(row_height);
            };
            const auto fits = variants.load(row_edge);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(std::string("items.") + (std::string(spacing) == "mode_select_spacing" ?
                "single_match" : "one_player_practice"), [&](rapidjson::Document& d)
            {
                row_edge(d);
                d["main_menu"][spacing]["y"].SetFloat(std::nextafter(row_height, 0.0f));
            }, &world);
            for (const double invalid : { 0.0, -1.0, 1e100 })
            {
                variants.rejects(std::string("items.") + (std::string(spacing) == "mode_select_spacing" ?
                    "single_match" : "one_player_practice"), [&](rapidjson::Document& d)
                {
                    exact_setup(d);
                    d["main_menu"][spacing]["y"].SetDouble(invalid);
                }, &world);
            }
        }
        const auto column_edge = [&](rapidjson::Document& d)
        {
            exact_setup(d);
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(12.75f);
        };
        const auto fits_column = variants.load(column_edge);
        fits_column.validate_resources(world.render, world.audio);
        variants.rejects("items.match_length", [&](rapidjson::Document& d)
        {
            column_edge(d);
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(std::nextafter(12.75f, 0.0f));
        }, &world);
        for (const double invalid : { 0.0, -1.0, 1e100 })
        {
            variants.rejects("items.match_length", [&](rapidjson::Document& d)
            {
                exact_setup(d);
                d["main_menu"]["mode_select_spacing"]["x"].SetDouble(invalid);
            }, &world);
        }

        const auto hint_edge = [&](rapidjson::Document& d)
        {
            exact_setup(d);
            // Keep unrelated complete Team/Weapon rows inside their paper while
            // this fixture moves the shared origin to the Mode hint edge.
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            d["main_menu"]["team_select"]["spacing"]["y"].SetFloat(110.0f);
            d["menu_style"]["layout"]["position"]["y"].SetFloat(204.5f);
            d["main_menu"]["mode_select_spacing"]["y"].SetInt(100);
        };
        const auto fits_hint = variants.load(hint_edge);
        fits_hint.validate_resources(world.render, world.audio);
        variants.rejects("solo_practice_hint", [&](rapidjson::Document& d)
        {
            hint_edge(d);
            d["menu_style"]["layout"]["position"]["y"].SetFloat(204.75f);
        }, &world);

        const auto left_edge = [&](rapidjson::Document& d)
        {
            exact_setup(d);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(149.5f);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
        };
        const auto touches_left = variants.load(left_edge);
        touches_left.validate_resources(world.render, world.audio);
        variants.rejects("items.single_match", [&](rapidjson::Document& d)
        {
            left_edge(d);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(149.25f);
        }, &world);
    }

    void check_options_palette_timing(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& main = original.main_menu();
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["options_palette_timing"].SetString("  A\xC2\xA0\xE2\x80\x94" "B\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().options_palette_timing == L"  A -B...  ",
            "Paused Options caption lost normalized punctuation or authored outer spaces");
        for (const char* invalid : { " \xC2\xA0 ", "line\nbreak", "line\rbreak", "tab\ttext", "control\x01", "caf\xC3\xA9", "\xC3" })
        {
            variants.rejects("options_palette_timing", [invalid](rapidjson::Document& d)
                { d["menu_text"]["options_palette_timing"].SetString(invalid, d.GetAllocator()); });
        }
        variants.rejects("options_palette_timing", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
            d["menu_text"]["options_palette_timing"].SetString("Glyph~");
        }, &world);
        variants.rejects("options_palette_timing", [](rapidjson::Document& d)
        {
            d["menu_text"]["options_palette_timing"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);

        const float right = (1920.0f + main.options_box_size.x) / 2.0f;
        const float row_top = main.options_position.y - options_art(main).focus_padding.y;
        const float width = right - main.options_value_x;
        const float height = row_top - main.options_heading_position.y;
        const auto add_font = [&](const std::string& name, int glyph_width, int glyph_height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool edge = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, edge ? glyph_width : 8,
                    edge ? glyph_height : 16), 0.0f, 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        for (const bool width_edge : { true, false })
        {
            const std::string name = width_edge ? "palette_timing_width_edge" : "palette_timing_height_edge";
            add_font(name, width_edge ? static_cast<int>(width) : 8, width_edge ? 20 : static_cast<int>(height));
            add_font(name + "_overflow", width_edge ? static_cast<int>(width) + 1 : 8,
                width_edge ? 20 : static_cast<int>(height) + 1);
            const auto edit = [&](rapidjson::Document& d, const std::string& font)
            {
                d["menu_style"]["fonts"]["detail"].SetString(font.c_str(), d.GetAllocator());
                d["menu_text"]["options_palette_timing"].SetString("~");
            };
            const auto edge = variants.load([&](rapidjson::Document& d) { edit(d, name); });
            edge.validate_resources(world.render, world.audio);
            std::wstring source = edge.menu_text().options_palette_timing;
            UiText caption("palette_timing", source, edge.menu().detail_font,
                { main.options_value_x, main.options_heading_position.y }, &world.render, main.art.ink);
            source = L"Changed after construction";
            require(caption.text() == L"~" && caption.bounds().x == main.options_value_x &&
                caption.bounds().y == main.options_heading_position.y &&
                (width_edge ? caption.bounds().right() == right : caption.bounds().bottom() == row_top),
                "Paused Options caption borrowed its copy, fitted its text or missed inclusive paper/card contact");
            caption.set_hidden(true); DrawList no_device; caption.draw(no_device);
            variants.rejects("options_palette_timing", [&](rapidjson::Document& d) { edit(d, name + "_overflow"); }, &world);
        }
        const int heading_width = static_cast<int>(main.options_value_x - main.options_heading_position.x);
        add_font("palette_timing_heading_edge", heading_width, 20);
        add_font("palette_timing_heading_overflow", heading_width + 1, 20);
        const auto heading = [](rapidjson::Document& d, const char* font)
        {
            d["menu_style"]["fonts"]["heading"].SetString(font, d.GetAllocator());
            d["menu_text"]["headings"]["options"].SetString("~");
            d["menu_text"]["options_palette_timing"].SetString("Fit");
        };
        const auto heading_edge = variants.load([&](rapidjson::Document& d) { heading(d, "palette_timing_heading_edge"); });
        heading_edge.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font(heading_edge.menu().heading_font), L"~").x ==
            main.options_value_x - main.options_heading_position.x,
            "Paused Options heading fixture lost exact contact with the caption anchor");
        variants.rejects("options_palette_timing", [&](rapidjson::Document& d)
            { heading(d, "palette_timing_heading_overflow"); }, &world);
    }

    void check_pause_text(Variants& variants, World& world)
    {
        struct PauseRole { const char* key; MenuItem item; };
        constexpr PauseRole pause_roles[]{ { "resume", MenuItem::resume }, { "restart", MenuItem::restart },
            { "options", MenuItem::options }, { "quit", MenuItem::quit }, { "yes", MenuItem::yes }, { "no", MenuItem::no } };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const auto& role : pause_roles)
            {
                d["menu_text"]["items"][role.key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& role : pause_roles)
        {
            require(normalized.menu_text().item(role.item) == L"A -B",
                "Pause item did not normalize its authored punctuation and NBSP");
        }
        for (const auto& role : pause_roles)
        {
            const char* key = role.key;
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
            for (const char* invalid : { "   ", "line\nbreak", "tab\ttext", "\xC3\xA9", "\xC3" })
            {
                variants.rejects(std::string("items.") + key, [key, invalid](rapidjson::Document& d)
                {
                    d["menu_text"]["items"][key].SetString(invalid, d.GetAllocator());
                });
            }
        }

        const auto exact_items = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& pause = original.pause_menu();
        const auto options_size = world.render.measure_text(world.render.resolve_sprite_font(original.menu().item_font),
            original.menu_text().item(MenuItem::options));
        const float options_height = options_size.y + pause.art.focus_padding.y * 2.0f;
        require(pause.layout.spacing.y == 120.0f && options_size.y == 82.0f && options_height == 118.0f,
            "Pause Options lost its actual descender height or the authored two-pixel row clearance");
        const auto tight_options = variants.load([&](rapidjson::Document& d)
            { d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(options_height); });
        tight_options.validate_resources(world.render, world.audio);
        variants.rejects("items.options", [&](rapidjson::Document& d)
        {
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(std::nextafter(options_height, 0.0f));
        }, &world);
        const auto top = (mattmath::Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f;
        const auto bottom = top + pause.box_size;
        const float left_position = top.x + pause.art.focus_padding.x;
        const float right_position = bottom.x - 24.0f - pause.art.focus_padding.x;
        const float bottom_position = bottom.y - 3.0f * pause.layout.spacing.y -
            20.0f - pause.art.focus_padding.y;
        const auto paper_edge = [&](const char* axis, float position, const char* field, float outside)
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["pause_menu"]["layout"]["position"][axis].SetFloat(position);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                edit(d);
                d["pause_menu"]["layout"]["position"][axis].SetFloat(outside);
            }, &world);
        };
        paper_edge("x", left_position, "items.resume", left_position - 0.25f);
        paper_edge("x", right_position, "items.resume", right_position + 0.25f);
        // A positive-height confirmation question must fit above the first
        // padded action, so that action cannot also touch the paper's top.
        // Exit heading paper-top and question/action contact are checked below.
        // Quit's last row reaches the full-height Pause paper, below the menu footer.
        paper_edge("y", bottom_position, "items.quit", bottom_position + 0.25f);

        const float row_height = 20.0f + 2.0f * pause.art.focus_padding.y;
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(row_height);
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.resume", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(std::nextafter(row_height, 0.0f));
        }, &world);
        for (const double invalid : { 0.0, -1.0, 1e100 })
        {
            variants.rejects("items.resume", [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["pause_menu"]["layout"]["spacing"]["y"].SetDouble(invalid);
            }, &world);
        }
        // The same short label fits Options; only its independent Pause placement overflows.
        variants.rejects("items.options", [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["pause_menu"]["layout"]["position"]["x"].SetFloat(right_position);
            d["menu_text"]["items"]["options"].SetString("FitFit");
        }, &world);
    }

    void select_connection_font(rapidjson::Document& d, const char* key, const char* font)
    {
        const std::string role(key);
        const char* field = role == "join_reconnect_suffix" ? "item" :
            role == "pause_title" || role == "pause_player_title" || role == "controller_lost" ? "heading" : "detail";
        d["menu_style"]["fonts"][field].SetString(font, d.GetAllocator());
    }

    void compact_connection_copy(rapidjson::Document& d)
    {
        for (const auto& entry : connection_text)
        {
            d["menu_text"]["connection"][entry.key].SetString("Fit");
        }
        d["menu_text"]["connection"]["pause_player_title"].SetString("P{player}");
        for (const char* role : {"heading", "item", "detail"})
        {
            d["menu_style"]["fonts"][role].SetString("menu_text_exact");
        }
    }

    void check_connection_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : connection_text)
            {
                d["menu_text"]["connection"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
            d["menu_text"]["connection"]["pause_player_title"].SetString(
                "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94{player}\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : connection_text)
        {
            require(normalized.menu_text().connection.*entry.field ==
                (std::string(entry.key) == "pause_player_title" ? L"  A \"B\"-{player}...  " : L"  A \"B\"-C...  "),
                std::string("Connection copy lost normalization or authored outer spaces: ") + entry.key);
            const std::string field = std::string("connection.") + entry.key;
            for (const char* invalid : {"   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["connection"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                select_connection_font(d, entry.key, "menu_text_limited");
                d["menu_text"]["connection"][entry.key].SetString(
                    std::string(entry.key) == "pause_player_title" ? "Glyph~{player}" : "Glyph~", d.GetAllocator());
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string long_copy = std::string(2000, 'W') +
                    (std::string(entry.key) == "pause_player_title" ? "{player}" : "");
                d["menu_text"]["connection"][entry.key].SetString(long_copy.c_str(), d.GetAllocator());
            }, &world);
        }
        for (int number = 1; number <= 4; ++number)
        {
            require(normalized.menu_text().pause_title_for_player(number) ==
                L"  A \"B\"-" + std::to_wstring(number) + L"...  ",
                "Pause player formatter changed normalized literals or did not replace its token");
        }
        for (const char* malformed : {"Pause P1", "{Player}", "{player }", "{player}{player}",
            "{player} {seconds}", "{{player}}", "{player} }", "{ {player}", "{player", "player}"})
        {
            variants.rejects("connection.pause_player_title", [&](rapidjson::Document& d)
                { d["menu_text"]["connection"]["pause_player_title"].SetString(malformed, d.GetAllocator()); });
        }
        for (const wchar_t* malformed : {L"Pause P1", L"{player}{player}", L"{player} {other}", L"{{player}}"})
        {
            MenuText mutated = normalized.menu_text();
            mutated.connection.pause_player_title = malformed;
            bool rejected = false;
            try { mutated.pause_title_for_player(1); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Directly mutated Pause formatter accepted an invalid template");
        }
        for (const char* format : {"{player}", "{player} AFTER", "BEFORE {player}", "BEFORE {player} AFTER"})
        {
            const auto copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["connection"]["pause_player_title"].SetString(format, d.GetAllocator()); });
            copy.validate_resources(world.render, world.audio);
            for (int number = 1; number <= 4; ++number)
            {
                const std::wstring expected = std::string(format) == "{player}" ? std::to_wstring(number) :
                    std::string(format) == "{player} AFTER" ? std::to_wstring(number) + L" AFTER" :
                    std::string(format) == "BEFORE {player}" ? L"BEFORE " + std::to_wstring(number) :
                    L"BEFORE " + std::to_wstring(number) + L" AFTER";
                require(copy.menu_text().pause_title_for_player(number) == expected,
                    "Pause token at a supported position changed its literal prefix or suffix");
            }
        }
        for (int number : {(std::numeric_limits<int>::min)(), -1, 0, 5, 99, (std::numeric_limits<int>::max)()})
        {
            bool rejected = false;
            try { normalized.menu_text().pause_title_for_player(number); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Pause title formatter accepted a number outside roster players 1-4");
        }
        // P4 must be validated after substitution, even if the original
        // template's letters/braces and the first three digits are drawable.
        std::vector<Glyph> missing_four;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character != U'4')
            {
                missing_four.push_back({character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f});
            }
        }
        world.render.add_font("menu_connection_no_four",
            std::make_unique<Font>(TextureHandle{}, std::move(missing_four), 20.0f));
        variants.rejects("connection.pause_player_title", [](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            select_connection_font(d, "pause_player_title", "menu_connection_no_four");
        }, &world);
    }

    template<class Action> void rejects_label_format(Action action)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid device/roster template or argument was formatted silently");
    }

    void check_label_text(Variants& variants, World& world, const MenuText& defaults)
    {
        // Literal expectations retain the original slot identities and roster
        // numbers independently of either production label formatter.
        constexpr const wchar_t* devices[]{L"Waiting to join", L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse"};
        for (int slot = -1; slot <= 4; ++slot)
        {
            const std::wstring device = devices[slot + 1];
            require(defaults.device_label(slot) == device, "Default device wording or pad slot numbering changed");
            for (int player = 0; player < 4; ++player)
            {
                require(defaults.roster_label(player, slot) == L"P" + std::to_wstring(player + 1) + L" - " + device,
                    "Default roster label used pad numbering in place of the roster player");
            }
        }
        for (int invalid : {-2, 5, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
        {
            rejects_label_format([&] { defaults.device_label(invalid); });
            rejects_label_format([&] { defaults.roster_label(0, invalid); });
        }
        for (int invalid : {-1, 4, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
        {
            rejects_label_format([&] { defaults.roster_label(invalid, 0); });
        }
        const auto authored = variants.load([](rapidjson::Document& d)
        {
            auto& labels = d["menu_text"]["labels"];
            labels["pad"].SetString("  Seat\xC2\xA0{pad}  ");
            labels["keyboard_mouse"].SetString("Keys\xE2\x80\x94mouse");
            labels["waiting"].SetString("Open\xE2\x80\xA6");
            labels["roster_row"].SetString("{device} / {player}");
        });
        authored.validate_resources(world.render, world.audio);
        require(authored.menu_text().labels.pad == L"  Seat {pad}  " &&
            authored.menu_text().labels.keyboard_mouse == L"Keys-mouse" &&
            authored.menu_text().labels.waiting == L"Open...",
            "Device labels lost normalized punctuation/NBSP or authored outer spaces");
        for (int slot = -1; slot <= 4; ++slot)
        {
            const std::wstring device = slot == -1 ? L"Open..." : slot == 4 ? L"Keys-mouse" :
                L"  Seat " + std::to_wstring(slot + 1) + L"  ";
            require(authored.menu_text().device_label(slot) == device, "Authored device label was replaced by fixed wording");
            for (int player = 0; player < 4; ++player)
            {
                require(authored.menu_text().roster_label(player, slot) == device + L" / P" + std::to_wstring(player + 1),
                    "Authored roster template lost its original token order or device copy");
            }
            if (slot >= 0 && slot < 4)
            {
                require(authored.menu_text().stick_connection(slot, true) == device + L" - connected" &&
                    authored.menu_text().stick_connection(slot, false) == device + L" - disconnected",
                    "Pad Sticks connection formatter ignored the captured authored device label");
            }
        }
        for (const auto& entry : label_text)
        {
            const std::string field = std::string("labels.") + entry.key;
            for (const char* invalid : {"   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["labels"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            const bool pad = entry.field == &MenuLabelText::pad;
            const bool row = entry.field == &MenuLabelText::roster_row;
            const std::vector<const char*> malformed = pad ? std::vector<const char*>{
                "Pad", "{Pad}", "{pad }", "{pad}{pad}", "{pad}{device}", "{{pad}}", "{pad", "pad}", "{pa{pad}d}"} :
                row ? std::vector<const char*>{"{player}", "{device}", "{Player}{device}", "{player }{device}",
                    "{player}{player}{device}", "{player}{device}{device}", "{player}{device}{pad}",
                    "{{player}}{device}", "{player}{device", "{de{player}vice}", "{pla{device}yer}"} :
                std::vector<const char*>{"{pad}", "{player}", "{device}", "Plain {", "Plain }", "{}"};
            for (const char* invalid : malformed)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["labels"][entry.key].SetString(invalid, d.GetAllocator()); });
                MenuText mutated = defaults;
                mutated.labels.*entry.field = std::wstring(invalid, invalid + std::strlen(invalid));
                rejects_label_format([&] { mutated.roster_label(3, pad ? 2 : row ? 0 :
                    entry.field == &MenuLabelText::waiting ? -1 : 4); });
                if (!row)
                {
                    rejects_label_format([&] { mutated.device_label(pad ? 2 :
                        entry.field == &MenuLabelText::waiting ? -1 : 4); });
                }
            }
            for (const wchar_t* invalid : {L"", L"   ", L"first\nsecond", L"first\rsecond", L"tab\ttext", L"bad\x7f"})
            {
                MenuText mutated = defaults;
                mutated.labels.*entry.field = invalid;
                rejects_label_format([&] { mutated.roster_label(0, pad ? 0 : row ? 0 :
                    entry.field == &MenuLabelText::waiting ? -1 : 4); });
            }
        }
    }

    void check_label_resources(Variants& variants, World& world)
    {
        for (const auto& entry : label_text)
        {
            const std::string field = std::string("labels.") + entry.key;
            const bool pad = entry.field == &MenuLabelText::pad;
            const bool row = entry.field == &MenuLabelText::roster_row;
            const std::string token = pad ? "{pad}" : row ? "{player}{device}" : "";
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string text = std::string(500, 'W') + token;
                d["menu_text"]["labels"][entry.key].SetString(text.c_str(), d.GetAllocator());
            }, &world);
            for (const char* role : {"item", "detail", "results"})
            {
                if (entry.field == &MenuLabelText::waiting && std::string(role) != "item") { continue; }
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    if (std::string(role) == "results")
                    {
                        d["results_menu"]["contribution_font"].SetString("menu_text_limited");
                    }
                    else { d["menu_style"]["fonts"][role].SetString("menu_text_limited"); }
                    d["menu_text"]["labels"][entry.key].SetString((token + '~').c_str(), d.GetAllocator());
                }, &world);
            }
        }
        for (const char* role : {"item_font", "help_font"})
        {
            variants.rejects("labels.pad", [&](rapidjson::Document& d)
            {
                d["controls_settings"][role].SetString("menu_text_limited");
                d["menu_text"]["labels"]["pad"].SetString("{pad}~");
            }, &world);
        }
        const auto add_font = [&](const char* name, int tilde_width)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0, character == U'~' ? tilde_width : 8, 16),
                    0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        add_font("menu_labels_pause_edge", 489);
        add_font("menu_labels_pause_over", 490);
        const auto pause = [](rapidjson::Document& d, const char* font)
        {
            d["menu_style"]["fonts"]["detail"].SetString(font, d.GetAllocator());
            d["menu_text"]["labels"]["pad"].SetString("{pad}");
            d["menu_text"]["labels"]["keyboard_mouse"].SetString("K");
            d["menu_text"]["labels"]["waiting"].SetString("W");
            d["menu_text"]["labels"]["roster_row"].SetString("{player}{device}~");
            d["main_menu"]["team_select"]["spacing"]["x"].SetFloat(273.0f);
            d["main_menu"]["weapon_select"]["spacing"]["x"].SetFloat(273.0f);
        };
        const auto edge = variants.load([&](rapidjson::Document& d) { pause(d, "menu_labels_pause_edge"); });
        edge.validate_resources(world.render, world.audio);
        // Pause is plain UiText. Its three-space separator and LF are part of
        // the complete measured list; a one-row-only bound misses this edge.
        UiText missing("missing", L"P11~   P22~\nP33~   P44~", edge.menu().detail_font,
            mattmath::Vector2F(435.0f, 315.0f), &world.render, Colour::black);
        require(missing.bounds() == mattmath::RectangleF(435.0f, 315.0f, 1050.0f, 40.0f),
            "Complete Pause missing-pad list lost its exact full-paper width, separator or second line");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d)
            { pause(d, "menu_labels_pause_over"); }, &world);

        add_font("menu_labels_weapon_edge", 466);
        const auto weapon = [](rapidjson::Document& d, bool outside)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_labels_weapon_edge");
            d["menu_text"]["labels"]["pad"].SetString("{pad}");
            d["menu_text"]["labels"]["keyboard_mouse"].SetString("K");
            d["menu_text"]["labels"]["waiting"].SetString("W");
            d["menu_text"]["labels"]["roster_row"].SetString("{player}{device}~");
            d["main_menu"]["team_select"]["spacing"]["x"].SetFloat(273.0f);
            d["main_menu"]["weapon_select"]["spacing"]["x"].SetFloat(outside ? 249.875f : 250.0f);
        };
        const auto weapon_edge = variants.load([&](rapidjson::Document& d) { weapon(d, false); });
        weapon_edge.validate_resources(world.render, world.audio);
        UiTextDropShadow weapon_label("weapon-label", L"P11~", weapon_edge.menu().detail_font,
            mattmath::Vector2F(150.0f, 400.0f), &world.render, Colour::black, Colour());
        UiTextDropShadow authored_shadow("team-label", L"P11~", weapon_edge.menu().detail_font,
            mattmath::Vector2F(150.0f, 400.0f), &world.render, Colour::black, Colour(), weapon_edge.menu().item_shadow_offset);
        // The weapon tile starts at 650 minus six units of icon padding and
        // the two-unit rule. Its implicit shadow differs from Team's 1.5.
        require(weapon_label.bounds() == mattmath::RectangleF(150.0f, 400.0f, 492.0f, 22.0f) &&
            authored_shadow.bounds().right() == 641.5f && weapon_label.bounds().right() == 642.0f,
            "Weapon label lost its retained default two-unit shadow at the exact padded tile boundary");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d) { weapon(d, true); }, &world);
    }

    void check_label_results_extrema(Variants& variants, World& world)
    {
        // Nonuniform ink, advances, bearings and heights exercise complete
        // composed bounds. Tiny bearings also retain sequential float rounding.
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool digit = character >= U'0' && character <= U'9';
            const int width = character == U'8' ? 80 : digit ? 1 : 16;
            glyphs.push_back({character, mattmath::RectangleI(0, 0, width, character == U'8' ? 24 : 16),
                character == U'8' ? 0.00003f : character == U'6' ? -5.0f : 0.0f, 0.0f,
                character == U'8' ? -80.0f : character == U'1' || character == U'7' ? 19.0f : 0.0f});
        }
        world.render.add_font("menu_labels_results_extrema", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const Font& font = *world.render.font(world.render.resolve_sprite_font("menu_labels_results_extrema"));
        constexpr const wchar_t* devices[]{L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse"};
        float maximum = 0.0f, maximum_at_100 = 0.0f;
        std::wstring attained;
        // One font and literal complete strings form an independent oracle for
        // every legal displayed suffix, rather than copying the glyph walker.
        for (int player = 1; player <= 4; ++player)
        {
            for (const wchar_t* device : devices)
            {
                for (const wchar_t* team : {L" | Team A | ", L" | Team B | "})
                {
                    const std::wstring prefix = L"P" + std::to_wstring(player) + L" - " + device + team;
                    maximum_at_100 = std::max(maximum_at_100, font.measure(prefix + L"100.0%").x);
                    for (int tenths = 0; tenths <= 1000; ++tenths)
                    {
                        const std::wstring text = prefix + std::to_wstring(tenths / 10) + L'.' +
                            std::to_wstring(tenths % 10) + L'%';
                        const float width = font.measure(text).x;
                        if (width > maximum) { maximum = width; attained = text; }
                    }
                }
            }
        }
        require(maximum > maximum_at_100 && font.measure(attained).y == 24.0f,
            "Results fixture failed to distinguish 100.0% from the widest legal percentage or variable glyph height");
        require(percentage_text(68.499f) == L"68.4%" && percentage_text(99.999f) == L"99.9%" &&
            percentage_text(100.0f) == L"100.0%", "Results numeric formatter changed truncation to rounding");
        const float x = 1610.0f - maximum;
        const auto edit = [&](rapidjson::Document& d, bool outside)
        {
            d["results_menu"]["contribution_font"].SetString("menu_labels_results_extrema");
            d["results_menu"]["contribution_position"]["x"].SetFloat(x + (outside ? 0.25f : 0.0f));
            d["results_menu"]["fill"]["box_size"]["x"].SetFloat(1280.0f);
            d["menu_text"]["records"]["new_best"].SetString("R");
            d["menu_text"]["records"]["stage_best"].SetString("R");
        };
        const auto fits = variants.load([&](rapidjson::Document& d) { edit(d, false); });
        fits.validate_resources(world.render, world.audio);
        UiTextDropShadow row("results-row", attained, fits.results_menu().contribution_font,
            mattmath::Vector2F(x, 815.0f), &world.render, Colour::black, Colour(), mattmath::Vector2F::ZERO);
        require(row.bounds().right() == 1610.0f && row.bounds().height == 24.0f,
            "Complete Results row lost its unscaled zero-shadow bounds or inclusive full suffix width");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d) { edit(d, true); }, &world);
    }

    void check_connection_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'~' ? width : 8, character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        const auto target = [](rapidjson::Document& d, const char* key, const std::string& font)
        {
            compact_connection_copy(d);
            select_connection_font(d, key, font.c_str());
            d["menu_text"]["connection"][key].SetString(
                std::string(key) == "pause_player_title" ? "~{player}" : "~", d.GetAllocator());
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        require(original.main_menu().setup_box_size == mattmath::Vector2F(1760.0f, 920.0f) &&
            original.main_menu().team_box_size == mattmath::Vector2F(1840.0f, 920.0f) &&
            original.menu().layout.position == mattmath::Vector2F(150.0f, 150.0f) &&
            original.main_menu().player_count_spacing.y == 100.0f &&
            original.pause_menu().box_size == mattmath::Vector2F(1050.0f, 900.0f),
            "Connection boundary fixtures no longer match the shipped setup/team/Pause papers");

        // Known eight-unit ordinary glyphs isolate one long supported field.
        // Pause text is plain and centered on the full 1920x1080 paper;
        // Join/Team retain positive shadow extents inside their inset paper.
        struct Boundary { const char* key; int width; int height; };
        constexpr Boundary boundaries[]{
            {"join_ready", 1675, 75}, {"join_waiting", 1675, 75},
            {"pause_title", 1050, 135}, {"pause_player_title", 1042, 135},
            {"controller_lost", 1050, 135}, {"all_connected", 1050, 122},
            {"teams_required", 1715, 65}
        };
        for (const auto& entry : boundaries)
        {
            const std::string name = std::string("menu_connection_") + entry.key;
            add_font(name + "_width_edge", entry.width, 20);
            add_font(name + "_width_over", entry.width + 1, 20);
            add_font(name + "_height_edge", 8, entry.height);
            add_font(name + "_height_over", 8, entry.height + 1);
            const std::string field = std::string("connection.") + entry.key;
            const auto width_edge = variants.load([&](rapidjson::Document& d)
                { target(d, entry.key, name + "_width_edge"); });
            width_edge.validate_resources(world.render, world.audio);
            const auto height_edge = variants.load([&](rapidjson::Document& d)
                { target(d, entry.key, name + "_height_edge"); });
            height_edge.validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d)
                { target(d, entry.key, name + "_width_over"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
                { target(d, entry.key, name + "_height_over"); }, &world);

            const bool pause_heading = std::string(entry.key) == "pause_title" ||
                std::string(entry.key) == "pause_player_title" || std::string(entry.key) == "controller_lost";
            if (pause_heading || std::string(entry.key) == "all_connected")
            {
                const auto role = std::find_if(std::begin(connection_text), std::end(connection_text),
                    [&](const ConnectionText& candidate) { return std::string(candidate.key) == entry.key; });
                require(role != std::end(connection_text), "Connection boundary role has no independent default");
                const std::wstring words = std::string(entry.key) == "pause_player_title" ?
                    width_edge.menu_text().pause_title_for_player(4) : width_edge.menu_text().connection.*role->field;
                const auto& pause = width_edge.pause_menu();
                const float y = pause_heading ? pause.heading_y : pause.status_y;
                UiText text("connection", words, pause_heading ? width_edge.menu().heading_font : width_edge.menu().detail_font,
                    mattmath::Vector2F(0.0f, y), &world.render, pause.art.ink);
                text.set_position(mattmath::Vector2F((1920.0f - text.bounds().width) / 2.0f, y));
                require(text.bounds().x == 435.0f && text.bounds().right() == 1485.0f &&
                    text.bounds().width == 1050.0f && text.bounds().height == 20.0f,
                    "Plain centered connection text changed its unscaled inclusive full-paper width");
            }
            else
            {
                const bool team = std::string(entry.key) == "teams_required";
                const auto& style = width_edge.menu();
                const mattmath::Vector2F position(150.0f, team ? 860.0f : 850.0f);
                UiTextDropShadow text("connection", L"~", style.detail_font, position, &world.render,
                    Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.detail_shadow_offset);
                require(text.bounds().x == 150.0f && text.bounds().right() == (team ? 1866.0f : 1826.0f),
                    "Join/Team connection text lost its retained transparent-shadow width at the inclusive inset edge");
                UiTextDropShadow tall("connection-tall", L"~", height_edge.menu().detail_font, position, &world.render,
                    Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), height_edge.menu().detail_shadow_offset);
                require(tall.bounds().bottom() == 926.0f,
                    "Join/Team actual glyph height and transparent shadow missed the inset paper bottom");
            }
        }

        // Reconnect is appended only to reserved pads. The keyboard/mouse
        // label is checked separately by the validator and never disconnected.
        // "P4 - Pad 4" is ten ordinary eight-unit glyphs. The 1594-unit suffix
        // plus a two-unit retained shadow exactly fills x=150 through x=1826.
        add_font("menu_connection_suffix_edge", 1594, 98);
        add_font("menu_connection_suffix_width_over", 1595, 98);
        add_font("menu_connection_suffix_height_over", 1594, 99);
        const auto suffix = [&](rapidjson::Document& d, const char* font)
        {
            target(d, "join_reconnect_suffix", font);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(2.0f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(2.0f);
        };
        const auto suffix_edge = variants.load([&](rapidjson::Document& d)
            { suffix(d, "menu_connection_suffix_edge"); });
        suffix_edge.validate_resources(world.render, world.audio);
        UiTextDropShadow reserved("reserved-pad", L"P4 - Pad 4~", suffix_edge.menu().item_font,
            mattmath::Vector2F(150.0f, 650.0f), &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), suffix_edge.menu().item_shadow_offset);
        require(reserved.bounds() == mattmath::RectangleF(150.0f, 650.0f, 1676.0f, 100.0f),
            "Reserved-pad suffix no longer touches the exact inclusive paper edge and next row");
        for (const char* overflow : {"menu_connection_suffix_width_over", "menu_connection_suffix_height_over"})
        {
            variants.rejects("connection.join_reconnect_suffix", [&](rapidjson::Document& d)
                { suffix(d, overflow); }, &world);
        }
        const auto team_order = [](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(172.5f);
        };
        const auto touches_heading = variants.load(team_order);
        UiTextDropShadow team_heading("team-heading", L"Fit", touches_heading.menu().heading_font,
            mattmath::Vector2F(150.0f, 150.0f), &world.render, Colour::black, Colour(), touches_heading.menu().heading_shadow_offset);
        UiTextDropShadow heading_warning("heading-warning", L"Fit", touches_heading.menu().detail_font,
            mattmath::Vector2F(150.0f, 172.5f), &world.render, Colour::black, Colour(), touches_heading.menu().detail_shadow_offset);
        require(team_heading.bounds().bottom() == 172.5f && heading_warning.bounds().y == team_heading.bounds().bottom(),
            "Team warning lost its exact heading-contact geometry");
        // Heading contact leaves no space for the four complete player cards.
        variants.rejects("labels.roster_row", team_order, &world);
        variants.rejects("connection.teams_required", [&](rapidjson::Document& d)
        {
            team_order(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(172.25f);
        }, &world);
        const auto full_row_edge = [&](rapidjson::Document& d)
        {
            compact_connection_copy(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(832.0f);
        };
        const auto touches_final_card = variants.load(full_row_edge);
        touches_final_card.validate_resources(world.render, world.audio);
        require(touches_final_card.main_menu().team_select.spacing == mattmath::Vector2F(250.0f, 125.0f) &&
            touches_final_card.main_menu().team_select.size == mattmath::Vector2F(50.0f, 50.0f) &&
            touches_final_card.main_menu().setup_focus_padding == mattmath::Vector2F(28.0f, 7.0f) &&
            touches_final_card.player_identity().ui_size == mattmath::Vector2F(54.0f, 42.0f) &&
            touches_final_card.player_identity().ui_gap == 12.0f,
            "Team complete-row boundary fixture no longer matches the authored icons, spacing, focus or badge");
        UiTextDropShadow final_label("final-label", L"P4 - Pad 4", touches_final_card.menu().detail_font,
            mattmath::Vector2F(150.0f, 775.0f), &world.render, Colour::black, Colour(), touches_final_card.menu().item_shadow_offset);
        mattmath::RectangleF final_card = mattmath::RectangleF::union_of(final_label.bounds(),
            mattmath::RectangleF(84.0f, 764.75f, 54.0f, 42.0f));
        final_card = mattmath::RectangleF::union_of(final_card, mattmath::RectangleF(650.0f, 775.0f, 150.0f, 50.0f));
        final_card.inflate(mattmath::Vector2F(28.0f, 7.0f));
        UiTextDropShadow final_warning("final-warning", L"Fit", touches_final_card.menu().detail_font,
            mattmath::Vector2F(150.0f, 832.0f), &world.render, Colour::black, Colour(), touches_final_card.menu().detail_shadow_offset);
        require(final_card.bottom() == 832.0f && final_warning.bounds().y == final_card.bottom() &&
            final_warning.bounds().bottom() == 853.0f,
            "Team warning lost exact complete-card contact including the badge, icons and focus padding");
        variants.rejects("labels.roster_row", [&](rapidjson::Document& d)
        {
            full_row_edge(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(831.75f);
        }, &world);
    }

    void check_confirmation_text(Variants& variants, World& world)
    {
        for (const auto& entry : confirmation_text)
        {
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["confirmations"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().confirmations.*entry.field == L"  A \"B\"-C...  ",
                std::string("Confirmation lost normalized punctuation or authored spaces: ") + entry.key);
            const std::string field = std::string("confirmations.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["confirmations"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                // All other menu fonts retain '~'. The actual question's
                // detail font must support it without relying on a stand-in.
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
                d["menu_text"]["confirmations"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["confirmations"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }

        const auto check_widgets = [&](const Presentation& look)
        {
            const auto& pause = look.pause_menu();
            const auto& style = look.menu();
            const auto font = world.render.resolve_sprite_font(style.detail_font);
            for (const auto& entry : confirmation_text)
            {
                std::wstring source = look.menu_text().confirmations.*entry.field;
                const auto measured = world.render.measure_text(font, source);
                const auto retained = source;
                UiText question(entry.key, source, style.detail_font,
                    mattmath::Vector2F(0.0f, pause.status_y), &world.render, pause.art.ink);
                source = L"Changed after construction";
                question.set_position(mattmath::Vector2F((1920.0f - question.bounds().width) / 2.0f, pause.status_y));
                const mattmath::RectangleF expected((1920.0f - measured.x) / 2.0f, pause.status_y, measured.x, measured.y);
                require(std::isfinite(measured.x) && std::isfinite(measured.y) && measured.x > 0.0f && measured.y > 0.0f &&
                    question.text() == retained && question.bounds() == expected,
                    std::string("Actual confirmation UiText borrowed text, scaled, wrapped or changed centering: ") + entry.key);
                question.set_hidden(true);
                DrawList no_device;
                question.draw(no_device);
            }
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        check_widgets(original);
        for (const size_t length : { size_t(3), size_t(100) })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                for (const auto& entry : confirmation_text)
                {
                    d["menu_text"]["confirmations"][entry.key].SetString(std::string(length, 'F').c_str(), d.GetAllocator());
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
    }

    void check_confirmation_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height,
            float x_offset = 0.0f, float y_offset = 0.0f, float line_spacing = 16.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool boundary = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary ? width : 8, boundary ? height : 16),
                    boundary ? x_offset : 0.0f, boundary ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        const auto use_font = [](rapidjson::Document& d, const std::string& name, const char* key)
        {
            d["menu_style"]["fonts"]["detail"].SetString(name.c_str(), d.GetAllocator());
            // Keep independent Weapon bodies clear of the compact label badges.
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            for (const auto& entry : confirmation_text) { d["menu_text"]["confirmations"][entry.key].SetString("Fit"); }
            d["menu_text"]["confirmations"][key].SetString("~");
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& pause = original.pause_menu();
        require(pause.box_size.x == 1050.0f && pause.box_size.y == 900.0f,
            "Confirmation paper fixtures no longer exercise the shipped full-height Pause paper");
        add_font("menu_confirmation_width_edge", 1050, 20);
        add_font("menu_confirmation_width_overflow", 1051, 20);
        // The shared font's unrelated two-line missing list must also fit;
        // retain each target '~' height while compacting only line spacing.
        add_font("menu_confirmation_action_edge", 8, 20, 0.0f, 0.0f, 1.0f);
        add_font("menu_confirmation_display_action_edge", 8, 40, 0.0f, 0.0f, 1.0f);
        for (const auto& entry : confirmation_text)
        {
            const std::string field = std::string("confirmations.") + entry.key;
            const auto width_edge = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_confirmation_width_edge", entry.key);
            });
            width_edge.validate_resources(world.render, world.audio);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(width_edge.menu().detail_font), L"~");
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f, pause.box_size);
            const mattmath::RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
            require(bounds.x == paper.x && bounds.right() == paper.right() && paper.contains(bounds),
                "Confirmation's unpadded centered text no longer touches both inclusive paper edges");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_confirmation_width_overflow", entry.key);
            }, &world);
            const auto action_edge = [&](rapidjson::Document& d)
            {
                const bool display = std::string(entry.key) == "display";
                use_font(d, display ? "menu_confirmation_display_action_edge" : "menu_confirmation_action_edge", entry.key);
                d["pause_menu"]["layout"]["position"]["y"].SetFloat(pause.status_y + (display ? 40.0f : 20.0f) +
                    (display ? original.main_menu().setup_focus_padding.y : pause.art.focus_padding.y));
            };
            const auto touches_action = variants.load(action_edge);
            touches_action.validate_resources(world.render, world.audio);
            const bool display = std::string(entry.key) == "display";
            const float question_height = display ? 40.0f : 20.0f;
            const float padding = display ? original.main_menu().setup_focus_padding.y : pause.art.focus_padding.y;
            require(touches_action.pause_menu().status_y + question_height == touches_action.pause_menu().layout.position.y - padding,
                "Confirmation action fixture lost exact unscaled bottom-to-padding contact");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                action_edge(d);
                d["pause_menu"]["layout"]["position"]["y"].SetFloat(pause.status_y + question_height - 0.25f + padding);
            }, &world);
        }

        // Exit is a Home item and a confirmation heading. Its second context
        // must use the heading font while preserving the existing item font.
        const auto heading_font = [](rapidjson::Document& d, const char* name)
        {
            d["menu_style"]["fonts"]["heading"].SetString(name, d.GetAllocator());
            d["menu_text"]["items"]["exit"].SetString("~");
        };
        variants.rejects("items.exit", [&](rapidjson::Document& d)
        {
            heading_font(d, "menu_text_limited");
        }, &world);
        const auto heading_width = variants.load([&](rapidjson::Document& d)
        {
            heading_font(d, "menu_confirmation_width_edge");
        });
        heading_width.validate_resources(world.render, world.audio);
        std::wstring heading_source = heading_width.menu_text().item(MenuItem::exit);
        const std::string retained_font = heading_width.menu().heading_font;
        UiText heading("exit-heading", heading_source, retained_font,
            mattmath::Vector2F(0.0f, pause.heading_y), &world.render, pause.art.ink);
        heading_source = L"Changed after construction";
        heading.set_position(mattmath::Vector2F((1920.0f - heading.bounds().width) / 2.0f, pause.heading_y));
        require(heading.text() == L"~" && heading.bounds() ==
            mattmath::RectangleF((1920.0f - pause.box_size.x) / 2.0f, pause.heading_y, pause.box_size.x, 20.0f),
            "Exit heading borrowed its copy, scaled, used the item font or missed the inclusive paper edges");
        heading.set_hidden(true);
        DrawList no_device;
        heading.draw(no_device);
        variants.rejects("items.exit", [&](rapidjson::Document& d)
        {
            heading_font(d, "menu_confirmation_width_overflow");
        }, &world);

        const auto heading_top = [&](rapidjson::Document& d)
        {
            heading_font(d, "menu_confirmation_action_edge");
            d["pause_menu"]["heading_y"].SetFloat(90.0f);
            d["pause_menu"]["status_y"].SetFloat(110.0f);
        };
        const auto touches_top_and_question = variants.load(heading_top);
        touches_top_and_question.validate_resources(world.render, world.audio);
        variants.rejects("items.exit", [&](rapidjson::Document& d)
        {
            heading_top(d);
            d["pause_menu"]["heading_y"].SetFloat(89.75f);
        }, &world);
        variants.rejects("items.exit", [&](rapidjson::Document& d)
        {
            heading_top(d);
            d["pause_menu"]["status_y"].SetFloat(109.75f);
        }, &world);
        add_font("menu_exit_heading_height_edge", 8, 135);
        add_font("menu_exit_heading_height_overflow", 8, 136);
        const auto tall_heading = variants.load([&](rapidjson::Document& d)
        {
            heading_font(d, "menu_exit_heading_height_edge");
        });
        tall_heading.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font(tall_heading.menu().heading_font), L"~").y ==
            pause.status_y - pause.heading_y,
            "Exit heading fixture no longer exercises actual tall glyphs beyond the font line spacing");
        variants.rejects("items.exit", [&](rapidjson::Document& d)
        {
            heading_font(d, "menu_exit_heading_height_overflow");
        }, &world);
        // Compact legal actions leave a question below the footer-centered
        // paper's bottom (930), but inside the actual full-height paper (990).
        std::vector<Glyph> small_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            small_glyphs.push_back({ character, mattmath::RectangleI(0, 0, 1, 2), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_confirmation_small_actions",
            std::make_unique<Font>(TextureHandle{}, std::move(small_glyphs), 2.0f));
        add_font("menu_confirmation_full_height", 8, 40);
        const auto full_height = variants.load([&](rapidjson::Document& d)
        {
            use_font(d, "menu_confirmation_full_height", "restart");
            d["menu_text"]["confirmations"]["quit"].SetString("~");
            d["menu_text"]["confirmations"]["exit"].SetString("~");
            d["menu_style"]["fonts"]["item"].SetString("menu_confirmation_small_actions");
            d["pause_menu"]["status_y"].SetFloat(900.0f);
            d["pause_menu"]["layout"]["position"]["y"].SetFloat(940.5f);
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["pause_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["pause_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["pause_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
            d["main_menu"]["setup_focus_padding"]["x"].SetFloat(0.5f);
            d["main_menu"]["setup_focus_padding"]["y"].SetFloat(0.5f);
            d["main_menu"]["setup_shadow_offset"].SetFloat(0.125f);
            d["main_menu"]["art"]["keyline_width"].SetFloat(0.125f);
        });
        full_height.validate_resources(world.render, world.audio);
        require(full_height.pause_menu().status_y + 40.0f >
            (full_height.menu().footer_y + full_height.pause_menu().box_size.y) / 2.0f,
            "Confirmation full-height fixture no longer distinguishes Pause's actual paper from a footer-centered paper");

        struct InvalidMeasure { const char* name; int width; int height; float x_offset; float y_offset; float line_spacing; };
        const float infinity = std::numeric_limits<float>::infinity();
        const InvalidMeasure invalid_measures[]{
            { "zero_width", 0, 20, 0.0f, 0.0f, 16.0f },
            { "zero_height", 8, 0, 0.0f, 0.0f, 0.0f },
            { "infinite_width", 8, 20, infinity, 0.0f, 16.0f },
            { "infinite_height", 8, 20, 0.0f, infinity, 16.0f }
        };
        for (const auto& invalid : invalid_measures)
        {
            const std::string name = std::string("menu_confirmation_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid confirmation metrics fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : confirmation_text)
            {
                variants.rejects(std::string("confirmations.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry.key);
                }, &world);
            }
            variants.rejects("items.exit", [&](rapidjson::Document& d)
            {
                heading_font(d, name.c_str());
            }, &world);
        }
    }

    void check_display_trial_text(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto keep_size = world.render.measure_text(world.render.resolve_sprite_font(original.menu().item_font),
            original.menu_text().item(MenuItem::keep));
        const auto tight_display = variants.load([](rapidjson::Document& d)
        {
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(115.0f);
            d["menu_text"]["items"]["options"].SetString("Fit");
        });
        tight_display.validate_resources(world.render, world.audio);
        require(keep_size.y == 82.0f && keep_size.y + tight_display.pause_menu().art.focus_padding.y * 2.0f >
            tight_display.pause_menu().layout.spacing.y && keep_size.y + tight_display.main_menu().setup_focus_padding.y * 2.0f <=
            tight_display.pause_menu().layout.spacing.y,
            "Keep's real descender no longer distinguishes compact Display padding from ordinary Pause padding in a tight row");
        const auto distinct_pause_keyline = [](rapidjson::Document& d)
        {
            // Isolate card clearance from stock descenders in the setup rows
            // that share the compact padding. Ordinary Pause art remains valid.
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            d["pause_menu"]["art"]["keyline_width"].SetFloat(5.0f);
            d["main_menu"]["setup_focus_padding"]["y"].SetFloat(8.0f);
        };
        const auto clear_display_card = variants.load(distinct_pause_keyline);
        clear_display_card.validate_resources(world.render, world.audio);
        require(clear_display_card.pause_menu().art.keyline_width == 5.0f &&
            clear_display_card.main_menu().art.keyline_width == 4.0f &&
            clear_display_card.main_menu().setup_focus_padding.y == 8.0f,
            "Display paper lost Pause's distinct keyline or rejected padding above the combined clearance threshold");
        for (const char* axis : { "y", "x" })
        {
            const auto path = variants.write([&](rapidjson::Document& d)
            {
                distinct_pause_keyline(d);
                d["main_menu"]["setup_focus_padding"][axis].SetFloat(7.0f);
            });
            bool contextual = false;
            std::string diagnostic = "accepted without exception";
            try
            {
                const Presentation invalid(path.string().c_str());
                invalid.validate_resources(world.render, world.audio);
            }
            catch (const std::runtime_error& error)
            {
                diagnostic = error.what();
                contextual = diagnostic.find(path.string()) != std::string::npos &&
                    diagnostic.find("main_menu.setup_focus_padding") != std::string::npos &&
                    diagnostic.find("main_menu.setup_shadow_offset") != std::string::npos &&
                    diagnostic.find("pause_menu.art.keyline_width") != std::string::npos;
            }
            require(contextual, std::string("Display paper clearance on ") + axis +
                " lost source or composed-art context: " + diagnostic);
        }
        require(original.menu_text().display_countdown(0) == L"Reverting in 0s" &&
            original.menu_text().display_countdown(15) == L"Reverting in 15s" &&
            original.menu_text().display_countdown(60) == L"Reverting in 60s",
            "Display countdown lost its complete numeric substitution");
        bool negative = false;
        try { std::ignore = original.menu_text().display_countdown(-1); }
        catch (const std::out_of_range&) { negative = true; }
        require(negative, "Display countdown accepted a negative remaining time");
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["display_trial_countdown"].SetString("  Revert\xC2\xA0{seconds}\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().display_countdown(9) == L"  Revert 9...  ",
            "Display countdown lost normalization, its token or authored outer spaces");
        for (const char* invalid : { "Missing seconds", "{seconds}{seconds}", "{seconds} {other}", "{{seconds}}",
            "{Seconds}", "{seconds", "seconds}", "{seconds}\nline", "{seconds}\ttext", "{seconds}\xC3\xA9" })
        {
            variants.rejects("display_trial_countdown", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["display_trial_countdown"].SetString(invalid, d.GetAllocator());
            });
        }
        for (const char* key : { "keep", "revert" })
        {
            const auto label = variants.load([key](rapidjson::Document& d)
                { d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B"); });
            label.validate_resources(world.render, world.audio);
            require(label.menu_text().item(std::string(key) == "keep" ? MenuItem::keep : MenuItem::revert) == L"A -B",
                "Display confirmation action lost normalized authored copy");
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        std::vector<Glyph> missing_digit;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character != U'9') { missing_digit.push_back({ character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f }); }
        }
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(missing_digit), 20.0f);
        limited->set_stand_in(U'?');
        world.render.add_font("display_countdown_missing_nine", std::move(limited));
        variants.rejects("display_trial_countdown", [](rapidjson::Document& d)
            { d["menu_style"]["fonts"]["detail"].SetString("display_countdown_missing_nine"); }, &world);
        const auto add_font = [&](const char* name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? width : 8,
                    character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        const auto countdown_font = [](rapidjson::Document& d, const char* name)
        {
            d["menu_style"]["fonts"]["detail"].SetString(name, d.GetAllocator());
            d["menu_text"]["display_trial_countdown"].SetString("~{seconds}");
        };
        add_font("display_countdown_width_edge", 1034, 20);
        add_font("display_countdown_width_overflow", 1035, 20);
        const int countdown_height = static_cast<int>((1080.0f + original.pause_menu().box_size.y) / 2.0f -
            original.pause_menu().layout.position.y - original.pause_menu().layout.spacing.y * 2.0f);
        add_font("display_countdown_height_edge", 8, countdown_height);
        add_font("display_countdown_height_overflow", 8, countdown_height + 1);
        for (const char* font : { "display_countdown_width_edge", "display_countdown_height_edge" })
        {
            const auto edge = variants.load([&](rapidjson::Document& d) { countdown_font(d, font); });
            edge.validate_resources(world.render, world.audio);
            MenuText copy = edge.menu_text();
            const std::wstring text = copy.display_countdown(15);
            const auto& pause = edge.pause_menu();
            UiText countdown("countdown", text, font, { 0.0f, pause.layout.position.y + pause.layout.spacing.y * 2.0f },
                &world.render, pause.art.ink);
            copy.display_trial_countdown = L"Changed {seconds}";
            countdown.set_position({ (1920.0f - countdown.bounds().width) / 2.0f, countdown.bounds().y });
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f, pause.box_size);
            require(countdown.text() == L"~15" && paper.contains(countdown.bounds()) &&
                (std::string(font) == "display_countdown_width_edge" ? countdown.bounds().width == 1050.0f : countdown.bounds().bottom() == 990.0f),
                "Display countdown borrowed copy, fitted its text, used line spacing instead of tall glyphs or missed inclusive paper edges");
            countdown.set_hidden(true); DrawList no_device; countdown.draw(no_device);
        }
        for (const char* font : { "display_countdown_width_overflow", "display_countdown_height_overflow" })
        {
            variants.rejects("display_trial_countdown", [&](rapidjson::Document& d) { countdown_font(d, font); }, &world);
        }
        variants.rejects("headings.options", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_confirmation_width_overflow");
            d["menu_text"]["headings"]["options"].SetString("~");
        }, &world);
    }

    void check_end_text(Variants& variants, World& world)
    {
        constexpr const char* end_keys[]{ "change_teams", "change_weapons", "change_level", "exit_to_main_menu" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : end_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(end_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::change_teams) + index)) == L"A -B",
                "End item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : end_keys)
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
            for (const char* invalid : { "   ", "line\nbreak", "tab\ttext", "\xC3\xA9", "\xC3" })
            {
                variants.rejects(std::string("items.") + key, [key, invalid](rapidjson::Document& d)
                {
                    d["menu_text"]["items"][key].SetString(invalid, d.GetAllocator());
                });
            }
        }

        const auto exact_items = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            // Keep the unrelated new row 0 copy at the same 24-unit width while
            // the original static-action right-edge and row 4 owners are tested.
            for (const auto& entry : end_continuation_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& end = original.end_menu();
        const auto top = (mattmath::Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f;
        const auto bottom = top + end.box_size;
        const float left_position = top.x + end.art.focus_padding.x;
        const float right_position = bottom.x - 24.0f - end.art.focus_padding.x;
        // The first static label is row 1. Exit moves from row 1 during an
        // ongoing series to row 4 after a completed match or series.
        const float bottom_position = bottom.y - 4.0f * end.layout.spacing.y -
            20.0f - end.art.focus_padding.y;
        const auto paper_edge = [&](const char* axis, float position, const char* field, float outside)
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["end_menu"]["layout"]["position"][axis].SetFloat(position);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                edit(d);
                d["end_menu"]["layout"]["position"][axis].SetFloat(outside);
            }, &world);
        };
        paper_edge("x", left_position, "items.change_teams", left_position - 0.25f);
        paper_edge("x", right_position, "items.change_teams", right_position + 0.25f);
        // A row-1 card at the paper top would put continuation row 0 above
        // the paper, leaving no valid heading before it. Heading paper-top
        // and first-row contacts are tested independently below instead.
        // Exact bottom contact is below the shared footer but within End's
        // full-height paper. At the failing position, rows 1-3 still fit;
        // only completed-match Exit on row 4 crosses that paper's boundary.
        require(bottom_position + end.layout.spacing.y * 3.0f + 20.0f +
            end.art.focus_padding.y + 0.25f < bottom.y,
            "End row-4 overflow fixture also overflows its earlier static action rows");
        paper_edge("y", bottom_position, "items.exit_to_main_menu", bottom_position + 0.25f);

        const float row_height = 20.0f + 2.0f * end.art.focus_padding.y;
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(row_height);
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.change_teams", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(std::nextafter(row_height, 0.0f));
        }, &world);
        for (const double invalid : { 0.0, -1.0, 1e100 })
        {
            variants.rejects("items.change_teams", [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["end_menu"]["layout"]["spacing"]["y"].SetDouble(invalid);
            }, &world);
        }

        // A longer Exit can overflow independently while the three other
        // static action labels retain their exact right-edge fit.
        variants.rejects("items.exit_to_main_menu", [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["end_menu"]["layout"]["position"]["x"].SetFloat(right_position);
            d["menu_text"]["items"]["exit_to_main_menu"].SetString("FitFit");
        }, &world);
    }

    void check_end_headings(Variants& variants, World& world)
    {
        for (const auto& entry : end_text)
        {
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().end.*entry.field == L"  A \"B\"-C...  ",
                std::string("End heading lost normalized punctuation or authored spaces: ") + entry.key);
            const std::string field = std::string("end.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["end"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                // Other menu fonts still support '~'; only the heading font
                // used by the actual End widget is missing the authored glyph.
                d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited");
                d["menu_text"]["end"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            const auto& end = look.end_menu();
            const auto& style = look.menu();
            const auto font = world.render.resolve_sprite_font(style.heading_font);
            for (const auto& entry : end_text)
            {
                std::wstring source = look.menu_text().end.*entry.field;
                const auto measured = world.render.measure_text(font, source);
                const auto retained = source;
                UiText heading(entry.key, source, style.heading_font,
                    mattmath::Vector2F(0.0f, end.heading_y), &world.render, end.art.ink);
                source = L"Changed after construction";
                heading.set_position(mattmath::Vector2F((1920.0f - heading.bounds().width) / 2.0f, end.heading_y));
                const mattmath::RectangleF expected((1920.0f - measured.x) / 2.0f, end.heading_y, measured.x, measured.y);
                require(std::isfinite(measured.x) && std::isfinite(measured.y) && measured.x > 0.0f && measured.y > 0.0f &&
                    heading.text() == retained && heading.bounds() == expected,
                    std::string("Actual End heading borrowed text, scaled, wrapped or changed centering: ") + entry.key);
                heading.set_hidden(true);
                DrawList no_device;
                heading.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const size_t length : { size_t(3), size_t(100) })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
                for (const auto& entry : end_text)
                {
                    d["menu_text"]["end"][entry.key].SetString(std::string(length, 'F').c_str(), d.GetAllocator());
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
    }

    void check_end_heading_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height,
            float x_offset = 0.0f, float y_offset = 0.0f, float line_spacing = 16.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool boundary = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary ? width : 8, boundary ? height : 16),
                    boundary ? x_offset : 0.0f, boundary ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        const auto use_font = [](rapidjson::Document& d, const std::string& name, const char* key)
        {
            d["menu_style"]["fonts"]["heading"].SetString(name.c_str(), d.GetAllocator());
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
            d["menu_text"]["end"][key].SetString("~");
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& end = original.end_menu();
        require(end.box_size == mattmath::Vector2F(980.0f, 820.0f),
            "End heading fixtures no longer exercise the shipped full-height paper");
        add_font("menu_end_heading_width_edge", 980, 20);
        add_font("menu_end_heading_width_overflow", 981, 20);
        add_font("menu_end_heading_action_edge", 8, 20);
        for (const auto& entry : end_text)
        {
            const std::string field = std::string("end.") + entry.key;
            const auto width_edge = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_width_edge", entry.key);
            });
            width_edge.validate_resources(world.render, world.audio);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(width_edge.menu().heading_font), L"~");
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f, end.box_size);
            const mattmath::RectangleF bounds((1920.0f - size.x) / 2.0f, end.heading_y, size.x, size.y);
            require(bounds.x == paper.x && bounds.right() == paper.right() && paper.contains(bounds),
                "End heading's unpadded centered text no longer touches both inclusive paper edges");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_width_overflow", entry.key);
            }, &world);
            const auto action_edge = [&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_action_edge", entry.key);
                d["end_menu"]["layout"]["position"]["y"].SetFloat(end.heading_y + 20.0f + end.art.focus_padding.y);
            };
            const auto touches_action = variants.load(action_edge);
            touches_action.validate_resources(world.render, world.audio);
            require(touches_action.end_menu().heading_y + 20.0f == touches_action.end_menu().layout.position.y -
                touches_action.end_menu().art.focus_padding.y,
                "End heading action fixture lost exact unscaled bottom-to-padding contact");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                action_edge(d);
                d["end_menu"]["layout"]["position"]["y"].SetFloat(end.heading_y + 19.75f + end.art.focus_padding.y);
            }, &world);
        }
        const auto paper_top = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
            d["end_menu"]["heading_y"].SetFloat(130.0f);
        };
        const auto touches_top = variants.load(paper_top);
        touches_top.validate_resources(world.render, world.audio);
        variants.rejects("end.match_complete", [&](rapidjson::Document& d)
        {
            paper_top(d);
            d["end_menu"]["heading_y"].SetFloat(129.75f);
        }, &world);

        // The heading's unscaled bottom at920 is below a footer-centered
        // paper's bottom890, but fits the actual full-height paper bottom950.
        std::vector<Glyph> small_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            small_glyphs.push_back({ character, mattmath::RectangleI(0, 0, 1, 2), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_end_heading_small_actions",
            std::make_unique<Font>(TextureHandle{}, std::move(small_glyphs), 2.0f));
        const auto full_height = variants.load([&](rapidjson::Document& d)
        {
            use_font(d, "menu_end_heading_action_edge", "match_complete");
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("~"); }
            d["menu_style"]["fonts"]["item"].SetString("menu_end_heading_small_actions");
            d["end_menu"]["heading_y"].SetFloat(900.0f);
            d["end_menu"]["layout"]["position"]["y"].SetFloat(920.5f);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["end_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["end_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["end_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
        });
        full_height.validate_resources(world.render, world.audio);
        require(full_height.end_menu().heading_y + 20.0f >
            (full_height.menu().footer_y + full_height.end_menu().box_size.y) / 2.0f,
            "End heading full-height fixture no longer distinguishes its actual paper from a footer-centered paper");

        struct InvalidMeasure { const char* name; int width; int height; float x_offset; float y_offset; float line_spacing; };
        const float infinity = std::numeric_limits<float>::infinity();
        const InvalidMeasure invalid_measures[]{
            { "zero_width", 0, 20, 0.0f, 0.0f, 16.0f },
            { "zero_height", 8, 0, 0.0f, 0.0f, 0.0f },
            { "infinite_width", 8, 20, infinity, 0.0f, 16.0f },
            { "infinite_height", 8, 20, 0.0f, infinity, 16.0f }
        };
        for (const auto& invalid : invalid_measures)
        {
            const std::string name = std::string("menu_end_heading_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid End heading fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : end_text)
            {
                variants.rejects(std::string("end.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry.key);
                }, &world);
            }
        }
    }

    constexpr MenuItem controls_fitted_items[]{ MenuItem::music_volume, MenuItem::sfx_volume, MenuItem::view_shake,
        MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone };

    void check_controls_text(Variants& variants, World& world)
    {
        constexpr const char* controls_keys[]{ "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : controls_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(controls_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::pad_sticks) + index)) == L"A -B",
                "Controls/pad-sticks item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : controls_keys)
        {
            for (const char* invalid : { "   ", "line\nbreak", "tab\ttext", "\xC3\xA9", "\xC3" })
            {
                variants.rejects(std::string("items.") + key, [key, invalid](rapidjson::Document& d)
                {
                    d["menu_text"]["items"][key].SetString(invalid, d.GetAllocator());
                });
            }
        }
        for (const char* key : { "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone", "apply", "back" })
        {
            // The main-menu font can draw this glyph. Controls' independently
            // selected item font must also support shared Apply and Back.
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            // The F alone exactly fills the Controls action column. The same
            // one-character string still fits its other uses in the main font.
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'F' ? 872 : 8, 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_controls_exact", std::make_unique<Font>(TextureHandle{}, glyphs, 20.0f));
        world.render.add_font("menu_controls_row_exact", std::make_unique<Font>(TextureHandle{}, glyphs, 68.0f));
        world.render.add_font("menu_controls_row_overflow", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 68.25f));
        const auto exact_controls = [](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_controls_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("i"); }
        };
        for (const char* key : { "pad_sticks", "apply", "back" })
        {
            const auto edge = [&](rapidjson::Document& d)
            {
                exact_controls(d);
                d["menu_text"]["items"][key].SetString("F");
            };
            const auto fits = variants.load(edge);
            fits.validate_resources(world.render, world.audio);
            for (const char* overflow : { "Fi", "FF" })
            {
                variants.rejects(std::string("items.") + key, [&](rapidjson::Document& d)
                {
                    edge(d);
                    d["menu_text"]["items"][key].SetString(overflow, d.GetAllocator());
                }, &world);
            }
        }
        variants.rejects("items.pad_sticks", [](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_tall");
        }, &world);
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_controls(d);
            d["controls_settings"]["item_font"].SetString("menu_controls_row_exact");
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.pad_sticks", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["controls_settings"]["item_font"].SetString("menu_controls_row_overflow");
        }, &world);
        const auto status_edge = [&](rapidjson::Document& d)
        {
            row_edge(d);
            // Nominal 60-high rows plus their shadow end at 868. The actual
            // 68-high text plus focus padding reaches 874 and must stay clear
            // of the status even when that nominal layout still parses.
            d["controls_settings"]["status_position"]["y"].SetFloat(874.0f);
        };
        const auto fits_status = variants.load(status_edge);
        fits_status.validate_resources(world.render, world.audio);
        variants.rejects("items.back", [&](rapidjson::Document& d)
        {
            status_edge(d);
            d["controls_settings"]["status_position"]["y"].SetFloat(873.75f);
        }, &world);
    }

    void check_controls_option_rows(Variants& variants, World& world)
    {
        const std::string authored(200, 'W');
        const auto look = variants.load([&](rapidjson::Document& d)
        {
            for (const MenuItem item : controls_fitted_items)
            {
                d["menu_text"]["items"][items[static_cast<size_t>(item)].key].SetString(authored.c_str(), d.GetAllocator());
            }
        });
        look.validate_resources(world.render, world.audio);
        const auto& controls = look.controls_settings();
        MenuStyle style = look.menu();
        require(controls.item_font != style.item_font,
            "Controls font coverage must exercise its separate item font from ordinary menu actions");
        style.item_font = controls.item_font;
        const auto font = world.render.resolve_sprite_font(style.item_font);
        const float label_width = controls.layout.spacing.x - controls.art.focus_padding.x;
        const mattmath::Vector2F value_limit(
            controls.controls_position.x - controls.art.focus_padding.x * 2.0f - controls.art.shadow_offset -
                controls.layout.position.x - controls.layout.spacing.x,
            controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
        require(label_width == 482.0f && value_limit == mattmath::Vector2F(332.0f, 68.0f),
            "Controls fitted-row fixture no longer exercises the shipped column and height limits");
        for (size_t index = 0; index < std::size(controls_fitted_items); ++index)
        {
            const int row_index = index < 5 ? static_cast<int>(index) + 1 : static_cast<int>(index) - 5;
            const auto position = controls.layout.position + mattmath::Vector2F(0.0f,
                controls.layout.spacing.y * static_cast<float>(row_index));
            const std::wstring& text = look.menu_text().item(controls_fitted_items[index]);
            const auto original = world.render.measure_text(font, text);
            require(original.x > label_width, "Long Controls label already fits without scaling");
            int adjustments = 0;
            OptionRow row("controls_fit", text, position,
                position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f), style,
                controls.art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; },
                &controls.art, value_limit);
            UiText* label = row_text(row, "controls_fit_label");
            UiText* value = row_text(row, "controls_fit_value");
            require(label != nullptr && value != nullptr && label->text() == text &&
                std::isfinite(label->bounds().width) && label->bounds().width > 0.0f &&
                label->bounds().width <= label_width + 0.01f && label->bounds().width < original.x &&
                label->bounds().height > 0.0f && label->bounds().height <= value_limit.y + 0.01f,
                "Actual Controls OptionRow truncated or failed to fit its authored label");
            const auto retained_label = label->bounds();
            row.set_value(std::wstring(100, L'W'));
            require(value->bounds().width > 0.0f && value->bounds().width <= value_limit.x + 0.01f &&
                value->bounds().height <= value_limit.y + 0.01f && label->bounds() == retained_label &&
                label->text() == text,
                "Controls value fitting changed its authored label or escaped the actual value limits");
            row.set_value(L"5%");
            require(value->text() == L"5%" && value->bounds().width > 0.0f &&
                value->bounds().width <= value_limit.x + 0.01f && label->bounds() == retained_label,
                "Shorter Controls values did not refit independently of the authored label");
            row.adjust(Direction::right);
            require(adjustments == 1, "Authored Controls label lost its right adjustment callback");
            row.adjust(Direction::left);
            require(adjustments == 0, "Authored Controls labels changed their adjustment callback");
            row.set_hidden(true);
            DrawList no_device;
            row.draw(no_device);
        }
        // Exercise the second fit step independently: this short label needs
        // no width reduction, while its font is just taller than Controls' 68.
        style.item_font = "menu_controls_row_overflow";
        OptionRow height_row("controls_height", L"i", controls.layout.position,
            controls.layout.position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f),
            style, controls.art.ink, &world.render, {}, &controls.art, value_limit);
        const UiText* fitted = row_text(height_row, "controls_height_label");
        require(fitted != nullptr && fitted->text() == L"i" && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 68.01f && fitted->bounds().height < 68.25f &&
            fitted->bounds().width > 0.0f && fitted->bounds().width < 8.0f,
            "Controls OptionRow did not preserve and shrink a height-constrained authored label");
    }

    void select_title_font(rapidjson::Document& d, const char* key, const char* font)
    {
        if (std::string(key) == "wordmark")
        {
            d["main_menu"]["fonts"]["title"].SetString(font, d.GetAllocator());
        }
        else
        {
            d["menu_style"]["fonts"][std::string(key) == "subtitle" ? "detail" : "item"].SetString(font, d.GetAllocator());
        }
    }

    void check_title_and_home_text(Variants& variants, World& world, const Presentation& defaults)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : title_text)
            {
                d["menu_text"]["title"][entry.key].SetString(" A\xC2\xA0\xE2\x80\x94" "B ");
            }
            d["menu_text"]["title"]["wordmark"].SetString(" A\xC2\xA0\xE2\x80\x94" "B \n C ");
            d["menu_text"]["home_subtitle"].SetString(" A\xC2\xA0\xE2\x80\x94" "B ");
        });
        require(normalized.menu_text().title.wordmark == L" A -B \n C " &&
            normalized.menu_text().title.subtitle == L" A -B " && normalized.menu_text().title.start == L" A -B " &&
            normalized.menu_text().title.no_controller == L" A -B " && normalized.menu_text().home_subtitle == L" A -B ",
            "Title/Home text did not preserve authored spacing/LF and normalize punctuation/NBSP");
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : title_text)
        {
            const std::string field = std::string("title.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["title"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (std::string(entry.key) != "wordmark")
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["title"][entry.key].SetString("line\nbreak");
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                select_title_font(d, entry.key, "menu_text_limited");
                d["menu_text"]["title"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["title"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
        {
            variants.rejects("title.wordmark", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["title"]["wordmark"].SetString(invalid, d.GetAllocator());
            });
        }
        variants.rejects("title.wordmark", [](rapidjson::Document& d)
        {
            d["menu_text"]["title"]["wordmark"].SetString("COLOUR\nWARS\nAGAIN");
        }, &world);
        variants.rejects("title.wordmark", [](rapidjson::Document& d)
        {
            d["main_menu"]["fonts"]["title"].SetString("menu_text_tall");
        }, &world);
        for (const char* invalid : { "   ", "line\nbreak", "line\rbreak", "tab\ttext", "\xC3", "\xC3\xA9" })
        {
            variants.rejects("home_subtitle", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["home_subtitle"].SetString(invalid, d.GetAllocator());
            });
        }
        variants.rejects("home_subtitle", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
            d["menu_text"]["home_subtitle"].SetString("Glyph~");
        }, &world);
        variants.rejects("home_subtitle", [](rapidjson::Document& d)
        {
            d["menu_text"]["home_subtitle"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);

        const auto font = world.render.resolve_sprite_font(defaults.main_menu().title_font);
        const auto measured = world.render.measure_text(font, defaults.menu_text().title.wordmark);
        const auto first_line = world.render.measure_text(font, L"COLOUR");
        const auto second_line = world.render.measure_text(font, L"WARS");
        require(measured.x == std::max(first_line.x, second_line.x) &&
            measured.y > std::max(first_line.y, second_line.y),
            "The authored wordmark no longer measures as two complete unscaled lines");
        const mattmath::Vector2F position((1920.0f - measured.x) / 2.0f, defaults.main_menu().title_y);
        UiText wordmark("wordmark", defaults.menu_text().title.wordmark, defaults.main_menu().title_font,
            position, &world.render, defaults.main_menu().art.ink);
        require(wordmark.text() == L"COLOUR\nWARS" &&
            wordmark.bounds() == mattmath::RectangleF(position, measured),
            "Actual UiText changed the wordmark's authored lines, centering or unscaled font bounds");
        wordmark.set_hidden(true);
        DrawList no_device;
        wordmark.draw(no_device);
    }

    void check_title_and_home_bounds(Variants& variants, World& world)
    {
        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("menu_text_exact");
            }
            d["main_menu"]["fonts"]["title"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : title_text) { d["menu_text"]["title"][entry.key].SetString("Fit"); }
            d["menu_text"]["title"]["wordmark"].SetString("Fit\nFit");
            d["menu_text"]["home_subtitle"].SetString("Fit");
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
        };
        const auto multiline = variants.load([&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_text"]["title"]["wordmark"].SetString("Fit\nFit\nFit");
        });
        multiline.validate_resources(world.render, world.audio);
        require(multiline.menu_text().title.wordmark == L"Fit\nFit\nFit",
            "Wordmark line breaks became a fixed two-line replacement instead of authored text");
        for (const auto& entry : title_text)
        {
            const size_t fits_count = std::string(entry.key) == "start" ? 158 : 165;
            const auto edge = [&](rapidjson::Document& d)
            {
                exact(d);
                d["menu_text"]["title"][entry.key].SetString(std::string(fits_count, 'F').c_str(), d.GetAllocator());
            };
            const auto fits = variants.load(edge);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(std::string("title.") + entry.key, [&](rapidjson::Document& d)
            {
                edge(d);
                d["menu_text"]["title"][entry.key].SetString(std::string(fits_count + 1, 'F').c_str(), d.GetAllocator());
            }, &world);
        }
        struct OrderEdge { const char* position; float value; float outside; const char* field; };
        constexpr OrderEdge title_edges[]{
            { "title_subtitle_y", 130.0f, 129.75f, "title.wordmark" },
            { "title_start_y", 628.0f, 627.75f, "title.subtitle" },
            { "title_no_pad_y", 718.0f, 717.75f, "title.start" },
            { "build_id_y", 820.0f, 819.75f, "title.no_controller" }
        };
        for (const auto& edge : title_edges)
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"][edge.position].SetFloat(edge.value);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(edge.field, [&](rapidjson::Document& d)
            {
                edit(d);
                d["main_menu"][edge.position].SetFloat(edge.outside);
            }, &world);
        }
        for (const auto& edge : { std::pair<float, float>{ 200.0f, 199.75f }, { 382.0f, 382.25f } })
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["home_subtitle_position"]["y"].SetFloat(edge.first);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects("home_subtitle", [&](rapidjson::Document& d)
            {
                edit(d);
                d["main_menu"]["home_subtitle_position"]["y"].SetFloat(edge.second);
            }, &world);
        }
        const auto home_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["home_subtitle_position"]["x"].SetFloat(1596.0f);
        };
        const auto fits_home = variants.load(home_edge);
        fits_home.validate_resources(world.render, world.audio);
        variants.rejects("home_subtitle", [&](rapidjson::Document& d)
        {
            home_edge(d);
            d["main_menu"]["home_subtitle_position"]["x"].SetFloat(1596.25f);
        }, &world);
    }

    void check_help_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : help_text)
            {
                d["menu_text"]["help"][entry.key].SetString(entry.multiline ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ", d.GetAllocator());
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : help_text)
        {
            require(normalized.menu_text().help.*entry.field == (entry.multiline ? L" A -B \n C " : L" A -B "),
                std::string("Help text lost authored spacing/LF or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("help.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["help"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        d["menu_text"]["help"][entry.key].SetString(invalid, d.GetAllocator());
                    });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["help"][entry.key].SetString("line\nbreak");
                });
            }
            // Heading, item and ordinary detail fonts still contain '~'.
            // Only this help font lacks it, and its stand-in must not hide that.
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["controls_settings"]["help_font"].SetString("menu_text_limited");
                d["menu_text"]["help"][entry.key].SetString("Glyph~");
            }, &world);
        }
        const auto single_line_blocks = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["controls"].SetString("Controls");
            d["menu_text"]["help"]["pad_association"].SetString("Slots");
        });
        single_line_blocks.validate_resources(world.render, world.audio);
        require(single_line_blocks.menu_text().help.controls == L"Controls" &&
            single_line_blocks.menu_text().help.pad_association == L"Slots",
            "Help blocks replaced authored copy with fixed line counts");
        variants.rejects("help.aim_cutoff", [](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);
    }

    void check_help_fitting(Variants& variants, World& world)
    {
        const auto check_blocks = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            const auto& controls = look.controls_settings();
            const auto font = world.render.resolve_sprite_font(controls.help_font);
            for (const auto& entry : help_text)
            {
                if (!entry.multiline) { continue; }
                const bool is_controls = std::string(entry.key) == "controls";
                const auto position = is_controls ? controls.controls_position : controls.stick_preview.association_position;
                const auto available = is_controls ? controls.controls_size :
                    controls.controls_position + controls.controls_size - position;
                std::wstring authored = look.menu_text().help.*entry.field;
                const auto measured = world.render.measure_text(font, authored);
                if (width_limited)
                {
                    require(measured.x > available.x && measured.y < available.y,
                        "Long help fixture does not independently exercise width fitting");
                }
                if (height_limited)
                {
                    require(measured.x < available.x && measured.y > available.y,
                        "Tall help fixture does not independently exercise height fitting");
                }
                UiText text(entry.key, authored, controls.help_font, position, &world.render, controls.art.ink);
                const auto retained = authored;
                authored = L"Changed after construction";
                require(text.text() == retained && text.bounds() == mattmath::RectangleF(position, measured),
                    "Actual help UiText borrowed its source or changed its unscaled measurement");
                text.set_scale(std::min({ 1.0f, available.x / measured.x, available.y / measured.y }));
                require(text.text() == retained && text.bounds().position() == position &&
                    std::isfinite(text.bounds().width) && std::isfinite(text.bounds().height) &&
                    text.bounds().width > 0.0f && text.bounds().height > 0.0f &&
                    text.bounds().width <= available.x + 0.01f && text.bounds().height <= available.y + 0.01f,
                    "Actual help UiText lost text, moved, or escaped its fitted help rectangle");
                if (width_limited || height_limited)
                {
                    require(text.bounds().width < measured.x && text.bounds().height < measured.y,
                        "Help block did not retain proportional down-scaling");
                }
                else
                {
                    require(text.bounds() == mattmath::RectangleF(position, measured),
                        "Short help blocks were enlarged above their authored font size");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        const auto short_blocks = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["controls"].SetString("Fit\nFit");
            d["menu_text"]["help"]["pad_association"].SetString("Fit\nFit");
        });
        short_blocks.validate_resources(world.render, world.audio);
        check_blocks(short_blocks, false, false);
        const auto wide_blocks = variants.load([](rapidjson::Document& d)
        {
            for (const char* key : { "controls", "pad_association" })
            {
                d["menu_text"]["help"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide_blocks.validate_resources(world.render, world.audio);
        check_blocks(wide_blocks, true, false);
        const auto tall_blocks = variants.load([](rapidjson::Document& d)
        {
            std::string lines = "Fit";
            for (int line = 1; line < 50; ++line) { lines += "\nFit"; }
            for (const char* key : { "controls", "pad_association" })
            {
                d["menu_text"]["help"][key].SetString(lines.c_str(), d.GetAllocator());
            }
        });
        tall_blocks.validate_resources(world.render, world.audio);
        check_blocks(tall_blocks, false, true);
    }

    void check_setting_percentage_text(Variants& variants, World& world, const MenuText& defaults)
    {
        for (int value = 0; value <= 100; ++value)
        {
            require(defaults.setting_percentage(value) == std::to_wstring(value) + L"%",
                "Default setting percentage changed its exact integer, percent sign or inclusive domain");
        }
        require(defaults.setting_percentage(17) == L"17%" && defaults.setting_percentage(100) == L"100%",
            "Setting percentage lost a non-step value or the inclusive upper endpoint");
        PlaySettings inherited;
        const int rounded_movement = static_cast<int>(std::lround(inherited.movement_deadzone(0, 0.999f) * 100.0f));
        require(inherited.sticks[0].movement_deadzone == -1 && rounded_movement == 100 &&
            defaults.setting_percentage(rounded_movement) == L"100%",
            "Inherited movement rounding no longer reaches 100 without clamping the saved deadzone profile");
        const auto reject_format = [](auto format)
        {
            bool rejected = false;
            try { format(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Invalid setting percentage argument or direct template formatted silently");
        };
        for (int invalid : {std::numeric_limits<int>::min(), -1, 101, std::numeric_limits<int>::max()})
        {
            reject_format([&] { defaults.setting_percentage(invalid); });
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["percentage"].SetString("  % {value}\xC2\xA0\xE2\x80\x9C" "level\xE2\x80\x9D\xE2\x80\x94" "set\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().values.percentage == L"  % {value} \"level\"-set...  " &&
            normalized.menu_text().setting_percentage(17) == L"  % 17 \"level\"-set...  ",
            "Setting percentage lost normalized punctuation, authored outer spaces or literal placement");
        const auto literal = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["percentage"].SetString("value={value} literal%%");
        });
        literal.validate_resources(world.render, world.audio);
        require(literal.menu_text().setting_percentage(0) == L"value=0 literal%%",
            "Setting percentage treated authored percent signs as another formatting language");
        for (const char* blank : {"   ", " \xC2\xA0 "})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["percentage"].SetString(blank, d.GetAllocator()); });
        }
        for (const char* extra : {"\n", "\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
            {
                const std::string invalid = std::string("{value}%") + extra;
                d["menu_text"]["values"]["percentage"].SetString(invalid.c_str(), d.GetAllocator());
            });
        }
        for (const char* invalid : {"No token", "{value}{value}", "{Value}", "{value }", "{{value}}", "{value",
            "value}", "{value}{unknown}", "{va{value}lue}", "{value}{count}", "{value}%{"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["percentage"].SetString(invalid, d.GetAllocator()); });
            MenuText mutated = defaults;
            mutated.values.percentage = std::wstring(invalid, invalid + std::strlen(invalid));
            reject_format([&] { mutated.setting_percentage(17); });
        }
        for (const wchar_t* invalid : {L"", L"   ", L"{value}\n", L"{value}\r", L"{value}\t", L"{value}\x01",
            L"{value}\x7f", L"{value}\u00e9", L"{value}\u00a0", L"{value}\u2014"})
        {
            MenuText mutated = defaults;
            mutated.values.percentage = invalid;
            reject_format([&] { mutated.setting_percentage(17); });
        }
        MenuText nul = defaults;
        nul.values.percentage.push_back(L'\0');
        reject_format([&] { nul.setting_percentage(17); });
    }

    void check_setting_percentage_rows(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, char32_t missing = U'\0', bool zero_digits = false,
            float tilde_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool zero = zero_digits && ((character >= U'0' && character <= U'9') || character == U'%');
                glyphs.push_back({character, mattmath::RectangleI(0, 0, zero ? 0 : character == U'~' ? width : 8,
                    character == U'~' ? height : 16), character == U'~' ? tilde_offset : 0.0f, 0.0f, 0.0f});
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        add_font("menu_percentage_width", 1000, 16);
        add_font("menu_percentage_height", 8, 200);
        add_font("menu_percentage_both", 2000, 2000);
        add_font("menu_percentage_missing8", 8, 16, U'8');
        add_font("menu_percentage_missing1", 8, 16, U'1');
        add_font("menu_percentage_zero", 8, 16, U'\0', true);
        add_font("menu_percentage_nonfinite", 8, 16, U'\0', false, std::numeric_limits<float>::infinity());
        const auto check_rows = [&](const Presentation& look, int fit)
        {
            const auto& controls = look.controls_settings();
            MenuStyle style = look.menu();
            style.item_font = controls.item_font;
            const auto& art = controls.art;
            const mattmath::Vector2F limit(controls.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset -
                controls.layout.position.x - controls.layout.spacing.x,
                controls.layout.spacing.y - art.focus_padding.y * 2.0f);
            require(limit == mattmath::Vector2F(332.0f, 68.0f), "Percentage rows lost the actual Controls/Sticks value limits");
            struct Use { int row; MenuItem label; };
            constexpr Use uses[]{{1, MenuItem::music_volume}, {2, MenuItem::sfx_volume},
                {1, MenuItem::move_deadzone}, {2, MenuItem::aim_deadzone}};
            for (const auto& use : uses)
            {
                const auto position = controls.layout.position + mattmath::Vector2F(0.0f,
                    controls.layout.spacing.y * static_cast<float>(use.row));
                const auto value_position = position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f);
                OptionRow row("percentage", look.menu_text().item(use.label), position, value_position,
                    style, art.ink, &world.render, {}, &art, limit);
                UiText* value = row_text(row, "percentage_value");
                UiText* label = row_text(row, "percentage_label");
                require(value != nullptr && label != nullptr, "Percentage fixture lost its real OptionRow widgets");
                const auto label_bounds = label->bounds();
                std::wstring copy = look.menu_text().setting_percentage(17);
                const auto retained = copy;
                const auto raw = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), retained);
                row.set_value(copy);
                copy = L"Changed after set_value";
                const auto bounds = value->bounds();
                require(value->text() == retained && bounds.position() == value_position && label->bounds() == label_bounds &&
                    bounds.width > 0.0f && bounds.height > 0.0f && bounds.width <= 332.01f && bounds.height <= 68.01f,
                    "Percentage OptionRow lost its copied text, fixed position/label or fitted dimensions");
                if (fit == 1)
                {
                    require(raw == mattmath::Vector2F(1024.0f, 20.0f) && std::abs(bounds.width - 332.0f) <= 0.01f &&
                        std::abs(bounds.height - 6.484375f) <= 0.01f,
                        "Long percentage copy did not retain its independent width-only fit");
                }
                else if (fit == 2 || fit == 3)
                {
                    const float expected_width = fit == 2 ? 10.88f : 68.816f;
                    require(raw == (fit == 2 ? mattmath::Vector2F(32.0f, 200.0f) : mattmath::Vector2F(2024.0f, 2000.0f)) &&
                        std::abs(bounds.height - 68.0f) <= 0.01f && std::abs(bounds.width - expected_width) <= 0.01f,
                        "Percentage copy did not fit height after its independent width decision");
                }
                else { require(bounds == mattmath::RectangleF(value_position, raw), "Default percentage was enlarged or unnecessarily shrunk"); }
                auto padded = bounds;
                padded.inflate(art.focus_padding);
                const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, style.footer_y) - controls.box_size) / 2.0f,
                    controls.box_size);
                require(paper.contains(padded) && padded.right() <= controls.controls_position.x &&
                    padded.bottom() <= controls.status_position.y && bounds.height + art.focus_padding.y * 2.0f <= controls.layout.spacing.y + 0.01f,
                    "Actual percentage value/focus padding escaped its paper, row, help column or status boundary");
                MenuText short_copy = look.menu_text();
                short_copy.values.percentage = L"{value}%";
                row.set_value(short_copy.setting_percentage(0));
                const auto short_size = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), L"0%");
                require(value->text() == L"0%" && value->bounds() == mattmath::RectangleF(value_position, short_size) &&
                    label->bounds() == label_bounds, "Short percentage replacement retained a stale shrink scale or changed its label");
            }
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio);
        check_rows(defaults, 0);
        const char* fonts[]{"menu_percentage_width", "menu_percentage_height", "menu_percentage_both"};
        for (int fit = 1; fit <= 3; ++fit)
        {
            const auto fitted = variants.load([&](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString(fonts[fit - 1], d.GetAllocator());
                d["menu_text"]["values"]["percentage"].SetString("~{value}%");
            });
            fitted.validate_resources(world.render, world.audio);
            check_rows(fitted, fit);
        }
        variants.rejects("values.percentage", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_missing8"); }, &world);
        // Pad 1's earlier selector guard owns this shared-font failure. The
        // independent missing8 case above reaches the new 101-output check.
        variants.rejects("labels.pad", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_missing1"); }, &world);
        variants.rejects("values.percentage", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_zero"); }, &world);
        for (const char* font : {"menu_text_limited", "menu_percentage_nonfinite"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString(font, d.GetAllocator());
                d["menu_text"]["values"]["percentage"].SetString("~{value}%");
            }, &world);
        }
        const auto unrelated_font = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["percentage"].SetString("~{value}%");
        });
        unrelated_font.validate_resources(world.render, world.audio);
    }

    void check_value_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : value_text)
            {
                d["menu_text"]["values"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : value_text)
        {
            require(normalized.menu_text().values.*entry.field == L"  A \"B\"-C...  ",
                std::string("Fixed setting value lost authored spacing or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("values.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["values"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["values"][entry.key].SetString("Glyph~");
            }, &world);
            if (entry.field != &MenuValueText::random)
            {
                // The Options font can draw this glyph, but the independently
                // selected Controls item font must support On and Off too.
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["controls_settings"]["item_font"].SetString("menu_text_limited");
                    d["menu_text"]["values"][entry.key].SetString("Glyph~");
                }, &world);
            }
        }
        const auto random_uses_shared_item_font = variants.load([](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_limited");
            // Keep the supported '~' probe below the arena preview border;
            // unrelated descenders in "Glyph" would exceed its name band.
            d["menu_text"]["values"]["random"].SetString("G~");
        });
        random_uses_shared_item_font.validate_resources(world.render, world.audio);
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? 0 : 8, 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_value_zero_width", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_value_zero_width"), L"~").x == 0.0f,
            "Nonpositive value-size fixture has unexpected font advance");
        for (const auto& entry : value_text)
        {
            variants.rejects(std::string("values.") + entry.key, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_value_zero_width");
                d["menu_text"]["values"][entry.key].SetString("~");
            }, &world);
            if (entry.field != &MenuValueText::random)
            {
                variants.rejects(std::string("values.") + entry.key, [&](rapidjson::Document& d)
                {
                    d["controls_settings"]["item_font"].SetString("menu_value_zero_width");
                    d["menu_text"]["values"][entry.key].SetString("~");
                }, &world);
            }
        }
    }

    void check_value_fitting(Variants& variants, World& world)
    {
        struct ValueUse { bool options; int row; MenuItem label; std::wstring MenuValueText::* field; };
        constexpr ValueUse uses[]{
            { true, 1, MenuItem::fullscreen, &MenuValueText::on },
            { true, 1, MenuItem::fullscreen, &MenuValueText::off },
            { true, 2, MenuItem::team_palette, &MenuValueText::random },
            { false, 3, MenuItem::view_shake, &MenuValueText::on },
            { false, 3, MenuItem::view_shake, &MenuValueText::off },
            { false, 4, MenuItem::aim_marker, &MenuValueText::on },
            { false, 4, MenuItem::aim_marker, &MenuValueText::off },
            { false, 5, MenuItem::reduce_effects, &MenuValueText::on },
            { false, 5, MenuItem::reduce_effects, &MenuValueText::off }
        };
        const auto check_rows = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            for (const auto& use : uses)
            {
                const auto& main = look.main_menu();
                const auto& controls = look.controls_settings();
                MenuStyle style = look.menu();
                if (!use.options) { style.item_font = controls.item_font; }
                const auto art = use.options ? options_art(main) : controls.art;
                const auto box_size = use.options ? main.options_box_size : controls.box_size;
                const auto spacing = use.options ? main.options_spacing : controls.layout.spacing;
                const auto label_position = (use.options ? main.options_position : controls.layout.position) +
                    mattmath::Vector2F(0.0f, spacing.y * static_cast<float>(use.row));
                const auto value_position = use.options ? mattmath::Vector2F(main.options_value_x, label_position.y) :
                    label_position + mattmath::Vector2F(spacing.x, 0.0f);
                const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, look.menu().footer_y) - box_size) / 2.0f, box_size);
                const mattmath::Vector2F limit(use.options ?
                    paper.right() - art.shadow_offset - art.focus_padding.x - value_position.x :
                    controls.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset - value_position.x,
                    spacing.y - art.focus_padding.y * 2.0f);
                require(limit == (use.options ? mattmath::Vector2F(930.0f, 86.0f) : mattmath::Vector2F(332.0f, 68.0f)),
                    "Authored value fixture no longer exercises the shipped Options/Controls fitting limits");
                const auto font = world.render.resolve_sprite_font(style.item_font);
                std::wstring authored = look.menu_text().values.*use.field;
                const auto measured = world.render.measure_text(font, authored);
                float scale = 1.0f;
                if (measured.x > limit.x) { scale = limit.x / measured.x; }
                const float width_scale = scale;
                if (measured.y * scale > limit.y) { scale = limit.y / measured.y; }
                require(width_limited == (measured.x > limit.x) &&
                    height_limited == (measured.y * width_scale > limit.y),
                    "Authored value fixture does not independently exercise its expected fitting constraints");
                int adjustments = 0;
                OptionRow row("authored_value", look.menu_text().item(use.label), label_position, value_position,
                    style, art.ink, &world.render,
                    [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; }, &art, limit);
                UiText* value = row_text(row, "authored_value_value");
                UiText* label = row_text(row, "authored_value_label");
                require(value != nullptr && label != nullptr, "Authored value fixture lost its real OptionRow text widgets");
                const auto retained_label = label->bounds();
                row.set_value(authored);
                const auto retained = authored;
                authored = L"Changed after set_value";
                const auto bounds = value->bounds();
                require(value->text() == retained && bounds.position() == value_position &&
                    std::isfinite(bounds.width) && std::isfinite(bounds.height) && bounds.width > 0.0f && bounds.height > 0.0f &&
                    std::abs(bounds.width - measured.x * scale) <= 0.01f &&
                    std::abs(bounds.height - measured.y * scale) <= 0.01f &&
                    bounds.width <= limit.x + 0.01f && bounds.height <= limit.y + 0.01f &&
                    bounds.height + art.focus_padding.y * 2.0f <= spacing.y + 0.01f && label->bounds() == retained_label,
                    "Actual OptionRow lost its value copy, moved it, changed its label, or escaped width-then-height fitting");
                auto padded = bounds;
                padded.inflate(art.focus_padding);
                require(paper.contains(padded) && (use.options ||
                    (padded.right() <= controls.controls_position.x && padded.bottom() <= controls.status_position.y)),
                    "Fitted setting value or focus padding escaped its paper, help column or status boundary");
                if (height_limited)
                {
                    require(std::abs(bounds.height + art.focus_padding.y * 2.0f - spacing.y) <= 0.01f,
                        "Height-fitted value did not retain inclusive contact with its padded row limit");
                }
                else if (width_limited)
                {
                    require(std::abs(bounds.width - limit.x) <= 0.01f,
                        "Width-fitted value did not retain inclusive contact with its column limit");
                }
                else { require(bounds == mattmath::RectangleF(value_position, measured), "Short values were enlarged"); }
                row.set_value(L"Fit");
                require(value->text() == L"Fit" && value->bounds() == mattmath::RectangleF(value_position,
                    world.render.measure_text(font, L"Fit")) && label->bounds() == retained_label,
                    "Changing a long/tall value retained its old scale or changed the authored label");
                row.adjust(Direction::right);
                require(adjustments == 1, "Authored value fitting lost the row's right adjustment action");
                row.adjust(Direction::left);
                require(adjustments == 0, "Authored value fitting changed the row's adjustment action");
                row.set_hidden(true);
                DrawList no_device;
                row.draw(no_device);
            }
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        require(defaults.menu().item_font != defaults.controls_settings().item_font,
            "Authored values must exercise the separate shipped Options and Controls fonts");
        defaults.validate_resources(world.render, world.audio);
        check_rows(defaults, false, false);
        const auto wide = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : value_text)
            {
                d["menu_text"]["values"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide.validate_resources(world.render, world.audio);
        check_rows(wide, true, false);
        for (const bool wide_glyph : { false, true })
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0,
                    character == U'~' && wide_glyph ? 2000 : 8,
                    character == U'~' ? (wide_glyph ? 2000 : 200) : 16), 0.0f, 0.0f, 0.0f });
            }
            const char* font_name = wide_glyph ? "menu_value_width_then_height" : "menu_value_height_only";
            world.render.add_font(font_name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
            const auto tall = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
                d["controls_settings"]["item_font"].SetString(font_name, d.GetAllocator());
                for (const auto& entry : value_text) { d["menu_text"]["values"][entry.key].SetString("~"); }
                // Random also supplies the arena name, whose existing fit
                // shrinks only to preview width. A narrow synthetic preview
                // keeps that separate consumer valid while all nine real
                // OptionRow uses still exercise their width/height limits.
                d["main_menu"]["stage_select"]["preview_size"]["x"].SetFloat(2.0f);
            });
            tall.validate_resources(world.render, world.audio);
            check_rows(tall, wide_glyph, true);
        }
    }

    void check_stage_random_fitting(Variants& variants, World& world)
    {
        const auto check_widget = [&](const Presentation& look, bool width_limited)
        {
            const auto& stage = look.main_menu().stage_select;
            const auto& style = look.menu();
            const auto font = world.render.resolve_sprite_font(style.item_font);
            std::wstring source = look.menu_text().values.random;
            const auto measured = world.render.measure_text(font, source);
            const auto retained = source;
            UiTextDropShadow name("random_stage_name", source, style.item_font, stage.name_position,
                &world.render, stage.unselected, style.shadow_colour, style.item_shadow_offset);
            source = L"Changed after construction";
            const mattmath::RectangleF raw = mattmath::RectangleF::union_of(
                mattmath::RectangleF(mattmath::Vector2F::ZERO, measured),
                mattmath::RectangleF(style.item_shadow_offset, measured));
            require(name.text() == retained && name.bounds() == mattmath::RectangleF::union_of(
                name.text_bounds_at(stage.name_position, 1.0f),
                name.text_bounds_at(stage.name_position + style.item_shadow_offset, 1.0f)) &&
                raw.width == measured.x + std::abs(style.item_shadow_offset.x) &&
                raw.height == measured.y + std::abs(style.item_shadow_offset.y) &&
                width_limited == (raw.width > stage.preview_size.x),
                "Actual Random stage name lost its source copy, complete signed shadow union or expected width fitting");
            const auto fit = [&]()
            {
                // The runtime explicitly measures at scale 1 before every
                // update, even when the previous displayed slot was shrunk.
                const auto bounds = mattmath::RectangleF::union_of(
                    name.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                    name.text_bounds_at(style.item_shadow_offset, 1.0f));
                const float scale = std::min(1.0f, stage.preview_size.x / bounds.width);
                name.set_scale(scale);
                name.set_shadow_scale(scale);
                name.set_shadow_offset(style.item_shadow_offset * scale);
                return scale;
            };
            const float scale = fit();
            const mattmath::RectangleF expected(stage.name_position + raw.position() * scale, raw.size() * scale);
            const auto actual = name.bounds();
            require(name.text() == retained && name.text_bounds() == name.text_bounds_at(stage.name_position, scale) &&
                std::abs(actual.x - expected.x) <= 0.01f && std::abs(actual.y - expected.y) <= 0.01f &&
                std::abs(actual.width - expected.width) <= 0.01f && std::abs(actual.height - expected.height) <= 0.01f &&
                name.shadow_scale() == scale && name.shadow_offset() == style.item_shadow_offset * scale &&
                std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.width) &&
                std::isfinite(actual.height) && actual.width > 0.0f && actual.height > 0.0f &&
                actual.width <= stage.preview_size.x + 0.01f &&
                std::abs(actual.height - raw.height * scale) <= 0.01f && actual.x >= 0.0f && actual.y >= 0.0f &&
                actual.right() <= 1920.0f && actual.bottom() <= stage.preview_position.y - style.rule_height + 0.01f,
                "Random stage name changed position, lost shadow scaling, wrapped, or escaped proportional width-only fitting");
            require(width_limited ? std::abs(actual.width - stage.preview_size.x) <= 0.01f : scale == 1.0f,
                "Random stage name did not fill its width budget or enlarged a short authored name");
            std::wstring replacement = L"Fit";
            name.set_text(replacement);
            replacement = L"Changed after set_text";
            const float reset_scale = fit();
            const auto reset = world.render.measure_text(font, L"Fit");
            require(reset_scale == 1.0f && name.text() == L"Fit" && name.shadow_scale() == 1.0f &&
                name.shadow_offset() == style.item_shadow_offset && name.bounds() == mattmath::RectangleF::union_of(
                    mattmath::RectangleF(stage.name_position, reset),
                    mattmath::RectangleF(stage.name_position + style.item_shadow_offset, reset)),
                "Replacing a fitted Random stage name retained its shrink scale, lost its copied text or failed to reset its shadow");
            name.set_hidden(true);
            DrawList no_device;
            name.draw(no_device);
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio);
        check_widget(defaults, false);
        for (const bool wide : { false, true })
        {
            for (const bool negative_shadow : { false, true })
            {
                const auto authored = variants.load([&](rapidjson::Document& d)
                {
                    d["menu_text"]["values"]["random"].SetString(wide ? std::string(200, 'W').c_str() : "Fit", d.GetAllocator());
                    if (negative_shadow)
                    {
                        d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
                        d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(-1.5f);
                    }
                });
                authored.validate_resources(world.render, world.audio);
                check_widget(authored, wide);
            }
        }
    }

    void check_stage_random_bounds(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const float preview_top = original.main_menu().stage_select.preview_position.y - original.menu().rule_height;
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            d["menu_text"]["values"]["random"].SetString("Fit");
        };
        struct Edge { const char* axis; float position; float outside; };
        const Edge edges[]{ { "x", 0.0f, -0.25f }, { "x", 1894.5f, 1894.75f },
            { "y", 0.0f, -0.25f }, { "y", preview_top - 21.5f, preview_top - 21.25f } };
        for (const auto& edge : edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["stage_select"]["name_position"][edge.axis].SetFloat(edge.position);
            };
            const auto touches = variants.load(contact);
            touches.validate_resources(world.render, world.audio);
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["name_position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        const auto signed_contact = [&](rapidjson::Document& d)
        {
            exact(d);
            for (const char* axis : { "x", "y" })
            {
                d["menu_style"]["shadow"]["item_offset"][axis].SetFloat(-1.5f);
                d["main_menu"]["stage_select"]["name_position"][axis].SetFloat(1.5f);
            }
        };
        const auto signed_shadow = variants.load(signed_contact);
        signed_shadow.validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                signed_contact(d);
                d["main_menu"]["stage_select"]["name_position"][axis].SetFloat(1.25f);
            }, &world);
        }
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? 2018 : 8,
                character == U'~' ? 160 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_stage_random_half_scale",
            std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto scaled_contact = [&](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_stage_random_half_scale");
            d["menu_text"]["values"]["random"].SetString("~");
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(2.0f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(2.0f);
            d["main_menu"]["stage_select"]["name_position"]["y"].SetFloat(preview_top - 81.0f);
        };
        const auto scaled = variants.load(scaled_contact);
        scaled.validate_resources(world.render, world.audio);
        const auto measured = world.render.measure_text(world.render.resolve_sprite_font(scaled.menu().item_font), L"~");
        require(measured == mattmath::Vector2F(2018.0f, 160.0f) &&
            scaled.main_menu().stage_select.preview_size.x / (measured.x + 2.0f) == 0.5f,
            "Random stage boundary fixture no longer fits the complete 2020x162 shadow union at exactly half scale");
        variants.rejects("values.random", [&](rapidjson::Document& d)
        {
            scaled_contact(d);
            d["main_menu"]["stage_select"]["name_position"]["y"].SetFloat(preview_top - 80.75f);
        }, &world);
        // OptionRow can fit this tall glyph to its height. The arena name
        // cannot: its existing behavior permits only proportional width fit.
        variants.rejects("values.random", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_value_height_only");
            d["menu_text"]["values"]["random"].SetString("~");
        }, &world);
        for (const char* axis : { "x", "y" })
        {
            for (const double position : { -1e100, 1e100 })
            {
                variants.rejects("values.random", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["name_position"][axis].SetDouble(position);
                }, &world);
            }
            variants.rejects("", [&](rapidjson::Document& d)
            {
                // This shared setting is also validated by earlier menu
                // consumers; any contextual menu-text rejection is correct.
                d["menu_style"]["shadow"]["item_offset"][axis].SetDouble(1e100);
            }, &world);
        }
        variants.rejects("values.random", [](rapidjson::Document& d)
        {
            // Positive when read as float, but too small to leave a positive
            // fitted scale for the authored name and its complete shadow.
            d["main_menu"]["stage_select"]["preview_size"]["x"].SetDouble(1e-45);
        }, &world);
        for (const double preview_y : { 0.0, -1e100, 1e100 })
        {
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                d["main_menu"]["stage_select"]["preview_position"]["y"].SetDouble(preview_y);
            }, &world);
        }
        const auto reject_preview_width = [&](double width)
        {
            const auto path = variants.write([&](rapidjson::Document& d)
            {
                d["main_menu"]["stage_select"]["preview_size"]["x"].SetDouble(width);
            });
            std::string diagnostic = "accepted without exception";
            try
            {
                const Presentation invalid(path.string().c_str());
                invalid.validate_resources(world.render, world.audio);
            }
            catch (const std::runtime_error& error) { diagnostic = error.what(); }
            require(diagnostic.find(path.filename().string()) != std::string::npos &&
                diagnostic.find("stage_select") != std::string::npos && diagnostic.find("preview_size") != std::string::npos,
                "Invalid Random stage preview width lost its source and geometry context: " + diagnostic);
        };
        for (const double width : { 0.0, -1.0, 1e100 }) { reject_preview_width(width); }
    }

    void check_stage_count_text(Variants& variants, World& world, const MenuText& defaults)
    {
        struct Navigation { size_t index; size_t total; const wchar_t* expected; };
        constexpr Navigation navigation[]{
            {1, 1, L"<   ARENA 1 / 1   >"}, {1, 10, L"<   ARENA 1 / 10   >"},
            {8, 10, L"<   ARENA 8 / 10   >"}, {10, 10, L"<   ARENA 10 / 10   >"},
            {9, 10, L"<   ARENA 9 / 10   >"}, {99, 100, L"<   ARENA 99 / 100   >"},
            {100, 100, L"<   ARENA 100 / 100   >"}
        };
        for (const auto& entry : navigation)
        {
            require(defaults.stage_navigation(entry.index, entry.total) == entry.expected,
                "Arena counter changed its one-based index, actual total or literal default wording");
        }
        for (size_t count : {size_t{1}, size_t{10}, size_t{100}})
        {
            require(defaults.stage_random_hint(count) == L"Random: " + std::to_wstring(count) +
                L" available arenas.\nConfirm to lock your choice.",
                "Random arena count changed its literal wording, including the original singular copy");
        }
        const size_t maximum = std::numeric_limits<size_t>::max();
        const std::wstring maximum_text = std::to_wstring(maximum);
        require(defaults.stage_navigation(maximum, maximum) == L"<   ARENA " + maximum_text + L" / " + maximum_text + L"   >" &&
            defaults.stage_random_hint(maximum) == L"Random: " + maximum_text +
            L" available arenas.\nConfirm to lock your choice.",
            "Arena count formatting narrowed size_t to a roster-sized or signed domain");
        const auto reject_format = [](auto format)
        {
            bool rejected = false;
            try { format(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Invalid arena count argument or direct template formatted silently");
        };
        for (const auto& entry : {Navigation{0, 0, L""}, Navigation{1, 0, L""}, Navigation{0, 1, L""},
            Navigation{2, 1, L""}, Navigation{maximum, maximum - 1, L""}})
        {
            reject_format([&] { defaults.stage_navigation(entry.index, entry.total); });
        }
        reject_format([&] { defaults.stage_random_hint(0); });
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["stage"]["numbered_navigation"].SetString("  {total}\xC2\xA0\xE2\x80\x94" "{index}\xE2\x80\xA6  ");
            d["menu_text"]["stage"]["random_hint"].SetString("  {count}\xC2\xA0\xE2\x80\x9C" "arenas\xE2\x80\x9D\nReady\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().stage.numbered_navigation == L"  {total} -{index}...  " &&
            normalized.menu_text().stage.random_hint == L"  {count} \"arenas\"\nReady...  " &&
            normalized.menu_text().stage_navigation(8, 10) == L"  10 -8...  " &&
            normalized.menu_text().stage_random_hint(10) == L"  10 \"arenas\"\nReady...  ",
            "Arena count text lost normalized punctuation, reversed token order, LF or authored outer spaces");
        const auto adjacent = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["stage"]["numbered_navigation"].SetString("{total}{index}");
            d["menu_text"]["stage"]["random_hint"].SetString("Choose {count} now");
        });
        adjacent.validate_resources(world.render, world.audio);
        require(adjacent.menu_text().stage_navigation(1, 10) == L"101" &&
            adjacent.menu_text().stage_random_hint(1) == L"Choose 1 now",
            "Arena count templates imposed token separators or a fixed Random hint line count");
        for (const auto& entry : stage_count_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const std::string valid = entry.multiline ? "{count}" : "{index}/{total}";
            for (const char* blank : {"   ", " \xC2\xA0 "})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stage"][entry.key].SetString(blank, d.GetAllocator()); });
            }
            for (const char* extra : {"\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string invalid = valid + extra;
                    d["menu_text"]["stage"][entry.key].SetString(invalid.c_str(), d.GetAllocator());
                });
            }
            const std::vector<std::string> malformed = entry.multiline ? std::vector<std::string>{
                "No token", "{count}{count}", "{Count}", "{count }", "{{count}}", "{count", "count}",
                "{count}{index}", "{co{count}unt}", "{count}{unknown}", "\n{count}", "{count}\n", "{count}\n \nF"} :
                std::vector<std::string>{"{index}", "{total}", "{index}{index}{total}", "{index}{total}{total}",
                    "{Index}/{total}", "{index }/{total}", "{{index}}/{total}", "{index}/{total", "{index}/{total}}",
                    "{index}{total}{count}", "{to{index}tal}", "{in{total}dex}", "{index}{total}{unknown}", "{index}\n{total}"};
            for (const auto& invalid : malformed)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stage"][entry.key].SetString(invalid.c_str(), d.GetAllocator()); });
                MenuText mutated = defaults;
                mutated.stage.*entry.field = std::wstring(invalid.begin(), invalid.end());
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            for (const wchar_t* extra : {L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014", L"\n"})
            {
                MenuText mutated = defaults;
                (mutated.stage.*entry.field) += extra;
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            for (const wchar_t* blank : {L"", L"   "})
            {
                MenuText mutated = defaults;
                mutated.stage.*entry.field = blank;
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            MenuText nul = defaults;
            (nul.stage.*entry.field).push_back(L'\0');
            reject_format([&]
            {
                if (entry.multiline) { nul.stage_random_hint(10); }
                else { nul.stage_navigation(8, 10); }
            });
        }
    }

    void check_stage_count_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int eight_width = 8, int tilde_width = 8,
            int tilde_height = 16, float tilde_y = 0.0f, char32_t missing = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'8' ? eight_width : character == U'~' ? tilde_width : 8,
                    character == U'~' ? tilde_height : 16), 0.0f, character == U'~' ? tilde_y : 0.0f, 0.0f});
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        const auto navigation = [](rapidjson::Document& d, const char* font_name = "menu_stage_count_proportional")
        {
            d["menu_style"]["fonts"]["detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["stage"]["numbered_navigation"].SetString("{index}/{total}");
        };
        const auto rejects_counts = [&](const std::string& key, auto edit, size_t total, size_t eligible)
        {
            const auto path = variants.write(edit);
            std::string diagnostic = "accepted without exception";
            try { Presentation(path.string().c_str()).validate_stage_counts(world.render, total, eligible); }
            catch (const std::runtime_error& error) { diagnostic = error.what(); }
            require(diagnostic.find(path.filename().string()) != std::string::npos &&
                diagnostic.find("menu_text.stage") != std::string::npos && diagnostic.find(key) != std::string::npos,
                "Invalid concrete arena count lost its source/role context: " + diagnostic);
        };
        add_font("menu_stage_count_proportional", 80);
        const auto counter_font = world.render.resolve_sprite_font("menu_stage_count_proportional");
        require(world.render.measure_text(counter_font, L"8/10").x == 104.0f &&
            world.render.measure_text(counter_font, L"10/10").x == 40.0f,
            "Proportional arena fixture no longer makes an earlier index wider than the final one");
        size_t widest = 0;
        float width = 0.0f;
        for (size_t index = 1; index <= 10; ++index)
        {
            const float measured = world.render.measure_text(counter_font, std::to_wstring(index) + L"/10").x;
            if (measured > width) { width = measured; widest = index; }
        }
        require(widest == 8 && width == 104.0f, "Full literal index oracle did not identify the independent worst row");
        const auto edge = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1030.75f);
        };
        const auto fits = variants.load(edge);
        fits.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow counter("count_record_room", L"8/10", "menu_stage_count_proportional",
            fits.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), fits.menu().detail_shadow_offset);
        require(counter.bounds().right() == 1135.75f &&
            1160.0f - counter.bounds().right() - fits.menu().focus_padding.x == 0.25f &&
            counter.shadow_scale() == 1.0f,
            "Unscaled counter no longer leaves exactly a quarter pixel for the adjacent record");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            edge(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1031.0f);
        }, 10, 0);
        const auto many = variants.load([&](rapidjson::Document& d) { navigation(d); });
        for (size_t total : {size_t{1}, size_t{9}, size_t{10}, size_t{99}, size_t{100}})
        {
            many.validate_stage_counts(world.render, total, 0);
        }
        rejects_counts("", [](rapidjson::Document&) {}, 0, 0);
        rejects_counts("", [](rapidjson::Document&) {}, 10, 11);
        const auto negative = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(53.5f);
        };
        const auto left = variants.load(negative);
        left.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow shadowed("count_negative_shadow", L"8/10", "menu_stage_count_proportional",
            left.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), left.menu().detail_shadow_offset);
        require(shadowed.bounds().x == 52.0f && shadowed.bounds().width == 105.5f,
            "Concrete counter lost its negative shadow at the outer paper's inclusive left edge");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            negative(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(53.25f);
        }, 10, 0);
        const auto bottom = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(905.0f);
        };
        const auto bottom_contact = variants.load(bottom);
        bottom_contact.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow bottom_label("count_bottom", L"8/10", "menu_stage_count_proportional",
            bottom_contact.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            bottom_contact.menu().shadow_colour, bottom_contact.menu().detail_shadow_offset);
        require(bottom_label.bounds().bottom() == 926.0f,
            "Concrete counter no longer touches the outer Stage paper before the footer");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            bottom(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(905.25f);
        }, 10, 0);
        for (char32_t digit = U'0'; digit <= U'9'; ++digit)
        {
            const std::string name = "menu_stage_count_missing_" + std::to_string(static_cast<unsigned int>(digit - U'0'));
            add_font(name, 8, 8, 16, 0.0f, digit);
            rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
            {
                navigation(d, name.c_str());
            }, 100, 0);
            rejects_counts("random_hint", [&](rapidjson::Document& d)
            {
                navigation(d, "menu_text_exact");
                d["menu_style"]["chrome"]["brand_font"].SetString(name.c_str(), d.GetAllocator());
                d["menu_text"]["stage"]["random_hint"].SetString("{count}");
            }, 100, digit == U'0' ? size_t{10} : static_cast<size_t>(digit - U'0'));
        }
        // The shared detail font reaches the existing 0-15 countdown guard
        // first. Preserve that diagnostic; the brand font reaches the new
        // Random all-digit guard even though its standalone sample is only 1.
        for (bool hint_font : {false, true})
        {
            variants.rejects(hint_font ? "stage.random_hint" : "display_trial_countdown", [&](rapidjson::Document& d)
            {
                if (hint_font) { d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_count_missing_6"); }
                else { d["menu_style"]["fonts"]["detail"].SetString("menu_stage_count_missing_6"); }
            }, &world);
        }
        add_font("menu_stage_count_hint_edge", 8, 943, 223);
        add_font("menu_stage_count_hint_over", 8, 943, 223, 0.5f);
        const auto hint_source = [&](rapidjson::Document& d, const char* name = "menu_stage_count_hint_edge")
        {
            navigation(d, "menu_text_exact");
            d["menu_style"]["chrome"]["brand_font"].SetString(name, d.GetAllocator());
            d["menu_text"]["stage"]["random_hint"].SetString("~{count}\nF");
        };
        const auto hint_edge = variants.load([&](rapidjson::Document& d) { hint_source(d); });
        hint_edge.validate_stage_counts(world.render, 10, 1);
        const auto& stage = hint_edge.main_menu().stage_select;
        const auto point = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f;
        UiTextDropShadow hint("count_hint", hint_edge.menu_text().stage_random_hint(1), "menu_stage_count_hint_edge",
            point, &world.render, Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), hint_edge.menu().detail_shadow_offset);
        const auto fit_hint = [&](const mattmath::Vector2F& shadow)
        {
            const auto raw = mattmath::RectangleF::union_of(hint.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                hint.text_bounds_at(shadow, 1.0f));
            const float scale = std::min(1.0f, 476.0f / raw.width);
            hint.set_scale(scale);
            hint.set_shadow_scale(scale);
            hint.set_shadow_offset(shadow * scale);
            return scale;
        };
        require(fit_hint(hint_edge.menu().detail_shadow_offset) == 0.5f && hint.text() == L"~1\nF" &&
            hint.bounds().width == 476.0f && hint.bounds().bottom() == 910.0f,
            "Random count hint no longer uses only width to fit its multiline text and scaled shadow");
        rejects_counts("random_hint", [&](rapidjson::Document& d) { hint_source(d, "menu_stage_count_hint_over"); }, 10, 1);
        const auto no_random = variants.load([&](rapidjson::Document& d) { hint_source(d, "menu_stage_count_hint_over"); });
        no_random.validate_stage_counts(world.render, 10, 0);
        const auto reversed = variants.load([&](rapidjson::Document& d)
        {
            hint_source(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
        });
        reversed.validate_stage_counts(world.render, 10, 1);
        const float reversed_scale = fit_hint(reversed.menu().detail_shadow_offset);
        require(reversed_scale > 0.0f && reversed_scale < 1.0f && hint.bounds().width <= 476.01f &&
            hint.shadow_offset() == reversed.menu().detail_shadow_offset * reversed_scale,
            "Random count width fitting discarded a negative shadow");
        MenuText short_copy = hint_edge.menu_text();
        short_copy.stage.random_hint = L"{count}";
        hint.set_text(short_copy.stage_random_hint(1));
        require(fit_hint(hint_edge.menu().detail_shadow_offset) == 1.0f && hint.text() == L"1" &&
            hint.shadow_scale() == 1.0f && hint.shadow_offset() == hint_edge.menu().detail_shadow_offset &&
            hint.bounds().width == 9.0f,
            "Short Random count replacement retained a stale shrink or scaled shadow");
        add_font("menu_stage_count_card_edge", 8, 943, 227);
        add_font("menu_stage_count_card_over", 8, 943, 227, 0.5f);
        const auto card_contact = [&](rapidjson::Document& d)
        {
            hint_source(d, "menu_stage_count_card_edge");
            d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(667.0f);
        };
        const auto card_edge = variants.load(card_contact);
        card_edge.validate_stage_counts(world.render, 10, 1);
        UiTextDropShadow card_label("count_hint_card", card_edge.menu_text().stage_random_hint(1),
            "menu_stage_count_card_edge", point, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), card_edge.menu().detail_shadow_offset);
        card_label.set_scale(0.5f);
        card_label.set_shadow_scale(0.5f);
        card_label.set_shadow_offset(card_edge.menu().detail_shadow_offset * 0.5f);
        require(card_label.text() == L"~1\nF" && card_label.bounds().width == 476.0f &&
            card_label.bounds().bottom() == 912.0f && card_label.bounds().bottom() + 14.0f == 926.0f,
            "Random count card no longer reaches the outer paper with its retained artwork inset");
        rejects_counts("random_hint", [&](rapidjson::Document& d)
        {
            card_contact(d);
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_count_card_over");
            d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(667.25f);
        }, 10, 1);
    }

    void check_stage_summary_text(Variants& variants, World& world, const MenuText& defaults)
    {
        require(defaults.stage_best_of_mode(3) == L"BEST OF 3" &&
            defaults.stage_best_of_mode(5) == L"BEST OF 5",
            "Arena series summary changed its literal default wording");
        constexpr const wchar_t* rosters[]{L"1 PLAYER", L"2 PLAYERS / 2 TEAMS",
            L"3 PLAYERS / 2 TEAMS", L"4 PLAYERS / 2 TEAMS"};
        for (int players = 1; players <= 4; ++players)
        {
            require(defaults.stage_roster(players) == rosters[players - 1],
                "Arena roster summary changed singular/plural wording or the fixed team count");
        }
        const auto reject_format = [](auto format)
        {
            bool rejected = false;
            try { format(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Invalid arena summary template or argument formatted silently");
        };
        for (int invalid : {std::numeric_limits<int>::min(), -1, 0, 1, 2, 4, 6, std::numeric_limits<int>::max()})
        {
            reject_format([&] { defaults.stage_best_of_mode(invalid); });
        }
        for (int invalid : {std::numeric_limits<int>::min(), -1, 0, 5, std::numeric_limits<int>::max()})
        {
            reject_format([&] { defaults.stage_roster(invalid); });
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            auto& stage = d["menu_text"]["stage"];
            stage["best_of_mode"].SetString("  {best_of}\xC2\xA0\xE2\x80\x9C" "rounds\xE2\x80\x9D\xE2\x80\x94" "best\xE2\x80\xA6  ");
            stage["solo_roster"].SetString("  {players}\xC2\xA0solo\xE2\x80\xA6  ");
            stage["versus_roster"].SetString("  {players}\xC2\xA0\xE2\x80\x94" "teams  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().stage.best_of_mode == L"  {best_of} \"rounds\"-best...  " &&
            normalized.menu_text().stage.solo_roster == L"  {players} solo...  " &&
            normalized.menu_text().stage.versus_roster == L"  {players} -teams  ",
            "Arena summary lost punctuation normalization, original tokens or authored outer spaces");
        for (int best_of : {3, 5})
        {
            require(normalized.menu_text().stage_best_of_mode(best_of) ==
                L"  " + std::to_wstring(best_of) + L" \"rounds\"-best...  ",
                "Arena series summary fixed token order or authored wording in code");
        }
        for (int players = 1; players <= 4; ++players)
        {
            require(normalized.menu_text().stage_roster(players) == L"  " + std::to_wstring(players) +
                (players == 1 ? L" solo...  " : L" -teams  "),
                "Arena roster summary ignored authored text or selected the wrong count branch");
        }
        for (const auto& entry : stage_summary_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const std::string token = entry.token;
            for (const char* blank : {"   ", " \xC2\xA0 "})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(blank, d.GetAllocator());
                });
            }
            for (const char* invalid : {"first\nsecond", "first\rsecond", "first\r\nsecond",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string invalid_text = token + invalid;
                    d["menu_text"]["stage"][entry.key].SetString(invalid_text.c_str(), d.GetAllocator());
                });
            }
            for (const bool array : {false, true})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    if (array) { d["menu_text"]["stage"][entry.key].SetArray(); }
                    else { d["menu_text"]["stage"][entry.key].SetInt(3); }
                });
            }
            const std::vector<std::string> malformed{
                "No token", token + token, "{" + token + "}", token + "{unknown}", token + "{", token + "}",
                entry.detail ? "{players}" : "{best_of}",
                entry.detail ? "{best_of }" : "{players }", entry.detail ? "{Best_of}" : "{Players}",
                entry.detail ? "{best_{best_of}of}" : "{pla{players}yers}"
            };
            for (const auto& invalid : malformed)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid.c_str(), d.GetAllocator());
                });
                MenuText mutated = defaults;
                mutated.stage.*entry.field = std::wstring(invalid.begin(), invalid.end());
                reject_format([&] { stage_summary(mutated, entry, entry.value); });
            }
            for (const wchar_t* invalid : {L"", L"   ", L"first\nsecond", L"first\rsecond", L"tab\ttext"})
            {
                MenuText mutated = defaults;
                mutated.stage.*entry.field = invalid;
                reject_format([&] { stage_summary(mutated, entry, entry.value); });
            }
            for (const wchar_t* extra : {L"\n", L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014"})
            {
                MenuText mutated = defaults;
                (mutated.stage.*entry.field) += extra;
                reject_format([&] { stage_summary(mutated, entry, entry.value); });
            }
            MenuText nul = defaults;
            (nul.stage.*entry.field).push_back(L'\0');
            reject_format([&] { stage_summary(nul, entry, entry.value); });
        }
    }

    void check_stage_summary_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int tilde_width, int tilde_height,
            char32_t digit = U'\0', float digit_x = 0.0f, float digit_y = 0.0f,
            int digit_height = 16, bool omit_digit = false)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (omit_digit && character == digit) { continue; }
                glyphs.push_back({character, mattmath::RectangleI(0, 0, character == U'~' ? tilde_width : 8,
                    character == U'~' ? tilde_height : character == digit ? digit_height : 16),
                    character == digit ? digit_x : 0.0f, character == digit ? digit_y : 0.0f, 0.0f});
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        const auto use_font = [](rapidjson::Document& d, const StageSummaryText& entry, const std::string& font)
        {
            if (entry.detail) { d["menu_style"]["fonts"]["detail"].SetString(font.c_str(), d.GetAllocator()); }
            else { d["menu_style"]["chrome"]["brand_font"].SetString(font.c_str(), d.GetAllocator()); }
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            const std::string value = std::string("~") + entry.token;
            d["menu_text"]["stage"][entry.key].SetString(value.c_str(), d.GetAllocator());
        };
        const auto widget = [&](const Presentation& look, const StageSummaryText& entry)
        {
            const auto& style = look.menu();
            const auto& stage = look.main_menu().stage_select;
            const std::wstring literal = L"~" + std::to_wstring(entry.value);
            const std::string& font = entry.detail ? style.detail_font : style.brand_font;
            const auto point = stage.briefing.position + stage.briefing_padding +
                stage.briefing.spacing * static_cast<float>(entry.row);
            std::wstring source = stage_summary(look.menu_text(), entry, entry.value);
            require(source == literal, "Boundary fixture did not format the independent literal arena summary");
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(font), literal);
            UiTextDropShadow label(entry.key, source, font, point, &world.render,
                Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.detail_shadow_offset);
            source = L"Changed after construction";
            const auto bounds = mattmath::RectangleF::union_of(mattmath::RectangleF(point, size),
                mattmath::RectangleF(point + style.detail_shadow_offset, size));
            require(label.text() == literal && label.text_bounds() == mattmath::RectangleF(point, size) &&
                label.bounds() == bounds && label.shadow_scale() == 1.0f &&
                label.shadow_offset() == style.detail_shadow_offset,
                "Arena summary did not retain literal text, actual role font and unscaled transparent shadow bounds");
            return bounds;
        };
        for (const auto& entry : stage_summary_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const std::string name = std::string("menu_stage_summary_") + entry.key;
            const char32_t last_digit = static_cast<char32_t>(U'0' + entry.value);
            add_font(name + "_width", 581, 20);
            const auto width = variants.load([&](rapidjson::Document& d) { use_font(d, entry, name + "_width"); });
            width.validate_resources(world.render, world.audio);
            const auto width_bounds = widget(width, entry);
            require(width_bounds.right() == 1852.0f && width_bounds.right() >
                width.main_menu().stage_select.briefing.position.x + width.main_menu().stage_select.briefing.size.x &&
                width_bounds.right() + 14.0f == 1866.0f,
                "Arena summary no longer overhangs the authored panel while touching the outer paper with its expanded card");
            // Only the last legal value gains a quarter-pixel bearing. Testing
            // one representative count would miss this complete-output overflow.
            add_font(name + "_width_over", 581, 20, last_digit, 0.25f);
            variants.rejects(field, [&](rapidjson::Document& d) { use_font(d, entry, name + "_width_over"); }, &world);
            add_font(name + "_row", 8, 74);
            const auto row = variants.load([&](rapidjson::Document& d) { use_font(d, entry, name + "_row"); });
            row.validate_resources(world.render, world.audio);
            const auto row_bounds = widget(row, entry);
            const auto& stage = row.main_menu().stage_select;
            require(row_bounds.bottom() == stage.briefing.position.y + stage.briefing_padding.y +
                stage.briefing.spacing.y * static_cast<float>(entry.row + 1),
                "Arena summary no longer reaches the next authored row with its complete one-pixel shadow");
            add_font(name + "_row_over", 8, 74, last_digit, 0.0f, 0.25f, 74);
            variants.rejects(field, [&](rapidjson::Document& d) { use_font(d, entry, name + "_row_over"); }, &world);
            const auto negative = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, entry, name + "_row");
                d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
                d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
            });
            negative.validate_resources(world.render, world.audio);
            require(widget(negative, entry).height == 75.5f,
                "Arena summary discarded the negative transparent shadow from its actual bounds");
            add_font(name + "_missing", 8, 16, last_digit, 0.0f, 0.0f, 16, true);
            variants.rejects(field, [&](rapidjson::Document& d) { use_font(d, entry, name + "_missing"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d) { use_font(d, entry, "menu_text_limited"); }, &world);
        }
        const auto compact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
        };
        const auto preview_contact = [&](rapidjson::Document& d)
        {
            compact(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["x"].SetFloat(1176.0f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-32.0f);
        };
        const auto touching = variants.load(preview_contact);
        touching.validate_resources(world.render, world.audio);
        const auto& stage = touching.main_menu().stage_select;
        const float text_left = stage.briefing.position.x + stage.briefing_padding.x + touching.menu().detail_shadow_offset.x;
        require(text_left == 1176.0f && text_left - 14.0f == 1162.0f &&
            stage.preview_position.x + stage.preview_size.x + touching.menu().rule_height == 1162.0f,
            "Expanded summary card no longer touches the actual two-pixel preview edge at its inclusive left boundary");
        variants.rejects("stage.best_of_mode", [&](rapidjson::Document& d)
        {
            preview_contact(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["x"].SetFloat(1175.75f);
        }, &world);
        const auto paper_left = [&](rapidjson::Document& d)
        {
            compact(d);
            auto& select = d["main_menu"]["stage_select"];
            select["briefing"]["position"]["x"].SetFloat(54.0f);
            select["preview_position"]["x"].SetFloat(1000.0f);
            select["preview_size"]["x"].SetFloat(200.0f);
            select["name_position"]["x"].SetFloat(1000.0f);
            select["navigation_position"]["x"].SetFloat(1000.0f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-20.0f);
        };
        const auto left = variants.load(paper_left);
        left.validate_resources(world.render, world.audio);
        const auto& left_stage = left.main_menu().stage_select;
        require(left_stage.briefing.position.x + left_stage.briefing_padding.x +
            left.menu().detail_shadow_offset.x - 14.0f == 52.0f &&
            (1920.0f - left.main_menu().stage_box_size.x) / 2.0f + 12.0f == 52.0f &&
            left_stage.briefing.position.x + left_stage.briefing.size.x < left_stage.preview_position.x - left.menu().rule_height,
            "Expanded summary left-paper contact lost its literal edge or separate preview arrangement");
        variants.rejects("stage.best_of_mode", [&](rapidjson::Document& d)
        {
            paper_left(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-20.25f);
        }, &world);
    }

    void check_stage_briefing_text(Variants& variants, World& world)
    {
        for (const auto& entry : stage_briefing_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(entry.multiline ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ", d.GetAllocator());
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().stage.*entry.field == (entry.multiline ? L" A -B \n C " : L" A -B "),
                std::string("Arena briefing lost authored spacing/LF or normalized punctuation: ") + entry.key);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                    });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString("line\nbreak");
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                select_stage_briefing_font(d, entry, "menu_text_limited");
                d["menu_text"]["stage"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            for (const auto& entry : stage_briefing_text)
            {
                MenuStyle style = look.menu();
                StageSelectPresentation stage = look.main_menu().stage_select;
                std::wstring source = look.menu_text().stage.*entry.field;
                const auto retained = source;
                const auto font_name = entry.detail ? style.detail_font : style.brand_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const auto measured = world.render.measure_text(font, source);
                const auto point = stage.briefing.position + stage.briefing_padding +
                    stage.briefing.spacing * static_cast<float>(entry.row);
                const auto shadow = style.detail_shadow_offset;
                const auto shadow_colour = style.shadow_colour;
                UiTextDropShadow text(entry.key, source, font_name, point, &world.render,
                    entry.detail ? style.heading_colour : style.highlight, shadow_colour, shadow);
                source = L"Changed after construction";
                style.brand_font = "gill_sans_mt_bold_48";
                style.detail_font = "gill_sans_mt_bold_48";
                style.detail_shadow_offset = mattmath::Vector2F(10.0f, 10.0f);
                style.shadow_colour = style.warning;
                stage.briefing.position += mattmath::Vector2F(40.0f, 40.0f);
                const auto expected = mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured),
                    mattmath::RectangleF(point + shadow, measured));
                require(text.text() == retained && text.text_bounds() == mattmath::RectangleF(point, measured) &&
                    text.bounds() == expected && text.shadow_offset() == shadow && text.shadow_scale() == 1.0f &&
                    text.shadow_color() == shadow_colour &&
                    std::isfinite(expected.width) && std::isfinite(expected.height) && expected.width > 0.0f && expected.height > 0.0f,
                    std::string("Actual arena briefing lost its copied text, captured font/position/shadow or unscaled bounds: ") + entry.key);
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        check_widgets(defaults);
        const auto& stage = defaults.main_menu().stage_select;
        const auto solo_size = world.render.measure_text(world.render.resolve_sprite_font(defaults.menu().brand_font),
            defaults.menu_text().stage.solo_objective);
        require(stage.briefing.position.x + stage.briefing_padding.x + solo_size.x + defaults.menu().detail_shadow_offset.x >
            stage.briefing.position.x + stage.briefing.size.x,
            "Shipped solo objective no longer demonstrates its preserved horizontal panel overhang");
        for (const bool negative : { false, true })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                for (const auto& entry : stage_briefing_text)
                {
                    const std::string text = entry.multiline ? "Fit\nFit\nFit\nFit\nFit\nFit\nFit" : std::string(80, 'F');
                    d["menu_text"]["stage"][entry.key].SetString(text.c_str(), d.GetAllocator());
                }
                if (negative)
                {
                    d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
                    d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
        const auto one_line = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["stage"]["solo_objective"].SetString("Solo");
            d["menu_text"]["stage"]["versus_objective"].SetString("Teams");
        });
        one_line.validate_resources(world.render, world.audio);
        require(one_line.menu_text().stage.solo_objective == L"Solo" && one_line.menu_text().stage.versus_objective == L"Teams",
            "Arena objective blocks replaced authored copy with a fixed line count");
    }

    void check_stage_briefing_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height,
            float x_offset = 0.0f, float y_offset = 0.0f, float line_spacing = 16.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool boundary = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary ? width : 8, boundary ? height : 16),
                    boundary ? x_offset : 0.0f, boundary ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        const auto use_font = [](rapidjson::Document& d, const std::string& name, const StageBriefingText& entry)
        {
            select_stage_briefing_font(d, entry, name.c_str());
            // Keep independent Weapon bodies clear of the compact label badges.
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            for (const auto& other : stage_briefing_text) { d["menu_text"]["stage"][other.key].SetString("Fit"); }
            d["menu_text"]["stage"][entry.key].SetString("~");
        };
        add_font("menu_stage_briefing_width_edge", 657, 20);
        add_font("menu_stage_briefing_width_overflow", 658, 20);
        for (const auto& entry : stage_briefing_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto width = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_briefing_width_edge", entry);
            });
            width.validate_resources(world.render, world.audio);
            const auto& stage = width.main_menu().stage_select;
            const float x = stage.briefing.position.x + stage.briefing_padding.x +
                stage.briefing.spacing.x * static_cast<float>(entry.row);
            require(x + 657.0f + width.menu().detail_shadow_offset.x == 1920.0f,
                "Arena briefing width fixture no longer touches the full design's inclusive right edge");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_briefing_width_overflow", entry);
            }, &world);
            const int height = (entry.next_row - entry.row) * 75 - 1;
            const std::string font = std::string("menu_stage_briefing_next_") + entry.key;
            add_font(font, 8, height);
            const auto next = variants.load([&](rapidjson::Document& d) { use_font(d, font, entry); });
            next.validate_resources(world.render, world.audio);
            require(static_cast<float>(height) + next.menu().detail_shadow_offset.y ==
                next.main_menu().stage_select.briefing.spacing.y * static_cast<float>(entry.next_row - entry.row),
                "Arena briefing boundary no longer reaches the next authored row with its complete shadow");
            const std::string overflow = font + "_overflow";
            add_font(overflow, 8, height + 1);
            variants.rejects(field, [&](rapidjson::Document& d) { use_font(d, overflow, entry); }, &world);
        }
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            for (const auto& entry : stage_briefing_text) { d["menu_text"]["stage"][entry.key].SetString("Fit"); }
            d["menu_text"]["stage"]["random_navigation"].SetString("F");
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1800.0f);
        };
        const auto screen_top = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["y"].SetFloat(-28.0f);
        };
        const auto top = variants.load(screen_top);
        UiTextDropShadow top_label("legacy_briefing_top", L"Fit", "menu_text_exact",
            top.main_menu().stage_select.briefing.position + top.main_menu().stage_select.briefing_padding,
            &world.render, Colour::black, top.menu().shadow_colour, top.menu().detail_shadow_offset);
        require(top_label.bounds().y == 0.0f,
            "Legacy briefing text no longer touches the inclusive design top at authored y=-28");
        // The old text contact remains exact, but an authored card above the
        // outer Stage paper is no longer a globally valid summary fixture.
        variants.rejects("stage.best_of_mode", screen_top, &world);
        variants.rejects("stage.briefing_label", [&](rapidjson::Document& d)
        {
            screen_top(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["y"].SetFloat(-28.25f);
        }, &world);
        const auto screen_left = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["x"].SetFloat(-32.0f);
            d["main_menu"]["stage_select"]["briefing"]["spacing"]["x"].SetFloat(1.0f);
        };
        const auto left = variants.load(screen_left);
        UiTextDropShadow left_label("legacy_briefing_left", L"Fit", "menu_text_exact",
            left.main_menu().stage_select.briefing.position + left.main_menu().stage_select.briefing_padding,
            &world.render, Colour::black, left.menu().shadow_colour, left.menu().detail_shadow_offset);
        require(left_label.bounds().x == 0.0f,
            "Legacy briefing text no longer touches the inclusive design left at authored x=-32");
        variants.rejects("stage.best_of_mode", screen_left, &world);
        variants.rejects("stage.briefing_label", [&](rapidjson::Document& d)
        {
            screen_left(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["x"].SetFloat(-32.25f);
        }, &world);
        const auto panel_top = [&](rapidjson::Document& d)
        {
            exact(d);
            // Keep unrelated Team cards clear of the raised warning shadow.
            d["main_menu"]["team_select"]["spacing"]["y"].SetFloat(120.0f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-28.0f);
        };
        variants.load(panel_top).validate_resources(world.render, world.audio);
        variants.rejects("stage.briefing_label", [&](rapidjson::Document& d)
        {
            panel_top(d);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-28.25f);
        }, &world);
        const float infinity = std::numeric_limits<float>::infinity();
        struct InvalidMeasure { const char* name; int width; int height; float x_offset; float y_offset; float line_spacing; };
        const InvalidMeasure invalid_measures[]{
            { "zero_width", 0, 20, 0.0f, 0.0f, 16.0f },
            { "zero_height", 8, 0, 0.0f, 0.0f, 0.0f },
            { "infinite_width", 8, 20, infinity, 0.0f, 16.0f },
            { "infinite_height", 8, 20, 0.0f, infinity, 16.0f }
        };
        for (const auto& invalid : invalid_measures)
        {
            const std::string name = std::string("menu_stage_briefing_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid arena briefing fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : stage_briefing_text)
            {
                variants.rejects(std::string("stage.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry);
                }, &world);
            }
        }
    }

    void check_stage_cue_text(Variants& variants, World& world)
    {
        for (const auto& entry : stage_cue_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(entry.hint ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ", d.GetAllocator());
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().stage.*entry.field == (entry.hint ? L" A -B \n C " : L" A -B "),
                std::string("Arena cue lost authored spacing/LF or normalized punctuation: ") + entry.key);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (entry.hint)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                    });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString("line\nbreak");
                });
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                }, &world);
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (entry.hint) { d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_limited"); }
                else { d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited"); }
                d["menu_text"]["stage"][entry.key].SetString("Glyph~");
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            for (const auto& entry : stage_cue_text)
            {
                MenuStyle style = look.menu();
                StageSelectPresentation stage = look.main_menu().stage_select;
                const auto font_name = entry.hint ? style.brand_font : style.detail_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const auto point = entry.hint ? stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f :
                    stage.navigation_position;
                const auto captured_shadow = style.detail_shadow_offset;
                std::wstring source = look.menu_text().stage.*entry.field;
                const auto retained = source;
                const auto measured = world.render.measure_text(font, source);
                UiTextDropShadow text(entry.key, L"", font_name, point, &world.render,
                    style.unhighlight, style.shadow_colour, captured_shadow);
                text.set_text(source);
                source = L"Changed after set_text";
                style.brand_font = "gill_sans_mt_bold_48";
                style.detail_font = "gill_sans_mt_bold_48";
                stage.navigation_position += mattmath::Vector2F(40.0f, 40.0f);
                stage.briefing.position += mattmath::Vector2F(40.0f, 40.0f);
                if (!entry.hint)
                {
                    style.detail_shadow_offset = mattmath::Vector2F(10.0f, 10.0f);
                    text.set_text(retained);
                    require(text.text() == retained && text.text_bounds() == mattmath::RectangleF(point, measured) &&
                        text.bounds() == mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured),
                            mattmath::RectangleF(point + captured_shadow, measured)) &&
                        text.shadow_offset() == captured_shadow && text.shadow_scale() == 1.0f,
                        "Actual arena navigation lost its copied text, captured font/position/shadow or unscaled bounds");
                }
                else
                {
                    const float original_budget = stage.briefing.size.x - stage.briefing_padding.x * 2.0f;
                    const auto fit = [&]()
                    {
                        const auto raw = mattmath::RectangleF::union_of(text.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                            text.text_bounds_at(style.detail_shadow_offset, 1.0f));
                        const float budget = stage.briefing.size.x - stage.briefing_padding.x * 2.0f;
                        const float scale = std::min(1.0f, budget / raw.width);
                        text.set_scale(scale);
                        text.set_shadow_scale(scale);
                        text.set_shadow_offset(style.detail_shadow_offset * scale);
                        return scale;
                    };
                    for (const bool changed_fit : { false, true })
                    {
                        if (changed_fit)
                        {
                            stage.briefing.size.x = stage.briefing_padding.x * 2.0f + original_budget / 2.0f;
                            style.detail_shadow_offset = mattmath::Vector2F(-1.5f, -1.5f);
                        }
                        text.set_text(retained);
                        const float scale = fit();
                        const auto expected = mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured * scale),
                            mattmath::RectangleF(point + style.detail_shadow_offset * scale, measured * scale));
                        const auto actual = text.bounds();
                        require(text.text() == retained && std::abs(actual.x - expected.x) <= 0.01f &&
                            std::abs(actual.y - expected.y) <= 0.01f && std::abs(actual.width - expected.width) <= 0.01f &&
                            std::abs(actual.height - expected.height) <= 0.01f && text.shadow_scale() == scale &&
                            text.shadow_offset() == style.detail_shadow_offset * scale && scale > 0.0f && scale <= 1.0f &&
                            actual.width <= stage.briefing.size.x - stage.briefing_padding.x * 2.0f + 0.01f,
                            "Actual arena hint lost its captured text/font/position or live width-only shadow fitting");
                    }
                    std::wstring replacement = L"Fit";
                    text.set_text(replacement);
                    replacement = L"Changed after replacement";
                    require(fit() == 1.0f && text.text() == L"Fit" && text.shadow_scale() == 1.0f &&
                        text.shadow_offset() == style.detail_shadow_offset &&
                        text.text_bounds() == mattmath::RectangleF(point, world.render.measure_text(font, L"Fit")),
                        "A short arena hint retained its old shrink scale, lost its copied text or used a replacement font/position");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const bool wide : { false, true })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"]["random_navigation"].SetString("Fit");
                for (const char* key : { "locked_hint", "browse_hint" })
                {
                    d["menu_text"]["stage"][key].SetString(wide ? std::string(200, 'W').c_str() : "Fit", d.GetAllocator());
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
        const auto multiline = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            for (const char* key : { "locked_hint", "browse_hint" })
            {
                d["menu_text"]["stage"][key].SetString("Fit\nFit\nFit\nFit\nFit");
            }
        });
        multiline.validate_resources(world.render, world.audio);
        check_widgets(multiline);
    }

    void check_stage_cue_bounds(Variants& variants, World& world)
    {
        const auto exact_navigation = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            d["menu_text"]["stage"]["random_navigation"].SetString("Fit");
        };
        struct Edge { const char* axis; float position; float outside; };
        const Edge nav_edges[]{ { "x", 0.0f, -0.25f }, { "x", 1895.0f, 1895.25f },
            { "y", 0.0f, -0.25f }, { "y", 939.0f, 939.25f },
            { "y", 832.0f, 831.75f }, { "x", 1205.0f, 1205.25f } };
        for (const auto& edge : nav_edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact_navigation(d);
                d["main_menu"]["stage_select"]["navigation_position"][edge.axis].SetFloat(edge.position);
            };
            variants.load(contact).validate_resources(world.render, world.audio);
            variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["navigation_position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        const auto panel_top = [&](rapidjson::Document& d)
        {
            exact_navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1230.0f);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(224.0f);
        };
        variants.load(panel_top).validate_resources(world.render, world.audio);
        variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
        {
            panel_top(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(224.25f);
        }, &world);
        const auto negative = [&](rapidjson::Document& d)
        {
            exact_navigation(d);
            for (const char* axis : { "x", "y" })
            {
                d["menu_style"]["shadow"]["detail_offset"][axis].SetFloat(-1.5f);
                d["main_menu"]["stage_select"]["navigation_position"][axis].SetFloat(1.5f);
            }
        };
        variants.load(negative).validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
            {
                negative(d);
                d["main_menu"]["stage_select"]["navigation_position"][axis].SetFloat(1.25f);
            }, &world);
        }
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? width : 8,
                    character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 16.0f));
        };
        const auto use_hint = [](rapidjson::Document& d, const std::string& font, const char* key)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString(font.c_str(), d.GetAllocator());
            d["menu_text"]["stage"]["locked_hint"].SetString("Fit");
            d["menu_text"]["stage"]["browse_hint"].SetString("Fit");
            d["menu_text"]["stage"][key].SetString("~");
        };
        add_font("menu_stage_hint_panel_edge", 951, 223);
        add_font("menu_stage_hint_panel_overflow", 951, 224);
        add_font("menu_stage_hint_footer_edge", 951, 323);
        add_font("menu_stage_hint_footer_overflow", 951, 324);
        for (const char* key : { "locked_hint", "browse_hint" })
        {
            const std::string field = std::string("stage.") + key;
            variants.load([&](rapidjson::Document& d) { use_hint(d, "menu_stage_hint_panel_edge", key); }).validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d) { use_hint(d, "menu_stage_hint_panel_overflow", key); }, &world);
            const auto footer = [&](rapidjson::Document& d)
            {
                use_hint(d, "menu_stage_hint_footer_edge", key);
                d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(800.0f);
            };
            const auto footer_contact = variants.load(footer);
            const auto& footer_stage = footer_contact.main_menu().stage_select;
            UiTextDropShadow hint("legacy_hint_footer", L"~", "menu_stage_hint_footer_edge",
                footer_stage.briefing.position + footer_stage.briefing_padding + footer_stage.briefing.spacing * 7.0f,
                &world.render, Colour::black, footer_contact.menu().shadow_colour,
                footer_contact.menu().detail_shadow_offset);
            hint.set_scale(0.5f);
            hint.set_shadow_scale(0.5f);
            hint.set_shadow_offset(footer_contact.menu().detail_shadow_offset * 0.5f);
            require(hint.bounds().width == 476.0f && hint.bounds().bottom() == 960.0f &&
                footer_stage.briefing.position.y + footer_stage.briefing.size.y == 1045.0f,
                "Legacy width-fitted hint no longer touches its exact footer at the original 800-high panel");
            // The fitted hint still reaches the footer; its backing card now
            // exceeds the outer Stage paper, which the new summary guard rejects.
            variants.rejects("stage.best_of_mode", footer, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                footer(d);
                d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_hint_footer_overflow");
            }, &world);
        }
        const auto hint_left = [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            d["menu_text"]["stage"]["locked_hint"].SetString("Fit");
            d["menu_text"]["stage"]["browse_hint"].SetString("Fit");
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(7.0f);
            d["main_menu"]["stage_select"]["briefing"]["spacing"]["x"].SetFloat(-1.0f);
        };
        variants.load(hint_left).validate_resources(world.render, world.audio);
        variants.rejects("stage.locked_hint", [&](rapidjson::Document& d)
        {
            hint_left(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-0.25f);
        }, &world);
        for (const float padding : { 270.0f, 271.0f })
        {
            variants.rejects("stage.locked_hint", [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
                for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
                d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(padding);
            }, &world);
        }
        for (const char* suffix : { "zero_width", "zero_height", "infinite_width", "infinite_height" })
        {
            for (const auto& entry : stage_cue_text)
            {
                variants.rejects(std::string("stage.") + entry.key, [&](rapidjson::Document& d)
                {
                    if (entry.hint)
                    {
                        use_hint(d, std::string("menu_stage_briefing_") + suffix, entry.key);
                    }
                    else
                    {
                        const std::string font = std::string("menu_stage_status_") + suffix;
                        d["menu_style"]["fonts"]["detail"].SetString(font.c_str(), d.GetAllocator());
                        d["menu_text"]["stage"][entry.key].SetString("~");
                    }
                }, &world);
            }
        }
        for (const char* axis : { "x", "y" })
        {
            for (const double position : { -1e100, 1e100 })
            {
                variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["navigation_position"][axis].SetDouble(position);
                }, &world);
            }
        }
    }

    void check_stage_status_text(Variants& variants, World& world)
    {
        for (const auto& entry : stage_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().stage.*entry.field == L"  A \"B\"-C...  ",
                std::string("Arena status lost normalized punctuation or authored spacing: ") + entry.key);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
                d["menu_text"]["stage"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            for (const auto& entry : stage_text)
            {
                MenuStyle style = look.menu();
                StageSelectPresentation stage = look.main_menu().stage_select;
                std::wstring source = look.menu_text().stage.*entry.field;
                const auto retained = source;
                const auto font = world.render.resolve_sprite_font(style.detail_font);
                const auto measured = world.render.measure_text(font, source);
                const auto point = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 6.0f;
                const auto shadow = style.detail_shadow_offset;
                UiTextDropShadow status("arena_status", L"", style.detail_font, point, &world.render,
                    style.heading_colour, style.shadow_colour, shadow);
                status.set_text(source);
                source = L"Changed after set_text";
                const auto expected = mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured),
                    mattmath::RectangleF(point + shadow, measured));
                style.detail_font = "gill_sans_mt_bold_24";
                style.detail_shadow_offset = mattmath::Vector2F(10.0f, 10.0f);
                stage.briefing.position += mattmath::Vector2F(40.0f, 40.0f);
                status.set_text(retained);
                status.set_colour(look.main_menu().stage_select.selected);
                require(status.text() == retained && status.text_bounds() == mattmath::RectangleF(point, measured) &&
                    status.bounds() == expected && status.shadow_offset() == shadow && status.shadow_scale() == 1.0f &&
                    std::isfinite(expected.width) && std::isfinite(expected.height) && expected.width > 0.0f && expected.height > 0.0f,
                    std::string("Actual arena status lost its copied text, captured font/position/shadow, or unscaled bounds: ") + entry.key);
                status.set_hidden(true);
                DrawList no_device;
                status.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const size_t length : { size_t(3), size_t(59) })
        {
            for (const bool negative : { false, true })
            {
                const auto authored = variants.load([&](rapidjson::Document& d)
                {
                    d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                    for (const auto& entry : stage_text)
                    {
                        d["menu_text"]["stage"][entry.key].SetString(std::string(length, 'F').c_str(), d.GetAllocator());
                    }
                    if (negative)
                    {
                        d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
                        d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
                    }
                });
                authored.validate_resources(world.render, world.audio);
                check_widgets(authored);
            }
        }
    }

    void check_stage_status_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height,
            float x_offset = 0.0f, float y_offset = 0.0f, float line_spacing = 16.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool boundary = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary ? width : 8, boundary ? height : 16),
                    boundary ? x_offset : 0.0f, boundary ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        const auto use_font = [](rapidjson::Document& d, const std::string& name, const char* key)
        {
            d["menu_style"]["fonts"]["detail"].SetString(name.c_str(), d.GetAllocator());
            // Keep independent Weapon bodies clear of the compact label badges.
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("Fit"); }
            d["menu_text"]["stage"][key].SetString("~");
        };
        add_font("menu_stage_status_width_edge", 507, 20);
        add_font("menu_stage_status_width_overflow", 508, 20);
        add_font("menu_stage_status_hint_edge", 8, 74);
        add_font("menu_stage_status_hint_overflow", 8, 75);
        for (const auto& entry : stage_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto width = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_width_edge", entry.key);
            });
            width.validate_resources(world.render, world.audio);
            const auto& stage = width.main_menu().stage_select;
            require(stage.briefing.position.x + stage.briefing_padding.x + 507.0f + width.menu().detail_shadow_offset.x ==
                stage.briefing.position.x + stage.briefing.size.x,
                "Arena status width fixture no longer touches the full briefing panel's right edge");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_width_overflow", entry.key);
            }, &world);
            const auto hint = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_hint_edge", entry.key);
            });
            hint.validate_resources(world.render, world.audio);
            require(74.0f + hint.menu().detail_shadow_offset.y == hint.main_menu().stage_select.briefing.spacing.y,
                "Arena status height fixture no longer reaches the authored hint row with its complete shadow");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_hint_overflow", entry.key);
            }, &world);
        }
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("Fit"); }
            // Keep the other briefing rows valid while these cases isolate
            // status geometry, including a full-design right-edge contact.
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            for (const auto& entry : stage_briefing_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
            for (const auto& entry : stage_cue_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1800.0f);
        };
        const auto left = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
        };
        variants.load(left).validate_resources(world.render, world.audio);
        variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
        {
            left(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.75f);
        }, &world);
        // A later positive-height hint must also fit the panel and footer,
        // making status-at-panel-bottom and status-at-design-bottom contacts
        // unreachable. Hint boundary cases below own those final contacts.
        const auto negative = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(3.0f);
            d["main_menu"]["stage_select"]["briefing"]["spacing"]["x"].SetFloat(-0.25f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
            d["menu_text"]["stage"]["locked_hint"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            d["menu_text"]["stage"]["browse_hint"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        };
        variants.load(negative).validate_resources(world.render, world.audio);
        variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
        {
            negative(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.75f);
        }, &world);
        struct ScreenEdge { const char* axis; float position; float outside; };
        // Shift the panel with the status: panel containment alone must not
        // make an offscreen status valid. The complete shadow touches design
        // edges even when the larger backing panel extends beyond them.
        // Status at y0 would put the earlier fixed briefing rows offscreen;
        // their new row-0 screen-top test replaces that unreachable contact.
        const ScreenEdge screen_edges[]{ { "x", -32.0f, -32.25f }, { "x", 1863.0f, 1863.25f } };
        for (const auto& edge : screen_edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["stage_select"]["briefing"]["position"][edge.axis].SetFloat(edge.position);
            };
            const auto old_contact = variants.load(contact);
            const auto& old_stage = old_contact.main_menu().stage_select;
            UiTextDropShadow status("legacy_status_design", L"Fit", "menu_text_exact",
                old_stage.briefing.position + old_stage.briefing_padding + old_stage.briefing.spacing * 6.0f,
                &world.render, Colour::black, old_contact.menu().shadow_colour, old_contact.menu().detail_shadow_offset);
            require(edge.position == -32.0f ? status.bounds().x == 0.0f : status.bounds().right() == 1920.0f,
                "Legacy status no longer touches its original inclusive design edge with the full shadow");
            // The literal status contact remains valid in design space; its
            // off-paper authored card is rejected by the new summary contract.
            variants.rejects("stage.best_of_mode", contact, &world);
            variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["briefing"]["position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        for (const char* axis : { "x", "y" })
        {
            for (const char* group : { "position", "spacing" })
            {
                variants.rejects("stage", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["briefing"][group][axis].SetDouble(1e100);
                }, &world);
            }
            variants.rejects("stage", [&](rapidjson::Document& d)
            {
                d["menu_style"]["shadow"]["detail_offset"][axis].SetDouble(1e100);
            }, &world);
        }
        const float infinity = std::numeric_limits<float>::infinity();
        struct InvalidMeasure { const char* name; int width; int height; float x_offset; float y_offset; float line_spacing; };
        const InvalidMeasure invalid_measures[]{
            { "zero_width", 0, 20, 0.0f, 0.0f, 16.0f },
            { "zero_height", 8, 0, 0.0f, 0.0f, 0.0f },
            { "infinite_width", 8, 20, infinity, 0.0f, 16.0f },
            { "infinite_height", 8, 20, 0.0f, infinity, 16.0f }
        };
        for (const auto& invalid : invalid_measures)
        {
            const std::string name = std::string("menu_stage_status_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid arena status metrics fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : stage_text)
            {
                variants.rejects(std::string("stage.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry.key);
                }, &world);
            }
        }
    }

    void check_status_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : status_text)
            {
                d["menu_text"]["status"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : status_text)
        {
            require(normalized.menu_text().status.*entry.field == L"  A \"B\"-C...  ",
                std::string("Save status lost authored spacing or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("status.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["status"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            // Each status checks its actual page font even though the other
            // menu font still supports '~'; font stand-ins cannot hide a gap.
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (entry.options) { d["prompt_bar"]["font"].SetString("menu_text_limited"); }
                else { d["controls_settings"]["help_font"].SetString("menu_text_limited"); }
                d["menu_text"]["status"][entry.key].SetString("Glyph~");
            }, &world);
        }
    }

    void check_status_fitting(Variants& variants, World& world)
    {
        const auto check_messages = [&](const Presentation& look, bool width_limited)
        {
            for (const auto& entry : status_text)
            {
                const auto& main = look.main_menu();
                const auto& controls = look.controls_settings();
                const auto position = entry.options ? main.options_status_position : controls.status_position;
                const auto box_size = entry.options ? main.options_box_size : controls.box_size;
                const auto art = entry.options ? options_art(main) : controls.art;
                const auto& font_name = entry.options ? look.prompt_bar().font : controls.help_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const float available = (1920.0f + box_size.x) / 2.0f -
                    art.shadow_offset - art.focus_padding.x - position.x;
                require(available == (entry.options ? 930.0f : 1590.0f),
                    "Save-status fixture no longer exercises the shipped page widths");
                std::wstring authored = look.menu_text().status.*entry.field;
                const auto measured = world.render.measure_text(font, authored);
                require(width_limited ? measured.x > available : measured.x < available,
                    "Save-status fixture does not independently exercise the expected width fitting");
                UiText text(entry.key, authored, font_name, position, &world.render, art.ink);
                const auto retained = authored;
                authored = L"Changed after construction";
                require(text.text() == retained && text.bounds() == mattmath::RectangleF(position, measured),
                    "Actual status UiText borrowed its source or changed its unscaled measurement");
                text.set_scale(1.0f);
                const float scale = std::min(1.0f, available / measured.x);
                text.set_scale(scale);
                require(text.text() == retained && text.bounds().position() == position &&
                    std::isfinite(text.bounds().width) && std::isfinite(text.bounds().height) &&
                    text.bounds().width > 0.0f && text.bounds().height > 0.0f &&
                    text.bounds().width <= available + 0.01f &&
                    std::abs(text.bounds().height - measured.y * scale) <= 0.01f,
                    "Actual status UiText lost text, moved, or changed its proportional width-only fitting");
                if (width_limited)
                {
                    require(std::abs(text.bounds().width - available) <= 0.01f &&
                        text.bounds().height < measured.y,
                        "Long save status did not retain the complete text and proportionally fit its page width");
                    text.set_text(L"Fit");
                    text.set_scale(1.0f);
                    require(text.text() == L"Fit" && text.bounds() ==
                        mattmath::RectangleF(position, world.render.measure_text(font, L"Fit")),
                        "Replacing a fitted save status retained its old shrink scale");
                }
                else
                {
                    require(text.bounds() == mattmath::RectangleF(position, measured),
                        "Short save statuses were enlarged above their authored font size");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        const auto short_messages = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
        });
        short_messages.validate_resources(world.render, world.audio);
        check_messages(short_messages, false);
        const auto wide_messages = variants.load([](rapidjson::Document& d)
        {
            // The original tall help/association blocks share the status band
            // at its far right. Compact blocks leave that wider message legal.
            d["menu_text"]["help"]["controls"].SetString("Fit");
            d["menu_text"]["help"]["pad_association"].SetString("Fit");
            for (const auto& entry : status_text)
            {
                d["menu_text"]["status"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide_messages.validate_resources(world.render, world.audio);
        check_messages(wide_messages, true);
    }

    void check_status_bounds(Variants& variants, World& world)
    {
        for (const auto& entry : status_text)
        {
            if (entry.options) { continue; }
            variants.rejects(std::string("status.") + entry.key, [&](rapidjson::Document& d)
            {
                d["menu_text"]["status"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        // This message has two consumers. Independently clear one page's help
        // and retain the other's original intersection to check both contexts.
        for (const char* compact_help : { "controls", "pad_association" })
        {
            variants.rejects("status.active_save_failed", [&](rapidjson::Document& d)
            {
                d["menu_text"]["help"][compact_help].SetString("Fit");
                d["menu_text"]["status"]["active_save_failed"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        for (const auto& entry : status_text)
        {
            const int height = entry.options ? 55 : 52;
            for (const int excess : { 0, 1 })
            {
                std::vector<Glyph> glyphs;
                for (char32_t character = U' '; character <= U'~'; ++character)
                {
                    glyphs.push_back({ character, mattmath::RectangleI(0, 0, 8,
                        character == U'~' ? height + excess : 16), 0.0f, 0.0f, 0.0f });
                }
                const std::string font_name = std::string("menu_status_") + entry.key +
                    (excess == 0 ? "_edge" : "_overflow");
                world.render.add_font(font_name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
                const auto edit = [&](rapidjson::Document& d)
                {
                    if (entry.options) { d["prompt_bar"]["font"].SetString(font_name.c_str(), d.GetAllocator()); }
                    else { d["controls_settings"]["help_font"].SetString(font_name.c_str(), d.GetAllocator()); }
                    for (const auto& message : status_text) { d["menu_text"]["status"][message.key].SetString("Fit"); }
                    d["menu_text"]["status"][entry.key].SetString("~");
                };
                if (excess == 0)
                {
                    const auto touches_bottom = variants.load(edit);
                    touches_bottom.validate_resources(world.render, world.audio);
                    const auto size = world.render.measure_text(world.render.resolve_sprite_font(font_name), L"~");
                    require(size == mattmath::Vector2F(8.0f, static_cast<float>(height)),
                        "Save-status inclusive paper fixture has unexpected glyph metrics");
                }
                else { variants.rejects(std::string("status.") + entry.key, edit, &world); }
            }
        }
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& main = original.main_menu();
        const auto art = options_art(main);
        const auto back_size = world.render.measure_text(world.render.resolve_sprite_font(original.menu().item_font),
            original.menu_text().item(MenuItem::back));
        const float back_right = main.options_position.x + back_size.x + art.focus_padding.x;
        const auto back_edge = [&](rapidjson::Document& d)
        {
            // The shipped status already overlaps Back vertically. Exact
            // horizontal contact remains legal; any positive area is rejected.
            d["main_menu"]["options_status_position"]["x"].SetFloat(back_right);
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
        };
        const auto beside_back = variants.load(back_edge);
        beside_back.validate_resources(world.render, world.audio);
        require(main.options_status_position.y < main.options_position.y + main.options_spacing.y * 6.0f +
            back_size.y + art.focus_padding.y,
            "Options status fixture no longer intersects the padded Back row vertically");
        variants.rejects("status.options_pending", [&](rapidjson::Document& d)
        {
            back_edge(d);
            d["main_menu"]["options_status_position"]["x"].SetFloat(back_right - 0.25f);
        }, &world);
        variants.rejects("status.options_pending", [](rapidjson::Document& d)
        {
            // This point passes the nominal inset parser but leaves zero
            // width after the status's existing shadow and right padding.
            d["main_menu"]["options_status_position"]["x"].SetFloat(1750.0f);
        }, &world);

        const auto preview_page = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            d["controls_settings"]["item_font"].SetString("menu_text_exact");
            d["controls_settings"]["help_font"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
            d["menu_text"]["help"]["controls"].SetString("Fit");
            d["controls_settings"]["heading_position"]["y"].SetFloat(40.0f);
            d["controls_settings"]["layout"]["position"]["y"].SetFloat(70.0f);
            d["controls_settings"]["layout"]["size"]["y"].SetFloat(1.0f);
            d["controls_settings"]["layout"]["spacing"]["y"].SetFloat(22.0f);
            d["controls_settings"]["art"]["focus_padding"]["y"].SetFloat(1.0f);
            d["controls_settings"]["art"]["keyline_width"].SetFloat(0.25f);
            d["controls_settings"]["art"]["shadow_offset"].SetFloat(0.25f);
            d["controls_settings"]["status_position"]["x"].SetFloat(221.75f);
        };
        // Compact but measured-valid rows leave status bands beside each
        // preview. The 1600x20 source fits to exactly 1590x19.875, so contact
        // with the track/caption top is an exact binary-float boundary.
        for (const float top : { 315.0f, 395.0f, 479.0f })
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                preview_page(d);
                d["menu_text"]["status"]["sticks_pending"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                d["controls_settings"]["status_position"]["y"].SetFloat(top - 19.875f);
            };
            const auto touches_preview = variants.load(contact);
            touches_preview.validate_resources(world.render, world.audio);
            variants.rejects("status.sticks_pending", [&](rapidjson::Document& d)
            {
                contact(d);
                d["controls_settings"]["status_position"]["y"].SetFloat(top - 19.625f);
            }, &world);
        }
        for (const char* key : { "sticks_pending", "sticks_saved", "active_save_failed" })
        {
            variants.rejects(std::string("status.") + key, [&](rapidjson::Document& d)
            {
                preview_page(d);
                d["menu_text"]["status"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                d["controls_settings"]["status_position"]["y"].SetFloat(500.0f);
            }, &world);
        }
    }

    std::wstring stick_readout(const MenuText& text, const char* key, int a = 0, int b = 0, int c = 0, int d = 0)
    {
        const std::string role(key);
        if (role == "connected") { return text.stick_connection(3, true); }
        if (role == "disconnected") { return text.stick_connection(3, false); }
        if (role == "raw_movement") { return text.stick_raw_movement(a); }
        if (role == "movement_response") { return text.stick_movement_response(a); }
        if (role == "aim_inactive") { return text.stick_aim_inactive(a, b); }
        return text.stick_aim_active(a, b, c, d);
    }

    template<class Action> void rejects_stick_format(Action action)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid Pad Sticks template/argument was formatted silently");
    }

    void compact_stick_readouts(rapidjson::Document& d)
    {
        auto& text = d["menu_text"]["stick_readouts"];
        text["connected"].SetString("{device}");
        text["disconnected"].SetString("{device}");
        text["raw_movement"].SetString("{value}");
        text["movement_response"].SetString("{value}");
        text["aim_inactive"].SetString("{raw_x}\n{raw_y}");
        text["aim_active"].SetString("{raw_x}\n{raw_y}\n{aim_x}\n{aim_y}");
    }

    void check_stick_readout_text(Variants& variants, World& world, const MenuText& defaults)
    {
        for (int slot = 0; slot < 4; ++slot)
        {
            require(defaults.stick_connection(slot, true) == L"Pad " + std::to_wstring(slot + 1) + L" - connected" &&
                defaults.stick_connection(slot, false) == L"Pad " + std::to_wstring(slot + 1) + L" - disconnected",
                "Pad Sticks device template used roster numbering or changed connected/disconnected copy");
        }
        for (int value = -100; value <= 100; ++value)
        {
            const auto a = std::to_wstring(value), b = std::to_wstring(-value);
            require(defaults.stick_raw_movement(value) == L"Left stick raw X: " + a + L"%" &&
                defaults.stick_movement_response(value) == L"Move response: " + a + L"%",
                "Pad Sticks single percentage formatter changed a rounded value or literal percent sign");
            require(defaults.stick_aim_inactive(value, -value) == L"Raw X: " + a + L"%\nRaw Y: " + b +
                L"%\nNo new aim\nRaw: hollow\nAim: solid" &&
                defaults.stick_aim_active(value, -value, -value, value) == L"Raw X: " + a + L"%\nRaw Y: " + b +
                L"%\nAim X: " + b + L"%\nAim Y: " + a + L"%\nRaw: hollow\nAim: solid",
                "Pad Sticks aim formatter changed axes, signs, percentages or authored line breaks");
        }
        for (int slot : {(std::numeric_limits<int>::min)(), -1, 4, 5, (std::numeric_limits<int>::max)()})
        {
            for (bool connected : {false, true})
            {
                rejects_stick_format([&] { defaults.stick_connection(slot, connected); });
            }
        }
        for (int value : {(std::numeric_limits<int>::min)(), -101, 101, (std::numeric_limits<int>::max)()})
        {
            rejects_stick_format([&] { defaults.stick_raw_movement(value); });
            rejects_stick_format([&] { defaults.stick_movement_response(value); });
            rejects_stick_format([&] { defaults.stick_aim_inactive(value, 0); });
            rejects_stick_format([&] { defaults.stick_aim_inactive(0, value); });
            rejects_stick_format([&] { defaults.stick_aim_active(value, 0, 0, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, value, 0, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, 0, value, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, 0, 0, value); });
        }
        for (const auto& entry : stick_readout_text)
        {
            const std::string field = std::string("stick_readouts.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                // The additional authored LF tests normalization independently
                // of the shipped six-line aim readout's available height.
                // Exact multiline overflow is exercised below with known glyphs.
                if (entry.multiline) { d["controls_settings"]["help_font"].SetString("menu_text_exact"); }
                const std::string original = d["menu_text"]["stick_readouts"][entry.key].GetString();
                const std::string changed = "  A\xC2\xA0\xE2\x80\x94" "B " + original + (entry.multiline ? "\n C  " : "  ");
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            });
            require(normalized.menu_text().stick_readouts.*entry.field == L"  A -B " + std::wstring(entry.text) +
                (entry.multiline ? L"\n C  " : L"  "), "Pad Sticks copy lost normalized literals or authored outer spaces/LF");
            normalized.validate_resources(world.render, world.audio);
            for (const char* invalid : {"   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stick_readouts"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (const wchar_t* extra : {L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9"})
            {
                MenuText mutated = defaults;
                (mutated.stick_readouts.*entry.field) += extra;
                rejects_stick_format([&] { stick_readout(mutated, entry.key); });
            }
            {
                MenuText mutated = defaults;
                mutated.stick_readouts.*entry.field = L" ";
                rejects_stick_format([&] { stick_readout(mutated, entry.key); });
            }
            const std::vector<const char*> invalid_lines = entry.multiline ?
                std::vector<const char*>{"\n", "\n\n", "\n \n", "\n \xC2\xA0 "} :
                std::vector<const char*>{"\nline"};
            for (const char* invalid : invalid_lines)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + invalid;
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = "\n" + std::string(d["menu_text"]["stick_readouts"][entry.key].GetString());
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            for (const char* token : entry.tokens)
            {
                for (bool duplicate : {false, true})
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        std::string changed = d["menu_text"]["stick_readouts"][entry.key].GetString();
                        if (duplicate) { changed += std::string(" ") + token; }
                        else { changed.erase(changed.find(token), std::strlen(token)); }
                        d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                    });
                    MenuText mutated = defaults;
                    std::wstring wide_token;
                    for (const char* character = token; *character != '\0'; ++character)
                    {
                        wide_token += static_cast<wchar_t>(*character);
                    }
                    auto& changed = mutated.stick_readouts.*entry.field;
                    if (duplicate) { changed += L" " + wide_token; }
                    else { changed.erase(changed.find(wide_token), wide_token.size()); }
                    rejects_stick_format([&] { stick_readout(mutated, entry.key); });
                }
            }
            for (const char* extra : {"{unknown}", "{value }", "{", "}"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + extra;
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["controls_settings"]["help_font"].SetString("menu_text_limited");
                const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + "~";
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string changed = std::string(2000, 'W') + d["menu_text"]["stick_readouts"][entry.key].GetString();
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            }, &world);
        }
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& text = d["menu_text"]["stick_readouts"];
            text["connected"].SetString("Ready: {device}!");
            text["disconnected"].SetString("Absent: {device}!");
            text["raw_movement"].SetString("literal% {value} literal%%");
            text["movement_response"].SetString("{value} movement");
            text["aim_inactive"].SetString("Y{raw_y} X{raw_x}");
            text["aim_active"].SetString("AY{aim_y} RX{raw_x}\nAX{aim_x} RY{raw_y}");
        });
        reordered.validate_resources(world.render, world.audio);
        const auto& copy = reordered.menu_text();
        require(copy.stick_connection(0, true) == L"Ready: Pad 1!" &&
            copy.stick_connection(3, false) == L"Absent: Pad 4!" &&
            copy.stick_raw_movement(-8) == L"literal% -8 literal%%" &&
            copy.stick_movement_response(17) == L"17 movement" &&
            copy.stick_aim_inactive(-17, 23) == L"Y23 X-17" &&
            copy.stick_aim_active(-17, 23, -4, 100) == L"AY100 RX-17\nAX-4 RY23",
            "Pad Sticks formatting fixed token order, line counts, axes or percent literals in code");
        for (const char* key : {"aim_inactive", "aim_active"})
        {
            for (const char* malformed : {"{raw_{raw_x}y}", "{raw_x}{raw_{raw_y}x}",
                "{raw_{raw_x}y}{aim_x}{aim_y}", "{raw_x}{raw_y}{aim_{aim_x}y}",
                "{raw_x}{raw_y}{aim_x}{aim_y}{raw_x}", "{raw_x}{raw_y}{aim_x}{aim_y}{unknown}",
                "{raw_x}{raw_y}{aim_x}{aim_y}{", "{{raw_x}}{raw_y}{aim_x}{aim_y}"})
            {
                variants.rejects(std::string("stick_readouts.") + key, [&](rapidjson::Document& d)
                    { d["menu_text"]["stick_readouts"][key].SetString(malformed, d.GetAllocator()); });
                MenuText mutated = defaults;
                std::wstring wide;
                for (const char* character = malformed; *character != '\0'; ++character)
                {
                    wide += static_cast<wchar_t>(*character);
                }
                (std::string(key) == "aim_active" ? mutated.stick_readouts.aim_active :
                    mutated.stick_readouts.aim_inactive) = wide;
                rejects_stick_format([&] { stick_readout(mutated, key); });
            }
        }
        std::vector<Glyph> missing_eight;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character != U'8')
            {
                missing_eight.push_back({character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f});
            }
        }
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(missing_eight), 20.0f);
        limited->set_stand_in(U'?');
        world.render.add_font("menu_stick_no_eight", std::move(limited));
        variants.rejects("stick_readouts.raw_movement", [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_stick_no_eight");
        }, &world);
    }

    void check_stick_readout_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'~' ? width : 8, character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        struct Boundary
        {
            const char* key;
            const char* width_template;
            const char* height_template;
            int glyph_width;
            int glyph_height;
            mattmath::Vector2F position;
            float width;
            float height;
        };
        const Boundary boundaries[]{
            {"connected", "{device}~", "{device}~", 580, 77, {1120.0f, 210.0f}, 620.0f, 77.0f},
            {"disconnected", "{device}~", "{device}~", 580, 77, {1120.0f, 210.0f}, 620.0f, 77.0f},
            {"raw_movement", "{value}~", "{value}~", 588, 28, {1120.0f, 287.0f}, 620.0f, 28.0f},
            {"movement_response", "{value}~", "{value}~", 588, 28, {1120.0f, 367.0f}, 620.0f, 28.0f},
            {"aim_inactive", "{raw_x}{raw_y}~", "{raw_x}{raw_y}\n~", 316, 230, {1360.0f, 500.0f}, 380.0f, 250.0f},
            {"aim_active", "{raw_x}{raw_y}{aim_x}{aim_y}~", "{raw_x}{raw_y}\n{aim_x}{aim_y}\n~",
                252, 210, {1360.0f, 500.0f}, 380.0f, 250.0f}
        };
        const auto use = [](rapidjson::Document& d, const char* key, const std::string& font, const char* text)
        {
            compact_stick_readouts(d);
            d["controls_settings"]["help_font"].SetString(font.c_str(), d.GetAllocator());
            d["menu_text"]["stick_readouts"][key].SetString(text, d.GetAllocator());
        };
        for (const auto& entry : boundaries)
        {
            const std::string name = std::string("menu_stick_") + entry.key;
            const std::string field = std::string("stick_readouts.") + entry.key;
            add_font(name + "_width_edge", entry.glyph_width, 20);
            add_font(name + "_width_over", entry.glyph_width + 1, 20);
            add_font(name + "_height_edge", 8, entry.glyph_height);
            add_font(name + "_height_over", 8, entry.glyph_height + 1);
            for (bool height : {false, true})
            {
                const auto fits = variants.load([&](rapidjson::Document& d)
                {
                    use(d, entry.key, name + (height ? "_height_edge" : "_width_edge"),
                        height ? entry.height_template : entry.width_template);
                });
                fits.validate_resources(world.render, world.audio);
                std::wstring source = stick_readout(fits.menu_text(), entry.key, -100, -100, -100, -100);
                const std::wstring expected = source;
                UiText text(entry.key, source, fits.controls_settings().help_font, entry.position,
                    &world.render, fits.controls_settings().art.ink);
                source = L"Changed after construction";
                require(text.text() == expected && text.bounds().position() == entry.position &&
                    (height ? text.bounds().height == entry.height : text.bounds().width == entry.width),
                    "Plain Pad Sticks UiText borrowed its copy, fitted its font or missed the inclusive readout boundary");
                if (height && entry.position.x == 1360.0f)
                {
                    require(text.bounds().bottom() == 750.0f && text.text().find(L'\n') != std::wstring::npos,
                        "Full multiline aim bounds failed to reach the association region exactly");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    use(d, entry.key, name + (height ? "_height_over" : "_width_over"),
                        height ? entry.height_template : entry.width_template);
                }, &world);
            }
        }
    }

    void check_stick_numeric_extrema(Variants& variants, World& world)
    {
        // Wide ink need not leave the greatest terminal pen. An 8 draws eighty
        // units but advances zero; 7 and 1 advance twenty while drawing little.
        // The negative bearing on 6 additionally exercises the clamp at zero.
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool digit = character >= U'0' && character <= U'9';
            const int width = character == U'|' ? 1000 : character == U'8' ? 80 : digit || character == U'-' ? 1 : 8;
            const float advance = character == U'8' ? -80.0f : character == U'7' || character == U'1' ? 19.0f : 0.0f;
            glyphs.push_back({character, mattmath::RectangleI(0, 0, width, 16),
                character == U'6' ? -5.0f : 0.0f, 0.0f, advance});
        }
        world.render.add_font("menu_stick_adversarial", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const Font& font = *world.render.font(world.render.resolve_sprite_font("menu_stick_adversarial"));
        float widest_number = 0.0f, maximum_terminal_pen = 0.0f, widest_pair = 0.0f;
        for (int first = -100; first <= 100; ++first)
        {
            const std::wstring a = std::to_wstring(first);
            widest_number = std::max(widest_number, font.measure(a).x);
            // This final, very wide marker makes its right edge dominate all
            // preceding ink, exposing the attained pen through Font::measure.
            maximum_terminal_pen = std::max(maximum_terminal_pen, font.measure(a + L"|").x - 1000.0f);
            for (int second = -100; second <= 100; ++second)
            {
                widest_pair = std::max(widest_pair, font.measure(a + std::to_wstring(second)).x);
            }
        }
        require(widest_number == 101.0f && maximum_terminal_pen == 41.0f && widest_pair == 142.0f &&
            font.measure(L"-100-100").x < widest_pair && font.measure(L"-78-78").x < widest_pair &&
            font.measure(L"-77-78").x == widest_pair,
            "Adversarial font did not independently distinguish widest ink, terminal pen and composed numeric extrema");
        const auto edit = [](rapidjson::Document& d, bool active, bool outside)
        {
            compact_stick_readouts(d);
            d["controls_settings"]["help_font"].SetString("menu_stick_adversarial");
            auto& text = d["menu_text"]["stick_readouts"];
            text["aim_inactive"].SetString("{raw_x}{raw_y}");
            if (active) { text["aim_active"].SetString("{raw_x}{raw_y}{aim_x}{aim_y}"); }
            d["controls_settings"]["stick_preview"]["aim_readout_position"]["x"].SetFloat(
                (active ? 1516.0f : 1598.0f) + (outside ? 0.25f : 0.0f));
        };
        for (bool active : {false, true})
        {
            const auto fits = variants.load([&](rapidjson::Document& d) { edit(d, active, false); });
            fits.validate_resources(world.render, world.audio);
            const std::wstring attained = active ? fits.menu_text().stick_aim_active(-77, -77, -77, -78) :
                fits.menu_text().stick_aim_inactive(-77, -78);
            const float width = active ? 224.0f : widest_pair;
            require(font.measure(attained).x == width &&
                fits.controls_settings().stick_preview.aim_readout_position.x + width == 1740.0f,
                "Composed numeric extrema failed to attain the exact full readout width");
            if (active)
            {
                require(width == maximum_terminal_pen * 3.0f + widest_number,
                    "Four-token adversarial fixture lost its independently attained pen/ink maximum");
            }
            variants.rejects(active ? "stick_readouts.aim_active" : "stick_readouts.aim_inactive",
                [&](rapidjson::Document& d) { edit(d, active, true); }, &world);
        }
    }

    void check_help_bounds(Variants& variants, World& world)
    {
        // A caption spans the remaining help column, rather than the narrower
        // 210-wide aim box. It keeps its unscaled font, unlike both help blocks.
        const auto caption_edge = [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_text_exact");
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(77, 'F').c_str(), d.GetAllocator());
        };
        const auto fits_caption = variants.load(caption_edge);
        fits_caption.validate_resources(world.render, world.audio);
        const auto& controls = fits_caption.controls_settings();
        const auto font = world.render.resolve_sprite_font(controls.help_font);
        const auto caption_size = world.render.measure_text(font, fits_caption.menu_text().help.aim_cutoff);
        require(caption_size.x > controls.stick_preview.aim.width && caption_size.x <= controls.controls_size.x,
            "Aim caption fixture does not distinguish the full help column from the square aim preview");
        const auto caption_position = controls.stick_preview.aim.position() - mattmath::Vector2F(0.0f,
            world.render.measure_text(font, L"Move").y + controls.art.focus_padding.y);
        UiText caption("caption", fits_caption.menu_text().help.aim_cutoff, controls.help_font,
            caption_position, &world.render, controls.art.ink);
        require(caption.text() == fits_caption.menu_text().help.aim_cutoff &&
            caption.bounds() == mattmath::RectangleF(caption_position, caption_size),
            "Actual aim caption changed its unscaled font or placement above the aim box");
        caption.set_hidden(true);
        DrawList no_device;
        caption.draw(no_device);
        variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
        {
            caption_edge(d);
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(78, 'F').c_str(), d.GetAllocator());
        }, &world);
        // These preview coordinates remain legal in the geometric parser, but
        // the measured caption now intrudes into the preceding response track.
        variants.rejects("help.aim_cutoff", [](rapidjson::Document& d)
        {
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(420.0f);
        }, &world);
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool boundary_glyph = character == U'F' || character == U'G';
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary_glyph ? 620 : 8,
                character == U'F' ? 28 : character == U'G' ? 29 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_help_caption_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto exact_caption = [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_help_caption_exact");
            d["menu_text"]["help"]["aim_cutoff"].SetString("F");
        };
        const auto touches_right_and_aim = variants.load(exact_caption);
        touches_right_and_aim.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_help_caption_exact"), L"F") ==
            mattmath::Vector2F(620.0f, 28.0f), "Caption boundary glyph has unexpected metrics");
        for (const char* overflow : { "Fi", "G" })
        {
            variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
            {
                exact_caption(d);
                d["menu_text"]["help"]["aim_cutoff"].SetString(overflow, d.GetAllocator());
            }, &world);
        }
        const auto touches_track = [&](rapidjson::Document& d)
        {
            exact_caption(d);
            // Move is 20 high, plus eight units of existing focus padding:
            // the 28-high F starts exactly at the response track's bottom.
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(447.0f);
        };
        const auto fits_track = variants.load(touches_track);
        fits_track.validate_resources(world.render, world.audio);
        variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
        {
            touches_track(d);
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(446.75f);
        }, &world);
    }
    void check_setting_duration_content(Variants& variants, World& world, const std::filesystem::path& content)
    {
        Registry<LevelLoadedInfo> levels("Duration acceptance level"); StageList stages;
        for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course" })
        {
            auto info = std::make_unique<LevelLoadedInfo>((content / "levels" / (std::string(name) + ".json")).string().c_str());
            info->validate(); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
            levels.add(name, std::move(info));
        }
        const GameTuning authored((content / "tuning.json").string().c_str());
        const rapidjson::Document tuning_source = read_document(content / "tuning.json");
        const auto tune = [&](float seconds, const std::vector<float>& choices)
        {
            rapidjson::Document d; d.CopyFrom(tuning_source, d.GetAllocator());
            d["match"]["duration"].SetFloat(seconds); auto& values = d["match"]["duration_choices"]; values.SetArray();
            for (const float choice : choices)
            {
                rapidjson::Value item(rapidjson::kObjectType);
                item.AddMember("seconds", choice, d.GetAllocator()); values.PushBack(item, d.GetAllocator());
            }
            rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(d.Accept(writer), "Cannot encode isolated duration tuning fixture");
            const auto path = variants.scratch / ("duration-tuning-" + std::to_string(variants.next++) + ".json");
            std::ofstream file(path, std::ios::binary); file.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            file.close(); require(file.good(), "Cannot write isolated duration tuning fixture");
            return GameTuning(path.string().c_str());
        };
        const auto complete = [&](const GameTuning& balance, const Presentation& look)
        { validate_game_content(balance, look, &world.render, &world.audio, levels, stages, PlaySettings{}); };
        const auto rejected = [](const std::string& owner, auto action)
        {
            bool refused = false; std::string diagnostic = "accepted without exception";
            try { action(); }
            catch (const std::runtime_error& error)
            { diagnostic = error.what(); refused = diagnostic.find(owner) != std::string::npos; }
            require(refused, "Duration resource failure lost its actual owner '" + owner + "': " + diagnostic);
        };
        const auto add_font = [&](const char* name, int marker_width, int marker_height,
            float marker_x, float marker_y, char32_t missing = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? marker_width : 8,
                    marker ? marker_height : 16), marker ? marker_x : 0.0f, marker ? marker_y : 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_duration_right", 1136, 16, 0.5f, 0.0f);
        add_font("menu_duration_right_overflow", 1136, 16, 0.75f, 0.0f);
        add_font("menu_duration_height", 8, 82, 0.0f, 0.5f);
        add_font("menu_duration_height_overflow", 8, 82, 0.0f, 0.75f);
        add_font("menu_duration_missing_plus", 8, 16, 0.0f, 0.0f, U'+');
        const auto marker = [](rapidjson::Document& d, const char* font_name)
        {
            d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["values"]["duration_minutes"].SetString("~{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        };
        const auto right = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_right"); });
        const auto height = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_height"); });
        const auto check_row = [&](const Presentation& look, bool right_edge, bool height_edge)
        {
            const MainMenuPresentation& main = look.main_menu(); MenuStyle style = look.menu();
            style.shadow_colour = Colour{};
            const mattmath::Vector2F label_position = style.layout.position + mattmath::Vector2F(0.0f, main.mode_select_spacing.y * 5.0f);
            const mattmath::Vector2F value_position = label_position + mattmath::Vector2F(main.mode_select_spacing.x * 2.0f, 0.0f);
            int adjustments = 0;
            OptionRow row("duration", L"Match Length", label_position, value_position, style, main.art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; });
            UiTextDropShadow* value_widget = nullptr;
            for (const auto& child : row.children())
            { if (child.first == "duration_value") { value_widget = dynamic_cast<UiTextDropShadow*>(child.second); } }
            require(value_widget != nullptr, "Duration row no longer retains its real invisible text shadow");
            std::wstring words = L"~4"; row.set_value(words); words = L"Changed after set_value";
            const auto measured = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), L"~4");
            const auto value_bounds = value_widget->bounds();
            require(value_widget->text() == L"~4" && value_bounds.position() == value_position &&
                value_bounds.size() == measured + mattmath::Vector2F(1.5f, 1.5f) &&
                label_position == mattmath::Vector2F(150.0f, 650.0f) && value_position == mattmath::Vector2F(650.0f, 650.0f),
                "Duration value changed copied text, unscaled font, inherited invisible shadow or authored columns");
            auto padded = row.bounds(); padded.inflate(main.setup_focus_padding);
            require(!right_edge || (measured == mattmath::Vector2F(1144.5f, 20.0f) &&
                padded.right() + main.setup_shadow_offset == 1826.0f),
                "Duration right-edge oracle no longer touches the actual setup interior at 1826");
            require(!height_edge || (measured == mattmath::Vector2F(16.0f, 82.5f) &&
                padded.height + main.setup_shadow_offset == 100.0f),
                "Duration height oracle no longer exactly touches its 100-unit complete-row limit");
            const mattmath::RectangleF panel((mattmath::Vector2F(1920.0f, style.footer_y) - main.setup_box_size) / 2.0f,
                main.setup_box_size);
            MenuPaperArtPresentation art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
            MainMenuModeSelect::check_duration_row(row, panel, art, main.mode_select_spacing.y);
            MainMenuModeSelect::validate_durations(&world.render, authored, look);
            auto produced = MainMenuModeSelect::make_duration_row(&world.render, look.menu(), main, look.menu_text());
            produced->set_value(L"~4");
            require(produced->bounds() == row.bounds(), "Actual Mode factory changed the independently constructed duration row");
            row.adjust(Direction::right); require(adjustments == 1, "Duration row lost its right adjustment callback");
            row.adjust(Direction::left);
            require(adjustments == 0 && value_widget->text() == L"~4", "Duration row lost its left adjustment callback or immutable displayed value");
            row.set_hidden(true); DrawList no_device; row.draw(no_device);
        };
        complete(authored, right); complete(authored, height);
        check_row(right, true, false); check_row(height, false, true);
        const auto right_overflow = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_right_overflow"); });
        const auto height_overflow = variants.load([&](rapidjson::Document& d) { marker(d, "menu_duration_height_overflow"); });
        // All old presentation checks still accept these: only the actual
        // composed numeric value adds the final glyph/quarter unit.
        right_overflow.validate_resources(world.render, world.audio);
        height_overflow.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_duration_right_overflow"), L"~4") ==
            mattmath::Vector2F(1144.75f, 20.0f) &&
            world.render.measure_text(world.render.resolve_sprite_font("menu_duration_height_overflow"), L"~4") ==
            mattmath::Vector2F(16.0f, 82.75f), "Duration overflow fixtures no longer add exactly one quarter unit");
        rejected("mode_select_spacing", [&] { complete(authored, right_overflow); });
        rejected("mode_select_spacing", [&] { complete(authored, height_overflow); });

        // An invisible negative item shadow still extends navigation geometry.
        // Use literal widgets to prove the detached arrow's left92 contact;
        // moving the entire row a quarter left must refuse that same geometry.
        const auto negative_shadow = [&](float label_x, float expected_arrow_left, bool fits)
        {
            MenuStyle style = right.menu(); style.item_shadow_offset = { -1.5f, -1.5f }; style.shadow_colour = Colour{};
            const MainMenuPresentation& main = right.main_menu();
            OptionRow row("negative_duration", L"Match Length", { label_x, 650.0f }, { label_x + 500.0f, 650.0f },
                style, main.art.ink, &world.render, {}); row.set_value(L"~4");
            MenuPaperArtPresentation art = main.art; art.focus_padding = main.setup_focus_padding; art.shadow_offset = main.setup_shadow_offset;
            auto padded = row.bounds(); padded.inflate(art.focus_padding);
            require(row.bounds().left() == label_x - 1.5f && row.bounds().top() == 648.5f &&
                padded.left() - art.keyline_width * 7.0f == expected_arrow_left,
                "Duration negative-shadow oracle lost its inherited extent or exact arrow edge");
            const mattmath::RectangleF panel(80.0f, 20.0f, 1760.0f, 920.0f);
            if (fits) { MainMenuModeSelect::check_duration_row(row, panel, art, 100.0f); }
            else { rejected("mode_select_spacing", [&] { MainMenuModeSelect::check_duration_row(row, panel, art, 100.0f); }); }
        };
        negative_shadow(149.5f, 92.0f, true); negative_shadow(149.25f, 91.75f, false);

        // The full startup/reload validator must inspect the live default AND
        // each authored choice. Neither is inferred from a fixed '240' stand-in.
        const auto narrow = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(790.0f);
            d["menu_text"]["values"]["duration_minutes"].SetString("{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        });
        narrow.validate_resources(world.render, world.audio);
        complete(authored, narrow);
        const GameTuning long_choice = tune(240.0f, { 60.0f, 6000000000.0f });
        const GameTuning long_default = tune(6000000000.0f, { 60.0f, 120.0f });
        rejected("mode_select_spacing", [&] { complete(long_choice, narrow); });
        rejected("mode_select_spacing", [&] { complete(long_default, narrow); });
        const auto missing_plus = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_duration_missing_plus");
            d["menu_text"]["values"]["duration_minutes"].SetString("{value}");
            d["menu_text"]["values"]["duration_default"].SetString("{value}");
        });
        missing_plus.validate_resources(world.render, world.audio);
        require(world.render.font(world.render.resolve_sprite_font("menu_duration_missing_plus"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_duration_missing_plus"))->contains(U'+'),
            "Duration missing-generated-glyph fixture lost its stand-in or missing plus");
        rejected("menu_text.values.duration_minutes", [&] { complete(long_choice, missing_plus); });
        rejected("menu_text.values.duration_default", [&] { complete(long_default, missing_plus); });
        for (const char* key : { "duration_minutes", "duration_default" })
        {
            variants.rejects(std::string("values.") + key, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["values"][key].SetString("~{value}");
            }, &world);
        }
        const auto long_words = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{value}";
            d["menu_text"]["values"]["duration_minutes"].SetString(words.c_str(), d.GetAllocator());
        });
        long_words.validate_resources(world.render, world.audio);
        rejected("mode_select_spacing", [&] { complete(authored, long_words); });
        // Preserve the established diagnostic owner before the appended paired
        // duration validation, even when both authored roles are invalid.
        const auto legacy_first = variants.load([](rapidjson::Document& d)
        {
            const std::string words(512, 'W');
            d["menu_text"]["home_subtitle"].SetString(words.c_str(), d.GetAllocator());
            d["menu_text"]["values"]["duration_minutes"].SetString((words + "{value}").c_str(), d.GetAllocator());
        });
        rejected("home_subtitle", [&] { complete(authored, legacy_first); });
    }

    void check_setting_duration_text(Variants& variants, const MenuText& defaults)
    {
        struct DurationExample { float seconds; const wchar_t* minutes; };
        const DurationExample examples[]{
            { 240.0f, L"4" }, { 90.0f, L"1.5" }, { 61.0f, L"1.01667" }, { 123.0f, L"2.05" },
            { 1.0f, L"0.0166667" }, { 6000000000.0f, L"1e+08" }, { 0.000006f, L"1e-07" },
            { std::numeric_limits<float>::min(), L"1.95916e-40" },
            { std::numeric_limits<float>::denorm_min(), L"0" },
            { std::numeric_limits<float>::max(), L"5.67137e+36" },
            // 599.9997f/60.0f rounds to 9.999995231628... before formatting.
            // Promoting the division to double instead prints 9.99999 at six digits.
            { 599.9997f, L"10" }
        };
        for (const auto& example : examples)
        {
            const std::wstring explicit_label = std::wstring(example.minutes) + L" min";
            require(defaults.setting_duration(example.seconds, false) == explicit_label &&
                defaults.setting_duration(example.seconds, true) == L"Default (" + explicit_label + L")",
                "Duration formatting changed FLOAT seconds/60.0f, six-significant defaultfloat copy or Default composition");
        }
        const auto rejects_format = [](auto format)
        {
            bool refused = false;
            try { format(); }
            catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid duration argument or directly mutated template formatted silently");
        };
        MenuText unused_wrapper = defaults; unused_wrapper.values.duration_default = L"{unknown}";
        require(unused_wrapper.setting_duration(240.0f, false) == L"4 min",
            "Explicit duration unexpectedly evaluated its unused Default wrapper");
        for (const float invalid : { 0.0f, -0.0f, -1.0f, -std::numeric_limits<float>::denorm_min(),
            std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            for (const bool use_default : { false, true })
            { rejects_format([&] { defaults.setting_duration(invalid, use_default); }); }
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["duration_minutes"].SetString(
                "  {value}\xC2\xA0\xE2\x80\x9C" "min\xE2\x80\x9D\xE2\x80\x94\xE2\x80\xA6  ");
            d["menu_text"]["values"]["duration_default"].SetString("  <{value}>  ");
        });
        require(normalized.menu_text().values.duration_minutes == L"  {value} \"min\"-...  " &&
            normalized.menu_text().setting_duration(90.0f, false) == L"  1.5 \"min\"-...  " &&
            normalized.menu_text().setting_duration(90.0f, true) == L"  <  1.5 \"min\"-...  >  ",
            "Duration templates lost authored outer spaces, Unicode normalization or literal wrapper placement");
        const auto reversed = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["duration_minutes"].SetString("minutes:%{value}%%");
            d["menu_text"]["values"]["duration_default"].SetString("{value}:default");
        });
        require(reversed.menu_text().setting_duration(123.0f, true) == L"minutes:%2.05%%:default" &&
            reversed.menu_text().setting_duration(123.0f, false) == L"minutes:%2.05%%",
            "Duration templates treated literal percent signs as formatting syntax or fixed the token's position");
        struct DurationTemplate { const char* key; std::wstring MenuValueText::* field; bool wrapper; };
        const DurationTemplate templates[]{
            { "duration_minutes", &MenuValueText::duration_minutes, false },
            { "duration_default", &MenuValueText::duration_default, true }
        };
        for (const auto& entry : templates)
        {
            const std::string field = std::string("values.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "{value}\n", "{value}\r", "{value}\r\n",
                "{value}\t", "{value}\x01", "{value}\x7f", "{value}caf\xC3\xA9", "{value}\xF0\x9F\x8E\xA8", "{value}\xC3\x28",
                "No token", "{value}{value}", "{Value}", "{value }", "{{value}}", "{value", "value}",
                "{value}{unknown}", "{va{value}lue}", "{value}%{", "{value}}", "{}{value}" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["values"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (int type = 0; type < 3; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& target = d["menu_text"]["values"][entry.key];
                    if (type == 0) { target.SetArray(); }
                    else if (type == 1) { target.SetObject(); }
                    else { target.SetInt(12); }
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["values"][entry.key].SetString("{value}\0tail", 12, d.GetAllocator());
            });
            for (const wchar_t* invalid : { L"", L"   ", L"No token", L"{value}{value}", L"{{value}}",
                L"{va{value}lue}", L"{value}{unknown}", L"{value", L"value}", L"{value}}", L"{value}%{",
                L"{value}\n", L"{value}\r", L"{value}\t", L"{value}\x01", L"{value}\x7f",
                L"{value}\u00e9", L"{value}\u00a0", L"{value}\u2014" })
            {
                MenuText mutated = defaults; mutated.values.*entry.field = invalid;
                rejects_format([&] { mutated.setting_duration(240.0f, entry.wrapper); });
                if (!entry.wrapper) { rejects_format([&] { mutated.setting_duration(240.0f, true); }); }
            }
            MenuText nul = defaults; (nul.values.*entry.field).push_back(L'\0');
            rejects_format([&] { nul.setting_duration(240.0f, entry.wrapper); });
        }
        // Length is a resource/layout concern, not a hidden formatter cap.
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{value}";
            d["menu_text"]["values"]["duration_minutes"].SetString(words.c_str(), d.GetAllocator());
        });
        require(long_supported.menu_text().setting_duration(240.0f, false) == std::wstring(512, L'W') + L"4",
            "Duration formatter truncated supported authored text or imposed an unrelated length cap");
    }
    void check_setting_resolution_resources(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int eight_width, int eight_height,
            char32_t missing = U'\0', bool zero_resolution = false, float eight_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool eight = character == U'8';
                const bool zero = zero_resolution && ((character >= U'0' && character <= U'9') || character == U'x');
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, zero ? 0 : eight ? eight_width : 8,
                    eight ? eight_height : 16), eight ? eight_offset : 0.0f, 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_resolution_width", 2000, 16);
        add_font("menu_resolution_height", 8, 200);
        add_font("menu_resolution_both", 2000, 2000);
        add_font("menu_resolution_missing9", 8, 16, U'9');
        add_font("menu_resolution_zero", 8, 16, U'\0', true);
        add_font("menu_resolution_nonfinite", 8, 16, U'\0', false, std::numeric_limits<float>::infinity());
        const auto check_row = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            const MainMenuPresentation& main = look.main_menu(); const MenuStyle& style = look.menu();
            const MenuPaperArtPresentation art = options_art(main);
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, style.footer_y) - main.options_box_size) / 2.0f,
                main.options_box_size);
            const mattmath::Vector2F value_position(main.options_value_x, main.options_position.y);
            const mattmath::Vector2F limit(paper.right() - art.shadow_offset - art.focus_padding.x - value_position.x,
                main.options_spacing.y - art.focus_padding.y * 2.0f);
            require(value_position == mattmath::Vector2F(820.0f, 220.0f) && limit == mattmath::Vector2F(930.0f, 86.0f),
                "Resolution fixture no longer exercises the actual Options value column and row");
            const FontHandle font_handle = world.render.resolve_sprite_font(style.item_font);
            const std::wstring expected = L"1280x720";
            const auto measured = world.render.measure_text(font_handle, expected);
            float expected_scale = 1.0f;
            if (measured.x > 930.0f) { expected_scale = 930.0f / measured.x; }
            require(width_limited == (expected_scale < 1.0f) &&
                height_limited == (measured.y * expected_scale > 86.0f),
                "Resolution font does not independently exercise its claimed width-then-height fitting path");
            if (measured.y * expected_scale > 86.0f) { expected_scale = 86.0f / measured.y; }
            int adjustments = 0;
            OptionRow row("resolution", L"Resolution", main.options_position, value_position, style, art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; }, &art, limit);
            UiText* value = row_text(row, "resolution_value"); UiText* label = row_text(row, "resolution_label");
            require(value != nullptr && label != nullptr, "Resolution row lost its actual plain-ink OptionRow children");
            const auto retained_label = label->bounds();
            std::wstring formatted = look.menu_text().setting_resolution(1280, 720);
            require(formatted == expected, "Resolution fitting oracle lost its independent literal");
            row.set_value(formatted); formatted = L"Changed after set_value";
            const auto bounds = value->bounds();
            require(value->text() == expected && bounds.position() == value_position && label->bounds() == retained_label &&
                std::abs(bounds.width - measured.x * expected_scale) <= 0.01f &&
                std::abs(bounds.height - measured.y * expected_scale) <= 0.01f &&
                bounds.width > 0.0f && bounds.height > 0.0f && bounds.width <= 930.01f && bounds.height <= 86.01f,
                "Resolution OptionRow lost copied text, its fixed column/label or proportional fitting");
            auto padded = bounds; padded.inflate(art.focus_padding);
            require(paper.contains(padded) && bounds.height + art.focus_padding.y * 2.0f <= main.options_spacing.y + 0.01f,
                "Resolution value or focus padding escaped the actual Options paper/row");
            if (height_limited) { require(std::abs(bounds.height - 86.0f) <= 0.01f, "Resolution height fitting lost inclusive row contact"); }
            else if (width_limited) { require(std::abs(bounds.width - 930.0f) <= 0.01f, "Resolution width fitting lost inclusive column contact"); }
            else { require(bounds == mattmath::RectangleF(value_position, measured), "Short resolution value was enlarged"); }
            row.set_value(look.menu_text().setting_resolution(1, 1));
            require(value->text() == L"1x1" && value->bounds() == mattmath::RectangleF(value_position,
                world.render.measure_text(font_handle, L"1x1")) && label->bounds() == retained_label,
                "Short resolution retained the previous long/tall shrink scale or changed its label");
            row.adjust(Direction::right); require(adjustments == 1, "Resolution row lost its right adjustment callback");
            row.adjust(Direction::left); require(adjustments == 0, "Resolution row lost its left adjustment callback");
            row.set_hidden(true); DrawList no_device; row.draw(no_device);
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio); check_row(defaults, false, false);
        for (int mode = 0; mode < 3; ++mode)
        {
            const char* names[]{ "menu_resolution_width", "menu_resolution_height", "menu_resolution_both" };
            const auto look = variants.load([&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["item"].SetString(names[mode], d.GetAllocator()); });
            look.validate_resources(world.render, world.audio);
            const auto raw = world.render.measure_text(world.render.resolve_sprite_font(names[mode]), L"1280x720");
            require(raw == (mode == 0 ? mattmath::Vector2F(2056.0f, 20.0f) : mode == 1 ?
                mattmath::Vector2F(64.0f, 200.0f) : mattmath::Vector2F(2056.0f, 2000.0f)),
                "Adversarial resolution glyph fixture changed its independent literal dimensions");
            check_row(look, mode != 1, mode != 0);
        }
        for (const char* font_name : { "menu_resolution_missing9", "menu_resolution_zero", "menu_resolution_nonfinite" })
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_resolution_missing9"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_resolution_missing9"))->contains(U'9'),
            "Resolution decimal-glyph fixture lost its stand-in or missing digit");
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Resolution literal-glyph fixture lost its stand-in or missing tilde");
        variants.rejects("values.resolution", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        const auto separate_controls = variants.load([](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_limited");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        });
        separate_controls.validate_resources(world.render, world.audio);
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{width}x{height}";
            d["menu_text"]["values"]["resolution"].SetString(words.c_str(), d.GetAllocator());
        });
        long_supported.validate_resources(world.render, world.audio);
        const MenuPaperArtPresentation art = options_art(long_supported.main_menu());
        OptionRow long_row("long_resolution", L"Resolution", { 280.0f, 220.0f }, { 820.0f, 220.0f },
            long_supported.menu(), art.ink, &world.render, {}, &art, { 930.0f, 86.0f });
        long_row.set_value(long_supported.menu_text().setting_resolution(1, 1));
        const UiText* long_value = row_text(long_row, "long_resolution_value");
        require(long_value != nullptr && long_value->text() == std::wstring(512, L'W') + L"1x1" &&
            std::abs(long_value->bounds().width - 930.0f) <= 0.01f && long_value->bounds().height < 86.0f,
            "Long supported resolution template was truncated or failed width-only fitting");
        // Preserve the existing value diagnostic before the appended resolution
        // guard when the shared item font cannot draw both authored literals.
        variants.rejects("values.on", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["on"].SetString("~");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        variants.rejects("values.duration_minutes", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["duration_minutes"].SetString("~{value}");
            d["menu_text"]["values"]["resolution"].SetString("~{width}x{height}");
        }, &world);
        Presentation mutated = defaults;
        const_cast<MenuText&>(mutated.menu_text()).values.resolution = L"{he{width}ight}";
        bool contextual = false;
        try { mutated.validate_resources(world.render, world.audio); }
        catch (const std::runtime_error& error)
        { contextual = std::string(error.what()).find("menu_text.values.resolution") != std::string::npos; }
        require(contextual, "Direct malformed resolution template escaped resource validation or lost its named role");
    }

    void check_setting_resolution_text(Variants& variants, const MenuText& defaults)
    {
        struct ResolutionExample { int width; int height; const wchar_t* label; };
        const ResolutionExample examples[]{
            { 1280, 720, L"1280x720" }, { 1920, 1080, L"1920x1080" },
            { 2560, 1440, L"2560x1440" }, { 3840, 2160, L"3840x2160" },
            { 1, 1, L"1x1" }, { 801, 601, L"801x601" }, { 12345, 67890, L"12345x67890" },
            { std::numeric_limits<int>::max(), 1, L"2147483647x1" },
            { 1, std::numeric_limits<int>::max(), L"1x2147483647" },
            { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), L"2147483647x2147483647" }
        };
        for (const auto& example : examples)
        {
            require(defaults.setting_resolution(example.width, example.height) == example.label,
                "Resolution formatter changed decimal dimensions, separator/order or the full positive integer domain");
        }
        const auto rejects_format = [](auto action)
        {
            bool refused = false;
            try { action(); }
            catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid resolution dimensions or directly mutated template formatted silently");
        };
        for (const int invalid : { 0, -1, std::numeric_limits<int>::min() })
        {
            rejects_format([&] { defaults.setting_resolution(invalid, 720); });
            rejects_format([&] { defaults.setting_resolution(1280, invalid); });
            rejects_format([&] { defaults.setting_resolution(invalid, invalid); });
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["resolution"].SetString(
                "  {height}\xC2\xA0\xE2\x80\x9C" "high\xE2\x80\x9D\xE2\x80\x94" "{width}\xE2\x80\xA6  ");
        });
        require(normalized.menu_text().values.resolution == L"  {height} \"high\"-{width}...  " &&
            normalized.menu_text().setting_resolution(801, 601) == L"  601 \"high\"-801...  ",
            "Resolution template lost normalized punctuation, authored outer spacing or reversed token order");
        const auto literals = variants.load([](rapidjson::Document& d)
            { d["menu_text"]["values"]["resolution"].SetString("H={height}% W={width}%%"); });
        require(literals.menu_text().setting_resolution(1, 23) == L"H=23% W=1%%",
            "Resolution template treated literal percent signs as another formatting language");
        for (const char* invalid : { "   ", " \xC2\xA0 ", "{width}x{height}\n", "{width}x{height}\r", "{width}x{height}\r\n",
            "{width}x{height}\t", "{width}x{height}\x01", "{width}x{height}\x7f", "{width}x{height}caf\xC3\xA9",
            "{width}x{height}\xF0\x9F\x8E\xA8", "{width}x{height}\xC3\x28", "No tokens", "{width}", "{height}",
            "{width}{width}{height}", "{width}{height}{height}", "{Width}{height}", "{width }{height}",
            "{{width}}{height}", "{width}{unknown}{height}", "{width}{height", "width}{height}",
            "{width}{height}}", "{width}{height}{", "{he{width}ight}", "{wi{width}dth}{height}", "{width}{he{height}ight}" })
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["resolution"].SetString(invalid, d.GetAllocator()); });
        }
        for (int type = 0; type < 3; ++type)
        {
            variants.rejects("values.resolution", [&](rapidjson::Document& d)
            {
                auto& field = d["menu_text"]["values"]["resolution"];
                if (type == 0) { field.SetArray(); }
                else if (type == 1) { field.SetObject(); }
                else { field.SetInt(1280); }
            });
        }
        variants.rejects("values.resolution", [](rapidjson::Document& d)
        {
            const std::string nul("{width}x{height}\0tail", 21);
            d["menu_text"]["values"]["resolution"].SetString(nul.data(), static_cast<rapidjson::SizeType>(nul.size()), d.GetAllocator());
        });
        for (const wchar_t* invalid : { L"", L"   ", L"{width}", L"{height}", L"{width}{width}{height}",
            L"{width}{height}{height}", L"{Width}{height}", L"{{width}}{height}", L"{width}{height}}", L"{width}{height}{",
            L"{width}{height", L"{width}{unknown}{height}", L"{he{width}ight}", L"{wi{width}dth}{height}", L"{width}{he{height}ight}",
            L"{width}{height}\n", L"{width}{height}\r", L"{width}{height}\t", L"{width}{height}\x01", L"{width}{height}\x7f",
            L"{width}{height}\u00e9", L"{width}{height}\u00a0", L"{width}{height}\u2014" })
        {
            MenuText mutated = defaults; mutated.values.resolution = invalid;
            rejects_format([&] { mutated.setting_resolution(1280, 720); });
        }
        MenuText nul = defaults; nul.values.resolution.push_back(L'\0');
        rejects_format([&] { nul.setting_resolution(1280, 720); });
        const auto long_supported = variants.load([](rapidjson::Document& d)
        {
            const std::string words = std::string(512, 'W') + "{width}x{height}";
            d["menu_text"]["values"]["resolution"].SetString(words.c_str(), d.GetAllocator());
        });
        require(long_supported.menu_text().setting_resolution(1, 1) == std::wstring(512, L'W') + L"1x1",
            "Resolution formatting truncated supported text or introduced an unrelated length cap");
    }
    void check_end_continuation_text(Variants& variants, World& world)
    {
        for (const auto& entry : end_continuation_text)
        {
            const std::string field = std::string("end.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().end.*entry.field == L"  A \"B\"-C...  ",
                "End continuation lost normalized punctuation or authored outer spaces: " + field);
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString("{next} % [again]"); });
            plain.validate_resources(world.render, world.audio);
            std::wstring source = plain.menu_text().end.*entry.field;
            const auto position = plain.end_menu().layout.position;
            const auto measured = world.render.measure_text(world.render.resolve_sprite_font(plain.menu().item_font), L"{next} % [again]");
            UiText text(entry.key, source, plain.menu().item_font, position, &world.render, plain.end_menu().art.ink);
            source = L"Changed after construction";
            require(text.text() == L"{next} % [again]" && text.bounds() == mattmath::RectangleF(position, measured),
                "Plain End continuation interpreted braces/percent, borrowed text, scaled or retained a text shadow: " + field);
            text.set_hidden(true); DrawList no_device; text.draw(no_device);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["end"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (int type = 0; type < 3; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& target = d["menu_text"]["end"][entry.key];
                    if (type == 0) { target.SetArray(); }
                    else if (type == 1) { target.SetObject(); }
                    else { target.SetInt(7); }
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["end"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().end.*entry.field).size() == 512,
                "Plain End continuation acquired a parser character cap: " + field);
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "End continuation missing-literal fixture lost its stand-in or absent glyph");
    }

    void check_end_continuation_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, float x_offset = 0.0f,
            float y_offset = 0.0f, float line_spacing = 20.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? width : 8, marker ? height : 16),
                    marker ? x_offset : 0.0f, marker ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        add_font("menu_continue_right", 722, 16);
        add_font("menu_continue_right_overflow", 722, 16, 0.25f);
        add_font("menu_continue_height", 8, 89);
        add_font("menu_continue_height_overflow", 8, 89, 0.0f, 0.25f);
        add_font("menu_continue_zero_width", 0, 20);
        add_font("menu_continue_zero_height", 8, 0, 0.0f, 0.0f, 0.0f);
        add_font("menu_continue_infinite_width", 8, 20, std::numeric_limits<float>::infinity());
        add_font("menu_continue_infinite_height", 8, 20, 0.0f, std::numeric_limits<float>::infinity());
        const auto use_font = [](rapidjson::Document& d, const char* key, const char* font_name)
        {
            d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["end"][key].SetString("~");
        };
        for (const auto& entry : end_continuation_text)
        {
            const std::string field = std::string("end.") + entry.key;
            for (const bool height_edge : { false, true })
            {
                const auto edge = variants.load([&](rapidjson::Document& d)
                    { use_font(d, entry.key, height_edge ? "menu_continue_height" : "menu_continue_right"); });
                edge.validate_resources(world.render, world.audio);
                const auto& end = edge.end_menu();
                const auto measured = world.render.measure_text(world.render.resolve_sprite_font(edge.menu().item_font), L"~");
                UiText value("continuation", L"~", edge.menu().item_font, end.layout.position, &world.render, end.art.ink);
                const mattmath::RectangleF paper(470.0f, 130.0f, 980.0f, 820.0f);
                require(value.bounds() == mattmath::RectangleF(700.0f, 330.0f,
                    height_edge ? 8.0f : 722.0f, height_edge ? 89.0f : 20.0f) &&
                    value.bounds().size() == measured, "Continuation edge fixture changed the actual plain unscaled UiText bounds");
                auto padded = value.bounds(); padded.inflate(end.art.focus_padding);
                require(paper.contains(padded) && (height_edge ? padded.height == 125.0f : padded.right() == 1450.0f),
                    "Continuation edge fixture lost exact inclusive text-plus-focus-padding contact");
                require(height_edge ? padded.height + end.art.shadow_offset > end.layout.spacing.y :
                    padded.right() + end.art.shadow_offset > paper.right(),
                    "Continuation fixture no longer distinguishes the existing plain rule from an added card-shade constraint");
                variants.rejects(field, [&](rapidjson::Document& d)
                    { use_font(d, entry.key, height_edge ? "menu_continue_height_overflow" : "menu_continue_right_overflow"); }, &world);
            }
            for (const char* font_name : { "menu_continue_zero_width", "menu_continue_zero_height",
                "menu_continue_infinite_width", "menu_continue_infinite_height" })
            {
                // Zero measured height needs zero font line spacing. The earlier
                // shared-font Join roster guard owns that globally invalid font.
                const std::string owner = std::string(font_name) == "menu_continue_zero_height" ?
                    "labels.roster_row with menu_text.labels.waiting at Join requires finite positive font line spacing" : field;
                variants.rejects(owner, [&](rapidjson::Document& d) { use_font(d, entry.key, font_name); }, &world);
            }
        }
        const auto full_height = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            d["menu_style"]["fonts"]["item"].SetString("menu_end_heading_small_actions");
            d["end_menu"]["heading_y"].SetFloat(900.0f);
            d["end_menu"]["layout"]["position"]["y"].SetFloat(920.5f);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["end_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["end_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["end_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
        });
        full_height.validate_resources(world.render, world.audio);
        const auto& end = full_height.end_menu();
        require(end.layout.position.y + 2.0f + end.art.focus_padding.y == 923.0f &&
            923.0f > (full_height.menu().footer_y + end.box_size.y) / 2.0f && 923.0f < 950.0f,
            "Continuation full-height fixture no longer distinguishes End paper from the menu-footer paper");
        variants.rejects("items.change_teams", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["items"]["change_teams"].SetString("~"); d["menu_text"]["end"]["rematch"].SetString("~");
        }, &world);
        variants.rejects("end.match_complete", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited");
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["end"]["match_complete"].SetString("~"); d["menu_text"]["end"]["rematch"].SetString("~");
        }, &world);
    }
    void check_results_plain_text(Variants& variants, World& world)
    {
        variants.rejects("results", [](rapidjson::Document& d) { d["menu_text"].RemoveMember("results"); });
        variants.rejects("results", [](rapidjson::Document& d) { d["menu_text"]["results"].SetArray(); });
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& group = d["menu_text"]["results"]; rapidjson::Value reverse(rapidjson::kObjectType);
            auto member = group.MemberEnd();
            while (member != group.MemberBegin())
            { --member; rapidjson::Value key; key.CopyFrom(member->name, d.GetAllocator());
                rapidjson::Value value; value.CopyFrom(member->value, d.GetAllocator());
                reverse.AddMember(key, value, d.GetAllocator()); }
            group = std::move(reverse);
        });
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : results_text)
                d["menu_text"]["results"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : results_text)
        {
            const std::string field = std::string("results.") + entry.key;
            require(reordered.menu_text().results.*entry.field == entry.text &&
                normalized.menu_text().results.*entry.field == L"  A \"B\"-C...  ",
                "Plain Results copy depends on key order or lost normalized punctuation/outer spacing: " + field);
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            for (int type = 0; type < 5; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& value = d["menu_text"]["results"][entry.key];
                    if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                    else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
                });
            }
            for (const char* invalid : { "", "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("{copy} % again"); });
            plain.validate_resources(world.render, world.audio);
            std::wstring source = plain.menu_text().results.*entry.field;
            const auto& look = plain.results_menu(); const auto& style = plain.menu();
            const bool heading = entry.field == &MenuResultsText::single_match_heading;
            const bool contribution = entry.field == &MenuResultsText::contribution_heading;
            const std::string font = contribution ? look.contribution_font : heading ? style.heading_font : style.detail_font;
            const auto measured = world.render.measure_text(world.render.resolve_sprite_font(font), L"{copy} % again");
            const mattmath::Vector2F position = contribution ? look.contribution_position :
                mattmath::Vector2F((1920.0f - measured.x) / 2.0f, heading ? look.heading_y : look.proceed_y);
            UiTextDropShadow widget(entry.key, source, font, position, &world.render,
                look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source = L"Changed after construction";
            require(widget.text() == L"{copy} % again" && widget.bounds() == mattmath::RectangleF(position, measured) &&
                widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
                "Actual Results text interpreted plain braces/percent, borrowed source, scaled or retained a shadow: " + field);
            DrawList no_device; widget.draw(no_device);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (contribution) { d["results_menu"]["contribution_font"].SetString("menu_text_limited"); }
                else { d["menu_style"]["fonts"][heading ? "heading" : "detail"].SetString("menu_text_limited"); }
                d["menu_text"]["results"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().results.*entry.field).size() == 512, "Plain Results text acquired a parser character cap");
            variants.rejects(contribution ? "records" : field, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Plain Results missing-literal fixture lost its stand-in or missing glyph");
    }

    void check_results_plain_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, float x_offset = 0.0f, float y_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? width : 8, marker ? height : 16),
                    marker ? x_offset : 0.0f, marker ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        add_font("menu_results_width", 1300, 16);
        add_font("menu_results_width_overflow", 1300, 16, 0.25f);
        add_font("menu_results_proceed_bottom", 8, 90);
        add_font("menu_results_proceed_bottom_overflow", 8, 90, 0.0f, 0.25f);
        add_font("menu_results_record_gap", 765, 16, 0.75f);
        add_font("menu_results_record_contact", 765, 16, 1.0f);
        add_font("menu_results_record_overlap", 765, 16, 1.25f);
        add_font("menu_results_roster_contact", 8, 34);
        add_font("menu_results_roster_overlap", 8, 34, 0.0f, 0.25f);
        add_font("menu_results_zero_width", 0, 16);
        add_font("menu_results_infinite_width", 8, 16, std::numeric_limits<float>::infinity());
        add_font("menu_results_infinite_height", 8, 16, 0.0f, std::numeric_limits<float>::infinity());
        const auto centered = [](rapidjson::Document& d, bool heading, const char* font_name)
        {
            d["menu_style"]["fonts"][heading ? "heading" : "detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"][heading ? "single_match_heading" : "proceed"].SetString("~");
        };
        for (const bool heading : { true, false })
        {
            const std::string field = heading ? "results.single_match_heading" : "results.proceed";
            const auto edge = variants.load([&](rapidjson::Document& d) { centered(d, heading, "menu_results_width"); });
            edge.validate_resources(world.render, world.audio);
            const float y = heading ? 125.0f : 880.0f;
            const std::string font = heading ? edge.menu().heading_font : edge.menu().detail_font;
            UiTextDropShadow widget("results-edge", L"~", font, { 310.0f, y }, &world.render,
                edge.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO);
            require(widget.bounds() == mattmath::RectangleF(310.0f, y, 1300.0f, 20.0f),
                "Centered Results edge lost its unscaled zero-offset shadow bounds");
            variants.rejects(field, [&](rapidjson::Document& d) { centered(d, heading, "menu_results_width_overflow"); }, &world);
            for (const char* font_name : { "menu_results_zero_width", "menu_results_infinite_width", "menu_results_infinite_height" })
                variants.rejects(field, [&](rapidjson::Document& d) { centered(d, heading, font_name); }, &world);
        }
        const auto top = variants.load([&](rapidjson::Document& d)
        { centered(d, true, "menu_results_width"); d["results_menu"]["heading_y"].SetFloat(110.0f); });
        top.validate_resources(world.render, world.audio);
        variants.rejects("results.single_match_heading", [&](rapidjson::Document& d)
        { centered(d, true, "menu_results_width"); d["results_menu"]["heading_y"].SetFloat(109.75f); }, &world);
        const auto bottom = variants.load([&](rapidjson::Document& d) { centered(d, false, "menu_results_proceed_bottom"); });
        bottom.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font(bottom.menu().detail_font), L"~").y +
            bottom.results_menu().proceed_y == 970.0f, "Results proceed text no longer touches the full paper bottom");
        variants.rejects("results.proceed", [&](rapidjson::Document& d)
            { centered(d, false, "menu_results_proceed_bottom_overflow"); }, &world);

        const auto contribution = [](rapidjson::Document& d, const char* font_name)
        {
            d["results_menu"]["contribution_font"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"]["contribution_heading"].SetString("~");
            d["menu_text"]["records"]["new_best"].SetString("R"); d["menu_text"]["records"]["stage_best"].SetString("R");
        };
        const auto record_gap = variants.load([&](rapidjson::Document& d) { contribution(d, "menu_results_record_gap"); });
        record_gap.validate_resources(world.render, world.audio);
        const auto font_handle = world.render.resolve_sprite_font(record_gap.results_menu().contribution_font);
        require(world.render.measure_text(font_handle, L"R 100.0%") == mattmath::Vector2F(64.0f, 20.0f) &&
            world.render.measure_text(font_handle, L"~") == mattmath::Vector2F(765.75f, 20.0f) &&
            480.0f + 765.75f == 1246.0f - 0.25f,
            "Authored contribution heading lost its independent quarter-unit strict record gap");
        // Record clearance is STRICT: contact already rejects before overlap.
        for (const char* font_name : { "menu_results_record_contact", "menu_results_record_overlap" })
            variants.rejects("records", [&](rapidjson::Document& d) { contribution(d, font_name); }, &world);
        const auto roster_contact = variants.load([&](rapidjson::Document& d) { contribution(d, "menu_results_roster_contact"); });
        roster_contact.validate_resources(world.render, world.audio);
        const auto& look = roster_contact.results_menu();
        const auto raw = world.render.measure_text(world.render.resolve_sprite_font(look.contribution_font), L"P1 - Pad 1 | Team A | 0.0%");
        require(raw.y == 20.0f && roster_contact.player_identity().ui_size == mattmath::Vector2F(54.0f, 42.0f) &&
            look.contribution_position.y + 34.0f == 669.0f &&
            look.contribution_position.y + look.contribution_spacing + (20.0f - 42.0f) / 2.0f == 669.0f,
            "Authored contribution heading lost exact contact with the first complete roster/badge row");
        variants.rejects("labels.roster_row with menu_text.labels.pad at Results", [&](rapidjson::Document& d)
            { contribution(d, "menu_results_roster_overlap"); }, &world);
        for (const char* font_name : { "menu_results_zero_width", "menu_results_infinite_width", "menu_results_infinite_height" })
            variants.rejects("results.contribution_heading", [&](rapidjson::Document& d) { contribution(d, font_name); }, &world);
        variants.rejects("values.on", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited"); d["menu_text"]["values"]["on"].SetString("~");
            d["menu_text"]["results"]["proceed"].SetString(std::string(512, 'W').c_str(), d.GetAllocator());
        }, &world);
    }
    void check_session_result_text(Variants& variants, World& world)
    {
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& group = d["menu_text"]["results"]; rapidjson::Value reverse(rapidjson::kObjectType);
            auto member = group.MemberEnd();
            while (member != group.MemberBegin())
            { --member; rapidjson::Value key; key.CopyFrom(member->name, d.GetAllocator());
                rapidjson::Value value; value.CopyFrom(member->value, d.GetAllocator());
                reverse.AddMember(key, value, d.GetAllocator()); }
            group = std::move(reverse);
        });
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : session_result_text)
                d["menu_text"]["results"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : session_result_text)
        {
            const std::string field = std::string("results.") + entry.key;
            require(reordered.menu_text().results.*entry.field == entry.text &&
                normalized.menu_text().results.*entry.field == L"  A \"B\"-C...  ",
                "Session result copy depends on key order or lost normalized punctuation/outer spacing: " + field);
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            for (int type = 0; type < 5; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& value = d["menu_text"]["results"][entry.key];
                    if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                    else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
                });
            }
            for (const char* invalid : { "", "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("{outcome} 100%"); });
            plain.validate_resources(world.render, world.audio);
            std::wstring source = plain.menu_text().results.*entry.field;
            const auto& look = plain.results_menu(); const auto& style = plain.menu();
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(style.detail_font), L"{outcome} 100%");
            const mattmath::Vector2F position((1920.0f - size.x) / 2.0f, look.session_result_y);
            UiTextDropShadow widget(entry.key, source, style.detail_font, position, &world.render,
                look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source = L"Changed after construction";
            require(widget.text() == L"{outcome} 100%" && widget.bounds() == mattmath::RectangleF(position, size) &&
                widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
                "Session result widget borrowed copy, formatted braces, fitted text or retained a shadow: " + field);
            DrawList no_device; widget.draw(no_device);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
                d["menu_text"]["results"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().results.*entry.field).size() == 512,
                "Session result copy acquired a parser character cap");
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Session result missing-literal fixture lost its stand-in or missing glyph");
    }

    void check_session_result_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, float x_offset = 0.0f, float y_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? width : 8, marker ? height : 16),
                    marker ? x_offset : 0.0f, marker ? y_offset : 0.0f, 0.0f });
            }
            // Positive line spacing keeps unrelated shared-font guards valid.
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        add_font("menu_session_result_width", 1300, 16);
        add_font("menu_session_result_width_overflow", 1300, 16, 0.25f);
        add_font("menu_session_result_bottom", 8, 565);
        add_font("menu_session_result_bottom_overflow", 8, 565, 0.0f, 0.25f);
        add_font("menu_session_result_zero_width", 0, 16);
        add_font("menu_session_result_infinite_width", 8, 16, std::numeric_limits<float>::infinity());
        add_font("menu_session_result_nan_width", 8, 16, std::numeric_limits<float>::quiet_NaN());
        add_font("menu_session_result_infinite_height", 8, 16, 0.0f, std::numeric_limits<float>::infinity());
        const auto select = [](rapidjson::Document& d, const char* key, const char* font_name)
        {
            d["menu_style"]["fonts"]["detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"][key].SetString("~");
        };
        for (const auto& entry : session_result_text)
        {
            const std::string field = std::string("results.") + entry.key;
            const auto edge = variants.load([&](rapidjson::Document& d) { select(d, entry.key, "menu_session_result_width"); });
            edge.validate_resources(world.render, world.audio);
            require(edge.results_menu().box_size == mattmath::Vector2F(1300.0f, 860.0f) &&
                edge.results_menu().session_result_y == 405.0f, "Session result boundary fixture changed the shipped paper/row");
            UiTextDropShadow widget("session-result-edge", L"~", edge.menu().detail_font, { 310.0f, 405.0f },
                &world.render, edge.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            require(widget.bounds() == mattmath::RectangleF(310.0f, 405.0f, 1300.0f, 20.0f),
                "Session result width must meet both paper edges at its actual unscaled row with no shadow");
            variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, "menu_session_result_width_overflow"); }, &world);
            const auto bottom = variants.load([&](rapidjson::Document& d) { select(d, entry.key, "menu_session_result_bottom"); });
            bottom.validate_resources(world.render, world.audio);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(bottom.menu().detail_font), L"~");
            require(size == mattmath::Vector2F(8.0f, 565.0f) && 405.0f + size.y == 970.0f,
                "Session result bottom edge must use the full-height paper without an extra collision/row-fit policy");
            UiTextDropShadow tall("session-result-bottom", L"~", bottom.menu().detail_font, { 956.0f, 405.0f },
                &world.render, bottom.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            require(tall.bounds() == mattmath::RectangleF(956.0f, 405.0f, 8.0f, 565.0f),
                "Session result height boundary was fitted or measured with a shadow");
            variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, "menu_session_result_bottom_overflow"); }, &world);
            for (const char* font_name : { "menu_session_result_zero_width", "menu_session_result_infinite_width",
                "menu_session_result_nan_width", "menu_session_result_infinite_height" })
                variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, font_name); }, &world);
        }
        variants.rejects("values.on", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited"); d["menu_text"]["values"]["on"].SetString("~");
            d["menu_text"]["results"]["series_team_a_wins"].SetString(std::string(512, 'W').c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("results.proceed", [](rapidjson::Document& d)
        {
            d["menu_text"]["results"]["proceed"].SetString(std::string(512, 'W').c_str(), d.GetAllocator());
            d["menu_text"]["results"]["series_continues"].SetString(std::string(512, 'W').c_str(), d.GetAllocator());
        }, &world);
    }
    void check_series_score_text(Variants& variants, const MenuText& defaults)
    {
        require(std::size(series_score_examples) == 26, "Series-score oracle must cover all 26 legal ledgers");
        int per_target[3]{};
        for (size_t index = 0; index < std::size(series_score_examples); ++index)
        {
            const auto& example = series_score_examples[index]; ++per_target[example.target - 1];
            for (size_t prior = 0; prior < index; ++prior)
                require(example.team_a != series_score_examples[prior].team_a ||
                    example.team_b != series_score_examples[prior].team_b || example.target != series_score_examples[prior].target,
                    "Series-score oracle contains a duplicated ledger");
            const std::wstring expected = L"Wins: A " + std::to_wstring(example.team_a) + L" - B " +
                std::to_wstring(example.team_b) + L"  |  First to " + std::to_wstring(example.target);
            require(defaults.results.score_line(example.team_a, example.team_b, example.target) == expected,
                "Series-score formatter changed the independent original composition for a legal ledger");
        }
        require(per_target[0] == 3 && per_target[1] == 8 && per_target[2] == 15,
            "Series-score oracle lost a target or included an impossible joint completion");
        const auto refuses = [](auto action)
        {
            bool refused = false; try { action(); } catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid series-score domain or directly mutated template formatted silently");
        };
        for (const int target : { -1, 0, 4, (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
            refuses([&] { defaults.results.score_line(0, 0, target); });
        for (const int target : { 1, 2, 3 })
        {
            refuses([&] { defaults.results.score_line(target, target, target); });
            for (const int wins : { -1, target + 1, (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
            {
                refuses([&] { defaults.results.score_line(wins, 0, target); });
                refuses([&] { defaults.results.score_line(0, wins, target); });
            }
        }
        struct OrderedScore { const char* text; const wchar_t* expected; };
        const OrderedScore orders[]{
            { "{team_a_wins}/{team_b_wins}/{target}", L"1/0/2" },
            { "{team_a_wins}/{target}/{team_b_wins}", L"1/2/0" },
            { "{team_b_wins}/{team_a_wins}/{target}", L"0/1/2" },
            { "{team_b_wins}/{target}/{team_a_wins}", L"0/2/1" },
            { "{target}/{team_a_wins}/{team_b_wins}", L"2/1/0" },
            { "{target}/{team_b_wins}/{team_a_wins}", L"2/0/1" },
            { "{team_a_wins}%:{team_b_wins}%%:{target}", L"1%:0%%:2" }
        };
        for (const auto& order : orders)
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"]["series_score"].SetString(order.text, d.GetAllocator()); });
            require(authored.menu_text().results.score_line(1, 0, 2) == order.expected,
                "Series-score template depends on token order or interpreted a literal percent sign");
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["results"]["series_score"].SetString(
                "  {target}\xC2\xA0{team_b_wins}\xE2\x80\x94{team_a_wins}\xE2\x80\xA6  ");
        });
        require(normalized.menu_text().results.series_score == L"  {target} {team_b_wins}-{team_a_wins}...  " &&
            normalized.menu_text().results.score_line(1, 0, 2) == L"  2 0-1...  ",
            "Series-score template lost normalized punctuation, outer spaces or authored token order");
        variants.rejects("results.series_score", [](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember("series_score"); });
        for (int type = 0; type < 5; ++type)
            variants.rejects("results.series_score", [&](rapidjson::Document& d)
            {
                auto& value = d["menu_text"]["results"]["series_score"];
                if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
            });
        for (const char* invalid : { "", "   ", "No tokens", "{team_a_wins}{team_b_wins}", "{team_a_wins}{target}",
            "{team_b_wins}{target}", "{team_a_wins}{team_a_wins}{team_b_wins}{target}",
            "{team_a_wins}{team_b_wins}{team_b_wins}{target}", "{team_a_wins}{team_b_wins}{target}{target}",
            "{team_a_wins}{team_b_wins}{target}{unknown}", "{Team_a_wins}{team_b_wins}{target}",
            "{team_a_wins }{team_b_wins}{target}", "{{team_a_wins}}{team_b_wins}{target}",
            "{team_a_wins}{team_b_wins}{target", "{team_a_wins}{team_b_wins}{target}{", "{team_a_wins}{team_b_wins}{target}}",
            "{team_{team_a_wins}b_wins}{target}", "{team_a_wins}{tar{team_b_wins}get}" })
        {
            variants.rejects("results.series_score", [&](rapidjson::Document& d)
                { d["menu_text"]["results"]["series_score"].SetString(invalid, d.GetAllocator()); });
            MenuResultsText mutated = defaults.results; mutated.series_score = std::wstring(invalid, invalid + std::strlen(invalid));
            refuses([&] { mutated.score_line(1, 0, 2); });
        }
        const std::string tokens = "{team_a_wins}{team_b_wins}{target}";
        for (const char* suffix : { "\n", "\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            variants.rejects("results.series_score", [&](rapidjson::Document& d)
            { const std::string invalid = tokens + suffix; d["menu_text"]["results"]["series_score"].SetString(invalid.c_str(), d.GetAllocator()); });
        for (const wchar_t* suffix : { L"\n", L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014" })
        {
            MenuResultsText mutated = defaults.results; mutated.series_score += suffix;
            refuses([&] { mutated.score_line(1, 0, 2); });
        }
        variants.rejects("results.series_score", [&](rapidjson::Document& d)
        {
            const std::string invalid = tokens + '\0' + "tail";
            d["menu_text"]["results"]["series_score"].SetString(invalid.data(), static_cast<rapidjson::SizeType>(invalid.size()), d.GetAllocator());
        });
        MenuResultsText nul = defaults.results; nul.series_score.push_back(L'\0');
        refuses([&] { nul.score_line(1, 0, 2); });
        const auto long_copy = variants.load([&](rapidjson::Document& d)
        { const std::string text = std::string(512, 'W') + tokens;
            d["menu_text"]["results"]["series_score"].SetString(text.c_str(), d.GetAllocator()); });
        require(long_copy.menu_text().results.score_line(1, 0, 1) == std::wstring(512, L'W') + L"101",
            "Series-score template acquired a character cap or altered a long supported literal prefix");
    }

    void check_series_score_resources(Variants& variants, World& world, const Presentation& defaults)
    {
        const auto add_font = [&](const std::string& name, int marker_width, int marker_height,
            int one_width = 8, float marker_x = 0.0f, float marker_y = 0.0f, char32_t omitted = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == omitted) { continue; }
                glyphs.push_back({ character, mattmath::RectangleI(0, 0,
                    character == U'~' ? marker_width : character == U'1' ? one_width : 8,
                    character == U'~' ? marker_height : 16),
                    character == U'~' ? marker_x : 0.0f, character == U'~' ? marker_y : 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_score_width", 1276, 16);
        add_font("menu_score_width_overflow", 1276, 16, 8, 0.25f);
        add_font("menu_score_bottom", 8, 630);
        add_font("menu_score_bottom_overflow", 8, 630, 8, 0.0f, 0.25f);
        add_font("menu_score_proportional", 1212, 16, 40);
        add_font("menu_score_proportional_overflow", 1212, 16, 40, 0.25f);
        add_font("menu_score_infinite_width", 8, 16, 8, std::numeric_limits<float>::infinity());
        add_font("menu_score_nan_width", 8, 16, 8, std::numeric_limits<float>::quiet_NaN());
        add_font("menu_score_infinite_height", 8, 16, 8, 0.0f, std::numeric_limits<float>::infinity());
        const auto select = [](rapidjson::Document& d, const char* font_name)
        {
            d["menu_style"]["fonts"]["detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"]["series_score"].SetString("~{team_a_wins}{team_b_wins}{target}");
        };
        const auto edge = variants.load([&](rapidjson::Document& d) { select(d, "menu_score_width"); });
        edge.validate_resources(world.render, world.audio);
        require(edge.results_menu().box_size == mattmath::Vector2F(1300.0f, 860.0f) && edge.results_menu().session_score_y == 340.0f,
            "Series-score boundary fixture changed the full-height Results paper or actual row");
        std::wstring source = edge.menu_text().results.score_line(1, 0, 1);
        require(source == L"~101", "Series-score boundary must format the independently expected literal");
        UiTextDropShadow widget("series-score-edge", source, edge.menu().detail_font, { 310.0f, 340.0f }, &world.render,
            edge.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
        source = L"Changed after construction";
        require(widget.bounds() == mattmath::RectangleF(310.0f, 340.0f, 1300.0f, 20.0f) && widget.text() == L"~101" &&
            widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
            "Series-score widget lost unscaled centered copy or gained a shadow at the exact left/right paper edges");
        DrawList no_device; widget.draw(no_device);
        variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, "menu_score_width_overflow"); }, &world);
        const auto bottom = variants.load([&](rapidjson::Document& d) { select(d, "menu_score_bottom"); });
        bottom.validate_resources(world.render, world.audio);
        UiTextDropShadow tall("series-score-bottom", L"~101", bottom.menu().detail_font, { 944.0f, 340.0f }, &world.render,
            bottom.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
        require(tall.bounds() == mattmath::RectangleF(944.0f, 340.0f, 32.0f, 630.0f) && tall.bounds().bottom() == 970.0f,
            "Series-score height must use full paper containment without a new row-fit or overlap restriction");
        variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, "menu_score_bottom_overflow"); }, &world);
        const auto proportional = variants.load([&](rapidjson::Document& d) { select(d, "menu_score_proportional"); });
        proportional.validate_resources(world.render, world.audio);
        const auto handle = world.render.resolve_sprite_font(proportional.menu().detail_font);
        float maximum = 0.0f; int widest_count = 0;
        for (const auto& example : series_score_examples)
        {
            const std::wstring literal = L"~" + std::to_wstring(example.team_a) + std::to_wstring(example.team_b) + std::to_wstring(example.target);
            const int ones = (example.team_a == 1 ? 1 : 0) + (example.team_b == 1 ? 1 : 0) + (example.target == 1 ? 1 : 0);
            const auto size = world.render.measure_text(handle, literal);
            require(proportional.menu_text().results.score_line(example.team_a, example.team_b, example.target) == literal &&
                size == mattmath::Vector2F(1212.0f + 40.0f * static_cast<float>(ones) + 8.0f * static_cast<float>(3 - ones), 20.0f),
                "All-ledger proportional-font oracle lost a real digit width or composed score");
            if (size.x > maximum) { maximum = size.x; widest_count = 1; }
            else if (size.x == maximum) { ++widest_count; }
        }
        require(maximum == 1300.0f && widest_count == 4 &&
            world.render.measure_text(handle, L"~112") == mattmath::Vector2F(1300.0f, 20.0f) &&
            world.render.measure_text(handle, L"~323") == mattmath::Vector2F(1236.0f, 20.0f),
            "Series-score resource fixture must require smaller-score ledgers, not assume digit3 or the terminal ledger is widest");
        variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, "menu_score_proportional_overflow"); }, &world);
        for (const char* font_name : { "menu_score_infinite_width", "menu_score_nan_width", "menu_score_infinite_height" })
            variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, font_name); }, &world);
        variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, "menu_text_limited"); }, &world);
        for (int digit = 0; digit <= 3; ++digit)
        {
            const auto missing = static_cast<char32_t>(U'0' + digit);
            const std::string name = "menu_score_missing" + std::to_string(digit);
            add_font(name, 8, 16, 8, 0.0f, 0.0f, missing);
            const auto font = world.render.font(world.render.resolve_sprite_font(name));
            const std::wstring literal = defaults.menu_text().results.score_line(0, 0, digit < 2 ? 1 : digit);
            require(font->contains(U'?') && !font->contains(missing) && literal.find(static_cast<wchar_t>(missing)) != std::wstring::npos &&
                font->measure(literal).x > 0.0f, "Generated score digit must be genuinely absent despite a functioning stand-in");
            // The preserved countdown0..15 check already owns these shared-font
            // missing digits before the new score's exhaustive resource loop.
            variants.rejects("display_trial_countdown", [&](rapidjson::Document& d)
                { d["menu_style"]["fonts"]["detail"].SetString(name.c_str(), d.GetAllocator()); }, &world);
        }
        variants.rejects("results.series_score", [](rapidjson::Document& d)
        {
            const std::string text = std::string(512, 'W') + "{team_a_wins}{team_b_wins}{target}";
            d["menu_text"]["results"]["series_score"].SetString(text.c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("results.series_continues", [](rapidjson::Document& d)
        {
            const std::string text = std::string(512, 'W');
            d["menu_text"]["results"]["series_continues"].SetString(text.c_str(), d.GetAllocator());
            d["menu_text"]["results"]["series_score"].SetString((text + "{team_a_wins}{team_b_wins}{target}").c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("values.on", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited"); d["menu_text"]["values"]["on"].SetString("~");
            d["menu_text"]["results"]["series_score"].SetString("~{team_a_wins}{team_b_wins}{target}");
        }, &world);
        Presentation mutated = defaults;
        const_cast<MenuText&>(mutated.menu_text()).results.series_score = L"{team_{team_a_wins}b_wins}{target}";
        bool contextual = false;
        try { mutated.validate_resources(world.render, world.audio); }
        catch (const std::runtime_error& error)
        { contextual = std::string(error.what()).find("menu_text.results.series_score") != std::string::npos; }
        require(contextual, "Directly manufactured score tokens escaped resource validation or lost the named score role");
    }
    void check_round_heading_text(Variants& variants, const MenuText& defaults)
    {
        struct RoundExample { int number; const wchar_t* expected; };
        const RoundExample examples[]{
            { 1, L"Round 1" }, { 9, L"Round 9" }, { 10, L"Round 10" }, { 99, L"Round 99" },
            { 100, L"Round 100" }, { 999, L"Round 999" }, { 1000, L"Round 1000" }, { 10000, L"Round 10000" },
            { 100000, L"Round 100000" }, { 1000000, L"Round 1000000" }, { 10000000, L"Round 10000000" },
            { 100000000, L"Round 100000000" }, { 1000000000, L"Round 1000000000" },
            { 1999999999, L"Round 1999999999" }, { 2122222222, L"Round 2122222222" },
            { (std::numeric_limits<int>::max)(), L"Round 2147483647" }
        };
        for (const auto& example : examples)
            require(defaults.results.round_title(example.number) == example.expected,
                "Numbered Results heading changed its original decimal composition or positive-int domain");
        const auto refuses = [](auto action)
        {
            bool refused = false; try { action(); } catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid round or directly mutated round-heading template formatted silently");
        };
        for (const int number : { 0, -1, (std::numeric_limits<int>::min)() })
            refuses([&] { defaults.results.round_title(number); });
        const auto reordered = variants.load([](rapidjson::Document& d)
            { d["menu_text"]["results"]["round_heading"].SetString("{round}% = Round"); });
        require(reordered.menu_text().results.round_title(12345) == L"12345% = Round",
            "Numbered Results heading lost authored token position or interpreted a literal percent sign");
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["results"]["round_heading"].SetString("  {round}\xC2\xA0\xE2\x80\x9C" "round\xE2\x80\x9D\xE2\x80\x94...  ");
        });
        require(normalized.menu_text().results.round_heading == L"  {round} \"round\"-...  " &&
            normalized.menu_text().results.round_title(10) == L"  10 \"round\"-...  ",
            "Numbered Results heading lost normalized punctuation, token position or authored outer spaces");
        variants.rejects("results.round_heading", [](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember("round_heading"); });
        for (int type = 0; type < 5; ++type)
            variants.rejects("results.round_heading", [&](rapidjson::Document& d)
            {
                auto& value = d["menu_text"]["results"]["round_heading"];
                if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
            });
        for (const char* invalid : { "", "   ", "No token", "{Round}", "{round }", "{round}{round}", "{{round}}",
            "{round}{unknown}", "{round", "round}", "{round}{", "{round}}", "{ro{round}und}" })
        {
            variants.rejects("results.round_heading", [&](rapidjson::Document& d)
                { d["menu_text"]["results"]["round_heading"].SetString(invalid, d.GetAllocator()); });
            MenuResultsText mutated = defaults.results; mutated.round_heading = std::wstring(invalid, invalid + std::strlen(invalid));
            refuses([&] { mutated.round_title(1); });
        }
        for (const char* suffix : { "\n", "\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            variants.rejects("results.round_heading", [&](rapidjson::Document& d)
            { const std::string invalid = std::string("{round}") + suffix;
                d["menu_text"]["results"]["round_heading"].SetString(invalid.c_str(), d.GetAllocator()); });
        for (const wchar_t* suffix : { L"\n", L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014" })
        {
            MenuResultsText mutated = defaults.results; mutated.round_heading += suffix;
            refuses([&] { mutated.round_title(1); });
        }
        variants.rejects("results.round_heading", [](rapidjson::Document& d)
        {
            const std::string invalid("{round}\0tail", 12);
            d["menu_text"]["results"]["round_heading"].SetString(invalid.data(), static_cast<rapidjson::SizeType>(invalid.size()), d.GetAllocator());
        });
        MenuResultsText nul = defaults.results; nul.round_heading.push_back(L'\0');
        refuses([&] { nul.round_title(1); });
        const auto long_copy = variants.load([](rapidjson::Document& d)
        { const std::string text = std::string(512, 'W') + "{round}";
            d["menu_text"]["results"]["round_heading"].SetString(text.c_str(), d.GetAllocator()); });
        require(long_copy.menu_text().results.round_title(123) == std::wstring(512, L'W') + L"123",
            "Numbered Results heading acquired a character cap or lost its long supported literal prefix");
    }
    void check_round_heading_resources(Variants& variants, World& world, const Presentation& defaults)
    {
        const auto add_font = [&](const std::string& name, float line_spacing, auto alter, char32_t omitted = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == omitted) { continue; }
                Glyph glyph{ character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f };
                alter(glyph); glyphs.push_back(glyph);
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        const auto wide = [&](const char* name, char32_t digit, float marker_offset)
        {
            add_font(name, 20.0f, [&](Glyph& glyph)
            {
                if (glyph.character == digit) { glyph.subrect.width = 16; }
                if (glyph.character == U'~') { glyph.subrect.width = 1148; glyph.x_offset = marker_offset; }
            });
        };
        wide("menu_round_wide9", U'9', 0.0f); wide("menu_round_wide9_overflow", U'9', 0.25f);
        wide("menu_round_wide2", U'2', 0.0f); wide("menu_round_wide2_overflow", U'2', 0.25f);
        add_font("menu_round_bottom", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'~') { glyph.subrect.height = 845; } });
        add_font("menu_round_bottom_overflow", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'~') { glyph.subrect.height = 845; glyph.y_offset = 0.25f; } });
        const auto signed_metrics = [&](const char* name, float marker_offset)
        {
            add_font(name, 20.0f, [&](Glyph& glyph)
            {
                if (glyph.character == U'9') { glyph.x_offset = -2.25f; glyph.x_advance = 1.5f; }
                if (glyph.character == U'8') { glyph.x_offset = 0.25f; glyph.x_advance = -1.5f; }
                if (glyph.character == U'~') { glyph.subrect.width = 1219; glyph.x_offset = marker_offset; }
            });
        };
        signed_metrics("menu_round_signed", 1.0f); signed_metrics("menu_round_signed_overflow", 1.25f);
        add_font("menu_round_zero_width", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'1') { glyph.subrect.width = 0; glyph.x_advance = -2.0f; } });
        add_font("menu_round_zero_height", 0.0f, [](Glyph& glyph)
            { if (glyph.character == U'1') { glyph.subrect.height = 0; } });
        add_font("menu_round_zero_both", 0.0f, [](Glyph& glyph)
            { if (glyph.character == U'1') { glyph.subrect.width = 0; glyph.subrect.height = 0; glyph.x_advance = -2.0f; } });
        add_font("menu_round_zero_spacing", 0.0f, [](Glyph&) {});
        add_font("menu_round_negative_spacing", -4.0f, [](Glyph&) {});
        add_font("menu_round_negative_infinite_spacing", -std::numeric_limits<float>::infinity(), [](Glyph&) {});
        add_font("menu_round_infinite_bearing", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'9') { glyph.x_offset = std::numeric_limits<float>::infinity(); } });
        add_font("menu_round_nan_bearing", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'9') { glyph.x_offset = std::numeric_limits<float>::quiet_NaN(); } });
        add_font("menu_round_infinite_height", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'9') { glyph.y_offset = std::numeric_limits<float>::infinity(); } });
        add_font("menu_round_infinite_advance", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'9') { glyph.x_advance = std::numeric_limits<float>::infinity(); } });
        add_font("menu_round_overflowing_pen", 20.0f, [](Glyph& glyph)
            { if (glyph.character == U'9') { glyph.x_advance = (std::numeric_limits<float>::max)(); } });
        const auto select = [](rapidjson::Document& d, const char* font_name, const char* text = "~{round}")
        {
            d["menu_style"]["fonts"]["heading"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"]["round_heading"].SetString(text, d.GetAllocator());
        };
        const auto wide_nines = variants.load([&](rapidjson::Document& d) { select(d, "menu_round_wide9"); });
        wide_nines.validate_resources(world.render, world.audio);
        const auto nines_font = world.render.resolve_sprite_font(wide_nines.menu().heading_font);
        require(world.render.measure_text(nines_font, L"~1999999999") == mattmath::Vector2F(1300.0f, 20.0f) &&
            world.render.measure_text(nines_font, L"~2147483647") == mattmath::Vector2F(1228.0f, 20.0f),
            "The positive-int heading maximum cannot be inferred from the numeric INT_MAX string");
        require(wide_nines.results_menu().box_size == mattmath::Vector2F(1300.0f, 860.0f) &&
            wide_nines.results_menu().heading_y == 125.0f, "Numbered heading boundary changed the actual paper or row");
        std::wstring source = wide_nines.menu_text().results.round_title(1999999999);
        UiTextDropShadow widget("round-heading-edge", source, wide_nines.menu().heading_font, { 310.0f, 125.0f },
            &world.render, wide_nines.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
        source.clear();
        require(widget.text() == L"~1999999999" && widget.bounds() == mattmath::RectangleF(310.0f, 125.0f, 1300.0f, 20.0f) &&
            widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
            "Numbered Results heading lost owned captured copy, unscaled paper contact or its zero-offset shadow");
        DrawList no_device; widget.draw(no_device);
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_wide9_overflow"); }, &world);
        const auto wide_twos = variants.load([&](rapidjson::Document& d) { select(d, "menu_round_wide2"); });
        wide_twos.validate_resources(world.render, world.audio);
        const auto twos_font = world.render.resolve_sprite_font(wide_twos.menu().heading_font);
        require(world.render.measure_text(twos_font, L"~2122222222") == mattmath::Vector2F(1300.0f, 20.0f) &&
            world.render.measure_text(twos_font, L"~2222222222") == mattmath::Vector2F(1308.0f, 20.0f),
            "The numbered-heading bound must exclude the genuinely wider illegal ten-digit string");
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_wide2_overflow"); }, &world);
        const auto bottom = variants.load([&](rapidjson::Document& d) { select(d, "menu_round_bottom"); });
        bottom.validate_resources(world.render, world.audio);
        UiTextDropShadow tall("round-heading-bottom", L"~1", bottom.menu().heading_font, { 952.0f, 125.0f },
            &world.render, bottom.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
        require(tall.bounds() == mattmath::RectangleF(952.0f, 125.0f, 16.0f, 845.0f) && tall.bounds().bottom() == 970.0f,
            "Numbered heading must meet the full paper bottom without a new collision or row-height restriction");
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_bottom_overflow"); }, &world);
        const auto signed_look = variants.load([&](rapidjson::Document& d) { select(d, "menu_round_signed", "{round}~"); });
        signed_look.validate_resources(world.render, world.audio);
        const auto signed_font = world.render.resolve_sprite_font(signed_look.menu().heading_font);
        require(world.render.measure_text(signed_font, L"9~") == mattmath::Vector2F(1229.5f, 20.0f) &&
            world.render.measure_text(signed_font, L"99~") == mattmath::Vector2F(1236.75f, 20.0f) &&
            world.render.measure_text(signed_font, L"1999999999~") == mattmath::Vector2F(1293.25f, 20.0f) &&
            world.render.measure_text(signed_font, L"2147483647~") == mattmath::Vector2F(1298.75f, 20.0f) &&
            world.render.measure_text(signed_font, L"2000000000~") == mattmath::Vector2F(1300.0f, 20.0f),
            "Signed fractional bearings/advances must preserve the clamp and carry terminal pen, rather than ink extent, into the suffix");
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_signed_overflow", "{round}~"); }, &world);
        const auto zero_width_font = world.render.resolve_sprite_font("menu_round_zero_width");
        const auto zero_height_font = world.render.resolve_sprite_font("menu_round_zero_height");
        require(world.render.measure_text(zero_width_font, L"1") == mattmath::Vector2F(0.0f, 20.0f) &&
            world.render.measure_text(zero_width_font, L"2") == mattmath::Vector2F(8.0f, 20.0f) &&
            world.render.measure_text(zero_height_font, L"1") == mattmath::Vector2F(8.0f, 0.0f) &&
            world.render.measure_text(zero_height_font, L"2") == mattmath::Vector2F(8.0f, 16.0f),
            "Minimum-size witnesses must contain a genuinely zero path as well as a positive path");
        for (const char* font_name : { "menu_round_zero_width", "menu_round_zero_height", "menu_round_zero_both" })
        {
            variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, font_name, "{round}"); }, &world);
            for (const char* text : { "{round}~", "~{round}" })
            {
                const auto rescued = variants.load([&](rapidjson::Document& d) { select(d, font_name, text); });
                rescued.validate_resources(world.render, world.audio);
                const auto literal = rescued.menu_text().results.round_title(1);
                require(world.render.measure_text(world.render.resolve_sprite_font(rescued.menu().heading_font), literal).x > 0.0f &&
                    world.render.measure_text(world.render.resolve_sprite_font(rescued.menu().heading_font), literal).y > 0.0f,
                    "A visible prefix or suffix must rescue the zero-size digit path without borrowing unrelated maxima");
            }
        }
        for (const char* font_name : { "menu_round_zero_spacing", "menu_round_negative_spacing" })
        {
            const auto visible = variants.load([&](rapidjson::Document& d) { select(d, font_name, "{round}"); });
            visible.validate_resources(world.render, world.audio);
            require(world.render.measure_text(world.render.resolve_sprite_font(visible.menu().heading_font), L"1") == mattmath::Vector2F(8.0f, 16.0f),
                "A blanket positive line-spacing requirement must not reject a positive measured single-line heading");
        }
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_round_negative_infinite_spacing"), L"1") == mattmath::Vector2F(8.0f, 16.0f),
            "Positive glyph height must genuinely mask negative infinite line spacing in the raw Font measurement witness");
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_negative_infinite_spacing", "{round}"); }, &world);
        for (const char* font_name : { "menu_round_infinite_bearing", "menu_round_nan_bearing", "menu_round_infinite_height", "menu_round_infinite_advance" })
            variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, font_name, "{round}"); }, &world);
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_round_overflowing_pen"), L"9") == mattmath::Vector2F(8.0f, 20.0f),
            "The overflowing-pen fixture must leave the single-glyph measured ink finite before the unsafe carried advance");
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_round_overflowing_pen", "{round}"); }, &world);
        variants.rejects("results.round_heading", [&](rapidjson::Document& d) { select(d, "menu_text_limited"); }, &world);
        for (int digit = 0; digit <= 9; ++digit)
        {
            const auto omitted = static_cast<char32_t>(U'0' + digit);
            const std::string name = "menu_round_missing" + std::to_string(digit);
            add_font(name, 20.0f, [](Glyph&) {}, omitted);
            const auto font = world.render.font(world.render.resolve_sprite_font(name));
            require(font->contains(U'?') && !font->contains(omitted) && font->measure(std::to_wstring(digit == 0 ? 10 : digit)).x > 0.0f,
                "Round-heading digit fixture must retain a working stand-in while omitting an actual generated decimal");
            // Pause's existing P1-P4 title check owns those four missing digits.
            const char* owner = digit >= 1 && digit <= 4 ? "connection.pause_player_title" : "results.round_heading";
            variants.rejects(owner, [&](rapidjson::Document& d) { select(d, name.c_str(), "{round}"); }, &world);
        }
        variants.rejects("results.round_heading", [](rapidjson::Document& d)
        { const std::string text = std::string(512, 'W') + "{round}";
            d["menu_text"]["results"]["round_heading"].SetString(text.c_str(), d.GetAllocator()); }, &world);
        variants.rejects("headings.home", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited"); d["menu_text"]["headings"]["home"].SetString("~");
            d["menu_text"]["results"]["round_heading"].SetString("~{round}");
        }, &world);
        variants.rejects("results.series_score", [](rapidjson::Document& d)
        {
            const std::string long_copy(512, 'W');
            d["menu_text"]["results"]["series_score"].SetString((long_copy + "{team_a_wins}{team_b_wins}{target}").c_str(), d.GetAllocator());
            d["menu_text"]["results"]["round_heading"].SetString((long_copy + "{round}").c_str(), d.GetAllocator());
        }, &world);
        Presentation mutated = defaults;
        const_cast<MenuText&>(mutated.menu_text()).results.round_heading = L"{ro{round}und}";
        bool contextual = false;
        try { mutated.validate_resources(world.render, world.audio); }
        catch (const std::runtime_error& error)
        { contextual = std::string(error.what()).find("menu_text.results.round_heading") != std::string::npos; }
        require(contextual, "Directly manufactured round token escaped resource validation or lost its named heading role");
    }
    void check_competitive_winner_text(Variants& variants, World& world)
    {
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& group = d["menu_text"]["results"]; rapidjson::Value reverse(rapidjson::kObjectType);
            auto member = group.MemberEnd();
            while (member != group.MemberBegin())
            { --member; rapidjson::Value key; key.CopyFrom(member->name, d.GetAllocator());
                rapidjson::Value value; value.CopyFrom(member->value, d.GetAllocator());
                reverse.AddMember(key, value, d.GetAllocator()); }
            group = std::move(reverse);
        });
        const auto normalized = variants.load([](rapidjson::Document& d)
        { for (const auto& entry : competitive_winner_text)
            d["menu_text"]["results"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  "); });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : competitive_winner_text)
        {
            const std::string field = std::string("results.") + entry.key;
            require(reordered.menu_text().results.*entry.field == entry.text &&
                normalized.menu_text().results.*entry.field == L"  A \"B\"-C...  ",
                "Competitive winner copy depends on key order or lost normalization/outer spacing: " + field);
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            for (int type = 0; type < 5; ++type)
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& value = d["menu_text"]["results"][entry.key];
                    if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                    else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
                });
            for (const char* invalid : { "", "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("{team} 100%!"); });
            plain.validate_resources(world.render, world.audio);
            std::wstring source = plain.menu_text().results.*entry.field;
            const auto& look = plain.results_menu();
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(plain.menu().item_font), L"{team} 100%!");
            const mattmath::Vector2F position((1920.0f - size.x) / 2.0f, look.winner_y);
            UiTextDropShadow widget(entry.key, source, plain.menu().item_font, position, &world.render,
                look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source.clear();
            require(widget.text() == L"{team} 100%!" && widget.bounds() == mattmath::RectangleF(position, size) &&
                widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
                "Competitive winner widget interpreted literal braces, borrowed copy, fitted text or retained a shadow: " + field);
            DrawList no_device; widget.draw(no_device);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["results"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().results.*entry.field).size() == 512, "Competitive winner copy acquired a parser character cap");
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Competitive winner missing-literal fixture lost its stand-in or missing glyph");
    }

    void check_competitive_winner_selection(const MenuText& defaults)
    {
        MenuResultsText custom = defaults.results;
        custom.competitive_team_a_wins = L"A {wins}%!";
        custom.competitive_team_b_wins = L"B {wins}%!";
        custom.competitive_draw = L"Equal {paint}%!";
        const auto check = [&](LevelEndInfo result, const std::wstring& expected_default, const std::wstring& expected_custom)
        {
            result.player_areas.front() = 13.25f; result.player_areas.back() = -3.5f;
            result.team_colours = TeamColour(Colour::red, Colour::blue);
            const LevelEndInfo before = result;
            std::wstring selected = result.winning_team_string(custom);
            require(selected == expected_custom && result.winning_team_string(defaults.results) == expected_default,
                "Competitive/solo winner selection changed the independent original branch or its plain literal copy");
            selected.clear();
            require(result.winning_team_string(custom) == expected_custom &&
                std::memcmp(&result.team_a_area, &before.team_a_area, sizeof(result.team_a_area)) == 0 &&
                std::memcmp(&result.team_b_area, &before.team_b_area, sizeof(result.team_b_area)) == 0 &&
                std::memcmp(&result.total_area, &before.total_area, sizeof(result.total_area)) == 0 &&
                result.player_areas == before.player_areas && result.team_a_played == before.team_a_played &&
                result.team_b_played == before.team_b_played && result.team_colours.team_a == before.team_colours.team_a &&
                result.team_colours.team_b == before.team_colours.team_b,
                "Winner selection borrowed its returned string or changed area bits, participation, per-player credit or palette");
        };
        const float infinity = std::numeric_limits<float>::infinity();
        const float nan = std::numeric_limits<float>::quiet_NaN();
        struct CompetitiveExample { float team_a; float team_b; const wchar_t* expected; const wchar_t* expected_custom; };
        const CompetitiveExample competitive[]{
            { 2.0f, 1.0f, L"Team A Wins!", L"A {wins}%!" }, { 1.0f, 2.0f, L"Team B Wins!", L"B {wins}%!" },
            { 0.0f, 0.0f, L"Draw!", L"Equal {paint}%!" }, { 2.0f, 2.0f, L"Draw!", L"Equal {paint}%!" },
            { -1.0f, -2.0f, L"Team A Wins!", L"A {wins}%!" }, { -2.0f, -1.0f, L"Team B Wins!", L"B {wins}%!" },
            { -1.0f, -1.0f, L"Draw!", L"Equal {paint}%!" }, { 0.0f, -0.0f, L"Draw!", L"Equal {paint}%!" },
            { infinity, 1.0f, L"Team A Wins!", L"A {wins}%!" }, { 1.0f, infinity, L"Team B Wins!", L"B {wins}%!" },
            { infinity, infinity, L"Draw!", L"Equal {paint}%!" }, { -infinity, 1.0f, L"Team B Wins!", L"B {wins}%!" },
            { 1.0f, -infinity, L"Team A Wins!", L"A {wins}%!" }, { -infinity, -infinity, L"Draw!", L"Equal {paint}%!" },
            { nan, 1.0f, L"Draw!", L"Equal {paint}%!" }, { 1.0f, nan, L"Draw!", L"Equal {paint}%!" },
            { nan, nan, L"Draw!", L"Equal {paint}%!" }
        };
        for (const bool both_played : { false, true })
            for (const auto& example : competitive)
            {
                LevelEndInfo result; result.team_a_area = example.team_a; result.team_b_area = example.team_b;
                result.team_a_played = both_played; result.team_b_played = both_played; result.total_area = -7.0f;
                check(result, example.expected, example.expected_custom);
            }
        struct SoloExample { float team_a; float team_b; float total; const wchar_t* expected; };
        const SoloExample solo[]{
            { 100.0f, 25.0f, 1000.0f, L"12.5% Painted" }, { 68.499f, 0.0f, 100.0f, L"68.4% Painted" },
            { -25.0f, 0.0f, 100.0f, L"-25.0% Painted" }, { 200.0f, 0.0f, 100.0f, L"200.0% Painted" },
            { 0.0f, 0.0f, 100.0f, L"0.0% Painted" }, { 100.0f, 200.0f, 0.0f, L"0.0% Painted" },
            { 100.0f, 200.0f, -1.0f, L"0.0% Painted" }, { 100.0f, 25.0f, infinity, L"0.0% Painted" }
        };
        for (const bool team_a_played : { false, true })
        {
            for (const auto& example : solo)
            {
                LevelEndInfo result; result.team_a_area = example.team_a; result.team_b_area = example.team_b; result.total_area = example.total;
                result.team_a_played = team_a_played; result.team_b_played = !team_a_played;
                check(result, example.expected, example.expected);
                require(result.winning_team_string(MenuResultsText{}) == example.expected,
                    "Solo coverage must return before inspecting competitive copy, even when that record is empty");
            }
            for (const auto& example : { SoloExample{ nan, 0.0f, 100.0f, nullptr }, SoloExample{ infinity, 0.0f, 100.0f, nullptr },
                SoloExample{ -infinity, 0.0f, 100.0f, nullptr }, SoloExample{ infinity, -infinity, 100.0f, nullptr },
                SoloExample{ 1.0f, 2.0f, nan, nullptr }, SoloExample{ infinity, 0.0f, infinity, nullptr }, SoloExample{ nan, infinity, -1.0f, nullptr } })
            {
                LevelEndInfo result; result.team_a_area = example.team_a; result.team_b_area = example.team_b; result.total_area = example.total;
                result.team_a_played = team_a_played; result.team_b_played = !team_a_played;
                // Preserve the old FLOAT operation order and the existing numeric
                // formatter's platform spelling of NaN/Inf; this batch owns copy.
                const float ratio = example.total <= 0.0f ? 0.0f : (example.team_a + example.team_b) / example.total;
                const std::wstring expected = percentage_text(100.0f * ratio) + L" Painted";
                check(result, expected, expected);
                require(result.winning_team_string(MenuResultsText{}) == expected, "Nonfinite solo coverage was sanitized or read competitive copy");
            }
        }
        require(custom.competitive_team_a_wins == L"A {wins}%!" && custom.competitive_team_b_wins == L"B {wins}%!" &&
            custom.competitive_draw == L"Equal {paint}%!", "Winner selection changed its supplied competitive text record");
    }

    void check_competitive_winner_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int marker_width, int marker_height, float marker_x = 0.0f, float marker_y = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? marker_width : 8, character == U'~' ? marker_height : 16),
                    character == U'~' ? marker_x : 0.0f, character == U'~' ? marker_y : 0.0f, 0.0f });
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?'); world.render.add_font(name, std::move(font));
        };
        add_font("menu_competitive_width", 1300, 16); add_font("menu_competitive_width_overflow", 1300, 16, 0.25f);
        add_font("menu_competitive_bottom", 8, 715); add_font("menu_competitive_bottom_overflow", 8, 715, 0.0f, 0.25f);
        add_font("menu_competitive_zero_width", 0, 16);
        add_font("menu_competitive_infinite_width", 8, 16, std::numeric_limits<float>::infinity());
        add_font("menu_competitive_nan_width", 8, 16, std::numeric_limits<float>::quiet_NaN());
        add_font("menu_competitive_infinite_height", 8, 16, 0.0f, std::numeric_limits<float>::infinity());
        const auto select = [](rapidjson::Document& d, const char* key, const char* font_name)
        {
            d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"][key].SetString("~");
        };
        for (const auto& entry : competitive_winner_text)
        {
            const std::string field = std::string("results.") + entry.key;
            const auto edge = variants.load([&](rapidjson::Document& d) { select(d, entry.key, "menu_competitive_width"); });
            edge.validate_resources(world.render, world.audio);
            require(edge.results_menu().box_size == mattmath::Vector2F(1300.0f, 860.0f) && edge.results_menu().winner_y == 255.0f,
                "Competitive winner boundary fixture changed the actual paper or row");
            UiTextDropShadow widget("competitive-edge", L"~", edge.menu().item_font, { 310.0f, 255.0f }, &world.render,
                edge.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            require(widget.bounds() == mattmath::RectangleF(310.0f, 255.0f, 1300.0f, 20.0f),
                "Competitive winner must meet both full-paper horizontal edges without fitting or an ornament inset");
            variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, "menu_competitive_width_overflow"); }, &world);
            const auto bottom = variants.load([&](rapidjson::Document& d) { select(d, entry.key, "menu_competitive_bottom"); });
            bottom.validate_resources(world.render, world.audio);
            require(world.render.measure_text(world.render.resolve_sprite_font(bottom.menu().item_font), L"~") == mattmath::Vector2F(8.0f, 715.0f) &&
                255.0f + 715.0f == 970.0f, "Competitive winner height must meet full paper without a new collision or row-fit restriction");
            variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, "menu_competitive_bottom_overflow"); }, &world);
            for (const char* font_name : { "menu_competitive_zero_width", "menu_competitive_infinite_width", "menu_competitive_nan_width", "menu_competitive_infinite_height" })
                variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry.key, font_name); }, &world);
        }
        variants.rejects("items.play", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited"); d["menu_text"]["items"]["play"].SetString("~");
            d["menu_text"]["results"]["competitive_draw"].SetString("~");
        }, &world);
        variants.rejects("results.series_score", [](rapidjson::Document& d)
        {
            const std::string text(512, 'W');
            d["menu_text"]["results"]["series_score"].SetString((text + "{team_a_wins}{team_b_wins}{target}").c_str(), d.GetAllocator());
            d["menu_text"]["results"]["competitive_draw"].SetString(text.c_str(), d.GetAllocator());
        }, &world);
    }
    void check_contribution_team_text(Variants& variants, World& world)
    {
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& group = d["menu_text"]["results"]; rapidjson::Value reverse(rapidjson::kObjectType);
            auto member = group.MemberEnd();
            while (member != group.MemberBegin())
            { --member; rapidjson::Value key; key.CopyFrom(member->name, d.GetAllocator());
                rapidjson::Value value; value.CopyFrom(member->value, d.GetAllocator());
                reverse.AddMember(key, value, d.GetAllocator()); }
            group = std::move(reverse);
        });
        const auto normalized = variants.load([](rapidjson::Document& d)
        { for (const auto& entry : contribution_team_text)
            d["menu_text"]["results"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  "); });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : contribution_team_text)
        {
            const std::string field = std::string("results.") + entry.key;
            const std::string resource_role = std::string("labels.roster_row.Results.menu_text.results.") + entry.key;
            require(reordered.menu_text().results.*entry.field == entry.text &&
                normalized.menu_text().results.*entry.field == L"  A \"B\"-C...  ",
                "Contribution separator depends on key order or lost normalized punctuation/outer spaces: " + field);
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            for (int type = 0; type < 5; ++type)
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& value = d["menu_text"]["results"][entry.key];
                    if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                    else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
                });
            for (const char* invalid : { "", "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("  {side} %  "); });
            plain.validate_resources(world.render, world.audio);
            require(plain.menu_text().results.*entry.field == L"  {side} %  ", "Contribution separator was trimmed or interpreted as a template");
            std::wstring source = L"P1 - Pad 1" + (plain.menu_text().results.*entry.field) + L"12.5%";
            const auto& look = plain.results_menu();
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(look.contribution_font), L"P1 - Pad 1  {side} %  12.5%");
            const auto position = look.contribution_position + mattmath::Vector2F(0.0f, look.contribution_spacing);
            UiTextDropShadow widget(entry.key, source, look.contribution_font, position, &world.render,
                look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source.clear();
            require(widget.text() == L"P1 - Pad 1  {side} %  12.5%" && widget.bounds() == mattmath::RectangleF(position, size) &&
                widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
                "Contribution row lost full literal composition, owned text, its actual font or explicit zero shadow");
            DrawList no_device; widget.draw(no_device);
            variants.rejects(resource_role, [&](rapidjson::Document& d)
            {
                d["results_menu"]["contribution_font"].SetString("menu_text_limited"); d["menu_text"]["results"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().results.*entry.field).size() == 512, "Contribution separator acquired a parser character cap");
            variants.rejects(resource_role, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "Contribution separator missing-literal fixture lost its functioning stand-in");
    }

    void check_contribution_team_roster(World& world, const Presentation& defaults)
    {
        MenuLevelSettings settings; settings.set_player_count(2); settings.begin_input_claims(3);
        require(settings.claim_input_slot(4), "Sparse Results roster must claim Pad4 and keyboard/mouse");
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
        LevelEndInfo result; result.total_area = 100.0f; result.player_areas[3] = 12.5f; result.player_areas[4] = 87.5f;
        const auto original_roster = settings.player_settings(); const auto original_areas = result.player_areas;
        MenuText custom = defaults.menu_text(); custom.results.contribution_team_a = L" <{A}> "; custom.results.contribution_team_b = L" /{B}/ ";
        const auto compose = [&](int index, const MenuText& text)
        {
            const auto player = settings.player_settings().at(static_cast<size_t>(index));
            return settings.player_label(index, text) + (player.team == PlayerTeam::a ?
                text.results.contribution_team_a : text.results.contribution_team_b) +
                result.player_percentage_string(paint_owners::index(player.num, index));
        };
        const wchar_t* canonical[]{ L"P1 - Pad 4 | Team A | 12.5%", L"P2 - Keyboard/mouse | Team B | 87.5%" };
        const wchar_t* authored[]{ L"P1 - Pad 4 <{A}> 12.5%", L"P2 - Keyboard/mouse /{B}/ 87.5%" };
        for (int index = 0; index < 2; ++index)
        {
            require(compose(index, defaults.menu_text()) == canonical[index] && compose(index, custom) == authored[index],
                "Contribution composition changed default bytes, authored team selection or sparse owner credit");
            const auto& look = defaults.results_menu();
            const auto position = look.contribution_position + mattmath::Vector2F(0.0f, look.contribution_spacing * static_cast<float>(index + 1));
            std::wstring source = compose(index, custom);
            UiTextDropShadow widget("sparse-contribution", source, look.contribution_font, position, &world.render,
                look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source.clear();
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(look.contribution_font), authored[index]);
            require(widget.text() == authored[index] && widget.bounds() == mattmath::RectangleF(position, size),
                "Sparse contribution widget lost literal authored row ownership or its full measured suffix");
            const auto current = settings.player_settings().at(static_cast<size_t>(index));
            require(current.num == original_roster[static_cast<size_t>(index)].num && current.team == original_roster[static_cast<size_t>(index)].team &&
                current.weapon == original_roster[static_cast<size_t>(index)].weapon && result.player_areas == original_areas && result.total_area == 100.0f,
                "Contribution text inspection mutated roster, weapon choices or retained owner credit");
        }
        settings.set_player_team(0, PlayerTeam::b); settings.set_player_team(1, PlayerTeam::a);
        require(compose(0, custom) == L"P1 - Pad 4 /{B}/ 12.5%" && compose(1, custom) == L"P2 - Keyboard/mouse <{A}> 87.5%" &&
            result.player_areas == original_areas && custom.results.contribution_team_a == L" <{A}> " && custom.results.contribution_team_b == L" /{B}/ ",
            "Swapping teams must change only the authored separator, retaining device-slot percentages and source copy");
    }

    void check_contribution_team_bounds(Variants& variants, World& world)
    {
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool digit = character >= U'0' && character <= U'9';
            const bool marker = character == U'~' || character == U'^';
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? 240 : character == U'8' ? 80 : digit ? 1 : 16,
                character == U'^' ? 32 : character == U'~' || character == U'8' ? 24 : 16),
                marker ? 0.375f : character == U'8' ? 0.00003f : character == U'6' ? -5.0f : 0.0f, 0.0f,
                marker ? 0.625f : character == U'8' ? -80.0f : character == U'1' || character == U'7' ? 19.0f : 0.0f });
        }
        auto font_owner = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f); font_owner->set_stand_in(U'?');
        world.render.add_font("menu_contribution_teams", std::move(font_owner));
        const Font& font = *world.render.font(world.render.resolve_sprite_font("menu_contribution_teams"));
        constexpr const wchar_t* devices[]{ L"Pad 1", L"Pad 2", L"Pad 3", L"Pad 4", L"Keyboard/mouse" };
        constexpr const wchar_t* separators[]{ L" {A}~", L"^{B} " };
        float maximum[2]{}, maximum_at_100[2]{}; std::wstring attained[2]; int attained_player[2]{};
        // Measure complete literal rows through Font, independently of the
        // production cached-prefix/suffix walk and all regrouped fragment sums.
        for (int team = 0; team < 2; ++team)
            for (int player = 1; player <= 4; ++player)
                for (const wchar_t* device : devices)
                {
                    const std::wstring prefix = L"P" + std::to_wstring(player) + L" - " + device + separators[team];
                    maximum_at_100[team] = std::max(maximum_at_100[team], font.measure(prefix + L"100.0%").x);
                    for (int tenth = 0; tenth <= 1000; ++tenth)
                    {
                        const std::wstring row = prefix + std::to_wstring(tenth / 10) + L'.' + std::to_wstring(tenth % 10) + L'%';
                        const float width = font.measure(row).x;
                        if (width > maximum[team]) { maximum[team] = width; attained[team] = row; attained_player[team] = player; }
                    }
                }
        require(font.measure(L"P4 - Keyboard/mouse {A}~0.0%").x == font.measure(L"P4 - Keyboard/mouse {A}~").x + font.measure(L"0.0%").x + 0.625f &&
            font.measure(L"P4 - Keyboard/mouse {A}~6.6%").x == font.measure(L"P4 - Keyboard/mouse {A}~").x + font.measure(L"6.6%").x - 4.375f,
            "Authored separator must carry its terminal advance and signed suffix bearing into the complete measured row");
        const auto edit = [&](rapidjson::Document& d, float x)
        {
            d["results_menu"]["contribution_font"].SetString("menu_contribution_teams");
            d["results_menu"]["contribution_position"]["x"].SetFloat(x); d["results_menu"]["contribution_spacing"].SetFloat(46.0f);
            d["results_menu"]["fill"]["box_size"]["x"].SetFloat(1280.0f);
            d["menu_text"]["results"]["contribution_heading"].SetString("R");
            d["menu_text"]["records"]["new_best"].SetString("R"); d["menu_text"]["records"]["stage_best"].SetString("R");
            d["menu_text"]["results"]["contribution_team_a"].SetString(" {A}~"); d["menu_text"]["results"]["contribution_team_b"].SetString("^{B} ");
        };
        for (int team = 0; team < 2; ++team)
        {
            require(maximum[team] > maximum_at_100[team] && maximum[team] < 1234.0f &&
                font.measure(attained[team]).y == (team == 0 ? 24.0f : 32.0f),
                "Contribution font must distinguish earlier percentage extrema and the two authored team heights");
            const float x = 1610.0f - maximum[team];
            const auto select_team = [&](rapidjson::Document& d, bool outside)
            {
                edit(d, x + (outside ? 0.25f : 0.0f));
                d["menu_text"]["results"][team == 0 ? "contribution_team_b" : "contribution_team_a"].SetString(team == 0 ? "^" : "~", d.GetAllocator());
            };
            const auto edge = variants.load([&](rapidjson::Document& d) { select_team(d, false); });
            edge.validate_resources(world.render, world.audio);
            require(edge.player_identity().ui_size == mattmath::Vector2F(54.0f, 42.0f) && edge.player_identity().ui_gap == 12.0f,
                "Contribution badge boundary fixture changed the shipped badge size/gap");
            const float y = 635.0f + 46.0f * static_cast<float>(attained_player[team]);
            UiTextDropShadow row("authored-contribution-edge", attained[team], edge.results_menu().contribution_font,
                { x, y }, &world.render, Colour::black, Colour{}, mattmath::Vector2F::ZERO, true);
            const auto badge = UiPlayerBadge::before(row.bounds(), edge.player_identity());
            const auto full_row = mattmath::RectangleF::union_of(row.bounds(), badge);
            const float top = y - (team == 0 ? 9.0f : 5.0f);
            require(row.bounds().right() == 1610.0f && badge == mattmath::RectangleF(x - 66.0f, top, 54.0f, 42.0f) &&
                full_row == mattmath::RectangleF(x - 66.0f, top, 1610.0f - (x - 66.0f), 42.0f),
                "Complete contribution text and actual badge lost exact full-paper contact or team-specific vertical placement");
            const std::string owner = std::string("labels.roster_row.Results.menu_text.results.") + contribution_team_text[team].key;
            variants.rejects(owner, [&](rapidjson::Document& d) { select_team(d, true); }, &world);
        }
        const auto left = variants.load([&](rapidjson::Document& d) { edit(d, 376.0f); });
        left.validate_resources(world.render, world.audio);
        require(UiPlayerBadge::before(mattmath::RectangleF(376.0f, 681.0f, 100.0f, 24.0f), left.player_identity()) ==
            mattmath::RectangleF(310.0f, 672.0f, 54.0f, 42.0f), "Complete first-row badge must meet the Results paper left edge");
        variants.rejects("labels.roster_row.Results.menu_text.results.contribution_team_a", [&](rapidjson::Document& d) { edit(d, 375.75f); }, &world);
        // A's shorter text gives the higher badge top, while B's taller text
        // gives the lower badge bottom: their legal-team envelope is 46 high.
        require(681.0f - 9.0f == 672.0f && 681.0f + 37.0f == 718.0f && 727.0f - 9.0f == 718.0f,
            "Mixed-team row envelope must touch the next row at exactly46 units");
        variants.rejects("labels.roster_row.Results.menu_text.results.contribution_team_a.and menu_text.results.contribution_team_b", [&](rapidjson::Document& d)
        { edit(d, 376.0f); d["results_menu"]["contribution_spacing"].SetFloat(45.75f); }, &world);
        variants.rejects("results.contribution_heading", [](rapidjson::Document& d)
        {
            d["results_menu"]["contribution_font"].SetString("menu_text_limited");
            d["menu_text"]["results"]["contribution_heading"].SetString("~"); d["menu_text"]["results"]["contribution_team_a"].SetString("~");
        }, &world);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: menu_text.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "menu-text-fixtures");
        World world(content);
        const Presentation defaults((content / "presentation.json").string().c_str());
        check_defaults(defaults);
        defaults.validate_resources(world.render, world.audio);
        check_keyed_content(variants);
        check_text_and_fonts(variants, world, content);
        check_inclusive_bounds(variants, world);
        check_item_bounds(variants, world);
        check_restore_defaults_text(variants, world);
        check_options_palette_timing(variants, world);
        check_setup_text(variants, world);
        check_pause_text(variants, world);
        check_connection_text(variants, world);
        check_connection_bounds(variants, world);
        check_label_text(variants, world, defaults.menu_text());
        check_label_resources(variants, world);
        check_label_results_extrema(variants, world);
        check_confirmation_text(variants, world);
        check_confirmation_bounds(variants, world);
        check_display_trial_text(variants, world);
        check_end_text(variants, world);
        check_end_headings(variants, world);
        check_end_heading_bounds(variants, world);
        check_controls_text(variants, world);
        check_controls_option_rows(variants, world);
        check_setting_percentage_text(variants, world, defaults.menu_text());
        check_setting_percentage_rows(variants, world);
        check_value_text(variants, world);
        check_value_fitting(variants, world);
        check_stage_random_fitting(variants, world);
        check_stage_random_bounds(variants, world);
        check_stage_count_text(variants, world, defaults.menu_text());
        check_stage_count_bounds(variants, world);
        check_stage_summary_text(variants, world, defaults.menu_text());
        check_stage_summary_bounds(variants, world);
        check_stage_briefing_text(variants, world);
        check_stage_briefing_bounds(variants, world);
        check_stage_status_text(variants, world);
        check_stage_status_bounds(variants, world);
        check_stage_cue_text(variants, world);
        check_stage_cue_bounds(variants, world);
        check_title_and_home_text(variants, world, defaults);
        check_title_and_home_bounds(variants, world);
        check_help_text(variants, world);
        check_help_fitting(variants, world);
        check_help_bounds(variants, world);
        check_stick_readout_text(variants, world, defaults.menu_text());
        check_stick_readout_bounds(variants, world);
        check_stick_numeric_extrema(variants, world);
        check_status_text(variants, world);
        check_status_fitting(variants, world);
        check_status_bounds(variants, world);
        check_option_row_fitting(variants, world);
        check_prompt_bar(world, defaults);
        check_setting_duration_text(variants, defaults.menu_text());
        check_setting_duration_content(variants, world, content);
        check_setting_resolution_text(variants, defaults.menu_text());
        check_setting_resolution_resources(variants, world);
        check_end_continuation_text(variants, world);
        check_end_continuation_bounds(variants, world);
        check_results_plain_text(variants, world);
        check_results_plain_bounds(variants, world);
        check_session_result_text(variants, world);
        check_session_result_bounds(variants, world);
        check_series_score_text(variants, defaults.menu_text());
        check_series_score_resources(variants, world, defaults);
        check_round_heading_text(variants, defaults.menu_text());
        check_round_heading_resources(variants, world, defaults);
        check_competitive_winner_text(variants, world);
        check_competitive_winner_selection(defaults.menu_text());
        check_competitive_winner_bounds(variants, world);
        check_contribution_team_text(variants, world);
        check_contribution_team_roster(world, defaults);
        check_contribution_team_bounds(variants, world);
        std::cout << "PASS shared menu text: exact headings/items/complete legends, keyed and enum guards, normalization, "
            "font/paper/row/status/confirmation/End-heading bounds, unscaled centered confirmation/End-heading copy, width-only save-status fitting, "
            "authored OptionRow value fitting, connection schema/player tokens/fresh glyphs and exact Join/Pause/Team bounds, "
            "all101 setting percentages with captured Controls/Sticks value fitting, "
            "authored duration templates, FLOAT/defaultfloat compatibility and actual tuning/default/choice row validation, "
            "authored resolution templates with full positive integer dimensions, strict original tokens and captured Options fitting, "
            "four plain End continuation labels with literal braces, actual-font glyphs and unscaled row0 padding boundaries, "
            "three plain Results roles with zero-offset shadows, centered paper bounds and authored record/roster clearance dependencies, "
            "four plain Session result outcomes with owned zero-shadow text and inclusive unscaled Results-paper bounds, "
            "strict authored series-score tokens, all26 legal ledgers and proportional-font full-paper boundaries, "
            "authored numbered Results headings over the full positive-int domain, "
            "plain competitive winners with original float/solo selection and inclusive actual item-font paper bounds, "
            "authored contribution team separators with sparse-slot credit, sequential full-row font bounds and complete mixed-team badge clearance, "
            "device/roster labels with original tokens, actual fonts, full Pause lists and exact Results percentage bounds, "
            "shadow-inclusive Random arena fitting, unscaled arena briefing/status/navigation, "
            "finite authored series/roster templates and expanded-card boundaries, "
            "actual stage-count templates, all-index proportional bounds and width-fitted Random counts, "
            "Pad Sticks exact-token/range/201-value/schema guards, multiline boundaries and adversarial pen/ink extrema, "
            "width-only arena hints and actual PromptBar measurement compatibility.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL shared menu text: " << error.what() << '\n';
        return 1;
    }
}
