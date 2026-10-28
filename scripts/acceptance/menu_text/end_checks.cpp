#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The End menu: text, headings and continuation labels.
// check_end_heading_bounds registers menu_end_heading_small_actions for
// check_end_continuation_bounds.
namespace menu_text_acceptance
{
    void check_end_text(Variants& variants, World& world)
    {
        constexpr const char* end_keys[]{ "change_teams", "change_weapons", "change_level", "exit_to_main_menu" };
        const auto normalized = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : end_keys)
            {
                d["menu_text"]["items"][key].SetString("A\xC2\xA0\xE2\x80\x94" "B");
            }
        });
        normalized.validate_resources(world.render, world.audio);
        for (size_t index = 0; index < std::size(end_keys); ++index)
        {
            require(normalized.menu_text().item(static_cast<MenuItem>(
                static_cast<size_t>(MenuItem::change_teams) + index)) == L"A -B",
                "End item did not normalize its authored punctuation and NBSP");
        }
        for (const char* key : end_keys)
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
            for (const char* invalid : { "   ", "line\nbreak", "tab\ttext", "\xC3\xA9", "\xC3" })
            {
                variants.rejects(std::string("items.") + key, [key, invalid](rapidjson::Document& d)
                {
                    d["menu_text"]["items"][key].SetString(invalid, d.GetAllocator());
                });
            }
        }

        const auto exact_items = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            // Keep the unrelated new row 0 copy at the same 24-unit width while
            // the original static-action right-edge and row 4 owners are tested.
            for (const auto& entry : end_continuation_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& end = original.end_menu();
        const auto top = (mattmath::Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f;
        const auto bottom = top + end.box_size;
        const float left_position = top.x + end.art.focus_padding.x;
        const float right_position = bottom.x - 24.0f - end.art.focus_padding.x;
        // The first static label is row 1. Exit sits on row 3 between a
        // series' rounds and row 4 after a completed match or series, a row
        // lower under Watch Replay; the validation checks rows 1 and 5, which
        // bound every row between. Watch Replay, on row 1, is checked first.
        const float bottom_position = bottom.y - 5.0f * end.layout.spacing.y -
            20.0f - end.art.focus_padding.y;
        const auto paper_edge = [&](const char* axis, float position, const char* field, float outside)
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["end_menu"]["layout"]["position"][axis].SetFloat(position);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                edit(d);
                d["end_menu"]["layout"]["position"][axis].SetFloat(outside);
            }, &world);
        };
        paper_edge("x", left_position, "items.watch_replay", left_position - 0.25f);
        paper_edge("x", right_position, "items.watch_replay", right_position + 0.25f);
        // A row-1 card at the paper top would put continuation row 0 above
        // the paper, leaving no valid heading before it. Heading paper-top
        // and first-row contacts are tested independently below instead.
        // Exact bottom contact is below the shared footer but within End's
        // full-height paper. At the failing position, rows 1-4 still fit;
        // only Exit on row 5, below Watch Replay, crosses that paper's boundary.
        require(bottom_position + end.layout.spacing.y * 4.0f + 20.0f +
            end.art.focus_padding.y + 0.25f < bottom.y,
            "End row-5 overflow fixture also overflows its earlier static action rows");
        paper_edge("y", bottom_position, "items.exit_to_main_menu", bottom_position + 0.25f);

        const float row_height = 20.0f + 2.0f * end.art.focus_padding.y;
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(row_height);
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.watch_replay", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(std::nextafter(row_height, 0.0f));
        }, &world);
        for (const double invalid : { 0.0, -1.0, 1e100 })
        {
            variants.rejects("items.change_teams", [&](rapidjson::Document& d)
            {
                exact_items(d);
                d["end_menu"]["layout"]["spacing"]["y"].SetDouble(invalid);
            }, &world);
        }

        // A longer Exit can overflow independently while the three other
        // static action labels retain their exact right-edge fit.
        variants.rejects("items.exit_to_main_menu", [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["end_menu"]["layout"]["position"]["x"].SetFloat(right_position);
            d["menu_text"]["items"]["exit_to_main_menu"].SetString("FitFit");
        }, &world);
    }

    void check_end_headings(Variants& variants, World& world)
    {
        for (const auto& entry : end_text)
        {
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString(
                    "  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().end.*entry.field == L"  A \"B\"-C...  ",
                std::string("End heading lost normalized punctuation or authored spaces: ") + entry.key);
            const std::string field = std::string("end.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak",
                "tab\ttext", "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["end"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                // Other menu fonts still support '~'; only the heading font
                // used by the actual End widget is missing the authored glyph.
                d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited");
                d["menu_text"]["end"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        const auto check_widgets = [&](const Presentation& look)
        {
            const auto& end = look.end_menu();
            const auto& style = look.menu();
            const auto font = world.render.resolve_sprite_font(style.heading_font);
            for (const auto& entry : end_text)
            {
                std::wstring source = look.menu_text().end.*entry.field;
                const auto measured = world.render.measure_text(font, source);
                const auto retained = source;
                UiText heading(entry.key, source, style.heading_font,
                    mattmath::Vector2F(0.0f, end.heading_y), &world.render, end.art.ink);
                source = L"Changed after construction";
                heading.set_position(mattmath::Vector2F((1920.0f - heading.bounds().width) / 2.0f, end.heading_y));
                const mattmath::RectangleF expected((1920.0f - measured.x) / 2.0f, end.heading_y, measured.x, measured.y);
                require(std::isfinite(measured.x) && std::isfinite(measured.y) && measured.x > 0.0f && measured.y > 0.0f &&
                    heading.text() == retained && heading.bounds() == expected,
                    std::string("Actual End heading borrowed text, scaled, wrapped or changed centering: ") + entry.key);
                heading.set_hidden(true);
                DrawList no_device;
                heading.draw(no_device);
            }
        };
        check_widgets(variants.load([](rapidjson::Document&) {}));
        for (const size_t length : { size_t(3), size_t(100) })
        {
            const auto authored = variants.load([&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
                for (const auto& entry : end_text)
                {
                    d["menu_text"]["end"][entry.key].SetString(std::string(length, 'F').c_str(), d.GetAllocator());
                }
            });
            authored.validate_resources(world.render, world.audio);
            check_widgets(authored);
        }
    }

    void check_end_heading_bounds(Variants& variants, World& world)
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
            d["menu_style"]["fonts"]["heading"].SetString(name.c_str(), d.GetAllocator());
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
            d["menu_text"]["end"][key].SetString("~");
        };
        const auto original = variants.load([](rapidjson::Document&) {});
        const auto& end = original.end_menu();
        require(end.box_size == mattmath::Vector2F(980.0f, 900.0f),
            "End heading fixtures no longer exercise the shipped full-height paper");
        add_font("menu_end_heading_width_edge", 980, 20);
        add_font("menu_end_heading_width_overflow", 981, 20);
        add_font("menu_end_heading_action_edge", 8, 20);
        add_font("menu_end_heading_tall_edge", 8, 40);
        for (const auto& entry : end_text)
        {
            const std::string field = std::string("end.") + entry.key;
            const auto width_edge = variants.load([&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_width_edge", entry.key);
            });
            width_edge.validate_resources(world.render, world.audio);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(width_edge.menu().heading_font), L"~");
            const mattmath::RectangleF paper((mattmath::Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f, end.box_size);
            const mattmath::RectangleF bounds((1920.0f - size.x) / 2.0f, end.heading_y, size.x, size.y);
            require(bounds.x == paper.x && bounds.right() == paper.right() && paper.contains(bounds),
                "End heading's unpadded centered text no longer touches both inclusive paper edges");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_width_overflow", entry.key);
            }, &world);
            const auto action_edge = [&](rapidjson::Document& d)
            {
                use_font(d, "menu_end_heading_action_edge", entry.key);
                d["end_menu"]["layout"]["position"]["y"].SetFloat(end.heading_y + 20.0f + end.art.focus_padding.y);
            };
            const auto touches_action = variants.load(action_edge);
            touches_action.validate_resources(world.render, world.audio);
            require(touches_action.end_menu().heading_y + 20.0f == touches_action.end_menu().layout.position.y -
                touches_action.end_menu().art.focus_padding.y,
                "End heading action fixture lost exact unscaled bottom-to-padding contact");
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                action_edge(d);
                d["end_menu"]["layout"]["position"]["y"].SetFloat(end.heading_y + 19.75f + end.art.focus_padding.y);
            }, &world);
        }
        const auto paper_top = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("Fit"); }
            d["end_menu"]["heading_y"].SetFloat(90.0f);
        };
        const auto touches_top = variants.load(paper_top);
        touches_top.validate_resources(world.render, world.audio);
        variants.rejects("end.match_complete", [&](rapidjson::Document& d)
        {
            paper_top(d);
            d["end_menu"]["heading_y"].SetFloat(89.75f);
        }, &world);

        // The heading's unscaled bottom at940 is below a footer-centered
        // paper's bottom930, but fits the actual full-height paper bottom990.
        std::vector<Glyph> small_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            small_glyphs.push_back({ character, mattmath::RectangleI(0, 0, 1, 2), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_end_heading_small_actions",
            std::make_unique<Font>(TextureHandle{}, std::move(small_glyphs), 2.0f));
        const auto full_height = variants.load([&](rapidjson::Document& d)
        {
            use_font(d, "menu_end_heading_tall_edge", "match_complete");
            for (const auto& entry : end_text) { d["menu_text"]["end"][entry.key].SetString("~"); }
            d["menu_style"]["fonts"]["item"].SetString("menu_end_heading_small_actions");
            d["end_menu"]["heading_y"].SetFloat(900.0f);
            d["end_menu"]["layout"]["position"]["y"].SetFloat(940.5f);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["end_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["end_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["end_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
        });
        full_height.validate_resources(world.render, world.audio);
        require(full_height.end_menu().heading_y + 40.0f >
            (full_height.menu().footer_y + full_height.end_menu().box_size.y) / 2.0f,
            "End heading full-height fixture no longer distinguishes its actual paper from a footer-centered paper");

        struct InvalidMeasure { const char* name; int width; int height; float x_offset; float y_offset; float line_spacing; };
        const float infinity = std::numeric_limits<float>::infinity();
        const InvalidMeasure invalid_measures[]{
            { "zero_width", 0, 20, 0.0f, 0.0f, 16.0f },
            { "zero_height", 8, 0, 0.0f, 0.0f, 0.0f },
            { "infinite_width", 8, 20, infinity, 0.0f, 16.0f },
            { "infinite_height", 8, 20, 0.0f, infinity, 16.0f }
        };
        for (const auto& invalid : invalid_measures)
        {
            const std::string name = std::string("menu_end_heading_") + invalid.name;
            add_font(name, invalid.width, invalid.height, invalid.x_offset, invalid.y_offset, invalid.line_spacing);
            const auto size = world.render.measure_text(world.render.resolve_sprite_font(name), L"~");
            require(!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f,
                "Invalid End heading fixture no longer measures a zero or nonfinite extent");
            for (const auto& entry : end_text)
            {
                variants.rejects(std::string("end.") + entry.key, [&](rapidjson::Document& d)
                {
                    use_font(d, name, entry.key);
                }, &world);
            }
        }
    }

    void check_end_continuation_text(Variants& variants, World& world)
    {
        for (const auto& entry : end_continuation_text)
        {
            const std::string field = std::string("end.") + entry.key;
            const auto normalized = variants.load([&](rapidjson::Document& d)
            {
                d["menu_text"]["end"][entry.key].SetString("  A\xC2\xA0\xE2\x80\x9C" "B\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            });
            normalized.validate_resources(world.render, world.audio);
            require(normalized.menu_text().end.*entry.field == L"  A \"B\"-C...  ",
                "End continuation lost normalized punctuation or authored outer spaces: " + field);
            const auto plain = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString("{next} % [again]"); });
            plain.validate_resources(world.render, world.audio);
            std::wstring source = plain.menu_text().end.*entry.field;
            const auto position = plain.end_menu().layout.position;
            const auto measured = world.render.measure_text(world.render.resolve_sprite_font(plain.menu().item_font), L"{next} % [again]");
            UiText text(entry.key, source, plain.menu().item_font, position, &world.render, plain.end_menu().art.ink);
            source = L"Changed after construction";
            require(text.text() == L"{next} % [again]" && text.bounds() == mattmath::RectangleF(position, measured),
                "Plain End continuation interpreted braces/percent, borrowed text, scaled or retained a text shadow: " + field);
            text.set_hidden(true); DrawList no_device; text.draw(no_device);
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\nbreak", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["end"][entry.key].SetString(invalid, d.GetAllocator()); });
            }
            for (int type = 0; type < 3; ++type)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    auto& target = d["menu_text"]["end"][entry.key];
                    if (type == 0) { target.SetArray(); }
                    else if (type == 1) { target.SetObject(); }
                    else { target.SetInt(7); }
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString("A\0B", 3, d.GetAllocator()); });
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["end"][entry.key].SetString("~");
            }, &world);
            const auto long_copy = variants.load([&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); });
            require((long_copy.menu_text().end.*entry.field).size() == 512,
                "Plain End continuation acquired a parser character cap: " + field);
            variants.rejects(field, [&](rapidjson::Document& d)
                { d["menu_text"]["end"][entry.key].SetString(std::string(512, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        require(world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'?') &&
            !world.render.font(world.render.resolve_sprite_font("menu_text_limited"))->contains(U'~'),
            "End continuation missing-literal fixture lost its stand-in or absent glyph");
    }

    void check_end_continuation_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const char* name, int width, int height, float x_offset = 0.0f,
            float y_offset = 0.0f, float line_spacing = 20.0f)
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                const bool marker = character == U'~';
                glyphs.push_back({ character, mattmath::RectangleI(0, 0, marker ? width : 8, marker ? height : 16),
                    marker ? x_offset : 0.0f, marker ? y_offset : 0.0f, 0.0f });
            }
            world.render.add_font(name, std::make_unique<Font>(TextureHandle{}, std::move(glyphs), line_spacing));
        };
        add_font("menu_continue_right", 722, 16);
        add_font("menu_continue_right_overflow", 722, 16, 0.25f);
        add_font("menu_continue_height", 8, 81);
        add_font("menu_continue_height_overflow", 8, 81, 0.0f, 0.25f);
        add_font("menu_continue_zero_width", 0, 20);
        add_font("menu_continue_zero_height", 8, 0, 0.0f, 0.0f, 0.0f);
        add_font("menu_continue_infinite_width", 8, 20, std::numeric_limits<float>::infinity());
        add_font("menu_continue_infinite_height", 8, 20, 0.0f, std::numeric_limits<float>::infinity());
        const auto use_font = [](rapidjson::Document& d, const char* key, const char* font_name)
        {
            d["menu_style"]["fonts"]["item"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["end"][key].SetString("~");
        };
        for (const auto& entry : end_continuation_text)
        {
            const std::string field = std::string("end.") + entry.key;
            for (const bool height_edge : { false, true })
            {
                const auto edge = variants.load([&](rapidjson::Document& d)
                    { use_font(d, entry.key, height_edge ? "menu_continue_height" : "menu_continue_right"); });
                edge.validate_resources(world.render, world.audio);
                const auto& end = edge.end_menu();
                const auto measured = world.render.measure_text(world.render.resolve_sprite_font(edge.menu().item_font), L"~");
                UiText value("continuation", L"~", edge.menu().item_font, end.layout.position, &world.render, end.art.ink);
                const mattmath::RectangleF paper(470.0f, 90.0f, 980.0f, 900.0f);
                require(value.bounds() == mattmath::RectangleF(700.0f, 300.0f,
                    height_edge ? 8.0f : 722.0f, height_edge ? 81.0f : 20.0f) &&
                    value.bounds().size() == measured, "Continuation edge fixture changed the actual plain unscaled UiText bounds");
                auto padded = value.bounds(); padded.inflate(end.art.focus_padding);
                require(paper.contains(padded) && (height_edge ? padded.height == 117.0f : padded.right() == 1450.0f),
                    "Continuation edge fixture lost exact inclusive text-plus-focus-padding contact");
                require(height_edge ? padded.height + end.art.shadow_offset > end.layout.spacing.y :
                    padded.right() + end.art.shadow_offset > paper.right(),
                    "Continuation fixture no longer distinguishes the existing plain rule from an added card-shade constraint");
                variants.rejects(field, [&](rapidjson::Document& d)
                    { use_font(d, entry.key, height_edge ? "menu_continue_height_overflow" : "menu_continue_right_overflow"); }, &world);
            }
            for (const char* font_name : { "menu_continue_zero_width", "menu_continue_zero_height",
                "menu_continue_infinite_width", "menu_continue_infinite_height" })
            {
                // Zero measured height needs zero font line spacing. The earlier
                // shared-font Join roster guard owns that globally invalid font.
                const std::string owner = std::string(font_name) == "menu_continue_zero_height" ?
                    "labels.roster_row with menu_text.labels.waiting at Join requires finite positive font line spacing" : field;
                variants.rejects(owner, [&](rapidjson::Document& d) { use_font(d, entry.key, font_name); }, &world);
            }
        }
        const auto full_height = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            d["menu_style"]["fonts"]["item"].SetString("menu_end_heading_small_actions");
            d["end_menu"]["heading_y"].SetFloat(900.0f);
            d["end_menu"]["layout"]["position"]["y"].SetFloat(940.5f);
            d["end_menu"]["layout"]["spacing"]["y"].SetFloat(3.0f);
            d["end_menu"]["art"]["focus_padding"]["y"].SetFloat(0.5f);
            d["end_menu"]["art"]["keyline_width"].SetFloat(0.125f);
            d["end_menu"]["art"]["shadow_offset"].SetFloat(0.125f);
        });
        full_height.validate_resources(world.render, world.audio);
        const auto& end = full_height.end_menu();
        require(end.layout.position.y + 2.0f + end.art.focus_padding.y == 943.0f &&
            943.0f > (full_height.menu().footer_y + end.box_size.y) / 2.0f && 943.0f < 990.0f,
            "Continuation full-height fixture no longer distinguishes End paper from the menu-footer paper");
        variants.rejects("items.change_teams", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["items"]["change_teams"].SetString("~"); d["menu_text"]["end"]["rematch"].SetString("~");
        }, &world);
        variants.rejects("end.match_complete", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_limited");
            d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
            d["menu_text"]["end"]["match_complete"].SetString("~"); d["menu_text"]["end"]["rematch"].SetString("~");
        }, &world);
    }
}
