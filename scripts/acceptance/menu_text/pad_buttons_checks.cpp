#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

namespace menu_text_acceptance
{
    void check_pad_buttons_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : pad_buttons_text)
            {
                const char* copy = entry.multiline ? " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ";
                d["menu_text"]["pad_buttons"][entry.key].SetString(copy, d.GetAllocator());
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : pad_buttons_text)
        {
            const std::wstring expected = entry.multiline ? L" A -B \n C " : L" A -B ";
            require(normalized.menu_text().pad_buttons.*entry.field == expected,
                std::string("Pad Buttons text lost authored spacing or normalization: ") + entry.key);
            const std::string field = std::string("pad_buttons.") + entry.key;
            for (const char* invalid : { "   ", "line\rbreak", "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["pad_buttons"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB" })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["pad_buttons"][entry.key].SetString(invalid, d.GetAllocator()); });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["pad_buttons"][entry.key].SetString("line\nbreak", d.GetAllocator()); });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string key(entry.key);
                if (key == "heading") { d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited"); }
                else { d["controls_settings"][entry.row || key == "reset" ? "item_font" : "help_font"].SetString("menu_text_limited"); }
                d["menu_text"]["pad_buttons"][entry.key].SetString("Glyph~", d.GetAllocator());
            }, &world);
        }

        const auto shipped = variants.load([](rapidjson::Document&) {});
        const auto& copy = shipped.menu_text().pad_buttons;
        const auto& look = shipped.controls_settings();
        const auto help_font = world.render.resolve_sprite_font(look.help_font);
        const mattmath::RectangleF help_bounds(look.controls_position, look.controls_size);
        MenuStyle style = shipped.menu(); style.item_font = look.item_font;
        const mattmath::Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
            look.art.shadow_offset - look.layout.position.x - look.layout.spacing.x,
            look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
        const std::array<const std::wstring*, 6> rows{ &copy.jump, &copy.jump_alternate, &copy.shoot,
            &copy.shoot_alternate, &copy.pause, &copy.pause_alternate };
        const auto listed = pad_controls();
        std::vector<PadControl> values(listed.begin(), listed.end());
        values.push_back(PadControl::none);
        for (size_t index = 0; index < rows.size(); ++index)
        {
            require(&copy.row(index) == rows[index], "Pad Buttons row order changed");
            const mattmath::Vector2F position = look.layout.position +
                mattmath::Vector2F(0.0f, look.layout.spacing.y * static_cast<float>(index + 1));
            OptionRow row("pad_buttons", copy.row(index), position, position + mattmath::Vector2F(look.layout.spacing.x, 0.0f),
                style, look.art.ink, &world.render, {}, &look.art, value_limit);
            for (PadControl control : values)
            {
                const std::wstring label(pad_control_label(control));
                row.set_value(label);
                const UiText* value = row_text(row, "pad_buttons_value");
                require(value && value->text() == label && value->bounds().width <= value_limit.x &&
                    value->bounds().height <= value_limit.y && value->bounds().width > 0.0f && value->bounds().height > 0.0f,
                    "Actual Pad Buttons OptionRow lost a complete control label or exceeded its measured column");
            }
        }
        require(world.render.can_render(help_font, copy.help), "Pad Buttons help lost font coverage");
        const auto raw = world.render.measure_text(help_font, copy.help);
        const float scale = std::min({ 1.0f, look.controls_size.x / raw.x, look.controls_size.y / raw.y });
        UiText actual("pad_buttons_help", copy.help, look.help_font, look.controls_position, &world.render, look.art.ink);
        actual.set_scale(scale);
        require(std::isfinite(scale) && scale > 0.0f && help_bounds.contains(actual.bounds()),
            "Pad Buttons help escaped the real-font fitted help region");
        variants.rejects("pad_buttons.heading", [](rapidjson::Document& d)
        { const std::string wide(200, 'W'); d["menu_text"]["pad_buttons"]["heading"].SetString(wide.c_str(), d.GetAllocator()); }, &world);
    }
}
