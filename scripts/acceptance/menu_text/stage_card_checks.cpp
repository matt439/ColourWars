#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The arena card's summary and briefing.
namespace menu_text_acceptance
{
    namespace
    {
    std::wstring stage_summary(const MenuText& text, const StageSummaryText& entry, int value)
    {
        return entry.detail ? text.stage_best_of_mode(value) : text.stage_roster(value);
    }

    void select_stage_briefing_font(rapidjson::Document& d, const StageBriefingText& entry, const char* name)
    {
        if (entry.detail) { d["menu_style"]["fonts"]["detail"].SetString(name, d.GetAllocator()); }
        else { d["menu_style"]["chrome"]["brand_font"].SetString(name, d.GetAllocator()); }
    }
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
}
