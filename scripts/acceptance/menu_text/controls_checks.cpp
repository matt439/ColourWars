#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Controls/audio rows. check_controls_text registers
// menu_controls_row_overflow for check_controls_option_rows. The help text
// is in help_checks.cpp.
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
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone", "keyboard_jump" };
        constexpr MenuItem controls_roles[]{ MenuItem::pad_sticks, MenuItem::music_volume, MenuItem::sfx_volume,
            MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::pad,
            MenuItem::move_deadzone, MenuItem::aim_deadzone, MenuItem::keyboard_jump };
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
            require(normalized.menu_text().item(controls_roles[index]) == L"A -B",
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
            "reduce_effects", "pad", "move_deadzone", "aim_deadzone", "keyboard_jump", "apply", "back" })
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
        world.render.add_font("menu_controls_row_exact", std::make_unique<Font>(TextureHandle{}, glyphs, 58.0f));
        world.render.add_font("menu_controls_row_overflow", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 58.25f));
        const auto exact_controls = [](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_controls_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("i"); }
        };
        for (const char* key : { "pad_sticks", "back" })
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
        // World loads the shipped SpriteFontFile metrics without a graphics
        // device. The shipped face keeps its descenders inside its 58 line
        // spacing, so its actions fit the 58-high row unscaled. The regression
        // this guards came from a face whose descenders reached 62, so alias
        // the shipped glyphs with g/j/p/q/y stretched to that depth, distinct
        // from the synthetic exact-boundary fonts registered below.
        const Font* shipped_font = world.render.font(world.render.resolve_sprite_font("lato_black_36"));
        require(shipped_font->line_spacing() == 58.0f, "Shipped Controls action font changed its line spacing");
        std::vector<Glyph> shipped_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const Glyph* glyph = shipped_font->find(character);
            require(glyph != nullptr, "Shipped action regression font lacks a printable glyph");
            Glyph deep = *glyph;
            if (character == U'g' || character == U'j' || character == U'p' || character == U'q' || character == U'y')
            {
                deep.subrect.height = static_cast<int>(62.0f - deep.y_offset);
            }
            shipped_glyphs.push_back(deep);
        }
        world.render.add_font("menu_controls_action_shipped", std::make_unique<Font>(TextureHandle{},
            std::move(shipped_glyphs), shipped_font->line_spacing()));
        const auto real_actions = variants.load([](rapidjson::Document& d)
        { d["controls_settings"]["item_font"].SetString("menu_controls_action_shipped"); });
        real_actions.validate_resources(world.render, world.audio);
        const auto& real_controls = real_actions.controls_settings();
        const float action_width = real_controls.controls_position.x - real_controls.art.focus_padding.x * 2.0f -
            real_controls.art.shadow_offset - real_controls.layout.position.x;
        const float action_height = real_controls.layout.spacing.y - real_controls.art.focus_padding.y * 2.0f;
        require(action_width == 842.0f && action_height == 58.0f,
            "Controls actions lost their authored full-column width or row-height limit");
        const auto action_font = world.render.resolve_sprite_font(real_controls.item_font);
        for (const MenuItem item : { MenuItem::keyboard_jump, MenuItem::apply })
        {
            const auto& copy = real_actions.menu_text().item(item);
            const auto shipped = world.render.measure_text(world.render.resolve_sprite_font("lato_black_36"), copy);
            require(shipped.y == 58.0f && shipped.y <= action_height,
                "Shipped Controls action no longer fits its 58-high row unscaled");
            const auto raw = world.render.measure_text(action_font, copy);
            require(raw == mattmath::Vector2F(shipped.x, 62.0f) &&
                raw.y + real_controls.art.focus_padding.y * 2.0f == 78.0f &&
                raw.y + real_controls.art.focus_padding.y * 2.0f > real_controls.layout.spacing.y,
                "Action regression no longer exercises 62-high descenders in a 74-high row");
            UiText text("shipped-action", copy, real_controls.item_font, real_controls.layout.position,
                &world.render, real_controls.art.ink);
            const float scale = std::min({ 1.0f, action_width / raw.x, action_height / raw.y });
            text.set_scale(scale);
            require(scale == 58.0f / 62.0f && text.text() == copy && text.bounds().height == 58.0f &&
                text.bounds().width == raw.x * scale,
                "Deep-descender action did not retain its complete label with a uniform 58/62 fit");
        }
        std::vector<Glyph> action_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            action_glyphs.push_back({ character, mattmath::RectangleI(0, 0,
                character == U'F' ? 842 : 8, character == U'F' ? 58 : 16),
                0.0f, character == U'H' ? 42.25f : 0.0f, 0.0f });
        }
        world.render.add_font("menu_controls_action_exact", std::make_unique<Font>(TextureHandle{}, action_glyphs, 20.0f));
        for (const bool nonfinite : { false, true })
        {
            auto invalid_glyphs = action_glyphs;
            for (Glyph& glyph : invalid_glyphs)
            {
                if (glyph.character != U'F') { continue; }
                if (nonfinite) { glyph.x_offset = std::numeric_limits<float>::infinity(); }
                else { glyph.subrect.width = 0; }
            }
            world.render.add_font(nonfinite ? "menu_controls_action_nonfinite" : "menu_controls_action_zero",
                std::make_unique<Font>(TextureHandle{}, std::move(invalid_glyphs), 20.0f));
        }
        for (const char* key : { "keyboard_jump", "apply" })
        {
            const bool keyboard = std::string(key) == "keyboard_jump";
            const std::array<int, 2> rows{ keyboard ? 1 : 7, keyboard ? -1 : 3 };
            for (const char* copy : { "i", "F", "Fi", "FF", "H" })
            {
                const auto fitted = variants.load([&](rapidjson::Document& d)
                {
                    exact_controls(d);
                    d["controls_settings"]["item_font"].SetString("menu_controls_action_exact");
                    d["menu_text"]["items"][key].SetString(copy, d.GetAllocator());
                });
                fitted.validate_resources(world.render, world.audio);
                const auto& look = fitted.controls_settings();
                const std::wstring words(copy, copy + std::strlen(copy));
                const auto raw = world.render.measure_text(world.render.resolve_sprite_font(look.item_font), words);
                const float scale = std::min({ 1.0f, action_width / raw.x, action_height / raw.y });
                for (const int row : rows)
                {
                    if (row < 0) { continue; }
                    const auto position = look.layout.position + mattmath::Vector2F(0.0f,
                        look.layout.spacing.y * static_cast<float>(row));
                    UiText text("fitted-action", words, look.item_font, position, &world.render, look.art.ink);
                    text.set_scale(scale);
                    require(std::isfinite(scale) && scale > 0.0f && text.text() == words &&
                        text.bounds() == mattmath::RectangleF(position, raw * scale) &&
                        text.bounds().width > 0.0f && text.bounds().width <= action_width &&
                        text.bounds().height > 0.0f && text.bounds().height <= action_height,
                        "Fitted action truncated its copy, changed proportions or escaped its column/row");
                    require((words == L"i" || words == L"F") ? scale == 1.0f : scale < 1.0f,
                        "Action boundary did not distinguish exact/short labels from width/height overflow");
                    if (words == L"F")
                    {
                        require(text.bounds().size() == mattmath::Vector2F(842.0f, 58.0f),
                            "Exact action glyph did not touch both fitted limits");
                    }
                }
            }
            for (const char* invalid_font : { "menu_controls_action_zero", "menu_controls_action_nonfinite" })
            {
                variants.rejects(std::string("items.") + key, [&](rapidjson::Document& d)
                {
                    exact_controls(d);
                    d["controls_settings"]["item_font"].SetString(invalid_font, d.GetAllocator());
                    d["menu_text"]["items"][key].SetString("F");
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
            // Nine nominal 54-high rows plus their shadow end at 866. The
            // 58-high text plus focus padding reaches 868 and must stay clear
            // of the status even when that nominal layout still parses.
            d["controls_settings"]["status_position"]["y"].SetFloat(868.0f);
        };
        const auto fits_status = variants.load(status_edge);
        fits_status.validate_resources(world.render, world.audio);
        const auto& status_look = fits_status.controls_settings();
        const float measured_back_bottom = status_look.layout.position.y + status_look.layout.spacing.y * 8.0f +
            world.render.measure_text(world.render.resolve_sprite_font(status_look.item_font), L"i").y +
            status_look.art.focus_padding.y;
        require(measured_back_bottom == 868.0f && measured_back_bottom == status_look.status_position.y,
            "Controls status boundary does not touch the actual ninth row's measured font and focus padding");
        variants.rejects("items.back", [&](rapidjson::Document& d)
        {
            status_edge(d);
            d["controls_settings"]["status_position"]["y"].SetFloat(867.75f);
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
        require(controls.layout.size.y == 54.0f && controls.layout.spacing.y == 74.0f &&
            label_width == 482.0f && value_limit == mattmath::Vector2F(332.0f, 58.0f),
            "Controls fitted-row fixture no longer exercises the shipped column and height limits");
        for (size_t index = 0; index < std::size(controls_fitted_items); ++index)
        {
            // Controls keeps Sticks/Keyboard Jump first; its five shared
            // settings now occupy rows2-6. The Sticks child's rows stay0-2.
            const int row_index = index < 5 ? static_cast<int>(index) + 2 : static_cast<int>(index) - 5;
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
        // no width reduction, while its font is just taller than Controls' 58.
        style.item_font = "menu_controls_row_overflow";
        OptionRow height_row("controls_height", L"i", controls.layout.position,
            controls.layout.position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f),
            style, controls.art.ink, &world.render, {}, &controls.art, value_limit);
        const UiText* fitted = row_text(height_row, "controls_height_label");
        require(fitted != nullptr && fitted->text() == L"i" && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 58.01f && fitted->bounds().height < 58.25f &&
            fitted->bounds().width > 0.0f && fitted->bounds().width < 8.0f,
            "Controls OptionRow did not preserve and shrink a height-constrained authored label");
    }
}
