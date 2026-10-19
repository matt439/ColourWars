#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Controls/audio rows and the help text. check_controls_text registers
// menu_controls_row_overflow for check_controls_option_rows.
namespace menu_text_acceptance
{
    namespace
    {
    constexpr MenuItem controls_fitted_items[]{ MenuItem::music_volume, MenuItem::sfx_volume, MenuItem::view_shake,
        MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone };
    }

    void check_controls_text(Variants& variants, World& world)
    {
        constexpr const char* controls_keys[]{ "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : controls_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(controls_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::pad_sticks) + index)) == L"A -B",
                "Controls/pad-sticks item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : controls_keys)
        {
            for (const char* invalid : { "   ", "line\nbreak", "tab\ttext", "\xC3\xA9", "\xC3" })
            {
                variants.rejects(std::string("items.") + key, [key, invalid](rapidjson::Document& d)
                {
                    d["menu_text"]["items"][key].SetString(invalid, d.GetAllocator());
                });
            }
        }
        for (const char* key : { "pad_sticks", "music_volume", "sfx_volume", "view_shake", "aim_marker",
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone", "apply", "back" })
        {
            // The main-menu font can draw this glyph. Controls' independently
            // selected item font must also support shared Apply and Back.
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            // The F alone exactly fills the Controls action column. The same
            // one-character string still fits its other uses in the main font.
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'F' ? 872 : 8, 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_controls_exact", std::make_unique<Font>(TextureHandle{}, glyphs, 20.0f));
        world.render.add_font("menu_controls_row_exact", std::make_unique<Font>(TextureHandle{}, glyphs, 68.0f));
        world.render.add_font("menu_controls_row_overflow", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 68.25f));
        const auto exact_controls = [](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_controls_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("i"); }
        };
        for (const char* key : { "pad_sticks", "apply", "back" })
        {
            const auto edge = [&](rapidjson::Document& d)
            {
                exact_controls(d);
                d["menu_text"]["items"][key].SetString("F");
            };
            const auto fits = variants.load(edge);
            fits.validate_resources(world.render, world.audio);
            for (const char* overflow : { "Fi", "FF" })
            {
                variants.rejects(std::string("items.") + key, [&](rapidjson::Document& d)
                {
                    edge(d);
                    d["menu_text"]["items"][key].SetString(overflow, d.GetAllocator());
                }, &world);
            }
        }
        variants.rejects("items.pad_sticks", [](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_tall");
        }, &world);
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_controls(d);
            d["controls_settings"]["item_font"].SetString("menu_controls_row_exact");
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.pad_sticks", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["controls_settings"]["item_font"].SetString("menu_controls_row_overflow");
        }, &world);
        const auto status_edge = [&](rapidjson::Document& d)
        {
            row_edge(d);
            // Nominal 60-high rows plus their shadow end at 868. The actual
            // 68-high text plus focus padding reaches 874 and must stay clear
            // of the status even when that nominal layout still parses.
            d["controls_settings"]["status_position"]["y"].SetFloat(874.0f);
        };
        const auto fits_status = variants.load(status_edge);
        fits_status.validate_resources(world.render, world.audio);
        variants.rejects("items.back", [&](rapidjson::Document& d)
        {
            status_edge(d);
            d["controls_settings"]["status_position"]["y"].SetFloat(873.75f);
        }, &world);
    }

