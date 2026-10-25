#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Menu items, Restore defaults, the setup pages and the Pause menu.
namespace menu_text_acceptance
{
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
            original.menu().item_font == "lato_black_48",
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
        require(pause.layout.spacing.y == 120.0f && options_size.y == 77.0f &&
            options_height <= pause.layout.spacing.y - 2.0f,
            "Pause Options lost its measured height or the authored two-pixel minimum row clearance");
        const auto tight_options = variants.load([&](rapidjson::Document& d)
            { d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(options_height); });
        tight_options.validate_resources(world.render, world.audio);
        // No shipped Pause item has ink below the line spacing, so every row
        // is exactly options_height and the first row is the one rejected.
        variants.rejects("items.resume", [&](rapidjson::Document& d)
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
}
