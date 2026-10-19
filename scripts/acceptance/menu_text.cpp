#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/option_row.h"
#include "game/states/prompt_bar.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
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
        for (const auto& entry : stage_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena status default changed: ") + entry.key);
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
            for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end", "stage", "records", "connection", "stick_readouts" })
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
        for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end", "stage", "records", "connection", "stick_readouts" })
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
        for (const auto& entry : stick_readout_text) { required("stick_readouts", entry.key); }
        for (const auto& entry : value_text) { required("values", entry.key); }
        for (const auto& entry : confirmation_text) { required("confirmations", entry.key); }
        for (const auto& entry : end_text) { required("end", entry.key); }
        for (const auto& entry : record_text) { required("records", entry.key); }
        for (const auto& entry : stage_text) { required("stage", entry.key); }
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
        touches_heading.validate_resources(world.render, world.audio);
        variants.rejects("connection.teams_required", [&](rapidjson::Document& d)
        {
            team_order(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(172.25f);
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
            for (const auto& entry : confirmation_text) { d["menu_text"]["confirmations"][entry.key].SetString("Fit"); }
            d["menu_text"]["confirmations"][key].SetString("~");
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& pause = original.pause_menu();
        require(pause.box_size.x == 1050.0f && pause.box_size.y == 900.0f,
            "Confirmation paper fixtures no longer exercise the shipped full-height Pause paper");
        add_font("menu_confirmation_width_edge", 1050, 20);
        add_font("menu_confirmation_width_overflow", 1051, 20);
        add_font("menu_confirmation_action_edge", 8, 20);
        add_font("menu_confirmation_display_action_edge", 8, 40);
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
        variants.load(screen_top).validate_resources(world.render, world.audio);
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
        variants.load(screen_left).validate_resources(world.render, world.audio);
        variants.rejects("stage.briefing_label", [&](rapidjson::Document& d)
        {
            screen_left(d);
            d["main_menu"]["stage_select"]["briefing"]["position"]["x"].SetFloat(-32.25f);
        }, &world);
        const auto panel_top = [&](rapidjson::Document& d)
        {
            exact(d);
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
            variants.load(footer).validate_resources(world.render, world.audio);
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
            variants.load(contact).validate_resources(world.render, world.audio);
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
        check_confirmation_text(variants, world);
        check_confirmation_bounds(variants, world);
        check_display_trial_text(variants, world);
        check_end_text(variants, world);
        check_end_headings(variants, world);
        check_end_heading_bounds(variants, world);
        check_controls_text(variants, world);
        check_controls_option_rows(variants, world);
        check_value_text(variants, world);
        check_value_fitting(variants, world);
        check_stage_random_fitting(variants, world);
        check_stage_random_bounds(variants, world);
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
        std::cout << "PASS shared menu text: exact headings/items/complete legends, keyed and enum guards, normalization, "
            "font/paper/row/status/confirmation/End-heading bounds, unscaled centered confirmation/End-heading copy, width-only save-status fitting, "
            "authored OptionRow value fitting, connection schema/player tokens/fresh glyphs and exact Join/Pause/Team bounds, "
            "shadow-inclusive Random arena fitting, unscaled arena briefing/status/navigation, "
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
