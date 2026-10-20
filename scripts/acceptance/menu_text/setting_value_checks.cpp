#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Setting percentages and On/Off/Random values.
namespace menu_text_acceptance
{
    void check_setting_percentage_text(Variants& variants, World& world, const MenuText& defaults)
    {
        for (int value = 0; value <= 100; ++value)
        {
            require(defaults.setting_percentage(value) == std::to_wstring(value) + L"%",
                "Default setting percentage changed its exact integer, percent sign or inclusive domain");
        }
        require(defaults.setting_percentage(17) == L"17%" && defaults.setting_percentage(100) == L"100%",
            "Setting percentage lost a non-step value or the inclusive upper endpoint");
        PlaySettings inherited;
        const int rounded_movement = static_cast<int>(std::lround(inherited.movement_deadzone(0, 0.999f) * 100.0f));
        require(inherited.sticks[0].movement_deadzone == -1 && rounded_movement == 100 &&
            defaults.setting_percentage(rounded_movement) == L"100%",
            "Inherited movement rounding no longer reaches 100 without clamping the saved deadzone profile");
        const auto reject_format = [](auto format)
        {
            bool rejected = false;
            try { format(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Invalid setting percentage argument or direct template formatted silently");
        };
        for (int invalid : {std::numeric_limits<int>::min(), -1, 101, std::numeric_limits<int>::max()})
        {
            reject_format([&] { defaults.setting_percentage(invalid); });
        }
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["percentage"].SetString("  % {value}\xC2\xA0\xE2\x80\x9C" "level\xE2\x80\x9D\xE2\x80\x94" "set\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().values.percentage == L"  % {value} \"level\"-set...  " &&
            normalized.menu_text().setting_percentage(17) == L"  % 17 \"level\"-set...  ",
            "Setting percentage lost normalized punctuation, authored outer spaces or literal placement");
        const auto literal = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["values"]["percentage"].SetString("value={value} literal%%");
        });
        literal.validate_resources(world.render, world.audio);
        require(literal.menu_text().setting_percentage(0) == L"value=0 literal%%",
            "Setting percentage treated authored percent signs as another formatting language");
        for (const char* blank : {"   ", " \xC2\xA0 "})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["percentage"].SetString(blank, d.GetAllocator()); });
        }
        for (const char* extra : {"\n", "\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
            {
                const std::string invalid = std::string("{value}%") + extra;
                d["menu_text"]["values"]["percentage"].SetString(invalid.c_str(), d.GetAllocator());
            });
        }
        for (const char* invalid : {"No token", "{value}{value}", "{Value}", "{value }", "{{value}}", "{value",
            "value}", "{value}{unknown}", "{va{value}lue}", "{value}{count}", "{value}%{"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
                { d["menu_text"]["values"]["percentage"].SetString(invalid, d.GetAllocator()); });
            MenuText mutated = defaults;
            mutated.values.percentage = std::wstring(invalid, invalid + std::strlen(invalid));
            reject_format([&] { mutated.setting_percentage(17); });
        }
        for (const wchar_t* invalid : {L"", L"   ", L"{value}\n", L"{value}\r", L"{value}\t", L"{value}\x01",
            L"{value}\x7f", L"{value}\u00e9", L"{value}\u00a0", L"{value}\u2014"})
        {
            MenuText mutated = defaults;
            mutated.values.percentage = invalid;
            reject_format([&] { mutated.setting_percentage(17); });
        }
        MenuText nul = defaults;
        nul.values.percentage.push_back(L'\0');
        reject_format([&] { nul.setting_percentage(17); });
    }

    void check_setting_percentage_rows(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, char32_t missing = U'\0', bool zero_digits = false,
            float tilde_offset = 0.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                const bool zero = zero_digits && ((character >= U'0' && character <= U'9') || character == U'%');
                glyphs.push_back({character, mattmath::RectangleI(0, 0, zero ? 0 : character == U'~' ? width : 8,
                    character == U'~' ? height : 16), character == U'~' ? tilde_offset : 0.0f, 0.0f, 0.0f});
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        add_font("menu_percentage_width", 1000, 16);
        add_font("menu_percentage_height", 8, 200);
        add_font("menu_percentage_both", 2000, 2000);
        add_font("menu_percentage_exact", 308, 58);
        add_font("menu_percentage_missing8", 8, 16, U'8');
        add_font("menu_percentage_missing1", 8, 16, U'1');
        add_font("menu_percentage_zero", 8, 16, U'\0', true);
        add_font("menu_percentage_nonfinite", 8, 16, U'\0', false, std::numeric_limits<float>::infinity());
        const auto check_rows = [&](const Presentation& look, int fit)
        {
            const auto& controls = look.controls_settings();
            MenuStyle style = look.menu();
            style.item_font = controls.item_font;
            const auto& art = controls.art;
            const mattmath::Vector2F limit(controls.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset -
                controls.layout.position.x - controls.layout.spacing.x,
                controls.layout.spacing.y - art.focus_padding.y * 2.0f);
            require(limit == mattmath::Vector2F(332.0f, 58.0f), "Percentage rows lost the actual Controls/Sticks value limits");
            struct Use { int row; MenuItem label; };
            constexpr Use uses[]{{2, MenuItem::music_volume}, {3, MenuItem::sfx_volume},
                {1, MenuItem::move_deadzone}, {2, MenuItem::aim_deadzone}};
            for (const auto& use : uses)
            {
                const auto position = controls.layout.position + mattmath::Vector2F(0.0f,
                    controls.layout.spacing.y * static_cast<float>(use.row));
                const auto value_position = position + mattmath::Vector2F(controls.layout.spacing.x, 0.0f);
                OptionRow row("percentage", look.menu_text().item(use.label), position, value_position,
                    style, art.ink, &world.render, {}, &art, limit);
                UiText* value = row_text(row, "percentage_value");
                UiText* label = row_text(row, "percentage_label");
                require(value != nullptr && label != nullptr, "Percentage fixture lost its real OptionRow widgets");
                const auto label_bounds = label->bounds();
                std::wstring copy = look.menu_text().setting_percentage(17);
                const auto retained = copy;
                const auto raw = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), retained);
                row.set_value(copy);
                copy = L"Changed after set_value";
                const auto bounds = value->bounds();
                require(value->text() == retained && bounds.position() == value_position && label->bounds() == label_bounds &&
                    bounds.width > 0.0f && bounds.height > 0.0f && bounds.width <= 332.01f && bounds.height <= 58.01f,
                    "Percentage OptionRow lost its copied text, fixed position/label or fitted dimensions");
                if (fit == 1)
                {
                    require(raw == mattmath::Vector2F(1024.0f, 20.0f) && std::abs(bounds.width - 332.0f) <= 0.01f &&
                        std::abs(bounds.height - 6.484375f) <= 0.01f,
                        "Long percentage copy did not retain its independent width-only fit");
                }
                else if (fit == 2 || fit == 3)
                {
                    const float expected_width = fit == 2 ? 9.28f : 58.696f;
                    require(raw == (fit == 2 ? mattmath::Vector2F(32.0f, 200.0f) : mattmath::Vector2F(2024.0f, 2000.0f)) &&
                        std::abs(bounds.height - 58.0f) <= 0.01f && std::abs(bounds.width - expected_width) <= 0.01f,
                        "Percentage copy did not fit height after its independent width decision");
                }
                else if (fit == 4)
                {
                    require(raw == mattmath::Vector2F(332.0f, 58.0f) && bounds == mattmath::RectangleF(value_position, raw),
                        "Exact percentage copy lost inclusive contact with its unscaled column and row limits");
                }
                else { require(bounds == mattmath::RectangleF(value_position, raw), "Default percentage was enlarged or unnecessarily shrunk"); }
                auto padded = bounds;
                padded.inflate(art.focus_padding);
                const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, style.footer_y) - controls.box_size) / 2.0f,
                    controls.box_size);
                require(paper.contains(padded) && padded.right() <= controls.controls_position.x &&
                    padded.bottom() <= controls.status_position.y && bounds.height + art.focus_padding.y * 2.0f <= controls.layout.spacing.y + 0.01f,
                    "Actual percentage value/focus padding escaped its paper, row, help column or status boundary");
                MenuText short_copy = look.menu_text();
                short_copy.values.percentage = L"{value}%";
                row.set_value(short_copy.setting_percentage(0));
                const auto short_size = world.render.measure_text(world.render.resolve_sprite_font(style.item_font), L"0%");
                require(value->text() == L"0%" && value->bounds() == mattmath::RectangleF(value_position, short_size) &&
                    label->bounds() == label_bounds, "Short percentage replacement retained a stale shrink scale or changed its label");
            }
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio);
        check_rows(defaults, 0);
        const char* fonts[]{"menu_percentage_width", "menu_percentage_height", "menu_percentage_both", "menu_percentage_exact"};
        for (int fit = 1; fit <= 4; ++fit)
        {
            const auto fitted = variants.load([&](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString(fonts[fit - 1], d.GetAllocator());
                d["menu_text"]["values"]["percentage"].SetString("~{value}%");
            });
            fitted.validate_resources(world.render, world.audio);
            check_rows(fitted, fit);
        }
        variants.rejects("values.percentage", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_missing8"); }, &world);
        // Pad 1's earlier selector guard owns this shared-font failure. The
        // independent missing8 case above reaches the new 101-output check.
        variants.rejects("labels.pad", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_missing1"); }, &world);
        variants.rejects("values.percentage", [](rapidjson::Document& d)
            { d["controls_settings"]["item_font"].SetString("menu_percentage_zero"); }, &world);
        for (const char* font : {"menu_text_limited", "menu_percentage_nonfinite"})
        {
            variants.rejects("values.percentage", [&](rapidjson::Document& d)
            {
                d["controls_settings"]["item_font"].SetString(font, d.GetAllocator());
                d["menu_text"]["values"]["percentage"].SetString("~{value}%");
            }, &world);
        }
        const auto unrelated_font = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["values"]["percentage"].SetString("~{value}%");
        });
        unrelated_font.validate_resources(world.render, world.audio);
    }

    void check_value_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : value_text)
            {
                d["menu_text"]["values"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : value_text)
        {
            require(normalized.menu_text().values.*entry.field == L"  A \"B\"-C...  ",
                std::string("Fixed setting value lost authored spacing or punctuation/NBSP normalization: ") + entry.key);
            const std::string field = std::string("values.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["values"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["values"][entry.key].SetString("Glyph~");
            }, &world);
            if (entry.field != &MenuValueText::random)
            {
                // The Options font can draw this glyph, but the independently
                // selected Controls item font must support On and Off too.
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["controls_settings"]["item_font"].SetString("menu_text_limited");
                    d["menu_text"]["values"][entry.key].SetString("Glyph~");
                }, &world);
            }
        }
        const auto random_uses_shared_item_font = variants.load([](rapidjson::Document& d)
        {
            d["controls_settings"]["item_font"].SetString("menu_text_limited");
            // Keep the supported '~' probe below the arena preview border;
            // unrelated descenders in "Glyph" would exceed its name band.
            d["menu_text"]["values"]["random"].SetString("G~");
        });
        random_uses_shared_item_font.validate_resources(world.render, world.audio);
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? 0 : 8, 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_value_zero_width", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_value_zero_width"), L"~").x == 0.0f,
            "Nonpositive value-size fixture has unexpected font advance");
        for (const auto& entry : value_text)
        {
            variants.rejects(std::string("values.") + entry.key, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_value_zero_width");
                d["menu_text"]["values"][entry.key].SetString("~");
            }, &world);
            if (entry.field != &MenuValueText::random)
            {
                variants.rejects(std::string("values.") + entry.key, [&](rapidjson::Document& d)
                {
                    d["controls_settings"]["item_font"].SetString("menu_value_zero_width");
                    d["menu_text"]["values"][entry.key].SetString("~");
                }, &world);
            }
        }
    }

    void check_value_fitting(Variants& variants, World& world)
    {
        struct ValueUse { bool options; int row; MenuItem label; std::wstring MenuValueText::* field; };
        constexpr ValueUse uses[]{
            { true, 1, MenuItem::fullscreen, &MenuValueText::on },
            { true, 1, MenuItem::fullscreen, &MenuValueText::off },
            { true, 2, MenuItem::team_palette, &MenuValueText::random },
            { false, 4, MenuItem::view_shake, &MenuValueText::on },
            { false, 4, MenuItem::view_shake, &MenuValueText::off },
            { false, 5, MenuItem::aim_marker, &MenuValueText::on },
            { false, 5, MenuItem::aim_marker, &MenuValueText::off },
            { false, 6, MenuItem::reduce_effects, &MenuValueText::on },
            { false, 6, MenuItem::reduce_effects, &MenuValueText::off }
        };
        const auto check_rows = [&](const Presentation& look, bool width_limited, bool height_limited)
        {
            for (const auto& use : uses)
            {
                const auto& main = look.main_menu();
                const auto& controls = look.controls_settings();
                MenuStyle style = look.menu();
                if (!use.options) { style.item_font = controls.item_font; }
                const auto art = use.options ? options_art(main) : controls.art;
                const auto box_size = use.options ? main.options_box_size : controls.box_size;
                const auto spacing = use.options ? main.options_spacing : controls.layout.spacing;
                const auto label_position = (use.options ? main.options_position : controls.layout.position) +
                    mattmath::Vector2F(0.0f, spacing.y * static_cast<float>(use.row));
                const auto value_position = use.options ? mattmath::Vector2F(main.options_value_x, label_position.y) :
                    label_position + mattmath::Vector2F(spacing.x, 0.0f);
                const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, look.menu().footer_y) - box_size) / 2.0f, box_size);
                const mattmath::Vector2F limit(use.options ?
                    paper.right() - art.shadow_offset - art.focus_padding.x - value_position.x :
                    controls.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset - value_position.x,
                    spacing.y - art.focus_padding.y * 2.0f);
                require(limit == (use.options ? mattmath::Vector2F(930.0f, 86.0f) : mattmath::Vector2F(332.0f, 58.0f)),
                    "Authored value fixture no longer exercises the shipped Options/Controls fitting limits");
                const auto font = world.render.resolve_sprite_font(style.item_font);
                std::wstring authored = look.menu_text().values.*use.field;
                const auto measured = world.render.measure_text(font, authored);
                float scale = 1.0f;
                if (measured.x > limit.x) { scale = limit.x / measured.x; }
                const float width_scale = scale;
                if (measured.y * scale > limit.y) { scale = limit.y / measured.y; }
                require(width_limited == (measured.x > limit.x) &&
                    height_limited == (measured.y * width_scale > limit.y),
                    "Authored value fixture does not independently exercise its expected fitting constraints");
                int adjustments = 0;
                OptionRow row("authored_value", look.menu_text().item(use.label), label_position, value_position,
                    style, art.ink, &world.render,
                    [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; }, &art, limit);
                UiText* value = row_text(row, "authored_value_value");
                UiText* label = row_text(row, "authored_value_label");
                require(value != nullptr && label != nullptr, "Authored value fixture lost its real OptionRow text widgets");
                const auto retained_label = label->bounds();
                row.set_value(authored);
                const auto retained = authored;
                authored = L"Changed after set_value";
                const auto bounds = value->bounds();
                require(value->text() == retained && bounds.position() == value_position &&
                    std::isfinite(bounds.width) && std::isfinite(bounds.height) && bounds.width > 0.0f && bounds.height > 0.0f &&
                    std::abs(bounds.width - measured.x * scale) <= 0.01f &&
                    std::abs(bounds.height - measured.y * scale) <= 0.01f &&
                    bounds.width <= limit.x + 0.01f && bounds.height <= limit.y + 0.01f &&
                    bounds.height + art.focus_padding.y * 2.0f <= spacing.y + 0.01f && label->bounds() == retained_label,
                    "Actual OptionRow lost its value copy, moved it, changed its label, or escaped width-then-height fitting");
                auto padded = bounds;
                padded.inflate(art.focus_padding);
                require(paper.contains(padded) && (use.options ||
                    (padded.right() <= controls.controls_position.x && padded.bottom() <= controls.status_position.y)),
                    "Fitted setting value or focus padding escaped its paper, help column or status boundary");
                if (height_limited)
                {
                    require(std::abs(bounds.height + art.focus_padding.y * 2.0f - spacing.y) <= 0.01f,
                        "Height-fitted value did not retain inclusive contact with its padded row limit");
                }
                else if (width_limited)
                {
                    require(std::abs(bounds.width - limit.x) <= 0.01f,
                        "Width-fitted value did not retain inclusive contact with its column limit");
                }
                else { require(bounds == mattmath::RectangleF(value_position, measured), "Short values were enlarged"); }
                row.set_value(L"Fit");
                require(value->text() == L"Fit" && value->bounds() == mattmath::RectangleF(value_position,
                    world.render.measure_text(font, L"Fit")) && label->bounds() == retained_label,
                    "Changing a long/tall value retained its old scale or changed the authored label");
                row.adjust(Direction::right);
                require(adjustments == 1, "Authored value fitting lost the row's right adjustment action");
                row.adjust(Direction::left);
                require(adjustments == 0, "Authored value fitting changed the row's adjustment action");
                row.set_hidden(true);
                DrawList no_device;
                row.draw(no_device);
            }
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        require(defaults.menu().item_font != defaults.controls_settings().item_font,
            "Authored values must exercise the separate shipped Options and Controls fonts");
        defaults.validate_resources(world.render, world.audio);
        check_rows(defaults, false, false);
        const auto wide = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : value_text)
            {
                d["menu_text"]["values"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }
        });
        wide.validate_resources(world.render, world.audio);
        check_rows(wide, true, false);
        for (const bool wide_glyph : { false, true })
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({ character, mattmath::RectangleI(0, 0,
                    character == U'~' && wide_glyph ? 2000 : 8,
                    character == U'~' ? (wide_glyph ? 2000 : 200) : 16), 0.0f, 0.0f, 0.0f });
            }
            const char* font_name = wide_glyph ? "menu_value_width_then_height" : "menu_value_height_only";
            world.render.add_font(font_name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
            const auto tall = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
                d["controls_settings"]["item_font"].SetString(font_name, d.GetAllocator());
                for (const auto& entry : value_text) { d["menu_text"]["values"][entry.key].SetString("~"); }
                // Random also supplies the arena name, whose existing fit
                // shrinks only to preview width. A narrow synthetic preview
                // keeps that separate consumer valid while all nine real
                // OptionRow uses still exercise their width/height limits.
                d["main_menu"]["stage_select"]["preview_size"]["x"].SetFloat(2.0f);
            });
            tall.validate_resources(world.render, world.audio);
            check_rows(tall, wide_glyph, true);
        }
    }
}
