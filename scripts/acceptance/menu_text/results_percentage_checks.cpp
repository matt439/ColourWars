#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"
#include "game/carried/ui_text_drop_shadow.h"

// Results: percentage wrappers, FLOAT numbers and paper bounds.
namespace menu_text_acceptance
{
    void check_result_percentage_text(Variants& variants, const MenuText& defaults)
    {
        const auto refuses = [](auto action)
        {
            bool refused = false; try { action(); } catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Malformed original Results percentage template formatted silently");
        };
        for (const auto& entry : result_percentage_text)
        {
            const std::string field = std::string("results.") + entry.key;
            for (int tenth = 0; tenth <= 1000; ++tenth)
            {
                const std::wstring value = std::to_wstring(tenth / 10) + L'.' + std::to_wstring(tenth % 10) + L'%';
                const std::wstring expected = entry.field == &MenuResultsText::solo_coverage ? value + L" Painted" :
                    std::wstring(entry.field == &MenuResultsText::team_a_percentage ? L"A " : L"B ") + value;
                require((defaults.results.*entry.line)(value) == expected, "Results reference percentage changed its original default composition");
            }
            for (const std::wstring& value : { std::wstring{}, std::wstring(L"-0.0%"), std::wstring(L"-250.0%"),
                std::wstring(L"1000000.0%"), std::wstring(L"nan(ind)%"), std::wstring(L"inf%"),
                std::wstring(L"{percentage}{unknown}"), std::wstring(L"text\n\t\u00e9"), std::wstring(L"A\0B", 3) })
            {
                MenuResultsText authored = defaults.results; authored.*entry.field = L"<{percentage}>%";
                std::wstring output = (authored.*entry.line)(value);
                require(output == L"<" + value + L">%", "An already-rendered percentage was parsed, capped, normalized or rescanned for tokens");
                output.clear();
                require((authored.*entry.line)(value) == L"<" + value + L">%" && authored.*entry.field == L"<{percentage}>%",
                    "Results percentage wrapper returned borrowed text or changed its original template");
                authored.*entry.field = L"{percentage}";
                require((authored.*entry.line)(value) == value, "A bare percentage token must preserve even an empty or non-drawable supplied value");
            }
            const auto normalized = variants.load([&](rapidjson::Document& d)
            { d["menu_text"]["results"][entry.key].SetString("  {percentage}\xC2\xA0\xE2\x80\x9C" "value\xE2\x80\x9D\xE2\x80\x94...  "); });
            require(normalized.menu_text().results.*entry.field == L"  {percentage} \"value\"-...  " &&
                (normalized.menu_text().results.*entry.line)(L"12.5%") == L"  12.5% \"value\"-...  ",
                "Percentage wrapper lost normalized punctuation, authored token position or outer spaces");
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            for (int type = 0; type < 5; ++type)
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& value = d["menu_text"]["results"][entry.key];
                    if (type == 0) { value.SetNull(); } else if (type == 1) { value.SetBool(true); }
                    else if (type == 2) { value.SetInt(3); } else if (type == 3) { value.SetArray(); } else { value.SetObject(); }
                });
            for (const char* invalid : { "", "   ", "No token", "{Percentage}", "{percentage }", "{percentage}{percentage}",
                "{{percentage}}", "{percentage}{unknown}", "{percentage", "percentage}", "{percentage}{", "{percentage}}", "{per{percentage}centage}" })
            {
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
                MenuResultsText mutated = defaults.results; mutated.*entry.field = std::wstring(invalid, invalid + std::strlen(invalid));
                refuses([&] { (mutated.*entry.line)(L"{percentage}"); });
            }
            for (const char* suffix : { "\n", "\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
                variants.rejects(field, [&](rapidjson::Document& d)
                { const std::string text = std::string("{percentage}") + suffix;
                    d["menu_text"]["results"][entry.key].SetString(text.c_str(), d.GetAllocator()); });
            for (const wchar_t* suffix : { L"\n", L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014" })
            {
                MenuResultsText mutated = defaults.results; (mutated.*entry.field) += suffix;
                refuses([&] { (mutated.*entry.line)(L"12.5%"); });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            { const std::string text = std::string("{percentage}") + '\0' + "tail";
                d["menu_text"]["results"][entry.key].SetString(text.data(), static_cast<rapidjson::SizeType>(text.size()), d.GetAllocator()); });
            MenuResultsText nul = defaults.results; (nul.*entry.field).push_back(L'\0');
            refuses([&] { (nul.*entry.line)(L"12.5%"); });
            const auto long_copy = variants.load([&](rapidjson::Document& d)
            { const std::string text = std::string(512, 'W') + "{percentage}";
                d["menu_text"]["results"][entry.key].SetString(text.c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().results.*entry.line)(L"12.5%") == std::wstring(512, L'W') + L"12.5%",
                "Results percentage template acquired a parser character cap");
        }
    }

    void check_result_percentage_numbers(const MenuText& defaults)
    {
        const float infinity = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
        const float largest = (std::numeric_limits<float>::max)(), tiny = std::numeric_limits<float>::denorm_min();
        struct AreaExample { float a; float b; float total; };
        const AreaExample examples[]{
            { 6.0f, 2.0f, 20.0f }, { 0.0f, 0.0f, 10.0f }, { -0.0f, 1.0f, 10.0f }, { 1.0f, -0.0f, 10.0f },
            { -1.0f, -3.0f, 10.0f }, { 2.0f, 1.0f, 10.0f }, { 1.0f, -1.0f, 10.0f },
            { largest, largest, 1.0f }, { -largest, -largest, 1.0f }, { tiny, 0.0f, largest },
            { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), tiny },
            { nan, 1.0f, 100.0f }, { 1.0f, nan, 100.0f }, { nan, nan, 100.0f },
            { infinity, 1.0f, 100.0f }, { 1.0f, infinity, 100.0f }, { -infinity, infinity, 100.0f },
            { infinity, infinity, 100.0f }, { -infinity, -infinity, 100.0f },
            { 1.0f, 2.0f, 0.0f }, { 1.0f, 2.0f, -1.0f }, { 1.0f, 2.0f, nan },
            { infinity, 0.0f, infinity }, { 1.0f, 2.0f, infinity }
        };
        MenuResultsText custom = defaults.results; custom.solo_coverage = L"Solo<{percentage}>";
        custom.team_a_percentage = L"<{percentage}> A"; custom.team_b_percentage = L"B=[{percentage}]";
        for (const bool a_played : { false, true })
            for (const bool b_played : { false, true })
                for (const auto& example : examples)
                {
                    LevelEndInfo result; result.team_a_area = example.a; result.team_b_area = example.b; result.total_area = example.total;
                    result.team_a_played = a_played; result.team_b_played = b_played;
                    result.player_areas.front() = 17.25f; const LevelEndInfo before = result;
                    // These are the exact pre-extraction FLOAT expressions.
                    // Only the unchanged formatter owns NaN/Inf spelling.
                    const float sum = example.a + example.b;
                    const float coverage = example.total <= 0.0f ? 0.0f : sum / example.total;
                    const bool solo = a_played != b_played;
                    const float a_ratio = solo ? (a_played ? coverage : 0.0f) :
                        example.a == 0.0f && example.b == 0.0f ? 0.5f : example.a / sum;
                    const float b_ratio = solo ? (b_played ? coverage : 0.0f) :
                        example.a == 0.0f && example.b == 0.0f ? 0.5f : example.b / sum;
                    const std::wstring a_value = percentage_text(100.0f * a_ratio), b_value = percentage_text(100.0f * b_ratio);
                    require(result.team_a_percentage_string(defaults.results) == L"A " + a_value &&
                        result.team_b_percentage_string(defaults.results) == L"B " + b_value &&
                        result.team_a_percentage_string(custom) == L"<" + a_value + L"> A" &&
                        result.team_b_percentage_string(custom) == L"B=[" + b_value + L"]",
                        "Result percentage wrappers changed FLOAT order, participation, numeric domain or source insertion");
                    if (solo)
                    {
                        const std::wstring value = percentage_text(100.0f * coverage);
                        require(result.winning_team_string(defaults.results) == value + L" Painted" &&
                            result.winning_team_string(custom) == L"Solo<" + value + L">",
                            "Solo wrapper changed its old coverage expression, numeric spelling or captured template");
                    }
                    require(std::memcmp(&result.team_a_area, &before.team_a_area, sizeof(result.team_a_area)) == 0 &&
                        std::memcmp(&result.team_b_area, &before.team_b_area, sizeof(result.team_b_area)) == 0 &&
                        std::memcmp(&result.total_area, &before.total_area, sizeof(result.total_area)) == 0 &&
                        result.player_areas == before.player_areas && result.team_a_played == before.team_a_played &&
                        result.team_b_played == before.team_b_played && result.team_colours.team_a == before.team_colours.team_a &&
                        result.team_colours.team_b == before.team_colours.team_b,
                        "Percentage inspection changed original float bits or any retained result state");
                }
        LevelEndInfo negative_zero; negative_zero.team_a_area = -0.0f; negative_zero.team_b_area = 1.0f;
        require(negative_zero.team_a_percentage_string(defaults.results) == L"A -0.0%" &&
            negative_zero.team_b_percentage_string(defaults.results) == L"B 100.0%",
            "Percentage extraction must preserve signed zero and exact original default prefixes");
        std::wstring owned = negative_zero.team_a_percentage_string(custom); owned.clear();
        require(negative_zero.team_a_percentage_string(custom) == L"<-0.0%> A" && custom.solo_coverage == L"Solo<{percentage}>" &&
            custom.team_a_percentage == L"<{percentage}> A" && custom.team_b_percentage == L"B=[{percentage}]",
            "Result percentage wrappers returned borrowed text or changed the supplied templates");
    }

    void check_result_percentage_resources(Variants& variants, World& world, const Presentation& defaults)
    {
        const auto add_font = [&](const std::string& name, int marker_width, int marker_height, int eight_width = 8,
            float marker_x = 0.0f, float marker_y = 0.0f, bool zero_numeric = false, char32_t omitted = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == omitted) { continue; }
                const bool numeric = (character >= U'0' && character <= U'9') || character == U'.' || character == U'%';
                const int width = character == U'~' ? marker_width : zero_numeric && numeric ? 0 : character == U'8' ? eight_width : 8;
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, width, character == U'~' ? marker_height : 16),
                    character == U'~' ? marker_x : 0.0f, character == U'~' ? marker_y : 0.0f, 0.0f });
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f); font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        add_font("menu_percentage_normal", 8, 16);
        add_font("menu_percentage_solo_edge", 1164, 16, 40); add_font("menu_percentage_solo_over", 1164, 16, 40, 0.25f);
        add_font("menu_percentage_team_edge", 826, 16, 40); add_font("menu_percentage_team_over", 826, 16, 40, 0.25f);
        add_font("menu_percentage_solo_bottom", 8, 715); add_font("menu_percentage_solo_bottom_over", 8, 715, 8, 0.0f, 0.25f);
        add_font("menu_percentage_team_bottom", 8, 452); add_font("menu_percentage_team_bottom_over", 8, 452, 8, 0.0f, 0.25f);
        add_font("menu_percentage_monitor_partial_zero", 0, 0);
        add_font("menu_percentage_monitor_zero_numbers", 8, 16, 8, 0.0f, 0.0f, true);
        add_font("menu_percentage_monitor_huge", 10000, 10000);
        add_font("menu_percentage_monitor_infinite", 8, 16, 8, std::numeric_limits<float>::infinity());
        add_font("menu_percentage_monitor_missing9", 8, 16, 8, 0.0f, 0.0f, false, U'9');
        const auto select = [](rapidjson::Document& d, const ResultPercentageText& entry, const char* font_name)
        {
            d["menu_style"]["fonts"][entry.field == &MenuResultsText::solo_coverage ? "item" : "detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"][entry.key].SetString("~{percentage}");
            d["opening"]["percent_font"].SetString("menu_percentage_normal");
        };
        for (const auto& entry : result_percentage_text)
        {
            const bool solo = entry.field == &MenuResultsText::solo_coverage;
            const bool right = entry.field == &MenuResultsText::team_b_percentage;
            const std::string field = std::string("results.") + entry.key;
            const auto edge = variants.load([&](rapidjson::Document& d)
                { select(d, entry, solo ? "menu_percentage_solo_edge" : "menu_percentage_team_edge"); });
            edge.validate_resources(world.render, world.audio);
            const auto font_name = solo ? edge.menu().item_font : edge.menu().detail_font;
            const auto handle = world.render.resolve_sprite_font(font_name);
            float maximum = 0.0f;
            for (int tenth = 0; tenth <= 1000; ++tenth)
            {
                const std::wstring literal = L"~" + std::to_wstring(tenth / 10) + L'.' + std::to_wstring(tenth % 10) + L'%';
                maximum = std::max(maximum, world.render.measure_text(handle, literal).x);
            }
            const float width = solo ? 1300.0f : 962.0f;
            require(maximum == width && world.render.measure_text(handle, L"~88.8%") == mattmath::Vector2F(width, 20.0f) &&
                world.render.measure_text(handle, L"~100.0%").x == width - 88.0f,
                "Reference percentage geometry must include the wider earlier string, rather than assume100.0% is widest");
            const auto& look = edge.results_menu();
            require(look.box_size == mattmath::Vector2F(1300.0f, 860.0f) && look.fill_box_size == mattmath::Vector2F(700.0f, 120.0f) &&
                look.fill_box_y_offset == 380.0f && look.team_fill_border == mattmath::Vector2F(10.0f) && look.art.focus_padding == mattmath::Vector2F(28.0f, 18.0f),
                "Percentage boundary fixture changed the actual fill anchors, full paper or single plate padding");
            const mattmath::Vector2F position = solo ? mattmath::Vector2F(310.0f, 255.0f) :
                right ? mattmath::Vector2F(338.0f, 500.0f) : mattmath::Vector2F(620.0f, 500.0f);
            std::wstring source = (edge.menu_text().results.*entry.line)(L"88.8%");
            colourwars::UiTextDropShadow widget(entry.key, source, font_name, position, &world.render, look.art.ink, Colour{}, mattmath::Vector2F::ZERO, true);
            source.clear();
            require(widget.text() == L"~88.8%" && widget.bounds() == mattmath::RectangleF(position, { width, 20.0f }) &&
                widget.shadow_offset() == mattmath::Vector2F::ZERO && widget.shadow_color() == Colour{},
                "Percentage widget lost full unscaled text, captured copy, actual anchor or zero shadow");
            if (!solo)
            {
                auto plate = widget.bounds(); plate.inflate(look.art.focus_padding);
                require(plate == mattmath::RectangleF(right ? 310.0f : 592.0f, 482.0f, 1018.0f, 56.0f),
                    "Actual left/right percentage plate must meet its paper edge after exactly one padding inflation");
            }
            DrawList no_device; widget.draw(no_device);
            variants.rejects(field, [&](rapidjson::Document& d)
                { select(d, entry, solo ? "menu_percentage_solo_over" : "menu_percentage_team_over"); }, &world);
            const auto bottom = variants.load([&](rapidjson::Document& d)
                { select(d, entry, solo ? "menu_percentage_solo_bottom" : "menu_percentage_team_bottom"); });
            bottom.validate_resources(world.render, world.audio);
            const auto bottom_font = world.render.resolve_sprite_font(solo ? bottom.menu().item_font : bottom.menu().detail_font);
            require(world.render.measure_text(bottom_font, L"~100.0%") == mattmath::Vector2F(56.0f, solo ? 715.0f : 452.0f) &&
                (solo ? 255.0f + 715.0f : 500.0f + 452.0f + 18.0f) == 970.0f,
                "Percentage height boundary must use its full solo paper or the actual percentage plate padding");
            variants.rejects(field, [&](rapidjson::Document& d)
                { select(d, entry, solo ? "menu_percentage_solo_bottom_over" : "menu_percentage_team_bottom_over"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d) { select(d, entry, "menu_text_limited"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            { const std::string text = std::string(512, 'W') + "{percentage}";
                d["menu_text"]["results"][entry.key].SetString(text.c_str(), d.GetAllocator()); }, &world);
            Presentation mutated = defaults; const_cast<MenuText&>(mutated.menu_text()).results.*entry.field = L"{per{percentage}centage}";
            bool contextual = false;
            try { mutated.validate_resources(world.render, world.audio); }
            catch (const std::runtime_error& error)
            { contextual = std::string(error.what()).find(std::string("menu_text.results.") + entry.key) != std::string::npos; }
            require(contextual, "Direct malformed percentage template escaped resource validation or lost its own role");
        }
        const auto monitor = [](rapidjson::Document& d, const ResultPercentageText& entry, const char* font_name, const char* text = "~{percentage}")
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_percentage_normal");
            d["opening"]["percent_font"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["results"][entry.key].SetString(text, d.GetAllocator());
        };
        for (size_t index = 1; index < std::size(result_percentage_text); ++index)
        {
            const auto& entry = result_percentage_text[index]; const std::string field = std::string("results.") + entry.key + " at monitor";
            variants.rejects(field, [&](rapidjson::Document& d) { monitor(d, entry, "menu_text_limited"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d) { monitor(d, entry, "menu_percentage_monitor_infinite"); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d) { monitor(d, entry, "menu_percentage_monitor_zero_numbers", "{percentage}"); }, &world);
            for (const char* name : { "menu_percentage_monitor_partial_zero", "menu_percentage_monitor_zero_numbers", "menu_percentage_monitor_huge" })
            {
                const auto composed = variants.load([&](rapidjson::Document& d) { monitor(d, entry, name); });
                composed.validate_resources(world.render, world.audio);
                const auto font = world.render.resolve_sprite_font(composed.opening().percent_font);
                const auto size = world.render.measure_text(font, L"~100.0%");
                require(size.x > 0.0f && size.y > 0.0f, "Monitor validation must measure a complete reference output");
                if (std::string(name) == "menu_percentage_monitor_partial_zero")
                    require(world.render.measure_text(font, L"~").x == 0.0f && size.x == 48.0f,
                        "An empty-width literal fragment must be allowed when its inserted monitor percentage is visible");
                if (std::string(name) == "menu_percentage_monitor_zero_numbers")
                    require(world.render.measure_text(font, L"100.0%").x == 0.0f && size.x == 8.0f,
                        "A visible literal must rescue an empty-width numeric fragment in the composed monitor output");
                if (std::string(name) == "menu_percentage_monitor_huge")
                    require(size.x > 1920.0f && size.y > 1080.0f,
                        "This monitor fixture must prove that existing RevealArt fitting, rather than new paper containment, remains authoritative");
            }
        }
        const auto missing = world.render.font(world.render.resolve_sprite_font("menu_percentage_monitor_missing9"));
        require(missing->contains(U'?') && !missing->contains(U'9') && missing->measure(L"A 0.9%").x > 0.0f,
            "Monitor generated-digit fixture must retain a working stand-in while genuinely omitting a later reference digit");
        variants.rejects("results.team_a_percentage at monitor", [](rapidjson::Document& d)
            { d["opening"]["percent_font"].SetString("menu_percentage_monitor_missing9"); }, &world);
        variants.rejects("results.competitive_draw", [](rapidjson::Document& d)
        {
            const std::string text(512, 'W'); d["menu_text"]["results"]["competitive_draw"].SetString(text.c_str(), d.GetAllocator());
            d["menu_text"]["results"]["solo_coverage"].SetString((text + "{percentage}").c_str(), d.GetAllocator());
        }, &world);
    }
}
