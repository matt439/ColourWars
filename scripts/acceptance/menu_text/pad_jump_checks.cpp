#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

namespace menu_text_acceptance
{
    void check_pad_jump_text(Variants& variants, World& world)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : pad_jump_text)
            {
                const char* copy = entry.button_template ? " A\xC2\xA0\xE2\x80\x94" "B {button} " :
                    (entry.multiline ? " A\xC2\xA0\xE2\x80\x94" "B \n C " : " A\xC2\xA0\xE2\x80\x94" "B ");
                d["menu_text"]["pad_jump"][entry.key].SetString(copy, d.GetAllocator());
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : pad_jump_text)
        {
            const std::wstring expected = entry.button_template ? L" A -B {button} " :
                (entry.multiline ? L" A -B \n C " : L" A -B ");
            require(normalized.menu_text().pad_jump.*entry.field == expected,
                std::string("Pad Jump text lost authored spacing, normalization or its template: ") + entry.key);
            const std::string field = std::string("pad_jump.") + entry.key;
            for (const char* invalid : { "   ", "line\rbreak", "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["pad_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            if (entry.multiline)
            {
                for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB" })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["pad_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
                }
            }
            else
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["pad_jump"][entry.key].SetString(entry.button_template ? "line\n{button}" : "line\nbreak", d.GetAllocator()); });
            }
            if (entry.button_template)
            {
                for (const char* invalid : { "Current", "{button} {button}", "{jump}", "{Button}", "{button", "button}",
                    "{{button}}", "{bu{button}tton}", "{button} {}", "{button} {unknown}", "{button}}" })
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["pad_jump"][entry.key].SetString(invalid, d.GetAllocator()); });
                }
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                if (std::string(entry.key) == "heading") { d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited"); }
                else { d["controls_settings"][std::string(entry.key) == "button" || std::string(entry.key) == "reset" ?
                    "item_font" : "help_font"].SetString("menu_text_limited"); }
                d["menu_text"]["pad_jump"][entry.key].SetString(entry.button_template ? "Glyph~ {button}" : "Glyph~", d.GetAllocator());
            }, &world);
        }

        const auto shipped = variants.load([](rapidjson::Document&) {});
        const auto& copy = shipped.menu_text().pad_jump;
        const auto& look = shipped.controls_settings();
        const auto help_font = world.render.resolve_sprite_font(look.help_font);
        const mattmath::RectangleF help_bounds(look.controls_position, look.controls_size);
        MenuStyle style = shipped.menu(); style.item_font = look.item_font;
        const mattmath::Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
            look.art.shadow_offset - look.layout.position.x - look.layout.spacing.x,
            look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
        for (std::wstring_view current : { L"A", L"B", L"X", L"Y" })
        {
            require(copy.current_button(current) == L"Current: " + std::wstring(current) + L" / LT" &&
                copy.pending_button(current) == L"Pending: " + std::wstring(current) + L" / LT",
                "Pad Jump wrappers changed selected face, LT alias or authored copy");
            OptionRow button("pad_jump", copy.button, look.layout.position + mattmath::Vector2F(0.0f, look.layout.spacing.y),
                look.layout.position + look.layout.spacing, style, look.art.ink, &world.render, {}, &look.art, value_limit);
            button.set_value(std::wstring(current));
            const UiText* value = row_text(button, "pad_jump_value");
            require(value && value->text() == current && value->bounds().width <= value_limit.x &&
                value->bounds().height <= value_limit.y && value->bounds().width > 0.0f && value->bounds().height > 0.0f,
                "Actual Pad Jump OptionRow lost a complete face-button label or exceeded its measured column");
            for (std::wstring_view pending : { L"A", L"B", L"X", L"Y" })
            {
                const std::wstring block = copy.help_block(current, pending);
                require(block == copy.current_button(current) + L'\n' + copy.pending_button(pending) + L'\n' + copy.help &&
                    world.render.can_render(help_font, block), "Pad Jump complete help lost current/pending identity or font coverage");
                const auto raw = world.render.measure_text(help_font, block);
                const float scale = std::min({ 1.0f, look.controls_size.x / raw.x, look.controls_size.y / raw.y });
                UiText actual("pad_jump_help", block, look.help_font, look.controls_position, &world.render, look.art.ink);
                actual.set_scale(scale);
                require(std::isfinite(scale) && scale > 0.0f && help_bounds.contains(actual.bounds()),
                    "Pad Jump help escaped the real-font fitted help region");
            }
        }
        for (std::wstring_view invalid : { L"", L"a", L"Start", L"LT", L"Space", L"AA", L"{button}" })
        {
            bool current_rejected = false, pending_rejected = false;
            try { std::ignore = copy.current_button(invalid); }
            catch (const std::invalid_argument&) { current_rejected = true; }
            try { std::ignore = copy.pending_button(invalid); }
            catch (const std::invalid_argument&) { pending_rejected = true; }
            require(current_rejected && pending_rejected, "Pad Jump formatter admitted an unsupported or noncanonical label");
        }
        variants.rejects("pad_jump.heading", [](rapidjson::Document& d)
        { const std::string wide(200, 'W'); d["menu_text"]["pad_jump"]["heading"].SetString(wide.c_str(), d.GetAllocator()); }, &world);
    }
}
