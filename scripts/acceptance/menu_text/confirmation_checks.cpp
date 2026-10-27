#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Confirmation prompts and the display trial. check_confirmation_bounds
// registers menu_confirmation_width_overflow for check_display_trial_text.
namespace menu_text_acceptance
{
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
            // Keep independent Weapon bodies clear of the compact label badges.
            d["main_menu"]["fonts"]["weapon_description"].SetString("menu_text_exact");
            for (const auto& entry : confirmation_text) { d["menu_text"]["confirmations"][entry.key].SetString("Fit"); }
            d["menu_text"]["confirmations"][key].SetString("~");
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& pause = original.pause_menu();
        require(pause.box_size.x == 1050.0f && pause.box_size.y == 900.0f,
            "Confirmation paper fixtures no longer exercise the shipped full-height Pause paper");
        add_font("menu_confirmation_width_edge", 1050, 20);
        add_font("menu_confirmation_width_overflow", 1051, 20);
        // The shared font's unrelated two-line missing list must also fit;
        // retain each target '~' height while compacting only line spacing.
        add_font("menu_confirmation_action_edge", 8, 20, 0.0f, 0.0f, 1.0f);
        add_font("menu_confirmation_display_action_edge", 8, 40, 0.0f, 0.0f, 1.0f);
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
        // The shipped item face keeps its descenders inside the line spacing,
        // so a face whose g/j/p/q/y hang below it makes Keep the one tall row.
        std::vector<Glyph> descender_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool descender = character == U'g' || character == U'j' || character == U'p' ||
                character == U'q' || character == U'y';
            descender_glyphs.push_back({ character, mattmath::RectangleI(0, 0, 8, descender ? 82 : 60),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_text_descender",
            std::make_unique<Font>(TextureHandle{}, std::move(descender_glyphs), 74.21875f));
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto keep_size = world.render.measure_text(world.render.resolve_sprite_font("menu_text_descender"),
            original.menu_text().item(MenuItem::keep));
        const auto tight_display = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_descender");
            d["pause_menu"]["layout"]["spacing"]["y"].SetFloat(115.0f);
            d["menu_text"]["items"]["options"].SetString("Fit");
            // Mode's rows sit 96 apart, too close for descender-tall labels;
            // only Keep's descender is under test here.
            for (const char* key : { "single_match", "match_length", "series_stage" })
            {
                d["menu_text"]["items"][key].SetString("Fit");
            }
        });
        tight_display.validate_resources(world.render, world.audio);
        require(keep_size.y == 82.0f && keep_size.y + tight_display.pause_menu().art.focus_padding.y * 2.0f >
            tight_display.pause_menu().layout.spacing.y && keep_size.y + tight_display.main_menu().setup_focus_padding.y * 2.0f <=
            tight_display.pause_menu().layout.spacing.y,
            "Keep's descender no longer distinguishes compact Display padding from ordinary Pause padding in a tight row");
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
}
