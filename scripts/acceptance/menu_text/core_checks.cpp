#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"
#include "game/carried/ui_text_drop_shadow.h"

// The defaults, keyed content, fonts and prompt bar every other check builds
// on. check_text_and_fonts and check_inclusive_bounds register the core fonts
// (see fixtures.h).
namespace menu_text_acceptance
{
    namespace
    {
    rapidjson::Value& text_field(rapidjson::Document& d, const char* group, const char* key)
    {
        return group == nullptr ? d["menu_text"][key] : d["menu_text"][group][key];
    }
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
        for (const auto& entry : keyboard_jump_text)
        {
            require(text.keyboard_jump.*entry.field == entry.text,
                std::string("Keyboard Jump menu default changed: ") + entry.key);
        }
        for (const auto& entry : pad_buttons_text)
        {
            require(text.pad_buttons.*entry.field == entry.text,
                std::string("Pad Buttons menu default changed: ") + entry.key);
        }
        require(text.keyboard_jump.release_key(L"W") == L"Release W to keep this choice." &&
            text.keyboard_jump.release_key(L"Right click") == L"Release Right click to keep this choice." &&
            text.keyboard_jump.row(0) == L"Move left" && text.keyboard_jump.row(2) == L"Jump" &&
            text.keyboard_jump.row(5) == L"Pause (also)",
            "Keyboard Keys wrappers changed their literal copy, row order or required key substitution");
        bool row_rejected = false;
        try { std::ignore = text.keyboard_jump.row(6); }
        catch (const std::out_of_range&) { row_rejected = true; }
        require(row_rejected, "Keyboard Keys row lookup accepted a seventh action");
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
        for (const auto& entry : result_percentage_text)
            require(text.results.*entry.field == entry.text && (text.results.*entry.line)(L"12.5%") == entry.example,
                std::string("Results percentage wrapper default changed: ") + entry.key);
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
        // The templates own the words, their order and the joins; the numbers are the caller's.
        require(text.records.new_best_line(L"68.4%") == L"NEW BEST 68.4%" &&
            text.records.stage_best_line(L"72.1%") == L"STAGE BEST 72.1%" &&
            text.records.played_line(2) == L"2 PLAYED" &&
            text.records.best_line(false, L"68.4%", 12) == L"BEST WIN 68.4% / 12 PLAYED" &&
            text.records.best_line(true, L"37.0%", 2) == L"BEST COVER 37.0% / 2 PLAYED",
            "Stage record templates no longer compose the shipped English lines");
        require(text.stage.random_mark == L"?", "Stage Select's Random mark default changed");
        for (const auto& entry : hud_text)
        {
            require(text.hud.*entry.field == entry.text && presentation.hud().labels.*entry.field == entry.text,
                std::string("HUD label default changed: ") + entry.key);
        }
        for (const auto& entry : countdown_text)
        {
            require(text.countdown.*entry.field == entry.text, std::string("Countdown heading default changed: ") + entry.key);
        }
        for (const auto& entry : controls_text_defaults)
        {
            require(text.controls.*entry.field == entry.text, std::string("Controls reminder default changed: ") + entry.key);
        }
        for (const auto& entry : control_label_text)
        {
            require(text.controls.labels.*entry.field == entry.text && text.keyboard_jump.labels.*entry.field == entry.text,
                std::string("Control label default changed, or Keyboard Keys lost it: ") + entry.key);
        }
        // The words come only from the copy: input_bindings.cpp holds legends,
        // and none for Space, None or Start.
        const ControlLabels& words = text.controls.labels;
        require(input_key_legend(Key::space).empty() && pad_control_legend(PadControl::none).empty() &&
            pad_control_legend(PadControl::start).empty() &&
            controls_text::key_label(words, Key::space) == words.space && controls_text::desktop_label(words, {}) == words.none &&
            controls_text::pad_label(words, PadControl::none) == words.none &&
            controls_text::pad_label(words, PadControl::start) == words.start &&
            controls_text::desktop_label(words, { Key::none, MouseButton::left }) == words.mouse_left &&
            controls_text::desktop_label(words, { Key::none, MouseButton::right }) == words.mouse_right &&
            controls_text::desktop_label(words, { Key::none, MouseButton::middle }) == words.mouse_middle &&
            controls_text::desktop_label(words, { Key::none, MouseButton::x1 }) == words.mouse_x1 &&
            controls_text::desktop_label(words, { Key::none, MouseButton::x2 }) == words.mouse_x2,
            "A control-label word did not come from presentation.json, or input_bindings.cpp kept one");
        for (const auto& entry : stage_briefing_text)
        {
            require(text.stage.*entry.field == entry.text, std::string("Arena briefing default changed: ") + entry.key);
        }
        // The pinned stages keep their names; a stage registered since may join
        // (user_stages holds the table to exactly the manifest's stages).
        require(text.stage_names.size() >= std::size(stage_names_text), "menu_text.stage_names lost a stage");
        for (const auto& entry : stage_names_text)
        {
            const std::wstring* name = text.stage_name(entry.key);
            require(name != nullptr && *name == entry.text &&
                text.stage_title(entry.key, "Authored elsewhere", "fixture") == entry.text,
                std::string("Stage name default changed, or stage_title ignored it: ") + entry.key);
        }
        // A stage the table does not name - a user stage - keeps its own text.
        require(text.stage_name("my_arena") == nullptr &&
            text.stage_title("my_arena", "My Arena", "fixture") == L"My Arena",
            "A user stage's display_name did not reach its title");
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
            for (const char* group : { "headings", "prompts", "items", "title", "help", "keyboard_jump", "pad_buttons", "status", "values", "confirmations", "end", "stage", "stage_names", "records", "connection", "stick_readouts", "labels", "hud", "countdown", "controls" })
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
        for (const char* group : { "headings", "prompts", "items", "title", "help", "keyboard_jump", "pad_buttons", "status", "values", "confirmations", "end", "stage", "stage_names", "records", "connection", "stick_readouts", "labels", "hud", "countdown", "controls" })
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
        for (const auto& entry : keyboard_jump_text) { required("keyboard_jump", entry.key); }
        for (const auto& entry : pad_buttons_text) { required("pad_buttons", entry.key); }
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
        variants.rejects("records.new_best", [](rapidjson::Document& d) { d["menu_text"]["records"]["new_best"].SetString("NEW BEST"); });
        variants.rejects("records.played", [](rapidjson::Document& d) { d["menu_text"]["records"]["played"].SetString("{count} {count} PLAYED"); });
        variants.rejects("records.best_win", [](rapidjson::Document& d) { d["menu_text"]["records"]["best_win"].SetString("BEST WIN {percentage}"); });
        required("stage", "random_mark");
        for (const auto& entry : hud_text) { required("hud", entry.key); }
        for (const auto& entry : countdown_text) { required("countdown", entry.key); }
        for (const auto& entry : controls_text_defaults) { required("controls", entry.key); }
        variants.rejects("controls.labels", [](rapidjson::Document& d) { d["menu_text"]["controls"].RemoveMember("labels"); });
        variants.rejects("controls.labels", [](rapidjson::Document& d) { d["menu_text"]["controls"]["labels"].SetArray(); });
        for (const auto& entry : control_label_text)
        {
            const std::string field = std::string("controls.labels.") + entry.key;
            const char* key = entry.key;
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["controls"]["labels"].RemoveMember(key); });
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["controls"]["labels"][key].SetNull(); });
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["controls"]["labels"][key].SetString(""); });
            variants.rejects(field, [key](rapidjson::Document& d)
                { d["menu_text"]["controls"]["labels"][key].SetString("line\nbreak"); });
        }
        for (const auto& entry : stage_names_text)
        {
            // The key set is the manifest's, so a missing entry is refused by
            // validate_stage_names once it loads (the user_stages case), not here.
            const std::string field = std::string("stage_names.") + entry.key;
            const char* key = entry.key;
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["stage_names"][key].SetBool(true); });
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["stage_names"][key].SetNull(); });
            variants.rejects(field, [key](rapidjson::Document& d) { d["menu_text"]["stage_names"][key].SetString(""); });
            variants.rejects(field, [key](rapidjson::Document& d)
                { d["menu_text"]["stage_names"][key].SetString("bad\0text", 8, d.GetAllocator()); });
            variants.rejects(field, [key](rapidjson::Document& d)
                { d["menu_text"]["stage_names"][key].SetString("line\nbreak"); });
            variants.rejects(field, [key](rapidjson::Document& d)
                { d["menu_text"]["stage_names"][key].SetString("Caf\xC3\xA9"); });
        }
        variants.rejects("stage_names.crossfeed", [](rapidjson::Document& d)
            { d["menu_text"]["stage_names"].AddMember("crossfeed", "Crossfeed Again", d.GetAllocator()); });
        for (const auto& entry : stage_text) { required("stage", entry.key); }
        for (const auto& entry : stage_count_text) { required("stage", entry.key); }
        for (const auto& entry : stage_summary_text) { required("stage", entry.key); }
        for (const auto& entry : stage_briefing_text) { required("stage", entry.key); }
        for (const auto& entry : stage_cue_text) { required("stage", entry.key); }
        for (const auto& entry : headings) { required("headings", entry.key); }
        for (const auto& entry : prompts) { required("prompts", entry.key); }
        for (const auto& entry : items) { required("items", entry.key); }
    }

    namespace
    {
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
        const SpriteFontFile font = read_sprite_font_file((content / "fonts/lato_black_24.spritefont").string());
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
        // The Random mark draws in the announcement font.
        variants.rejects("menu_text.stage.random_mark", [](rapidjson::Document& d)
            { d["menu_text"]["stage"]["random_mark"].SetString("Glyph~"); d["main_menu"]["fonts"]["announcement"].SetString("menu_text_limited"); }, &world);
        // It is drawn unscaled on the preview, so startup refuses copy that
        // would leave it in either direction instead of Stage Select throwing.
        variants.rejects("menu_text.stage.random_mark", [](rapidjson::Document& d)
            { d["menu_text"]["stage"]["random_mark"].SetString(std::string(40, 'W').c_str(), d.GetAllocator()); }, &world);
        variants.rejects("menu_text.stage.random_mark", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"]["preview_size"]["y"].SetFloat(20.0f); }, &world);
    }

    void check_prompt_bar(World& world, const Presentation& look)
    {
        const auto& style = look.prompt_bar();
        for (size_t index = 0; index < std::size(prompts); ++index)
        {
            const std::wstring& legend = look.menu_text().prompt(static_cast<MenuPrompt>(index));
            PromptBar bar("authored_prompt", legend, style, &world.render);
            colourwars::UiTextDropShadow reference("measured_prompt", legend, style.font, style.position, &world.render,
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
            glyphs.push_back({ character, labrador::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_text_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_text_exact"), L"Fit") ==
            labrador::Vector2F(24.0f, 20.0f), "Exact-boundary fixture font has unexpected metrics");
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
}
