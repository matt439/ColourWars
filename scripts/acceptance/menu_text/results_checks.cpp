#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Results: plain roles, session outcomes, competitive winners and
// contribution teams.
namespace menu_text_acceptance
{
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
        // Only the solo template is required here; competitive copy stays empty.
        MenuResultsText solo_only; solo_only.solo_coverage = defaults.results.solo_coverage;
        for (const bool team_a_played : { false, true })
        {
            for (const auto& example : solo)
            {
                LevelEndInfo result; result.team_a_area = example.team_a; result.team_b_area = example.team_b; result.total_area = example.total;
                result.team_a_played = team_a_played; result.team_b_played = !team_a_played;
                check(result, example.expected, example.expected);
                require(result.winning_team_string(solo_only) == example.expected,
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
                require(result.winning_team_string(solo_only) == expected, "Nonfinite solo coverage was sanitized or read competitive copy");
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
            d["results_menu"]["fill"]["box_size"]["x"].SetFloat(1264.0f);
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
    // ROAD#12: the four award lines are required, plain single-line copy that
    // must fit the Results panel at the end of every contribution row.
    void check_award_text(Variants& variants, World& world, const MenuText& defaults)
    {
        struct AwardText { const char* key; MatchAward award; const wchar_t* text; };
        constexpr AwardText awards[]{
            { "award_paint_laid", MatchAward::paint_laid, L"Most paint laid" },
            { "award_recaptured", MatchAward::recaptured, L"Most retaken" },
            { "award_airtime", MatchAward::airtime, L"Best airtime" },
            { "award_splats", MatchAward::splats, L"Most splats" }
        };
        for (const AwardText& entry : awards)
        {
            const std::string field = std::string("results.") + entry.key;
            require(defaults.results.award_line(entry.award) == entry.text, "Award default changed: " + field);
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"].RemoveMember(entry.key); });
            variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetInt(3); });
            for (const char* invalid : { "", "   ", "line\nbreak" })
                variants.rejects(field, [&](rapidjson::Document& d) { d["menu_text"]["results"][entry.key].SetString(invalid, d.GetAllocator()); });
            const auto custom = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("Top"); });
            custom.validate_resources(world.render, world.audio);
            require(custom.menu_text().results.award_line(entry.award) == L"Top", "Award copy was not read: " + field);
            const std::string wide(120, 'W');
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString(wide.c_str(), d.GetAllocator()); }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["results"][entry.key].SetString("Most \xE2\x98\x85", d.GetAllocator()); });
        }
    }
}
