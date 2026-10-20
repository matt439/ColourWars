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

    struct JumpLabel { Key key; const wchar_t* label; };
    constexpr JumpLabel jump_labels[]{
        { Key::a, L"A" }, { Key::b, L"B" }, { Key::c, L"C" }, { Key::d, L"D" }, { Key::e, L"E" },
        { Key::f, L"F" }, { Key::g, L"G" }, { Key::h, L"H" }, { Key::i, L"I" }, { Key::j, L"J" },
        { Key::k, L"K" }, { Key::l, L"L" }, { Key::m, L"M" }, { Key::n, L"N" }, { Key::o, L"O" },
        { Key::p, L"P" }, { Key::q, L"Q" }, { Key::r, L"R" }, { Key::s, L"S" }, { Key::t, L"T" },
        { Key::u, L"U" }, { Key::v, L"V" }, { Key::w, L"W" }, { Key::x, L"X" }, { Key::y, L"Y" },
        { Key::z, L"Z" }, { Key::space, L"Space" }
    };

    std::wstring expected_jump_help(const std::wstring& authored, const wchar_t* label)
    {
        const auto at = authored.find(L"{jump}");
        require(at != std::wstring::npos, "Controls help oracle requires its declared token");
        return authored.substr(0, at) + label + authored.substr(at + 6);
    }
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
        // device. Alias those glyphs to distinguish this regression from the
        // synthetic exact-boundary fonts registered below.
        const Font* shipped_font = world.render.font(world.render.resolve_sprite_font("gill_sans_mt_bold_36"));
        std::vector<Glyph> shipped_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const Glyph* glyph = shipped_font->find(character);
            require(glyph != nullptr, "Shipped action regression font lacks a printable glyph");
            shipped_glyphs.push_back(*glyph);
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
            const auto raw = world.render.measure_text(action_font, copy);
            require(raw == mattmath::Vector2F(item == MenuItem::keyboard_jump ? 355.0f : 139.0f, 62.0f) &&
                raw.y + real_controls.art.focus_padding.y * 2.0f == 78.0f &&
                raw.y + real_controls.art.focus_padding.y * 2.0f > real_controls.layout.spacing.y,
                "Shipped action regression no longer exercises the actual 62-high descenders in a 74-high row");
            UiText text("shipped-action", copy, real_controls.item_font, real_controls.layout.position,
                &world.render, real_controls.art.ink);
            const float scale = std::min({ 1.0f, action_width / raw.x, action_height / raw.y });
            text.set_scale(scale);
            require(scale == 58.0f / 62.0f && text.text() == copy && text.bounds().height == 58.0f &&
                text.bounds().width == raw.x * scale,
                "Shipped action did not retain its complete label with a uniform 58/62 fit");
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

    void check_help_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : help_text)
            {
                d["menu_text"]["help"][entry.key].SetString(std::string(entry.key) == "controls" ?
                    " A\xC2\xA0\xE2\x80\x94" "B {jump} \n C " : (entry.multiline ?
                    " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B "), d.GetAllocator());
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : help_text)
        {
            const bool controls = std::string(entry.key) == "controls";
            require(normalized.menu_text().help.*entry.field == (controls ? L" A -B {jump} \n C " :
                (entry.multiline ? L" A -B \n C " : L" A -B ")),
                std::string("Help text lost authored spacing/LF or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("help.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string copy = controls ? std::string("{jump}\n") + invalid : invalid;
                    d["menu_text"]["help"][entry.key].SetString(copy.c_str(), d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        const std::string copy = controls ? std::string("{jump}\n") + invalid : invalid;
                        d["menu_text"]["help"][entry.key].SetString(copy.c_str(), d.GetAllocator());
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
                d["menu_text"]["help"][entry.key].SetString(controls ? "Glyph~ {jump}" : "Glyph~", d.GetAllocator());
            }, &world);
        }
        const auto single_line_blocks = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["controls"].SetString("Controls {jump}");
            d["menu_text"]["help"]["pad_association"].SetString("Slots");
        });
        single_line_blocks.validate_resources(world.render, world.audio);
        require(single_line_blocks.menu_text().help.controls == L"Controls {jump}" &&
            controls_text::controls_help(single_line_blocks.menu_text().help.controls, Key::space) == L"Controls Space" &&
            single_line_blocks.menu_text().help.pad_association == L"Slots",
            "Help blocks replaced authored copy with fixed line counts");
        variants.rejects("help.aim_cutoff", [](rapidjson::Document& d)
        {
            d["menu_text"]["help"]["aim_cutoff"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);
        for (const char* invalid : { "Controls", "{jump} {jump}", "{key}", "{Jump}", "{jump", "jump}",
            "{{jump}}", "{ju{jump}mp}", "{jump} {}", "{jump} {unknown}", "{jump}}" })
        {
            variants.rejects("help.controls", [invalid](rapidjson::Document& d)
            { d["menu_text"]["help"]["controls"].SetString(invalid, d.GetAllocator()); });
            bool rejected = false;
            try { std::ignore = controls_text::controls_help(std::wstring(invalid, invalid + std::strlen(invalid)), Key::space); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Public controls-help formatter accepted a missing, duplicate or malformed jump token");
        }
        const auto keys = controls_text::jump_keys();
        require(keys.size() == std::size(jump_labels), "Controls formatting label domain changed from A-Z and Space");
        const auto& controls = single_line_blocks.controls_settings();
        const auto font = world.render.resolve_sprite_font(controls.help_font);
        for (size_t index = 0; index < std::size(jump_labels); ++index)
        {
            const JumpLabel& entry = jump_labels[index];
            require(keys[index] == entry.key, "Controls formatting domain depends on enum ordinals or lost a physical key");
            const std::wstring copy = controls_text::controls_help(single_line_blocks.menu_text().help.controls, entry.key);
            require(copy == expected_jump_help(single_line_blocks.menu_text().help.controls, entry.label) &&
                world.render.can_render(font, copy), "Actual Controls help font cannot draw an independently expected key variant");
            const auto measured = world.render.measure_text(font, copy);
            const float scale = std::min({ 1.0f, controls.controls_size.x / measured.x, controls.controls_size.y / measured.y });
            UiText widget("controls-key-variant", copy, controls.help_font, controls.controls_position, &world.render, controls.art.ink);
            widget.set_scale(scale);
            require(std::isfinite(scale) && scale > 0.0f && scale <= 1.0f &&
                mattmath::RectangleF(controls.controls_position, controls.controls_size).contains(widget.bounds()),
                "An actual-font Controls key variant lost its finite positive uniform fitting contract");
            const std::wstring expected_reminder = std::wstring(L"A / D  Move    |    S  Drop through\n") + entry.label +
                L"  Jump    |    Left click  Shoot\nMouse  Aim    |    Esc  Pause";
            require(controls_text::keyboard_controls_reminder(entry.key) == expected_reminder,
                "Keyboard opening reminder retained Space or changed its fixed movement/aim/pause instructions");
        }
        // The font has a working stand-in and can draw the complete default
        // Space copy. Only an actual substituted Z is absent from its atlas.
        const Font* source = world.render.font(font);
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character == U'Z') { continue; }
            const Glyph* glyph = source->find(character);
            require(glyph != nullptr, "Missing-Z font requires the actual complete Controls help font");
            glyphs.push_back(*glyph);
        }
        auto missing_z = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), source->line_spacing());
        missing_z->set_stand_in(U'?');
        world.render.add_font("menu_controls_help_without_z", std::move(missing_z));
        const auto missing_font = world.render.resolve_sprite_font("menu_controls_help_without_z");
        require(world.render.can_render(missing_font, L"Controls Space") && !world.render.can_render(missing_font, L"Z") &&
            world.render.measure_text(missing_font, L"Z").x > 0.0f,
            "Missing-Z font negative control does not distinguish atlas coverage from its positive stand-in");
        variants.rejects("help.controls", [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_controls_help_without_z");
            d["menu_text"]["help"]["controls"].SetString("{jump}");
        }, &world);

        const auto jump_normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : keyboard_jump_text)
            {
                const char* copy = entry.key_template ? " A\xC2\xA0\xE2\x80\x94" "B {key} " :
                    (entry.multiline ? " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ");
                d["menu_text"]["keyboard_jump"][entry.key].SetString(copy, d.GetAllocator());
            }
        });
        jump_normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : keyboard_jump_text)
        {
            const std::wstring expected = entry.key_template ? L" A -B {key} " :
                (entry.multiline ? L" A -B \n C " : L" A -B ");
            require(jump_normalized.menu_text().keyboard_jump.*entry.field == expected,
                std::string("Keyboard Jump text lost authored spacing, normalization or its template: ") + entry.key);
            const std::string field = std::string("keyboard_jump.") + entry.key;
            for (const char* invalid : { "   ", "line\rbreak", "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["keyboard_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            if (!entry.multiline)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["keyboard_jump"][entry.key].SetString(entry.key_template ? "line\n{key}" : "line\nbreak", d.GetAllocator()); });
            }
            else
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", "A\n \xC2\xA0 " })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["keyboard_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
                }
            }
            if (entry.key_template)
            {
                for (const char* invalid : { "Current", "{key} {key}", "{jump}", "{Key}", "{key", "key}",
                    "{{key}}", "{ke{key}y}", "{key} {}", "{key} {unknown}", "{key}}" })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["keyboard_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
                }
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const char* font_group = std::string(entry.key) == "heading" ? "heading" : nullptr;
                if (font_group != nullptr) { d["menu_style"]["fonts"][font_group].SetString("menu_text_limited"); }
                else { d["controls_settings"][std::string(entry.key) == "capture" || std::string(entry.key) == "reset" ?
                    "item_font" : "help_font"].SetString("menu_text_limited"); }
                d["menu_text"]["keyboard_jump"][entry.key].SetString(entry.key_template ? "Glyph~ {key}" : "Glyph~", d.GetAllocator());
            }, &world);
        }
        const auto shipped = variants.load([](rapidjson::Document&) {});
        const auto& jump = shipped.menu_text().keyboard_jump;
        const auto jump_font = world.render.resolve_sprite_font(shipped.controls_settings().help_font);
        for (const JumpLabel& entry : jump_labels)
        {
            const std::wstring label(entry.label);
            const std::wstring full_help = controls_text::controls_help(shipped.menu_text().help.controls, entry.key);
            require(full_help == expected_jump_help(help_text[0].text, entry.label) && world.render.can_render(jump_font, full_help),
                "A complete authored Controls help variant changed literal copy or lost actual font coverage");
            const auto full_size = world.render.measure_text(jump_font, full_help);
            const auto& look = shipped.controls_settings();
            const float full_scale = std::min({ 1.0f, look.controls_size.x / full_size.x, look.controls_size.y / full_size.y });
            UiText full_widget("complete-controls-key-variant", full_help, look.help_font, look.controls_position, &world.render, look.art.ink);
            full_widget.set_scale(full_scale);
            require(std::isfinite(full_scale) && full_scale > 0.0f && full_scale <= 1.0f &&
                mattmath::RectangleF(look.controls_position, look.controls_size).contains(full_widget.bounds()),
                "A complete actual-font Controls variant escaped its fitted help rectangle");
            const std::wstring current = std::wstring(L"Current: ") + label;
            const std::wstring pending = std::wstring(L"Pending: ") + label;
            const std::wstring release = std::wstring(L"Release ") + label + L" to keep this choice.";
            require(jump.current_key(label) == current && jump.pending_key(label) == pending && jump.release_key(label) == release &&
                world.render.can_render(jump_font, current) && world.render.can_render(jump_font, pending) &&
                world.render.can_render(jump_font, release), "Keyboard Jump's actual font lost one of its 27 complete formatted labels");
            require(jump.help_block(label, L"Space") == current + L"\nPending: Space\n" + jump.help &&
                jump.help_block(L"Space", label) == L"Current: Space\n" + pending + L'\n' + jump.help,
                "Keyboard Jump help joined the wrong current/pending labels or lost its captured authored help");
        }
        for (const wchar_t* invalid : { L"", L"space", L"a", L"Enter", L"A + B", L"{key}" })
        {
            for (const auto formatter : { &KeyboardJumpMenuText::current_key, &KeyboardJumpMenuText::pending_key,
                &KeyboardJumpMenuText::release_key })
            {
                bool rejected = false;
                try { std::ignore = (jump.*formatter)(invalid); }
                catch (const std::invalid_argument&) { rejected = true; }
                require(rejected, "Keyboard Jump formatter accepted a label outside A-Z and Space");
            }
        }
        for (const Key invalid : { Key::none, Key::enter, Key::escape })
        {
            bool rejected = false;
            try { std::ignore = controls_text::controls_help(shipped.menu_text().help.controls, invalid); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Controls help formatter accepted a key outside its finite label domain");
        }
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
                std::wstring authored = is_controls ? controls_text::controls_help(look.menu_text().help.controls, Key::space) :
                    look.menu_text().help.*entry.field;
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
            d["menu_text"]["help"]["controls"].SetString("Fit {jump}\nFit");
            d["menu_text"]["help"]["pad_association"].SetString("Fit\nFit");
        });
        short_blocks.validate_resources(world.render, world.audio);
        check_blocks(short_blocks, false, false);
        const auto wide_blocks = variants.load([](rapidjson::Document& d)
        {
            for (const char* key : { "controls", "pad_association" })
            {
                const std::string text = std::string(200, 'W') + (std::string(key) == "controls" ? " {jump}" : "");
                d["menu_text"]["help"][key].SetString(text.c_str(), d.GetAllocator());
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
                const std::string text = std::string(key) == "controls" ? "{jump}\n" + lines : lines;
                d["menu_text"]["help"][key].SetString(text.c_str(), d.GetAllocator());
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
            // The synthetic G is 620 wide to probe caption overflow. Keep it
            // out of this unrelated child status so the caption owns the case.
            d["menu_text"]["keyboard_jump"]["capture_cancelled"].SetString("Cancelled.");
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
