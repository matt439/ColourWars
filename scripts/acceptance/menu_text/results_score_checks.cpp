#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"
#include "game/carried/ui_text_drop_shadow.h"

// Results: series score and numbered round headings. Percentages are in
// results_percentage_checks.cpp.
namespace menu_text_acceptance
{
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
        colourwars::UiTextDropShadow widget("series-score-edge", source, edge.menu().detail_font, { 310.0f, 340.0f }, &world.render,
            edge.results_menu().art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
        source = L"Changed after construction";
        require(widget.bounds() == mattmath::RectangleF(310.0f, 340.0f, 1300.0f, 20.0f) && widget.text() == L"~101" &&
            widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
            "Series-score widget lost unscaled centered copy or gained a shadow at the exact left/right paper edges");
        DrawList no_device; widget.draw(no_device);
        variants.rejects("results.series_score", [&](rapidjson::Document& d) { select(d, "menu_score_width_overflow"); }, &world);
        const auto bottom = variants.load([&](rapidjson::Document& d) { select(d, "menu_score_bottom"); });
        bottom.validate_resources(world.render, world.audio);
        colourwars::UiTextDropShadow tall("series-score-bottom", L"~101", bottom.menu().detail_font, { 944.0f, 340.0f }, &world.render,
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
        colourwars::UiTextDropShadow widget("round-heading-edge", source, wide_nines.menu().heading_font, { 310.0f, 125.0f },
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
        colourwars::UiTextDropShadow tall("round-heading-bottom", L"~1", bottom.menu().heading_font, { 952.0f, 125.0f },
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
}
