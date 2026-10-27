#pragma once

// Team, Weapon and Stage page paper checks, with check_vector_definition, which
// only they use. Included by menu_presentation.cpp after setup_checks.h. They
// validate against the setup_exact and join_heading_tall fonts that
// check_setup_geometry registers on World, so main runs them after it.
namespace
{
    template<class Container>
    void check_vector_definition(Variants& variants, const char* owner, const char* key, Container container, bool positive)
    {
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d).RemoveMember(key); });
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetNull(); });
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetArray(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].RemoveMember(axis); });
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetBool(false); });
            for (const double invalid : { -0.25, 1e100 })
            {
                variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetDouble(invalid); });
            }
            if (positive)
            {
                variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetInt(0); });
            }
        }
    }

    void check_team_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "team_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* key : { "warning_position", "spacing", "size" })
        {
            check_vector_definition(variants, "main_menu.team_select", key, [](rapidjson::Document& d) -> rapidjson::Value&
                { return d["main_menu"]["team_select"]; }, std::string(key) != "warning_position");
        }
        for (const char* axis : { "x", "y" })
        {
            const float panel_limit = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "team_box_size", [axis, panel_limit](rapidjson::Document& d)
                { d["main_menu"]["team_box_size"][axis].SetFloat(panel_limit + 0.25f); });
            const float position_limit = std::string(axis) == "x" ? 1920.0f : 960.0f;
            variants.rejects("main_menu.team_select", "warning_position", [axis, position_limit](rapidjson::Document& d)
                { d["main_menu"]["team_select"]["warning_position"][axis].SetFloat(position_limit + 0.25f); });
        }
        const auto warning_definition_edges = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1920.0f);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(960.0f);
        });
        require(warning_definition_edges.main_menu().team_warning_position == Vector2F(1920.0f, 960.0f),
            "Inclusive Team warning position definition limits were rejected");
        const auto warning_definition_origin = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(0.0f);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(0.0f);
        });
        require(warning_definition_origin.main_menu().team_warning_position == Vector2F::ZERO,
            "Nonnegative Team warning position definitions rejected zero before resource bounds were checked");
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["team_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().team_box_size == Vector2F(1840.0f, 960.0f),
            "Team panel height did not follow its authored footer boundary");
        variants.rejects("main_menu", "team_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["team_box_size"]["y"].SetFloat(960.0f);
        });

        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("setup_exact");
            }
            for (const char* group : { "headings", "items" })
            {
                for (auto member = d["menu_text"][group].MemberBegin(); member != d["menu_text"][group].MemberEnd(); ++member)
                {
                    member->value.SetString("Fit");
                }
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
        // The complete fixed warning is 192x20 in this font; its retained
        // one-pixel shadow must stay inside Team's (52,32)..(1866,926) interior.
        require(world.render.measure_text(world.render.resolve_sprite_font("setup_exact"), L"Both teams need a player") ==
            Vector2F(192.0f, 20.0f), "Team warning fixture no longer measures the complete shipped wording");
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(52.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(51.75f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1673.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1673.25f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(905.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(905.25f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
        {
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.0f);
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(53.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(52.75f); });
        variants.rejects("main_menu.team_select", "warning_position", [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(100.0f);
        }, &world);
        check_edge("menu_text.headings", "teams", [](rapidjson::Document& d)
        {
            short_weapon_descriptions(d);
            d["main_menu"]["team_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(500.0f);
            d["menu_text"]["headings"]["teams"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        }, [](rapidjson::Document& d)
        {
            d["menu_text"]["headings"]["teams"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        });
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, RectangleI(0, 0, 8, character == U'y' ? 70 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("team_warning_descender", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("team_warning_descender"), L"Both teams need a player") ==
            Vector2F(192.0f, 70.0f), "Team warning did not measure its descender beyond the font's line spacing");
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("team_warning_descender");
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(855.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(855.25f); });
    }

    void check_weapon_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "weapon_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* key : { "spacing", "size" })
        {
            check_vector_definition(variants, "main_menu.weapon_select", key, [](rapidjson::Document& d) -> rapidjson::Value&
                { return d["main_menu"]["weapon_select"]; }, true);
        }
        for (const char* axis : { "x", "y" })
        {
            const float maximum = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "weapon_box_size", [axis, maximum](rapidjson::Document& d)
                { d["main_menu"]["weapon_box_size"][axis].SetFloat(maximum + 0.25f); });
        }
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["weapon_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().weapon_box_size == Vector2F(1840.0f, 960.0f),
            "Weapon paper height did not follow its authored footer boundary");
        variants.rejects("main_menu", "weapon_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["weapon_box_size"]["y"].SetFloat(960.0f);
        });
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("setup_exact");
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
        };
        const auto width_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            short_weapon_descriptions(d);
            d["main_menu"]["weapon_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            // Moving the shared origin also moves Join's unfitted legend.
            // Keep it short so Weapon's heading owns this paper-edge case.
            d["menu_text"]["connection"]["join_waiting"].SetString("Fit");
            short_worn_join_rows(d);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["weapons"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        };
        const auto width = variants.load(width_edge);
        width.validate_resources(world.render, world.audio);
        // Generic design bounds and the wider Setup/Team papers still accept
        // this text; the new Weapon paper alone owns the exact right edge.
        variants.rejects("menu_text.headings", "weapons", [&](rapidjson::Document& d)
        {
            width_edge(d);
            d["menu_text"]["headings"]["weapons"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("menu_text.headings", "weapons", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["weapons"].SetString("~");
        }, &world);
    }

    void check_stage_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "stage_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* axis : { "x", "y" })
        {
            const float maximum = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "stage_box_size", [axis, maximum](rapidjson::Document& d)
                { d["main_menu"]["stage_box_size"][axis].SetFloat(maximum + 0.25f); });
        }
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"].RemoveMember("selected_ink"); });
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"]["selected_ink"].SetBool(true); });
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"]["selected_ink"].SetNull(); });
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["stage_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().stage_box_size == Vector2F(1840.0f, 960.0f),
            "Stage paper height did not follow its authored footer boundary");
        variants.rejects("main_menu", "stage_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["stage_box_size"]["y"].SetFloat(960.0f);
        });
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("setup_exact");
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
        };
        const auto width_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            short_weapon_descriptions(d);
            d["main_menu"]["stage_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            // The shared origin must not make Join's separate legend the
            // failing role in this Stage heading boundary fixture.
            d["menu_text"]["connection"]["join_waiting"].SetString("Fit");
            short_worn_join_rows(d);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["arena"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        };
        const auto width = variants.load(width_edge);
        UiTextDropShadow heading("legacy_stage_heading", std::wstring(130, L'F'), "setup_exact",
            width.menu().layout.position, &world.render, Colour::black, width.menu().shadow_colour,
            width.menu().heading_shadow_offset);
        require(heading.bounds().right() == 1546.0f &&
            (1920.0f + width.main_menu().stage_box_size.x) / 2.0f - 14.0f == 1546.0f,
            "Legacy Stage heading no longer touches the exact 1200-wide paper's inclusive right edge");
        // The heading contact remains exact, but that narrow paper cannot
        // contain the authored briefing card required by the new summaries.
        variants.rejects("menu_text.stage", "best_of_mode", width_edge, &world);
        // Retain the original heading overflow and tall-glyph diagnostics.
        variants.rejects("menu_text.headings", "arena", [&](rapidjson::Document& d)
        {
            width_edge(d);
            d["menu_text"]["headings"]["arena"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("menu_text.headings", "arena", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["arena"].SetString("~");
        }, &world);
    }
}
