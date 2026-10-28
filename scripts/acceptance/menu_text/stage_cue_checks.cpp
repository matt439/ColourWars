#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The arena picker's navigation cues and status.
namespace menu_text_acceptance
{
    void check_stage_cue_text(Variants& variants, World& world)
    {
        for (const auto& entry : stage_cue_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(entry.hint ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ", d.GetAllocator());
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().stage.*entry.field == (entry.hint ? L" A -B \n C " : L" A -B "),
                std::string("Arena cue lost authored spacing/LF or normalized punctuation: ") + entry.key);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (entry.hint)
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
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                }, &world);
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (entry.hint) { d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_limited"); }
                else { d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited"); }
                d["menu_text"]["stage"][entry.key].SetString("Glyph~");
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            for (const auto& entry : stage_cue_text)
            {
                MenuStyle style = look.menu();
                StageSelectPresentation stage = look.main_menu().stage_select;
                const auto font_name = entry.hint ? style.brand_font : style.detail_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const auto point = entry.hint ? stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f :
                    stage.navigation_position;
                const auto captured_shadow = style.detail_shadow_offset;
                std::wstring source = look.menu_text().stage.*entry.field;
                const auto retained = source;
                const auto measured = world.render.measure_text(font, source);
                UiTextDropShadow text(entry.key, L"", font_name, point, &world.render,
                    style.unhighlight, style.shadow_colour, captured_shadow);
                text.set_text(source);
                source = L"Changed after set_text";
                style.brand_font = "lato_black_48";
                style.detail_font = "lato_black_48";
                stage.navigation_position += mattmath::Vector2F(40.0f, 40.0f);
                stage.briefing.position += mattmath::Vector2F(40.0f, 40.0f);
                if (!entry.hint)
                {
                    style.detail_shadow_offset = mattmath::Vector2F(10.0f, 10.0f);
                    text.set_text(retained);
                    require(text.text() == retained && text.text_bounds() == mattmath::RectangleF(point, measured) &&
                        text.bounds() == mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured),
                            mattmath::RectangleF(point + captured_shadow, measured)) &&
                        text.shadow_offset() == captured_shadow && text.shadow_scale() == 1.0f,
                        "Actual arena navigation lost its copied text, captured font/position/shadow or unscaled bounds");
                }
                else
                {
                    const float original_budget = stage.briefing.size.x - stage.briefing_padding.x * 2.0f;
                    const auto fit = [&]()
                    {
                        const auto raw = mattmath::RectangleF::union_of(text.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                            text.text_bounds_at(style.detail_shadow_offset, 1.0f));
                        const float budget = stage.briefing.size.x - stage.briefing_padding.x * 2.0f;
                        const float scale = std::min(1.0f, budget / raw.width);
                        text.set_scale(scale);
                        text.set_shadow_scale(scale);
                        text.set_shadow_offset(style.detail_shadow_offset * scale);
                        return scale;
                    };
                    for (const bool changed_fit : { false, true })
                    {
                        if (changed_fit)
                        {
                            stage.briefing.size.x = stage.briefing_padding.x * 2.0f + original_budget / 2.0f;
                            style.detail_shadow_offset = mattmath::Vector2F(-1.5f, -1.5f);
                        }
                        text.set_text(retained);
                        const float scale = fit();
                        const auto expected = mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured * scale),
                            mattmath::RectangleF(point + style.detail_shadow_offset * scale, measured * scale));
                        const auto actual = text.bounds();
                        require(text.text() == retained && std::abs(actual.x - expected.x) <= 0.01f &&
                            std::abs(actual.y - expected.y) <= 0.01f && std::abs(actual.width - expected.width) <= 0.01f &&
                            std::abs(actual.height - expected.height) <= 0.01f && text.shadow_scale() == scale &&
                            text.shadow_offset() == style.detail_shadow_offset * scale && scale > 0.0f && scale <= 1.0f &&
                            actual.width <= stage.briefing.size.x - stage.briefing_padding.x * 2.0f + 0.01f,
                            "Actual arena hint lost its captured text/font/position or live width-only shadow fitting");
                    }
                    std::wstring replacement = L"Fit";
                    text.set_text(replacement);
                    replacement = L"Changed after replacement";
                    require(fit() == 1.0f && text.text() == L"Fit" && text.shadow_scale() == 1.0f &&
                        text.shadow_offset() == style.detail_shadow_offset &&
                        text.text_bounds() == mattmath::RectangleF(point, world.render.measure_text(font, L"Fit")),
                        "A short arena hint retained its old shrink scale, lost its copied text or used a replacement font/position");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const bool wide : { false, true })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"]["random_navigation"].SetString("Fit");
                for (const char* key : { "locked_hint", "browse_hint", "user_hint" })
                {
                    d["menu_text"]["stage"][key].SetString(wide ? std::string(200, 'W').c_str() : "Fit", d.GetAllocator());
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
        const auto multiline = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            for (const char* key : { "locked_hint", "browse_hint", "user_hint" })
            {
                d["menu_text"]["stage"][key].SetString("Fit\nFit\nFit\nFit\nFit");
            }
        });
        multiline.validate_resources(world.render, world.audio);
        check_widgets(multiline);
    }

    void check_stage_cue_bounds(Variants& variants, World& world)
    {
        const auto exact_navigation = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            d["menu_text"]["stage"]["random_navigation"].SetString("Fit");
        };
        struct Edge { const char* axis; float position; float outside; };
        const Edge nav_edges[]{ { "x", 0.0f, -0.25f }, { "x", 1895.0f, 1895.25f },
            { "y", 0.0f, -0.25f }, { "y", 939.0f, 939.25f },
            { "y", 832.0f, 831.75f }, { "x", 1205.0f, 1205.25f } };
        for (const auto& edge : nav_edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact_navigation(d);
                d["main_menu"]["stage_select"]["navigation_position"][edge.axis].SetFloat(edge.position);
            };
            variants.load(contact).validate_resources(world.render, world.audio);
            variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["navigation_position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        const auto panel_top = [&](rapidjson::Document& d)
        {
            exact_navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1230.0f);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(224.0f);
        };
        variants.load(panel_top).validate_resources(world.render, world.audio);
        variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
        {
            panel_top(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(224.25f);
        }, &world);
        const auto negative = [&](rapidjson::Document& d)
        {
            exact_navigation(d);
            for (const char* axis : { "x", "y" })
            {
                d["menu_style"]["shadow"]["detail_offset"][axis].SetFloat(-1.5f);
                d["main_menu"]["stage_select"]["navigation_position"][axis].SetFloat(1.5f);
            }
        };
        variants.load(negative).validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
            {
                negative(d);
                d["main_menu"]["stage_select"]["navigation_position"][axis].SetFloat(1.25f);
            }, &world);
        }
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? width : 8,
                    character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 16.0f));
        };
        const auto use_hint = [](rapidjson::Document& d, const std::string& font, const char* key)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString(font.c_str(), d.GetAllocator());
            d["menu_text"]["stage"]["locked_hint"].SetString("Fit");
            d["menu_text"]["stage"]["browse_hint"].SetString("Fit");
            d["menu_text"]["stage"]["user_hint"].SetString("Fit");
            d["menu_text"]["stage"][key].SetString("~");
        };
        add_font("menu_stage_hint_panel_edge", 951, 223);
        add_font("menu_stage_hint_panel_overflow", 951, 224);
        add_font("menu_stage_hint_footer_edge", 951, 323);
        add_font("menu_stage_hint_footer_overflow", 951, 324);
        for (const char* key : { "locked_hint", "browse_hint", "user_hint" })
        {
            const std::string field = std::string("stage.") + key;
            variants.load([&](rapidjson::Document& d) { use_hint(d, "menu_stage_hint_panel_edge", key); }).validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d) { use_hint(d, "menu_stage_hint_panel_overflow", key); }, &world);
            const auto footer = [&](rapidjson::Document& d)
            {
                use_hint(d, "menu_stage_hint_footer_edge", key);
                d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(800.0f);
            };
            const auto footer_contact = variants.load(footer);
            const auto& footer_stage = footer_contact.main_menu().stage_select;
            UiTextDropShadow hint("legacy_hint_footer", L"~", "menu_stage_hint_footer_edge",
                footer_stage.briefing.position + footer_stage.briefing_padding + footer_stage.briefing.spacing * 7.0f,
                &world.render, Colour::black, footer_contact.menu().shadow_colour,
                footer_contact.menu().detail_shadow_offset);
            hint.set_scale(0.5f);
            hint.set_shadow_scale(0.5f);
            hint.set_shadow_offset(footer_contact.menu().detail_shadow_offset * 0.5f);
            require(hint.bounds().width == 476.0f && hint.bounds().bottom() == 960.0f &&
                footer_stage.briefing.position.y + footer_stage.briefing.size.y == 1045.0f,
                "Legacy width-fitted hint no longer touches its exact footer at the original 800-high panel");
            // The fitted hint still reaches the footer; its backing card now
            // exceeds the outer Stage paper, which the new summary guard rejects.
            variants.rejects("stage.best_of_mode", footer, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                footer(d);
                d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_hint_footer_overflow");
            }, &world);
        }
        const auto hint_left = [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            d["menu_text"]["stage"]["locked_hint"].SetString("Fit");
            d["menu_text"]["stage"]["browse_hint"].SetString("Fit");
            d["menu_text"]["stage"]["user_hint"].SetString("Fit");
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(7.0f);
            d["main_menu"]["stage_select"]["briefing"]["spacing"]["x"].SetFloat(-1.0f);
        };
        variants.load(hint_left).validate_resources(world.render, world.audio);
        variants.rejects("stage.locked_hint", [&](rapidjson::Document& d)
        {
            hint_left(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-0.25f);
        }, &world);
        for (const float padding : { 270.0f, 271.0f })
        {
            variants.rejects("stage.locked_hint", [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
                for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
                d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(padding);
            }, &world);
        }
        for (const char* suffix : { "zero_width", "zero_height", "infinite_width", "infinite_height" })
        {
            for (const auto& entry : stage_cue_text)
            {
                variants.rejects(std::string("stage.") + entry.key, [&](rapidjson::Document& d)
                {
                    if (entry.hint)
                    {
                        use_hint(d, std::string("menu_stage_briefing_") + suffix, entry.key);
                    }
                    else
                    {
                        const std::string font = std::string("menu_stage_status_") + suffix;
                        d["menu_style"]["fonts"]["detail"].SetString(font.c_str(), d.GetAllocator());
                        d["menu_text"]["stage"][entry.key].SetString("~");
                    }
                }, &world);
            }
        }
        for (const char* axis : { "x", "y" })
        {
            for (const double position : { -1e100, 1e100 })
            {
                variants.rejects("stage.random_navigation", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["navigation_position"][axis].SetDouble(position);
                }, &world);
            }
        }
    }

    void check_stage_status_text(Variants& variants, World& world)
    {
        for (const auto& entry : stage_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().stage.*entry.field == L"  A \"B\"-C...  ",
                std::string("Arena status lost normalized punctuation or authored spacing: ") + entry.key);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["stage"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
                d["menu_text"]["stage"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["stage"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            for (const auto& entry : stage_text)
            {
                MenuStyle style = look.menu();
                StageSelectPresentation stage = look.main_menu().stage_select;
                std::wstring source = look.menu_text().stage.*entry.field;
                const auto retained = source;
                const auto font = world.render.resolve_sprite_font(style.detail_font);
                const auto measured = world.render.measure_text(font, source);
                const auto point = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 6.0f;
                const auto shadow = style.detail_shadow_offset;
                UiTextDropShadow status("arena_status", L"", style.detail_font, point, &world.render,
                    style.heading_colour, style.shadow_colour, shadow);
                status.set_text(source);
                source = L"Changed after set_text";
                const auto expected = mattmath::RectangleF::union_of(mattmath::RectangleF(point, measured),
                    mattmath::RectangleF(point + shadow, measured));
                style.detail_font = "lato_black_24";
                style.detail_shadow_offset = mattmath::Vector2F(10.0f, 10.0f);
                stage.briefing.position += mattmath::Vector2F(40.0f, 40.0f);
                status.set_text(retained);
                status.set_colour(look.main_menu().stage_select.selected);
                require(status.text() == retained && status.text_bounds() == mattmath::RectangleF(point, measured) &&
                    status.bounds() == expected && status.shadow_offset() == shadow && status.shadow_scale() == 1.0f &&
                    std::isfinite(expected.width) && std::isfinite(expected.height) && expected.width > 0.0f && expected.height > 0.0f,
                    std::string("Actual arena status lost its copied text, captured font/position/shadow, or unscaled bounds: ") + entry.key);
                status.set_hidden(true);
                DrawList no_device;
                status.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const size_t length : { size_t(3), size_t(59) })
        {
            for (const bool negative : { false, true })
            {
                const auto authored = variants.load([&](rapidjson::Document& d)
                {
                    d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
                    for (const auto& entry : stage_text)
                    {
                        d["menu_text"]["stage"][entry.key].SetString(std::string(length, 'F').c_str(), d.GetAllocator());
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
        }
    }

    void check_stage_status_bounds(Variants& variants, World& world)
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
            for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("Fit"); }
            d["menu_text"]["stage"][key].SetString("~");
        };
        add_font("menu_stage_status_width_edge", 507, 20);
        add_font("menu_stage_status_width_overflow", 508, 20);
        add_font("menu_stage_status_hint_edge", 8, 74);
        add_font("menu_stage_status_hint_overflow", 8, 75);
        for (const auto& entry : stage_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const auto width = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_width_edge", entry.key);
            });
            width.validate_resources(world.render, world.audio);
            const auto& stage = width.main_menu().stage_select;
            require(stage.briefing.position.x + stage.briefing_padding.x + 507.0f + width.menu().detail_shadow_offset.x ==
                stage.briefing.position.x + stage.briefing.size.x,
                "Arena status width fixture no longer touches the full briefing panel's right edge");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_width_overflow", entry.key);
            }, &world);
            const auto hint = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_hint_edge", entry.key);
            });
            hint.validate_resources(world.render, world.audio);
            require(74.0f + hint.menu().detail_shadow_offset.y == hint.main_menu().stage_select.briefing.spacing.y,
                "Arena status height fixture no longer reaches the authored hint row with its complete shadow");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_stage_status_hint_overflow", entry.key);
            }, &world);
        }
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_exact");
            for (const auto& entry : stage_text) { d["menu_text"]["stage"][entry.key].SetString("Fit"); }
            // Keep the other briefing rows valid while these cases isolate
            // status geometry, including a full-design right-edge contact.
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_text_exact");
            for (const auto& entry : stage_briefing_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
            for (const auto& entry : stage_cue_text) { d["menu_text"]["stage"][entry.key].SetString("F"); }
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1800.0f);
        };
        const auto left = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
        };
        variants.load(left).validate_resources(world.render, world.audio);
        variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
        {
            left(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.75f);
        }, &world);
        // A later positive-height hint must also fit the panel and footer,
        // making status-at-panel-bottom and status-at-design-bottom contacts
        // unreachable. Hint boundary cases below own those final contacts.
        const auto negative = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_select"]["briefing_padding"]["x"].SetFloat(3.0f);
            d["main_menu"]["stage_select"]["briefing"]["spacing"]["x"].SetFloat(-0.25f);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
            d["menu_text"]["stage"]["locked_hint"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            d["menu_text"]["stage"]["browse_hint"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            d["menu_text"]["stage"]["user_hint"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        };
        variants.load(negative).validate_resources(world.render, world.audio);
        variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
        {
            negative(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.75f);
        }, &world);
        struct ScreenEdge { const char* axis; float position; float outside; };
        // Shift the panel with the status: panel containment alone must not
        // make an offscreen status valid. The complete shadow touches design
        // edges even when the larger backing panel extends beyond them.
        // Status at y0 would put the earlier fixed briefing rows offscreen;
        // their new row-0 screen-top test replaces that unreachable contact.
        const ScreenEdge screen_edges[]{ { "x", -32.0f, -32.25f }, { "x", 1863.0f, 1863.25f } };
        for (const auto& edge : screen_edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["stage_select"]["briefing"]["position"][edge.axis].SetFloat(edge.position);
            };
            const auto old_contact = variants.load(contact);
            const auto& old_stage = old_contact.main_menu().stage_select;
            UiTextDropShadow status("legacy_status_design", L"Fit", "menu_text_exact",
                old_stage.briefing.position + old_stage.briefing_padding + old_stage.briefing.spacing * 6.0f,
                &world.render, Colour::black, old_contact.menu().shadow_colour, old_contact.menu().detail_shadow_offset);
            require(edge.position == -32.0f ? status.bounds().x == 0.0f : status.bounds().right() == 1920.0f,
                "Legacy status no longer touches its original inclusive design edge with the full shadow");
            // The literal status contact remains valid in design space; its
            // off-paper authored card is rejected by the new summary contract.
            variants.rejects("stage.best_of_mode", contact, &world);
            variants.rejects("stage.browse_status", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["briefing"]["position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        for (const char* axis : { "x", "y" })
        {
            for (const char* group : { "position", "spacing" })
            {
                variants.rejects("stage", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["briefing"][group][axis].SetDouble(1e100);
                }, &world);
            }
            variants.rejects("stage", [&](rapidjson::Document& d)
            {
                d["menu_style"]["shadow"]["detail_offset"][axis].SetDouble(1e100);
            }, &world);
        }
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
            const std::string name = std::string("menu_stage_status_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid arena status metrics fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : stage_text)
            {
                variants.rejects(std::string("stage.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry.key);
                }, &world);
            }
        }
    }
}
