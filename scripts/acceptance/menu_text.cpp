#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/option_row.h"
#include "game/states/prompt_bar.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
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
        { "sticks_saved", L"Pad deadzones applied and saved.", &MenuStatusText::sticks_saved, false }
    };
    struct ValueText { const char* key; const wchar_t* text; std::wstring MenuValueText::* field; };
    constexpr ValueText value_text[]{
        { "on", L"On", &MenuValueText::on }, { "off", L"Off", &MenuValueText::off },
        { "random", L"Random", &MenuValueText::random }
    };
    struct ConfirmationText { const char* key; const wchar_t* text; std::wstring MenuConfirmationText::* field; };
    constexpr ConfirmationText confirmation_text[]{
        { "restart", L"Are you sure you want to restart?", &MenuConfirmationText::restart },
        { "quit", L"Are you sure you want to quit?", &MenuConfirmationText::quit }
    };
    struct EndText { const char* key; const wchar_t* text; std::wstring MenuEndText::* field; };
    constexpr EndText end_text[]{
        { "match_complete", L"Match Complete", &MenuEndText::match_complete },
        { "round_complete", L"Round Complete", &MenuEndText::round_complete },
        { "series_complete", L"Series Complete", &MenuEndText::series_complete }
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
        { "pad", L"Pad" }, { "move_deadzone", L"Move deadzone" }, { "aim_deadzone", L"Aim deadzone" }
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
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx" })
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
        for (const auto& entry : value_text)
        {
            require(text.values.*entry.field == entry.text, std::string("Fixed setting value default changed: ") + entry.key);
        }
        for (const auto& entry : confirmation_text)
        {
            require(text.confirmations.*entry.field == entry.text,
                std::string("Pause confirmation default changed: ") + entry.key);
        }
        for (const auto& entry : end_text)
        {
            require(text.end.*entry.field == entry.text, std::string("End heading default changed: ") + entry.key);
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
            for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end" })
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
        for (const char* group : { "headings", "prompts", "items", "title", "help", "status", "values", "confirmations", "end" })
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
        for (const auto& entry : title_text) { required("title", entry.key); }
        for (const auto& entry : help_text) { required("help", entry.key); }
        for (const auto& entry : status_text) { required("status", entry.key); }
        for (const auto& entry : value_text) { required("values", entry.key); }
        for (const auto& entry : confirmation_text) { required("confirmations", entry.key); }
        for (const auto& entry : end_text) { required("end", entry.key); }
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
        const auto& art = look.main_menu().art;
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
            "change_teams", "change_weapons", "change_level", "exit_to_main_menu" })
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
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(1.5f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(1.5f);
        };
        const float row_height = 20.0f + 1.5f;
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
            d["menu_style"]["layout"]["position"]["y"].SetFloat(238.5f);
            d["main_menu"]["mode_select_spacing"]["y"].SetInt(100);
        };
        const auto fits_hint = variants.load(hint_edge);
        fits_hint.validate_resources(world.render, world.audio);
        variants.rejects("solo_practice_hint", [&](rapidjson::Document& d)
        {
            hint_edge(d);
            d["menu_style"]["layout"]["position"]["y"].SetFloat(238.75f);
        }, &world);

        const auto left_edge = [&](rapidjson::Document& d)
        {
            exact_setup(d);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(1.5f);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
        };
        const auto touches_left = variants.load(left_edge);
        touches_left.validate_resources(world.render, world.audio);
        variants.rejects("items.single_match", [&](rapidjson::Document& d)
        {
            left_edge(d);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(1.25f);
        }, &world);
    }

    void check_pause_text(Variants& variants, World& world)
    {
        constexpr const char* pause_keys[]{ "resume", "restart", "quit", "yes", "no" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : pause_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(pause_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::resume) + index)) == L"A -B",
                "Pause item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : pause_keys)
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
        const auto& pause = original.pause_menu();
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
        // Question paper-top and action contact are checked independently below.
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
        variants.rejects("items.controls_audio", [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["pause_menu"]["layout"]["position"]["x"].SetFloat(right_position);
            d["menu_text"]["items"]["controls_audio"].SetString("FitFit");
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
                std::string("Pause confirmation lost normalized punctuation or authored spaces: ") + entry.key);
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
                use_font(d, "menu_confirmation_action_edge", entry.key);
                d["pause_menu"]["layout"]["position"]["y"].SetFloat(pause.status_y + 20.0f + pause.art.focus_padding.y);
            };
            const auto touches_action = variants.load(action_edge);
            touches_action.validate_resources(world.render, world.audio);
            require(touches_action.pause_menu().status_y + 20.0f == touches_action.pause_menu().layout.position.y -
                touches_action.pause_menu().art.focus_padding.y,
                "Confirmation action fixture lost exact unscaled bottom-to-padding contact");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                action_edge(d);
                d["pause_menu"]["layout"]["position"]["y"].SetFloat(pause.status_y + 19.75f + pause.art.focus_padding.y);
            }, &world);
        }
        const auto paper_top = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            for (const auto& entry : confirmation_text) { d["menu_text"]["confirmations"][entry.key].SetString("Fit"); }
            d["pause_menu"]["heading_y"].SetFloat(40.0f);
            d["pause_menu"]["status_y"].SetFloat(90.0f);
        };
        const auto touches_top = variants.load(paper_top);
        touches_top.validate_resources(world.render, world.audio);
        variants.rejects("confirmations.restart", [&](rapidjson::Document& d)
        {
            paper_top(d);
            d["pause_menu"]["status_y"].SetFloat(89.75f);
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
            d["menu_style"]["fonts"]["item"].SetString("menu_confirmation_small_actions");
            d["pause_menu"]["status_y"].SetFloat(900.0f);
            d["pause_menu"]["layout"]["position"]["y"].SetFloat(940.5f);
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["pause_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["pause_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["pause_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
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
        }
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
        const auto random_is_options_only = variants.load([](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_limited");
            d["menu_text"]["values"]["random"].SetString("Glyph~");
        });
        random_is_options_only.validate_resources(world.render, world.audio);
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
                const auto& art = use.options ? main.art : controls.art;
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
                require(limit == (use.options ? mattmath::Vector2F(924.0f, 84.0f) : mattmath::Vector2F(332.0f, 68.0f)),
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
            });
            tall.validate_resources(world.render, world.audio);
            check_rows(tall, wide_glyph, true);
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
                const auto& art = entry.options ? main.art : controls.art;
                const auto& font_name = entry.options ? look.prompt_bar().font : controls.help_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const float available = (1920.0f + box_size.x) / 2.0f -
                    art.shadow_offset - art.focus_padding.x - position.x;
                require(available == (entry.options ? 924.0f : 1590.0f),
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
        const auto back_size = world.render.measure_text(world.render.resolve_sprite_font(original.menu().item_font),
            original.menu_text().item(MenuItem::back));
        const float back_right = main.options_position.x + back_size.x + main.art.focus_padding.x;
        const auto back_edge = [&](rapidjson::Document& d)
        {
            // The shipped status already overlaps Back vertically. Exact
            // horizontal contact remains legal; any positive area is rejected.
            d["main_menu"]["options_status_position"]["x"].SetFloat(back_right);
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
        };
        const auto beside_back = variants.load(back_edge);
        beside_back.validate_resources(world.render, world.audio);
        require(main.options_status_position.y < main.options_position.y + main.options_spacing.y * 5.0f +
            back_size.y + main.art.focus_padding.y,
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
            d["main_menu"]["options_status_position"]["x"].SetFloat(1744.0f);
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
        check_setup_text(variants, world);
        check_pause_text(variants, world);
        check_confirmation_text(variants, world);
        check_confirmation_bounds(variants, world);
        check_end_text(variants, world);
        check_end_headings(variants, world);
        check_end_heading_bounds(variants, world);
        check_controls_text(variants, world);
        check_controls_option_rows(variants, world);
        check_value_text(variants, world);
        check_value_fitting(variants, world);
        check_title_and_home_text(variants, world, defaults);
        check_title_and_home_bounds(variants, world);
        check_help_text(variants, world);
        check_help_fitting(variants, world);
        check_help_bounds(variants, world);
        check_status_text(variants, world);
        check_status_fitting(variants, world);
        check_status_bounds(variants, world);
        check_option_row_fitting(variants, world);
        check_prompt_bar(world, defaults);
        std::cout << "PASS shared menu text: exact headings/items/complete legends, keyed and enum guards, normalization, "
            "font/paper/row/status/confirmation/End-heading bounds, unscaled centered confirmation/End-heading copy, width-only save-status fitting, "
            "authored OptionRow value fitting and actual PromptBar copy/measurement compatibility.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL shared menu text: " << error.what() << '\n';
        return 1;
    }
}