    void check_controls_option_rows(Variants& variants, World& world)
    {
        const std::string authored(200, 'W');
        const auto look = variants.load([&](rapidjson::Document& d)
        {
            for (const MenuItem item : controls_fitted_items)
            {
                d["menu_text"]["items"][items[static_cast<size_t>(item)].key].SetString(authored.c_str(), d.GetAllocator());
            }
        });
        look.validate_resources(world.render, world.audio);
        const auto& controls = look.controls_settings();
        MenuStyle style = look.menu();
        require(controls.item_font != style.item_font,
            "Controls font coverage must exercise its separate item font from ordinary menu actions");
        style.item_font = controls.item_font;
        const auto font = world.render.resolve_sprite_font(style.item_font);
        const float label_width = controls.layout.spacing.x - controls.art.focus_padding.x;
        const mattmath::Vector2F value_limit(
            controls.controls_position.x - controls.art.focus_padding.x * 2.0f - controls.art.shadow_offset -
                controls.layout.position.x - controls.layout.spacing.x,
            controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
        require(label_width == 482.0f && value_limit == mattmath::Vector2F(332.0f, 68.0f),
            "Controls fitted-row fixture no longer exercises the shipped column and height limits");
        for (size_t index = 0; index < std::size(controls_fitted_items); ++index)
        {
            const int row_index = index < 5 ? static_cast<int>(index) + 1 : static_cast<int>(index) - 5;
            const auto position = controls.layout.position + mattmath::Vector2F(0.0f,
                controls.layout.spacing.y * static_cast<float>(row_index));
            const std::wstring& text = look.menu_text().item(controls_fitted_items[index]);
            const auto original = world.render.measure_text(font, text);
            require(original.x > label_width, "Long Controls label already fits without scaling");
            int adjustments = 0;
            OptionRow row("controls_fit", text, position,
                position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f), style,
                controls.art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; },
                &controls.art, value_limit);
            UiText* label = row_text(row, "controls_fit_label");
            UiText* value = row_text(row, "controls_fit_value");
            require(label != nullptr && value != nullptr && label->text() == text &&
                std::isfinite(label->bounds().width) && label->bounds().width > 0.0f &&
                label->bounds().width <= label_width + 0.01f && label->bounds().width < original.x &&
                label->bounds().height > 0.0f && label->bounds().height <= value_limit.y + 0.01f,
                "Actual Controls OptionRow truncated or failed to fit its authored label");
            const auto retained_label = label->bounds();
            row.set_value(std::wstring(100, L'W'));
            require(value->bounds().width > 0.0f && value->bounds().width <= value_limit.x + 0.01f &&
                value->bounds().height <= value_limit.y + 0.01f && label->bounds() == retained_label &&
                label->text() == text,
                "Controls value fitting changed its authored label or escaped the actual value limits");
            row.set_value(L"5%");
            require(value->text() == L"5%" && value->bounds().width > 0.0f &&
                value->bounds().width <= value_limit.x + 0.01f && label->bounds() == retained_label,
                "Shorter Controls values did not refit independently of the authored label");
            row.adjust(Direction::right);
            require(adjustments == 1, "Authored Controls label lost its right adjustment callback");
            row.adjust(Direction::left);
            require(adjustments == 0, "Authored Controls labels changed their adjustment callback");
            row.set_hidden(true);
            DrawList no_device;
            row.draw(no_device);
        }
        // Exercise the second fit step independently: this short label needs
        // no width reduction, while its font is just taller than Controls' 68.
        style.item_font = "menu_controls_row_overflow";
        OptionRow height_row("controls_height", L"i", controls.layout.position,
            controls.layout.position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f),
            style, controls.art.ink, &world.render, {}, &controls.art, value_limit);
        const UiText* fitted = row_text(height_row, "controls_height_label");
        require(fitted != nullptr && fitted->text() == L"i" && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 68.01f && fitted->bounds().height < 68.25f &&
            fitted->bounds().width > 0.0f && fitted->bounds().width < 8.0f,
            "Controls OptionRow did not preserve and shrink a height-constrained authored label");
    }

    void check_help_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : help_text)
            {
                d["menu_text"]["help"][entry.key].SetString(entry.multiline ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ", d.GetAllocator());
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : help_text)
        {
            require(normalized.menu_text().help.*entry.field == (entry.multiline ? L" A -B \n C " : L" A -B "),
                std::string("Help text lost authored spacing/LF or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("help.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["help"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        d["menu_text"]["help"][entry.key].SetString(invalid, d.GetAllocator());
                    });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["help"][entry.key].SetString("line\nbreak");
                });
            }
            // Heading, item and ordinary detail fonts still contain '~'.
            // Only this help font lacks it, and its stand-in must not hide that.
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["controls_settings"]["help_font"].SetString("menu_text_limited");
                d["menu_text"]["help"][entry.key].SetString("Glyph~");
            }, &world);
        }
        const auto single_line_blocks = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["controls"].SetString("Controls");
            d["menu_text"]["help"]["pad_association"].SetString("Slots");
        });
        single_line_blocks.validate_resources(world.render, world.audio);
        require(single_line_blocks.menu_text().help.controls == L"Controls" &&
            single_line_blocks.menu_text().help.pad_association == L"Slots",
            "Help blocks replaced authored copy with fixed line counts");
        variants.rejects("help.aim_cutoff", [](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);
    }

    void check_help_fitting(Variants& variants, World& world)
    {
        const auto check_blocks = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            const auto& controls = look.controls_settings();
            const auto font = world.render.resolve_sprite_font(controls.help_font);
            for (const auto& entry : help_text)
            {
                if (!entry.multiline) { continue; }
                const bool is_controls = std::string(entry.key) == "controls";
                const auto position = is_controls ? controls.controls_position : controls.stick_preview.association_position;
                const auto available = is_controls ? controls.controls_size :
                    controls.controls_position + controls.controls_size - position;
                std::wstring authored = look.menu_text().help.*entry.field;
                const auto measured = world.render.measure_text(font, authored);
                if (width_limited)
                {
                    require(measured.x > available.x && measured.y < available.y,
                        "Long help fixture does not independently exercise width fitting");
                }
                if (height_limited)
                {
                    require(measured.x < available.x && measured.y > available.y,
                        "Tall help fixture does not independently exercise height fitting");
                }
                UiText text(entry.key, authored, controls.help_font, position, &world.render, controls.art.ink);
                const auto retained = authored;
                authored = L"Changed after construction";
                require(text.text() == retained && text.bounds() == mattmath::RectangleF(position, measured),
                    "Actual help UiText borrowed its source or changed its unscaled measurement");
                text.set_scale(std::min({ 1.0f, available.x / measured.x, available.y / measured.y }));
                require(text.text() == retained && text.bounds().position() == position &&
                    std::isfinite(text.bounds().width) && std::isfinite(text.bounds().height) &&
                    text.bounds().width > 0.0f && text.bounds().height > 0.0f &&
                    text.bounds().width <= available.x + 0.01f && text.bounds().height <= available.y + 0.01f,
                    "Actual help UiText lost text, moved, or escaped its fitted help rectangle");
                if (width_limited || height_limited)
                {
                    require(text.bounds().width < measured.x && text.bounds().height < measured.y,
                        "Help block did not retain proportional down-scaling");
                }
                else
                {
                    require(text.bounds() == mattmath::RectangleF(position, measured),
                        "Short help blocks were enlarged above their authored font size");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
            }
        };
        const auto short_blocks = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["controls"].SetString("Fit\nFit");
            d["menu_text"]["help"]["pad_association"].SetString("Fit\nFit");
        });
        short_blocks.validate_resources(world.render, world.audio);
        check_blocks(short_blocks, false, false);
        const auto wide_blocks = variants.load([](rapidjson::Document& d)
        {
            for (const char* key : { "controls", "pad_association" })
            {
                d["menu_text"]["help"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide_blocks.validate_resources(world.render, world.audio);
        check_blocks(wide_blocks, true, false);
        const auto tall_blocks = variants.load([](rapidjson::Document& d)
        {
            std::string lines = "Fit";
            for (int line = 1; line < 50; ++line) { lines += "\nFit"; }
            for (const char* key : { "controls", "pad_association" })
            {
                d["menu_text"]["help"][key].SetString(lines.c_str(), d.GetAllocator());
            }
        });
        tall_blocks.validate_resources(world.render, world.audio);
        check_blocks(tall_blocks, false, true);
    }

    void check_help_bounds(Variants& variants, World& world)
    {
        // A caption spans the remaining help column, rather than the narrower
        // 210-wide aim box. It keeps its unscaled font, unlike both help blocks.
        const auto caption_edge = [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_text_exact");
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(77, 'F').c_str(), d.GetAllocator());
        };
        const auto fits_caption = variants.load(caption_edge);
        fits_caption.validate_resources(world.render, world.audio);
        const auto& controls = fits_caption.controls_settings();
        const auto font = world.render.resolve_sprite_font(controls.help_font);
        const auto caption_size = world.render.measure_text(font, fits_caption.menu_text().help.aim_cutoff);
        require(caption_size.x > controls.stick_preview.aim.width && caption_size.x <= controls.controls_size.x,
            "Aim caption fixture does not distinguish the full help column from the square aim preview");
        const auto caption_position = controls.stick_preview.aim.position() - mattmath::Vector2F(0.0f,
            world.render.measure_text(font, L"Move").y + controls.art.focus_padding.y);
        UiText caption("caption", fits_caption.menu_text().help.aim_cutoff, controls.help_font,
            caption_position, &world.render, controls.art.ink);
        require(caption.text() == fits_caption.menu_text().help.aim_cutoff &&
            caption.bounds() == mattmath::RectangleF(caption_position, caption_size),
            "Actual aim caption changed its unscaled font or placement above the aim box");
        caption.set_hidden(true);
        DrawList no_device;
        caption.draw(no_device);
        variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
        {
            caption_edge(d);
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(78, 'F').c_str(), d.GetAllocator());
        }, &world);
        // These preview coordinates remain legal in the geometric parser, but
        // the measured caption now intrudes into the preceding response track.
        variants.rejects("help.aim_cutoff", [](rapidjson::Document& d)
        {
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(420.0f);
        }, &world);
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool boundary_glyph = character == U'F' || character == U'G';
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, boundary_glyph ? 620 : 8,
                character == U'F' ? 28 : character == U'G' ? 29 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_help_caption_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto exact_caption = [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_help_caption_exact");
            d["menu_text"]["help"]["aim_cutoff"].SetString("F");
        };
        const auto touches_right_and_aim = variants.load(exact_caption);
        touches_right_and_aim.validate_resources(world.render, world.audio);
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_help_caption_exact"), L"F") ==
            mattmath::Vector2F(620.0f, 28.0f), "Caption boundary glyph has unexpected metrics");
        for (const char* overflow : { "Fi", "G" })
        {
            variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
            {
                exact_caption(d);
                d["menu_text"]["help"]["aim_cutoff"].SetString(overflow, d.GetAllocator());
            }, &world);
        }
        const auto touches_track = [&](rapidjson::Document& d)
        {
            exact_caption(d);
            // Move is 20 high, plus eight units of existing focus padding:
            // the 28-high F starts exactly at the response track's bottom.
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(447.0f);
        };
        const auto fits_track = variants.load(touches_track);
        fits_track.validate_resources(world.render, world.audio);
        variants.rejects("help.aim_cutoff", [&](rapidjson::Document& d)
        {
            touches_track(d);
            d["controls_settings"]["stick_preview"]["aim"]["position"]["y"].SetFloat(446.75f);
        }, &world);
    }
}
