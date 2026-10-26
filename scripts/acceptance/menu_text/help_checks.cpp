#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// Help text: fonts, the Controls help {jump} formatting, fitting and bounds.
namespace menu_text_acceptance
{
    namespace
    {
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
            // Move is20 high plus8 focus padding. The28-high F starts at
            // 447-28=419, exactly the response track's395+24 bottom.
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
