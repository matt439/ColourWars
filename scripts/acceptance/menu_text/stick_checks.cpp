#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The Pad sticks readouts.
namespace menu_text_acceptance
{
    namespace
    {
    std::wstring stick_readout(const MenuText& text, const char* key, int a = 0, int b = 0, int c = 0, int d = 0)
    {
        const std::string role(key);
        if (role == "connected") { return text.stick_connection(3, true); }
        if (role == "disconnected") { return text.stick_connection(3, false); }
        if (role == "raw_movement") { return text.stick_raw_movement(a); }
        if (role == "movement_response") { return text.stick_movement_response(a); }
        if (role == "aim_inactive") { return text.stick_aim_inactive(a, b); }
        return text.stick_aim_active(a, b, c, d);
    }

    template<class Action> void rejects_stick_format(Action action)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid Pad Sticks template/argument was formatted silently");
    }

    void compact_stick_readouts(rapidjson::Document& d)
    {
        auto& text = d["menu_text"]["stick_readouts"];
        text["connected"].SetString("{device}");
        text["disconnected"].SetString("{device}");
        text["raw_movement"].SetString("{value}");
        text["movement_response"].SetString("{value}");
        text["aim_inactive"].SetString("{raw_x}\n{raw_y}");
        text["aim_active"].SetString("{raw_x}\n{raw_y}\n{aim_x}\n{aim_y}");
    }
    }

    void check_stick_readout_text(Variants& variants, World& world, const MenuText& defaults)
    {
        for (int slot = 0; slot < 4; ++slot)
        {
            require(defaults.stick_connection(slot, true) == L"Pad " + std::to_wstring(slot + 1) + L" - connected" &&
                defaults.stick_connection(slot, false) == L"Pad " + std::to_wstring(slot + 1) + L" - disconnected",
                "Pad Sticks device template used roster numbering or changed connected/disconnected copy");
        }
        for (int value = -100; value <= 100; ++value)
        {
            const auto a = std::to_wstring(value), b = std::to_wstring(-value);
            require(defaults.stick_raw_movement(value) == L"Left stick raw X: " + a + L"%" &&
                defaults.stick_movement_response(value) == L"Move response: " + a + L"%",
                "Pad Sticks single percentage formatter changed a rounded value or literal percent sign");
            require(defaults.stick_aim_inactive(value, -value) == L"Raw X: " + a + L"%\nRaw Y: " + b +
                L"%\nNo new aim\nRaw: hollow\nAim: solid" &&
                defaults.stick_aim_active(value, -value, -value, value) == L"Raw X: " + a + L"%\nRaw Y: " + b +
                L"%\nAim X: " + b + L"%\nAim Y: " + a + L"%\nRaw: hollow\nAim: solid",
                "Pad Sticks aim formatter changed axes, signs, percentages or authored line breaks");
        }
        for (int slot : {(std::numeric_limits<int>::min)(), -1, 4, 5, (std::numeric_limits<int>::max)()})
        {
            for (bool connected : {false, true})
            {
                rejects_stick_format([&] { defaults.stick_connection(slot, connected); });
            }
        }
        for (int value : {(std::numeric_limits<int>::min)(), -101, 101, (std::numeric_limits<int>::max)()})
        {
            rejects_stick_format([&] { defaults.stick_raw_movement(value); });
            rejects_stick_format([&] { defaults.stick_movement_response(value); });
            rejects_stick_format([&] { defaults.stick_aim_inactive(value, 0); });
            rejects_stick_format([&] { defaults.stick_aim_inactive(0, value); });
            rejects_stick_format([&] { defaults.stick_aim_active(value, 0, 0, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, value, 0, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, 0, value, 0); });
            rejects_stick_format([&] { defaults.stick_aim_active(0, 0, 0, value); });
        }
        for (const auto& entry : stick_readout_text)
        {
            const std::string field = std::string("stick_readouts.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                // The additional authored LF tests normalization independently
                // of the shipped six-line aim readout's available height.
                // Exact multiline overflow is exercised below with known glyphs.
                if (entry.multiline) { d["controls_settings"]["help_font"].SetString("menu_text_exact"); }
                const std::string original = d["menu_text"]["stick_readouts"][entry.key].GetString();
                const std::string changed = "  A\xC2\xA0\xE2\x80\x94" "B " + original + (entry.multiline ? "\n C  " : "  ");
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            });
            require(normalized.menu_text().stick_readouts.*entry.field == L"  A -B " + std::wstring(entry.text) +
                (entry.multiline ? L"\n C  " : L"  "), "Pad Sticks copy lost normalized literals or authored outer spaces/LF");
            normalized.validate_resources(world.render, world.audio);
            for (const char* invalid : {"   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stick_readouts"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (const wchar_t* extra : {L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9"})
            {
                MenuText mutated = defaults;
                (mutated.stick_readouts.*entry.field) += extra;
                rejects_stick_format([&] { stick_readout(mutated, entry.key); });
            }
            {
                MenuText mutated = defaults;
                mutated.stick_readouts.*entry.field = L" ";
                rejects_stick_format([&] { stick_readout(mutated, entry.key); });
            }
            const std::vector<const char*> invalid_lines = entry.multiline ?
                std::vector<const char*>{"\n", "\n\n", "\n \n", "\n \xC2\xA0 "} :
                std::vector<const char*>{"\nline"};
            for (const char* invalid : invalid_lines)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + invalid;
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            if (entry.multiline)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = "\n" + std::string(d["menu_text"]["stick_readouts"][entry.key].GetString());
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            for (const char* token : entry.tokens)
            {
                for (bool duplicate : {false, true})
                {
                    variants.rejects(field, [&](rapidjson::Document& d)
                    {
                        std::string changed = d["menu_text"]["stick_readouts"][entry.key].GetString();
                        if (duplicate) { changed += std::string(" ") + token; }
                        else { changed.erase(changed.find(token), std::strlen(token)); }
                        d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                    });
                    MenuText mutated = defaults;
                    std::wstring wide_token;
                    for (const char* character = token; *character != '\0'; ++character)
                    {
                        wide_token += static_cast<wchar_t>(*character);
                    }
                    auto& changed = mutated.stick_readouts.*entry.field;
                    if (duplicate) { changed += L" " + wide_token; }
                    else { changed.erase(changed.find(wide_token), wide_token.size()); }
                    rejects_stick_format([&] { stick_readout(mutated, entry.key); });
                }
            }
            for (const char* extra : {"{unknown}", "{value }", "{", "}"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + extra;
                    d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["controls_settings"]["help_font"].SetString("menu_text_limited");
                const std::string changed = std::string(d["menu_text"]["stick_readouts"][entry.key].GetString()) + "~";
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                const std::string changed = std::string(2000, 'W') + d["menu_text"]["stick_readouts"][entry.key].GetString();
                d["menu_text"]["stick_readouts"][entry.key].SetString(changed.c_str(), d.GetAllocator());
            }, &world);
        }
        const auto reordered = variants.load([](rapidjson::Document& d)
        {
            auto& text = d["menu_text"]["stick_readouts"];
            text["connected"].SetString("Ready: {device}!");
            text["disconnected"].SetString("Absent: {device}!");
            text["raw_movement"].SetString("literal% {value} literal%%");
            text["movement_response"].SetString("{value} movement");
            text["aim_inactive"].SetString("Y{raw_y} X{raw_x}");
            text["aim_active"].SetString("AY{aim_y} RX{raw_x}\nAX{aim_x} RY{raw_y}");
        });
        reordered.validate_resources(world.render, world.audio);
        const auto& copy = reordered.menu_text();
        require(copy.stick_connection(0, true) == L"Ready: Pad 1!" &&
            copy.stick_connection(3, false) == L"Absent: Pad 4!" &&
            copy.stick_raw_movement(-8) == L"literal% -8 literal%%" &&
            copy.stick_movement_response(17) == L"17 movement" &&
            copy.stick_aim_inactive(-17, 23) == L"Y23 X-17" &&
            copy.stick_aim_active(-17, 23, -4, 100) == L"AY100 RX-17\nAX-4 RY23",
            "Pad Sticks formatting fixed token order, line counts, axes or percent literals in code");
        for (const char* key : {"aim_inactive", "aim_active"})
        {
            for (const char* malformed : {"{raw_{raw_x}y}", "{raw_x}{raw_{raw_y}x}",
                "{raw_{raw_x}y}{aim_x}{aim_y}", "{raw_x}{raw_y}{aim_{aim_x}y}",
                "{raw_x}{raw_y}{aim_x}{aim_y}{raw_x}", "{raw_x}{raw_y}{aim_x}{aim_y}{unknown}",
                "{raw_x}{raw_y}{aim_x}{aim_y}{", "{{raw_x}}{raw_y}{aim_x}{aim_y}"})
            {
                variants.rejects(std::string("stick_readouts.") + key, [&](rapidjson::Document& d)
                    { d["menu_text"]["stick_readouts"][key].SetString(malformed, d.GetAllocator()); });
                MenuText mutated = defaults;
                std::wstring wide;
                for (const char* character = malformed; *character != '\0'; ++character)
                {
                    wide += static_cast<wchar_t>(*character);
                }
                (std::string(key) == "aim_active" ? mutated.stick_readouts.aim_active :
                    mutated.stick_readouts.aim_inactive) = wide;
                rejects_stick_format([&] { stick_readout(mutated, key); });
            }
        }
        std::vector<Glyph> missing_eight;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            if (character != U'8')
            {
                missing_eight.push_back({character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f});
            }
        }
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(missing_eight), 20.0f);
        limited->set_stand_in(U'?');
        world.render.add_font("menu_stick_no_eight", std::move(limited));
        variants.rejects("stick_readouts.raw_movement", [](rapidjson::Document& d)
        {
            d["controls_settings"]["help_font"].SetString("menu_stick_no_eight");
        }, &world);
    }

    void check_stick_readout_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int width, int height)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'~' ? width : 8, character == U'~' ? height : 16), 0.0f, 0.0f, 0.0f});
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        };
        struct Boundary
        {
            const char* key;
            const char* width_template;
            const char* height_template;
            int glyph_width;
            int glyph_height;
            mattmath::Vector2F position;
            float width;
            float height;
        };
        const Boundary boundaries[]{
            {"connected", "{device}~", "{device}~", 580, 77, {1120.0f, 210.0f}, 620.0f, 77.0f},
            {"disconnected", "{device}~", "{device}~", 580, 77, {1120.0f, 210.0f}, 620.0f, 77.0f},
            {"raw_movement", "{value}~", "{value}~", 588, 28, {1120.0f, 287.0f}, 620.0f, 28.0f},
            {"movement_response", "{value}~", "{value}~", 588, 28, {1120.0f, 367.0f}, 620.0f, 28.0f},
            {"aim_inactive", "{raw_x}{raw_y}~", "{raw_x}{raw_y}\n~", 316, 230, {1360.0f, 500.0f}, 380.0f, 250.0f},
            {"aim_active", "{raw_x}{raw_y}{aim_x}{aim_y}~", "{raw_x}{raw_y}\n{aim_x}{aim_y}\n~",
                252, 210, {1360.0f, 500.0f}, 380.0f, 250.0f}
        };
        const auto use = [](rapidjson::Document& d, const char* key, const std::string& font, const char* text)
        {
            compact_stick_readouts(d);
            d["controls_settings"]["help_font"].SetString(font.c_str(), d.GetAllocator());
            d["menu_text"]["stick_readouts"][key].SetString(text, d.GetAllocator());
        };
        for (const auto& entry : boundaries)
        {
            const std::string name = std::string("menu_stick_") + entry.key;
            const std::string field = std::string("stick_readouts.") + entry.key;
            add_font(name + "_width_edge", entry.glyph_width, 20);
            add_font(name + "_width_over", entry.glyph_width + 1, 20);
            add_font(name + "_height_edge", 8, entry.glyph_height);
            add_font(name + "_height_over", 8, entry.glyph_height + 1);
            for (bool height : {false, true})
            {
                const auto fits = variants.load([&](rapidjson::Document& d)
                {
                    use(d, entry.key, name + (height ? "_height_edge" : "_width_edge"),
                        height ? entry.height_template : entry.width_template);
                });
                fits.validate_resources(world.render, world.audio);
                std::wstring source = stick_readout(fits.menu_text(), entry.key, -100, -100, -100, -100);
                const std::wstring expected = source;
                UiText text(entry.key, source, fits.controls_settings().help_font, entry.position,
                    &world.render, fits.controls_settings().art.ink);
                source = L"Changed after construction";
                require(text.text() == expected && text.bounds().position() == entry.position &&
                    (height ? text.bounds().height == entry.height : text.bounds().width == entry.width),
                    "Plain Pad Sticks UiText borrowed its copy, fitted its font or missed the inclusive readout boundary");
                if (height && entry.position.x == 1360.0f)
                {
                    require(text.bounds().bottom() == 750.0f && text.text().find(L'\n') != std::wstring::npos,
                        "Full multiline aim bounds failed to reach the association region exactly");
                }
                text.set_hidden(true);
                DrawList no_device;
                text.draw(no_device);
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    use(d, entry.key, name + (height ? "_height_over" : "_width_over"),
                        height ? entry.height_template : entry.width_template);
                }, &world);
            }
        }
    }

    void check_stick_numeric_extrema(Variants& variants, World& world)
    {
        // Wide ink need not leave the greatest terminal pen. An 8 draws eighty
        // units but advances zero; 7 and 1 advance twenty while drawing little.
        // The negative bearing on 6 additionally exercises the clamp at zero.
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            const bool digit = character >= U'0' && character <= U'9';
            const int width = character == U'|' ? 1000 : character == U'8' ? 80 : digit || character == U'-' ? 1 : 8;
            const float advance = character == U'8' ? -80.0f : character == U'7' || character == U'1' ? 19.0f : 0.0f;
            glyphs.push_back({character, mattmath::RectangleI(0, 0, width, 16),
                character == U'6' ? -5.0f : 0.0f, 0.0f, advance});
        }
        world.render.add_font("menu_stick_adversarial", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const Font& font = *world.render.font(world.render.resolve_sprite_font("menu_stick_adversarial"));
        float widest_number = 0.0f, maximum_terminal_pen = 0.0f, widest_pair = 0.0f;
        for (int first = -100; first <= 100; ++first)
        {
            const std::wstring a = std::to_wstring(first);
            widest_number = std::max(widest_number, font.measure(a).x);
            // This final, very wide marker makes its right edge dominate all
            // preceding ink, exposing the attained pen through Font::measure.
            maximum_terminal_pen = std::max(maximum_terminal_pen, font.measure(a + L"|").x - 1000.0f);
            for (int second = -100; second <= 100; ++second)
            {
                widest_pair = std::max(widest_pair, font.measure(a + std::to_wstring(second)).x);
            }
        }
        require(widest_number == 101.0f && maximum_terminal_pen == 41.0f && widest_pair == 142.0f &&
            font.measure(L"-100-100").x < widest_pair && font.measure(L"-78-78").x < widest_pair &&
            font.measure(L"-77-78").x == widest_pair,
            "Adversarial font did not independently distinguish widest ink, terminal pen and composed numeric extrema");
        const auto edit = [](rapidjson::Document& d, bool active, bool outside)
        {
            compact_stick_readouts(d);
            d["controls_settings"]["help_font"].SetString("menu_stick_adversarial");
            auto& text = d["menu_text"]["stick_readouts"];
            text["aim_inactive"].SetString("{raw_x}{raw_y}");
            if (active) { text["aim_active"].SetString("{raw_x}{raw_y}{aim_x}{aim_y}"); }
            d["controls_settings"]["stick_preview"]["aim_readout_position"]["x"].SetFloat(
                (active ? 1516.0f : 1598.0f) + (outside ? 0.25f : 0.0f));
        };
        for (bool active : {false, true})
        {
            const auto fits = variants.load([&](rapidjson::Document& d) { edit(d, active, false); });
            fits.validate_resources(world.render, world.audio);
            const std::wstring attained = active ? fits.menu_text().stick_aim_active(-77, -77, -77, -78) :
                fits.menu_text().stick_aim_inactive(-77, -78);
            const float width = active ? 224.0f : widest_pair;
            require(font.measure(attained).x == width &&
                fits.controls_settings().stick_preview.aim_readout_position.x + width == 1740.0f,
                "Composed numeric extrema failed to attain the exact full readout width");
            if (active)
            {
                require(width == maximum_terminal_pen * 3.0f + widest_number,
                    "Four-token adversarial fixture lost its independently attained pen/ink maximum");
            }
            variants.rejects(active ? "stick_readouts.aim_active" : "stick_readouts.aim_inactive",
                [&](rapidjson::Document& d) { edit(d, active, true); }, &world);
        }
    }
}
