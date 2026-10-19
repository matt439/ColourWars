#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Options page rows, the palette timing note and the save-status text.
namespace menu_text_acceptance
{
    void check_option_row_fitting(Variants& variants, World& world)
    {
        const std::string authored(200, 'W');
        const auto look = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : { "resolution", "fullscreen", "team_palette" })
            {
                d["menu_text"]["items"][key].SetString(authored.c_str(), d.GetAllocator());
            }
        });
        // These labels are not plain text actions. Resource validation checks
        // their glyphs and fitted geometry, preserving the actual OptionRow's
        // ability to fit a long, otherwise valid label.
        look.validate_resources(world.render, world.audio);
        for (const char* key : { "resolution", "fullscreen", "team_palette" })
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        const MenuStyle& style = look.menu();
        const auto art = options_art(look.main_menu());
        const mattmath::Vector2F label_position(100.0f, 100.0f);
        const mattmath::Vector2F value_position(420.0f, 100.0f);
        const float label_column = value_position.x - label_position.x - art.focus_padding.x;
        const auto font = world.render.resolve_sprite_font(style.item_font);
        for (const MenuItem item : { MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette })
        {
            int adjustments = 0;
            const std::wstring& text = look.menu_text().item(item);
            const auto original = world.render.measure_text(font, text);
            require(original.x > label_column, "Long OptionRow fixture already fits without scaling");
            OptionRow row("fitted", text, label_position, value_position, style, art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; },
                &art, {160.0f, 1000.0f});
            UiText* label = row_text(row, "fitted_label");
            UiText* value = row_text(row, "fitted_value");
            require(label != nullptr && value != nullptr && label->text() == text &&
                std::isfinite(label->bounds().width) && label->bounds().width > 0.0f &&
                label->bounds().width <= label_column + 0.01f && label->bounds().width < original.x,
                "Actual OptionRow truncated or failed to fit its long authored label to its column");
            const auto retained_label = label->bounds();
            row.set_value(L"Current value");
            require(value->text() == L"Current value" && value->bounds().width <= 160.01f &&
                label->bounds() == retained_label && label->text() == text,
                "OptionRow value updates changed its fitted label or escaped the supplied value width");
            row.adjust(Direction::right);
            require(adjustments == 1, "Fitted OptionRow lost its right adjustment action");
            row.adjust(Direction::left);
            require(adjustments == 0, "Fitting an authored OptionRow label changed its adjustment actions");
        }

        const std::wstring short_label = L"Height constrained";
        const auto original = world.render.measure_text(font, short_label);
        require(original.y > 10.0f, "OptionRow height fixture already fits without scaling");
        OptionRow height_row("height", short_label, label_position, {10000.0f, 100.0f}, style,
            art.ink, &world.render, {}, &art, {160.0f, 10.0f});
        const UiText* fitted = row_text(height_row, "height_label");
        require(fitted != nullptr && fitted->text() == short_label && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 10.01f && fitted->bounds().height < original.y &&
            fitted->bounds().width < original.x,
            "Actual OptionRow did not preserve and shrink a height-constrained authored label");
    }

    void check_options_palette_timing(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& main = original.main_menu();
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["options_palette_timing"].SetString("  A\xC2\xA0\xE2\x80\x94" "B\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().options_palette_timing == L"  A -B...  ",
            "Paused Options caption lost normalized punctuation or authored outer spaces");
        for (const char* invalid : { " \xC2\xA0 ", "line\nbreak", "line\rbreak", "tab\ttext", "control\x01", "caf\xC3\xA9", "\xC3" })
        {
            variants.rejects("options_palette_timing", [invalid](rapidjson::Document& d)
                { d["menu_text"]["options_palette_timing"].SetString(invalid, d.GetAllocator()); });
        }
        variants.rejects("options_palette_timing", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
            d["menu_text"]["options_palette_timing"].SetString("Glyph~");
        }, &world);
        variants.rejects("options_palette_timing", [](rapidjson::Document& d)
        {
            d["menu_text"]["options_palette_timing"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);

        const float right = (1920.0f + main.options_box_size.x) / 2.0f;
        const float row_top = main.options_position.y - options_art(main).focus_padding.y;
        const float width = right - main.options_value_x;
        const float height = row_top - main.options_heading_position.y;
        const auto add_font = [&](const std::string& name, int glyph_width, int glyph_height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool edge = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, edge ? glyph_width : 8,
                    edge ? glyph_height : 16), 0.0f, 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        for (const bool width_edge : { true, false })
        {
            const std::string name = width_edge ? "palette_timing_width_edge" : "palette_timing_height_edge";
            add_font(name, width_edge ? static_cast<int>(width) : 8, width_edge ? 20 : static_cast<int>(height));
            add_font(name + "_overflow", width_edge ? static_cast<int>(width) + 1 : 8,
                width_edge ? 20 : static_cast<int>(height) + 1);
            const auto edit = [&](rapidjson::Document& d, const std::string& font)
            {
                d["menu_style"]["fonts"]["detail"].SetString(font.c_str(), d.GetAllocator());
                d["menu_text"]["options_palette_timing"].SetString("~");
            };
            const auto edge = variants.load([&](rapidjson::Document& d) { edit(d, name); });
            edge.validate_resources(world.render, world.audio);
            std::wstring source = edge.menu_text().options_palette_timing;
            UiText caption("palette_timing", source, edge.menu().detail_font,
                { main.options_value_x, main.options_heading_position.y }, &world.render, main.art.ink);
            source = L"Changed after construction";
            require(caption.text() == L"~" && caption.bounds().x == main.options_value_x &&
                caption.bounds().y == main.options_heading_position.y &&
                (width_edge ? caption.bounds().right() == right : caption.bounds().bottom() == row_top),
                "Paused Options caption borrowed its copy, fitted its text or missed inclusive paper/card contact");
            caption.set_hidden(true); DrawList no_device; caption.draw(no_device);
            variants.rejects("options_palette_timing", [&](rapidjson::Document& d) { edit(d, name + "_overflow"); }, &world);
        }
        const int heading_width = static_cast<int>(main.options_value_x - main.options_heading_position.x);
        add_font("palette_timing_heading_edge", heading_width, 20);
        add_font("palette_timing_heading_overflow", heading_width + 1, 20);
        const auto heading = [](rapidjson::Document& d, const char* font)
        {
            d["menu_style"]["fonts"]["heading"].SetString(font, d.GetAllocator());
            d["menu_text"]["headings"]["options"].SetString("~");
            d["menu_text"]["options_palette_timing"].SetString("Fit");
        };
        const auto heading_edge = variants.load([&](rapidjson::Document& d) { heading(d, "palette_timing_heading_edge"); });
        heading_edge.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font(heading_edge.menu().heading_font), L"~").x ==
            main.options_value_x - main.options_heading_position.x,
            "Paused Options heading fixture lost exact contact with the caption anchor");
        variants.rejects("options_palette_timing", [&](rapidjson::Document& d)
            { heading(d, "palette_timing_heading_overflow"); }, &world);
    }

    void check_status_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : status_text)
            {
                d["menu_text"]["status"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : status_text)
        {
            require(normalized.menu_text().status.*entry.field == L"  A \"B\"-C...  ",
                std::string("Save status lost authored spacing or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("status.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["status"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            // Each status checks its actual page font even though the other
            // menu font still supports '~'; font stand-ins cannot hide a gap.
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (entry.options) { d["prompt_bar"]["font"].SetString("menu_text_limited"); }
                else { d["controls_settings"]["help_font"].SetString("menu_text_limited"); }
                d["menu_text"]["status"][entry.key].SetString("Glyph~");
            }, &world);
        }
    }

    void check_status_fitting(Variants& variants, World& world)
    {
        const auto check_messages = [&](const Presentation& look, bool width_limited)
        {
            for (const auto& entry : status_text)
            {
                const auto& main = look.main_menu();
                const auto& controls = look.controls_settings();
                const auto position = entry.options ? main.options_status_position : controls.status_position;
                const auto box_size = entry.options ? main.options_box_size : controls.box_size;
                const auto art = entry.options ? options_art(main) : controls.art;
                const auto& font_name = entry.options ? look.prompt_bar().font : controls.help_font;
                const auto font = world.render.resolve_sprite_font(font_name);
                const float available = (1920.0f + box_size.x) / 2.0f -
                    art.shadow_offset - art.focus_padding.x - position.x;
                require(available == (entry.options ? 930.0f : 1590.0f),
                    "Save-status fixture no longer exercises the shipped page widths");
                std::wstring authored = look.menu_text().status.*entry.field;
                const auto measured = world.render.measure_text(font, authored);
                require(width_limited ? measured.x > available : measured.x < available,
                    "Save-status fixture does not independently exercise the expected width fitting");
                UiText text(entry.key, authored, font_name, position, &world.render, art.ink);
                const auto retained = authored;
                authored = L"Changed after construction";
                require(text.text() == retained && text.bounds() == mattmath::RectangleF(position, measured),
                    "Actual status UiText borrowed its source or changed its unscaled measurement");
                text.set_scale(1.0f);
                const float scale = std::min(1.0f, available / measured.x);
                text.set_scale(scale);
                require(text.text() == retained && text.bounds().position() == position &&
                    std::isfinite(text.bounds().width) && std::isfinite(text.bounds().height) &&
                    text.bounds().width > 0.0f && text.bounds().height > 0.0f &&
                    text.bounds().width <= available + 0.01f &&
                    std::abs(text.bounds().height - measured.y * scale) <= 0.01f,
                    "Actual status UiText lost text, moved, or changed its proportional width-only fitting");
                if (width_limited)
                {
                    require(std::abs(text.bounds().width - available) <= 0.01f &&
                        text.bounds().height < measured.y,
                        "Long save status did not retain the complete text and proportionally fit its page width");
                    text.set_text(L"Fit");
                    text.set_scale(1.0f);
                    require(text.text() == L"Fit" && text.bounds() ==
                        mattmath::RectangleF(position, world.render.measure_text(font, L"Fit")),
                        "Replacing a fitted save status retained its old shrink scale");
                }
                else
                {
                    require(text.bounds() == mattmath::RectangleF(position, measured),
                        "Short save statuses were enlarged above their authored font size");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        const auto short_messages = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
        });
        short_messages.validate_resources(world.render, world.audio);
        check_messages(short_messages, false);
        const auto wide_messages = variants.load([](rapidjson::Document& d)
        {
            // The original tall help/association blocks share the status band
            // at its far right. Compact blocks leave that wider message legal.
            d["menu_text"]["help"]["controls"].SetString("Fit");
            d["menu_text"]["help"]["pad_association"].SetString("Fit");
            for (const auto& entry : status_text)
            {
                d["menu_text"]["status"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide_messages.validate_resources(world.render, world.audio);
        check_messages(wide_messages, true);
    }

    void check_status_bounds(Variants& variants, World& world)
    {
        for (const auto& entry : status_text)
        {
            if (entry.options) { continue; }
            variants.rejects(std::string("status.") + entry.key, [&](rapidjson::Document& d)
            {
                d["menu_text"]["status"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        // This message has two consumers. Independently clear one page's help
        // and retain the other's original intersection to check both contexts.
        for (const char* compact_help : { "controls", "pad_association" })
        {
            variants.rejects("status.active_save_failed", [&](rapidjson::Document& d)
            {
                d["menu_text"]["help"][compact_help].SetString("Fit");
                d["menu_text"]["status"]["active_save_failed"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        for (const auto& entry : status_text)
        {
            const int height = entry.options ? 55 : 52;
            for (const int excess : { 0, 1 })
            {
                std::vector<Glyph> glyphs;
                for (char32_t character = U' '; character <= U'~'; ++character)
                {
                    glyphs.push_back({ character, mattmath::RectangleI(0, 0, 8,
                        character == U'~' ? height + excess : 16), 0.0f, 0.0f, 0.0f });
                }
                const std::string font_name = std::string("menu_status_") + entry.key +
                    (excess == 0 ? "_edge" : "_overflow");
                world.render.add_font(font_name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
                const auto edit = [&](rapidjson::Document& d)
                {
                    if (entry.options) { d["prompt_bar"]["font"].SetString(font_name.c_str(), d.GetAllocator()); }
                    else { d["controls_settings"]["help_font"].SetString(font_name.c_str(), d.GetAllocator()); }
                    for (const auto& message : status_text) { d["menu_text"]["status"][message.key].SetString("Fit"); }
                    d["menu_text"]["status"][entry.key].SetString("~");
                };
                if (excess == 0)
                {
                    const auto touches_bottom = variants.load(edit);
                    touches_bottom.validate_resources(world.render, world.audio);
                    const auto size = world.render.measure_text(world.render.resolve_sprite_font(font_name), L"~");
                    require(size == mattmath::Vector2F(8.0f, static_cast<float>(height)),
                        "Save-status inclusive paper fixture has unexpected glyph metrics");
                }
                else { variants.rejects(std::string("status.") + entry.key, edit, &world); }
            }
        }
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& main = original.main_menu();
        const auto art = options_art(main);
        const auto back_size = world.render.measure_text(world.render.resolve_sprite_font(original.menu().item_font),
            original.menu_text().item(MenuItem::back));
        const float back_right = main.options_position.x + back_size.x + art.focus_padding.x;
        const auto back_edge = [&](rapidjson::Document& d)
        {
            // The shipped status already overlaps Back vertically. Exact
            // horizontal contact remains legal; any positive area is rejected.
            d["main_menu"]["options_status_position"]["x"].SetFloat(back_right);
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
        };
        const auto beside_back = variants.load(back_edge);
        beside_back.validate_resources(world.render, world.audio);
        require(main.options_status_position.y < main.options_position.y + main.options_spacing.y * 6.0f +
            back_size.y + art.focus_padding.y,
            "Options status fixture no longer intersects the padded Back row vertically");
        variants.rejects("status.options_pending", [&](rapidjson::Document& d)
        {
            back_edge(d);
            d["main_menu"]["options_status_position"]["x"].SetFloat(back_right - 0.25f);
        }, &world);
        variants.rejects("status.options_pending", [](rapidjson::Document& d)
        {
            // This point passes the nominal inset parser but leaves zero
            // width after the status's existing shadow and right padding.
            d["main_menu"]["options_status_position"]["x"].SetFloat(1750.0f);
        }, &world);

        const auto preview_page = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            d["controls_settings"]["item_font"].SetString("menu_text_exact");
            d["controls_settings"]["help_font"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : status_text) { d["menu_text"]["status"][entry.key].SetString("Fit"); }
            d["menu_text"]["help"]["controls"].SetString("Fit");
            d["controls_settings"]["heading_position"]["y"].SetFloat(40.0f);
            d["controls_settings"]["layout"]["position"]["y"].SetFloat(70.0f);
            d["controls_settings"]["layout"]["size"]["y"].SetFloat(1.0f);
            d["controls_settings"]["layout"]["spacing"]["y"].SetFloat(22.0f);
            d["controls_settings"]["art"]["focus_padding"]["y"].SetFloat(1.0f);
            d["controls_settings"]["art"]["keyline_width"].SetFloat(0.25f);
            d["controls_settings"]["art"]["shadow_offset"].SetFloat(0.25f);
            d["controls_settings"]["status_position"]["x"].SetFloat(221.75f);
        };
        // Compact but measured-valid rows leave status bands beside each
        // preview. The 1600x20 source fits to exactly 1590x19.875, so contact
        // with the track/caption top is an exact binary-float boundary.
        for (const float top : { 315.0f, 395.0f, 479.0f })
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                preview_page(d);
                d["menu_text"]["status"]["sticks_pending"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                d["controls_settings"]["status_position"]["y"].SetFloat(top - 19.875f);
            };
            const auto touches_preview = variants.load(contact);
            touches_preview.validate_resources(world.render, world.audio);
            variants.rejects("status.sticks_pending", [&](rapidjson::Document& d)
            {
                contact(d);
                d["controls_settings"]["status_position"]["y"].SetFloat(top - 19.625f);
            }, &world);
        }
        for (const char* key : { "sticks_pending", "sticks_saved", "active_save_failed" })
        {
            variants.rejects(std::string("status.") + key, [&](rapidjson::Document& d)
            {
                preview_page(d);
                d["menu_text"]["status"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
                d["controls_settings"]["status_position"]["y"].SetFloat(500.0f);
            }, &world);
        }
    }
}
