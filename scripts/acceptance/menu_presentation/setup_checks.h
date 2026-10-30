#pragma once

// Setup paper checks: the Players/Join setup panel and its geometry. Included
// by menu_presentation.cpp after its shared helpers (require,
// short_weapon_descriptions, World, Variants). check_setup_geometry registers
// the setup_exact and join_heading_tall fonts on World, and the Team, Weapon
// and Stage paper checks in paper_checks.h use them, so main must run these
// first.
namespace
{
    void check_setup_panel(Variants& variants, World& world)
    {
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_box_size"); });
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"].SetArray(); });
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"].SetNull(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"].RemoveMember(axis); });
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"][axis].SetBool(true); });
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"][axis].SetString("920"); });
            for (const double invalid : { 0.0, -1.0, 1e100 })
            {
                variants.rejects("main_menu", "setup_box_size", [axis, invalid](rapidjson::Document& d)
                    { d["main_menu"]["setup_box_size"][axis].SetDouble(invalid); });
            }
            const float maximum = std::string(axis) == "x" ? 1760.0f : 920.0f;
            variants.rejects("main_menu", "setup_box_size", [axis, maximum](rapidjson::Document& d)
            {
                d["main_menu"]["setup_box_size"][axis].SetFloat(std::nextafter(maximum,
                    std::numeric_limits<float>::infinity()));
            });
        }
        const auto custom = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_box_size"]["x"].SetInt(1720);
            d["main_menu"]["setup_box_size"]["y"].SetInt(900);
            d["menu_style"]["layout"]["position"]["x"].SetInt(170);
        });
        custom.validate_resources(world.render, world.audio);
        require(custom.main_menu().setup_box_size == Vector2F(1720.0f, 900.0f),
            "Custom setup paper size was not loaded exactly");
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetInt(1000);
            d["main_menu"]["setup_box_size"]["y"].SetInt(960);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require((Vector2F(1920.0f, footer_boundary.menu().footer_y) -
            footer_boundary.main_menu().setup_box_size) / 2.0f == Vector2F(80.0f, 20.0f),
            "Setup paper no longer derives its vertical center from the authored footer");
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["setup_box_size"]["y"].SetInt(960);
        });
        variants.rejects("main_menu", "setup_focus_padding", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_focus_padding"); });
        variants.rejects("main_menu", "setup_focus_padding", [](rapidjson::Document& d)
            { d["main_menu"]["setup_focus_padding"].SetArray(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("main_menu", "setup_focus_padding", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"].RemoveMember(axis); });
            variants.rejects("main_menu", "setup_focus_padding", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"][axis].SetBool(false); });
            for (const double invalid : { 0.0, -1.0, 6.0, 1e100 })
            {
                variants.rejects("main_menu", "setup_focus_padding", [axis, invalid](rapidjson::Document& d)
                    { d["main_menu"]["setup_focus_padding"][axis].SetDouble(invalid); });
            }
            const float limit = std::string(axis) == "x" ? 48.0f : 24.0f;
            variants.rejects("main_menu", "setup_focus_padding", [axis, limit](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"][axis].SetFloat(limit + 0.25f); });
        }
        const auto padding_boundary = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_focus_padding"]["x"].SetInt(48);
            d["main_menu"]["setup_focus_padding"]["y"].SetInt(24);
        });
        require(padding_boundary.main_menu().setup_focus_padding == Vector2F(48.0f, 24.0f),
            "Inclusive setup focus-padding definition limits were rejected");
        // The setup pages draw with the setup shade, so each setup box must
        // exceed it plus the keyline edge, as art's box check does for its own
        // shade: 16 + 4 * 6 = 40 here, while art's 8 + 24 = 32 already passes.
        const auto setup_shade = [](float width)
        {
            return [width](rapidjson::Document& d)
            {
                d["main_menu"]["setup_shadow_offset"].SetFloat(16.0f);
                d["main_menu"]["setup_focus_padding"]["y"].SetFloat(24.0f);
                d["main_menu"]["team_box_size"]["x"].SetFloat(width);
            };
        };
        require(variants.load(setup_shade(40.25f)).main_menu().team_box_size.x == 40.25f,
            "A setup box just past the setup shade and keyline edge was rejected");
        variants.rejects("main_menu", "setup_shadow_offset", setup_shade(40.0f));
        const auto padding_minimum = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_focus_padding"]["x"].SetFloat(6.25f);
            d["main_menu"]["setup_focus_padding"]["y"].SetFloat(6.25f);
        });
        require(padding_minimum.main_menu().setup_focus_padding == Vector2F(6.25f, 6.25f),
            "Setup focus padding just above the shade-plus-keyline threshold was rejected");
        variants.rejects("main_menu", "setup_shadow_offset", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_shadow_offset"); });
        variants.rejects("main_menu", "setup_shadow_offset", [](rapidjson::Document& d)
            { d["main_menu"]["setup_shadow_offset"].SetBool(false); });
        for (const double invalid : { -0.25, 24.25, 1e100 })
        {
            variants.rejects("main_menu", "setup_shadow_offset", [invalid](rapidjson::Document& d)
                { d["main_menu"]["setup_shadow_offset"].SetDouble(invalid); });
        }
        const auto no_shade = variants.load([](rapidjson::Document& d)
            { d["main_menu"]["setup_shadow_offset"].SetFloat(0.0f); });
        no_shade.validate_resources(world.render, world.audio);
        require(no_shade.main_menu().setup_shadow_offset == 0.0f,
            "Inclusive zero setup-shadow limit was rejected");
    }

    void check_setup_geometry(Variants& variants, World& world)
    {
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("setup_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("setup_exact");
            }
            for (auto member = d["menu_text"]["items"].MemberBegin(); member != d["menu_text"]["items"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
        };
        const auto check_edge = [&](const char* owner, const char* field, auto edge, auto outside)
        {
            const auto fits = variants.load([&](rapidjson::Document& d) { exact(d); edge(d); });
            fits.validate_resources(world.render, world.audio);
            variants.rejects(owner, field, [&](rapidjson::Document& d)
            {
                exact(d);
                edge(d);
                outside(d);
            }, &world);
        };
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"]["x"].SetFloat(1756.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["setup_box_size"]["x"].SetFloat(1755.75f); });
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["menu_style"]["layout"]["position"]["x"].SetFloat(148.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["x"].SetFloat(147.75f); });
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
        {
            d["menu_style"]["layout"]["position"]["x"].SetFloat(149.5f);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
        }, [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["x"].SetFloat(149.25f); });
        check_edge("menu_text.headings", "mode", [](rapidjson::Document& d)
            { d["menu_style"]["layout"]["position"]["y"].SetFloat(32.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["y"].SetFloat(31.75f); });
        // Single Match's card starts at 150 + 2*96 - 7 = 335.
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["menu_style"]["shadow"]["heading_offset"]["y"].SetFloat(165.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["shadow"]["heading_offset"]["y"].SetFloat(165.25f); });
        check_edge("menu_text", "solo_practice_hint", [](rapidjson::Document& d)
        {
            // The hint follows Single Match's card by one card padding
            // (ROAD#20): x + 1528 + 1.5 + 28 + 2 + 28 + 25.5 = 1770, the
            // right margin, when the shared origin x is 157.
            d["menu_text"]["items"]["single_match"].SetString(std::string(191, 'F').c_str(), d.GetAllocator());
            d["menu_style"]["layout"]["position"]["x"].SetFloat(157.0f);
        }, [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["x"].SetFloat(157.25f); });
        // Join shares Count's spacing and adds a status at row seven. Its
        // complete 20-high text plus one-unit shadow reaches the inset bottom
        // at 65 + 7*120 + 21 = 926. A Count Back card at that bottom would
        // leave no space for this later status, so retain its old overflow
        // rejection separately instead of calling that layout a valid page.
        check_edge("menu_text.connection", "join_ready", [](rapidjson::Document& d)
        {
            d["menu_style"]["layout"]["position"]["y"].SetFloat(65.0f);
            d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.25f); });
        variants.rejects("menu_text.items", "back", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["layout"]["position"]["y"].SetFloat(175.5f);
            d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.25f);
        }, &world);
        for (const char* spacing : { "mode_select_spacing", "player_count_spacing" })
        {
            const char* field = std::string(spacing) == "mode_select_spacing" ? "single_match" : "one_player_practice";
            check_edge("menu_text.items", field, [spacing](rapidjson::Document& d)
                { d["main_menu"][spacing]["y"].SetFloat(37.5f); },
                [spacing](rapidjson::Document& d) { d["main_menu"][spacing]["y"].SetFloat(37.25f); });
        }
        variants.rejects("menu_text.items", "match_length", [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(900.0f);
        }, &world);

        // Keep Mode/Count and their detached arrows comfortably inside this
        // narrower shared paper. Only Join's authored heading reaches its edge;
        // both widths still fit the old generic design-area/right-margin check.
        check_edge("menu_text.headings", "join_players", [](rapidjson::Document& d)
        {
            short_weapon_descriptions(d);
            d["main_menu"]["setup_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["join_players"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        }, [](rapidjson::Document& d)
        {
            d["menu_text"]["headings"]["join_players"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        });

        std::vector<Glyph> join_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            join_glyphs.push_back({ character, RectangleI(0, 0, 8, character == U'~' ? 780 : 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("join_heading_tall", std::make_unique<Font>(TextureHandle{}, std::move(join_glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("join_heading_tall"), L"~") ==
            Vector2F(8.0f, 780.0f), "Join paper fixture did not retain its tall glyph beyond the font's line spacing");
        const auto ordinary_join = variants.load([&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
        });
        ordinary_join.validate_resources(world.render, world.audio);
        // A glyph taller than line spacing still fits the generic footer area,
        // but its complete shadow bounds cross Join's paper at y=926.
        variants.rejects("menu_text.headings", "join_players", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["join_players"].SetString("~");
        }, &world);
    }
}
