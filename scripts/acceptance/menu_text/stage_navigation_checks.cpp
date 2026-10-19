#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The arena picker's Random entry and stage count.
namespace menu_text_acceptance
{
    void check_stage_random_fitting(Variants& variants, World& world)
    {
        const auto check_widget = [&](const Presentation& look, bool width_limited)
        {
            const auto& stage = look.main_menu().stage_select;
            const auto& style = look.menu();
            const auto font = world.render.resolve_sprite_font(style.item_font);
            std::wstring source = look.menu_text().values.random;
            const auto measured = world.render.measure_text(font, source);
            const auto retained = source;
            UiTextDropShadow name("random_stage_name", source, style.item_font, stage.name_position,
                &world.render, stage.unselected, style.shadow_colour, style.item_shadow_offset);
            source = L"Changed after construction";
            const mattmath::RectangleF raw = mattmath::RectangleF::union_of(
                mattmath::RectangleF(mattmath::Vector2F::ZERO, measured),
                mattmath::RectangleF(style.item_shadow_offset, measured));
            require(name.text() == retained && name.bounds() == mattmath::RectangleF::union_of(
                name.text_bounds_at(stage.name_position, 1.0f),
                name.text_bounds_at(stage.name_position + style.item_shadow_offset, 1.0f)) &&
                raw.width == measured.x + std::abs(style.item_shadow_offset.x) &&
                raw.height == measured.y + std::abs(style.item_shadow_offset.y) &&
                width_limited == (raw.width > stage.preview_size.x),
                "Actual Random stage name lost its source copy, complete signed shadow union or expected width fitting");
            const auto fit = [&]()
            {
                // The runtime explicitly measures at scale 1 before every
                // update, even when the previous displayed slot was shrunk.
                const auto bounds = mattmath::RectangleF::union_of(
                    name.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                    name.text_bounds_at(style.item_shadow_offset, 1.0f));
                const float scale = std::min(1.0f, stage.preview_size.x / bounds.width);
                name.set_scale(scale);
                name.set_shadow_scale(scale);
                name.set_shadow_offset(style.item_shadow_offset * scale);
                return scale;
            };
            const float scale = fit();
            const mattmath::RectangleF expected(stage.name_position + raw.position() * scale, raw.size() * scale);
            const auto actual = name.bounds();
            require(name.text() == retained && name.text_bounds() == name.text_bounds_at(stage.name_position, scale) &&
                std::abs(actual.x - expected.x) <= 0.01f && std::abs(actual.y - expected.y) <= 0.01f &&
                std::abs(actual.width - expected.width) <= 0.01f && std::abs(actual.height - expected.height) <= 0.01f &&
                name.shadow_scale() == scale && name.shadow_offset() == style.item_shadow_offset * scale &&
                std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.width) &&
                std::isfinite(actual.height) && actual.width > 0.0f && actual.height > 0.0f &&
                actual.width <= stage.preview_size.x + 0.01f &&
                std::abs(actual.height - raw.height * scale) <= 0.01f && actual.x >= 0.0f && actual.y >= 0.0f &&
                actual.right() <= 1920.0f && actual.bottom() <= stage.preview_position.y - style.rule_height + 0.01f,
                "Random stage name changed position, lost shadow scaling, wrapped, or escaped proportional width-only fitting");
            require(width_limited ? std::abs(actual.width - stage.preview_size.x) <= 0.01f : scale == 1.0f,
                "Random stage name did not fill its width budget or enlarged a short authored name");
            std::wstring replacement = L"Fit";
            name.set_text(replacement);
            replacement = L"Changed after set_text";
            const float reset_scale = fit();
            const auto reset = world.render.measure_text(font, L"Fit");
            require(reset_scale == 1.0f && name.text() == L"Fit" && name.shadow_scale() == 1.0f &&
                name.shadow_offset() == style.item_shadow_offset && name.bounds() == mattmath::RectangleF::union_of(
                    mattmath::RectangleF(stage.name_position, reset),
                    mattmath::RectangleF(stage.name_position + style.item_shadow_offset, reset)),
                "Replacing a fitted Random stage name retained its shrink scale, lost its copied text or failed to reset its shadow");
            name.set_hidden(true);
            DrawList no_device;
            name.draw(no_device);
        };
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio);
        check_widget(defaults, false);
        for (const bool wide : { false, true })
        {
            for (const bool negative_shadow : { false, true })
            {
                const auto authored = variants.load([&](rapidjson::Document& d)
                {
                    d["menu_text"]["values"]["random"].SetString(wide ? std::string(200, 'W').c_str() : "Fit", d.GetAllocator());
                    if (negative_shadow)
                    {
                        d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
                        d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(-1.5f);
                    }
                });
                authored.validate_resources(world.render, world.audio);
                check_widget(authored, wide);
            }
        }
    }

    void check_stage_random_bounds(Variants& variants, World& world)
    {
        const auto original = variants.load([](rapidjson::Document&) {});
        const float preview_top = original.main_menu().stage_select.preview_position.y - original.menu().rule_height;
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            d["menu_text"]["values"]["random"].SetString("Fit");
        };
        struct Edge { const char* axis; float position; float outside; };
        const Edge edges[]{ { "x", 0.0f, -0.25f }, { "x", 1894.5f, 1894.75f },
            { "y", 0.0f, -0.25f }, { "y", preview_top - 21.5f, preview_top - 21.25f } };
        for (const auto& edge : edges)
        {
            const auto contact = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["stage_select"]["name_position"][edge.axis].SetFloat(edge.position);
            };
            const auto touches = variants.load(contact);
            touches.validate_resources(world.render, world.audio);
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                contact(d);
                d["main_menu"]["stage_select"]["name_position"][edge.axis].SetFloat(edge.outside);
            }, &world);
        }
        const auto signed_contact = [&](rapidjson::Document& d)
        {
            exact(d);
            for (const char* axis : { "x", "y" })
            {
                d["menu_style"]["shadow"]["item_offset"][axis].SetFloat(-1.5f);
                d["main_menu"]["stage_select"]["name_position"][axis].SetFloat(1.5f);
            }
        };
        const auto signed_shadow = variants.load(signed_contact);
        signed_shadow.validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                signed_contact(d);
                d["main_menu"]["stage_select"]["name_position"][axis].SetFloat(1.25f);
            }, &world);
        }
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, character == U'~' ? 2018 : 8,
                character == U'~' ? 160 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_stage_random_half_scale",
            std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto scaled_contact = [&](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_stage_random_half_scale");
            d["menu_text"]["values"]["random"].SetString("~");
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(2.0f);
            d["menu_style"]["shadow"]["item_offset"]["y"].SetFloat(2.0f);
            d["main_menu"]["stage_select"]["name_position"]["y"].SetFloat(preview_top - 81.0f);
        };
        const auto scaled = variants.load(scaled_contact);
        scaled.validate_resources(world.render, world.audio);
        const auto measured = world.render.measure_text(world.render.resolve_sprite_font(scaled.menu().item_font), L"~");
        require(measured == mattmath::Vector2F(2018.0f, 160.0f) &&
            scaled.main_menu().stage_select.preview_size.x / (measured.x + 2.0f) == 0.5f,
            "Random stage boundary fixture no longer fits the complete 2020x162 shadow union at exactly half scale");
        variants.rejects("values.random", [&](rapidjson::Document& d)
        {
            scaled_contact(d);
            d["main_menu"]["stage_select"]["name_position"]["y"].SetFloat(preview_top - 80.75f);
        }, &world);
        // OptionRow can fit this tall glyph to its height. The arena name
        // cannot: its existing behavior permits only proportional width fit.
        variants.rejects("values.random", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_value_height_only");
            d["menu_text"]["values"]["random"].SetString("~");
        }, &world);
        for (const char* axis : { "x", "y" })
        {
            for (const double position : { -1e100, 1e100 })
            {
                variants.rejects("values.random", [&](rapidjson::Document& d)
                {
                    d["main_menu"]["stage_select"]["name_position"][axis].SetDouble(position);
                }, &world);
            }
            variants.rejects("", [&](rapidjson::Document& d)
            {
                // This shared setting is also validated by earlier menu
                // consumers; any contextual menu-text rejection is correct.
                d["menu_style"]["shadow"]["item_offset"][axis].SetDouble(1e100);
            }, &world);
        }
        variants.rejects("values.random", [](rapidjson::Document& d)
        {
            // Positive when read as float, but too small to leave a positive
            // fitted scale for the authored name and its complete shadow.
            d["main_menu"]["stage_select"]["preview_size"]["x"].SetDouble(1e-45);
        }, &world);
        for (const double preview_y : { 0.0, -1e100, 1e100 })
        {
            variants.rejects("values.random", [&](rapidjson::Document& d)
            {
                d["main_menu"]["stage_select"]["preview_position"]["y"].SetDouble(preview_y);
            }, &world);
        }
        const auto reject_preview_width = [&](double width)
        {
            const auto path = variants.write([&](rapidjson::Document& d)
            {
                d["main_menu"]["stage_select"]["preview_size"]["x"].SetDouble(width);
            });
            std::string diagnostic = "accepted without exception";
            try
            {
                const Presentation invalid(path.string().c_str());
                invalid.validate_resources(world.render, world.audio);
            }
            catch (const std::runtime_error& error) { diagnostic = error.what(); }
            require(diagnostic.find(path.filename().string()) != std::string::npos &&
                diagnostic.find("stage_select") != std::string::npos && diagnostic.find("preview_size") != std::string::npos,
                "Invalid Random stage preview width lost its source and geometry context: " + diagnostic);
        };
        for (const double width : { 0.0, -1.0, 1e100 }) { reject_preview_width(width); }
    }

    void check_stage_count_text(Variants& variants, World& world, const MenuText& defaults)
    {
        struct Navigation { size_t index; size_t total; const wchar_t* expected; };
        constexpr Navigation navigation[]{
            {1, 1, L"<   ARENA 1 / 1   >"}, {1, 10, L"<   ARENA 1 / 10   >"},
            {8, 10, L"<   ARENA 8 / 10   >"}, {10, 10, L"<   ARENA 10 / 10   >"},
            {9, 10, L"<   ARENA 9 / 10   >"}, {99, 100, L"<   ARENA 99 / 100   >"},
            {100, 100, L"<   ARENA 100 / 100   >"}
        };
        for (const auto& entry : navigation)
        {
            require(defaults.stage_navigation(entry.index, entry.total) == entry.expected,
                "Arena counter changed its one-based index, actual total or literal default wording");
        }
        for (size_t count : {size_t{1}, size_t{10}, size_t{100}})
        {
            require(defaults.stage_random_hint(count) == L"Random: " + std::to_wstring(count) +
                L" available arenas.\nConfirm to lock your choice.",
                "Random arena count changed its literal wording, including the original singular copy");
        }
        const size_t maximum = std::numeric_limits<size_t>::max();
        const std::wstring maximum_text = std::to_wstring(maximum);
        require(defaults.stage_navigation(maximum, maximum) == L"<   ARENA " + maximum_text + L" / " + maximum_text + L"   >" &&
            defaults.stage_random_hint(maximum) == L"Random: " + maximum_text +
            L" available arenas.\nConfirm to lock your choice.",
            "Arena count formatting narrowed size_t to a roster-sized or signed domain");
        const auto reject_format = [](auto format)
        {
            bool rejected = false;
            try { format(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "Invalid arena count argument or direct template formatted silently");
        };
        for (const auto& entry : {Navigation{0, 0, L""}, Navigation{1, 0, L""}, Navigation{0, 1, L""},
            Navigation{2, 1, L""}, Navigation{maximum, maximum - 1, L""}})
        {
            reject_format([&] { defaults.stage_navigation(entry.index, entry.total); });
        }
        reject_format([&] { defaults.stage_random_hint(0); });
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["stage"]["numbered_navigation"].SetString("  {total}\xC2\xA0\xE2\x80\x94" "{index}\xE2\x80\xA6  ");
            d["menu_text"]["stage"]["random_hint"].SetString("  {count}\xC2\xA0\xE2\x80\x9C" "arenas\xE2\x80\x9D\nReady\xE2\x80\xA6  ");
        });
        normalized.validate_resources(world.render, world.audio);
        require(normalized.menu_text().stage.numbered_navigation == L"  {total} -{index}...  " &&
            normalized.menu_text().stage.random_hint == L"  {count} \"arenas\"\nReady...  " &&
            normalized.menu_text().stage_navigation(8, 10) == L"  10 -8...  " &&
            normalized.menu_text().stage_random_hint(10) == L"  10 \"arenas\"\nReady...  ",
            "Arena count text lost normalized punctuation, reversed token order, LF or authored outer spaces");
        const auto adjacent = variants.load([](rapidjson::Document& d)
        {
            d["menu_text"]["stage"]["numbered_navigation"].SetString("{total}{index}");
            d["menu_text"]["stage"]["random_hint"].SetString("Choose {count} now");
        });
        adjacent.validate_resources(world.render, world.audio);
        require(adjacent.menu_text().stage_navigation(1, 10) == L"101" &&
            adjacent.menu_text().stage_random_hint(1) == L"Choose 1 now",
            "Arena count templates imposed token separators or a fixed Random hint line count");
        for (const auto& entry : stage_count_text)
        {
            const std::string field = std::string("stage.") + entry.key;
            const std::string valid = entry.multiline ? "{count}" : "{index}/{total}";
            for (const char* blank : {"   ", " \xC2\xA0 "})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stage"][entry.key].SetString(blank, d.GetAllocator()); });
            }
            for (const char* extra : {"\r", "\r\n", "\t", "\x01", "\x7f", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28"})
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    const std::string invalid = valid + extra;
                    d["menu_text"]["stage"][entry.key].SetString(invalid.c_str(), d.GetAllocator());
                });
            }
            const std::vector<std::string> malformed = entry.multiline ? std::vector<std::string>{
                "No token", "{count}{count}", "{Count}", "{count }", "{{count}}", "{count", "count}",
                "{count}{index}", "{co{count}unt}", "{count}{unknown}", "\n{count}", "{count}\n", "{count}\n \nF"} :
                std::vector<std::string>{"{index}", "{total}", "{index}{index}{total}", "{index}{total}{total}",
                    "{Index}/{total}", "{index }/{total}", "{{index}}/{total}", "{index}/{total", "{index}/{total}}",
                    "{index}{total}{count}", "{to{index}tal}", "{in{total}dex}", "{index}{total}{unknown}", "{index}\n{total}"};
            for (const auto& invalid : malformed)
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                    { d["menu_text"]["stage"][entry.key].SetString(invalid.c_str(), d.GetAllocator()); });
                MenuText mutated = defaults;
                mutated.stage.*entry.field = std::wstring(invalid.begin(), invalid.end());
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            for (const wchar_t* extra : {L"\r", L"\t", L"\x01", L"\x7f", L"\u00e9", L"\u00a0", L"\u2014", L"\n"})
            {
                MenuText mutated = defaults;
                (mutated.stage.*entry.field) += extra;
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            for (const wchar_t* blank : {L"", L"   "})
            {
                MenuText mutated = defaults;
                mutated.stage.*entry.field = blank;
                reject_format([&]
                {
                    if (entry.multiline) { mutated.stage_random_hint(10); }
                    else { mutated.stage_navigation(8, 10); }
                });
            }
            MenuText nul = defaults;
            (nul.stage.*entry.field).push_back(L'\0');
            reject_format([&]
            {
                if (entry.multiline) { nul.stage_random_hint(10); }
                else { nul.stage_navigation(8, 10); }
            });
        }
    }

    void check_stage_count_bounds(Variants& variants, World& world)
    {
        const auto add_font = [&](const std::string& name, int eight_width = 8, int tilde_width = 8,
            int tilde_height = 16, float tilde_y = 0.0f, char32_t missing = U'\0')
        {
            std::vector<Glyph> glyphs;
            for (char32_t character = U' '; character <= U'~'; ++character)
            {
                if (character == missing) { continue; }
                glyphs.push_back({character, mattmath::RectangleI(0, 0,
                    character == U'8' ? eight_width : character == U'~' ? tilde_width : 8,
                    character == U'~' ? tilde_height : 16), 0.0f, character == U'~' ? tilde_y : 0.0f, 0.0f});
            }
            auto font = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f);
            font->set_stand_in(U'?');
            world.render.add_font(name, std::move(font));
        };
        const auto navigation = [](rapidjson::Document& d, const char* font_name = "menu_stage_count_proportional")
        {
            d["menu_style"]["fonts"]["detail"].SetString(font_name, d.GetAllocator());
            d["menu_text"]["stage"]["numbered_navigation"].SetString("{index}/{total}");
        };
        const auto rejects_counts = [&](const std::string& key, auto edit, size_t total, size_t eligible)
        {
            const auto path = variants.write(edit);
            std::string diagnostic = "accepted without exception";
            try { Presentation(path.string().c_str()).validate_stage_counts(world.render, total, eligible); }
            catch (const std::runtime_error& error) { diagnostic = error.what(); }
            require(diagnostic.find(path.filename().string()) != std::string::npos &&
                diagnostic.find("menu_text.stage") != std::string::npos && diagnostic.find(key) != std::string::npos,
                "Invalid concrete arena count lost its source/role context: " + diagnostic);
        };
        add_font("menu_stage_count_proportional", 80);
        const auto counter_font = world.render.resolve_sprite_font("menu_stage_count_proportional");
        require(world.render.measure_text(counter_font, L"8/10").x == 104.0f &&
            world.render.measure_text(counter_font, L"10/10").x == 40.0f,
            "Proportional arena fixture no longer makes an earlier index wider than the final one");
        size_t widest = 0;
        float width = 0.0f;
        for (size_t index = 1; index <= 10; ++index)
        {
            const float measured = world.render.measure_text(counter_font, std::to_wstring(index) + L"/10").x;
            if (measured > width) { width = measured; widest = index; }
        }
        require(widest == 8 && width == 104.0f, "Full literal index oracle did not identify the independent worst row");
        const auto edge = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1030.75f);
        };
        const auto fits = variants.load(edge);
        fits.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow counter("count_record_room", L"8/10", "menu_stage_count_proportional",
            fits.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), fits.menu().detail_shadow_offset);
        require(counter.bounds().right() == 1135.75f &&
            1160.0f - counter.bounds().right() - fits.menu().focus_padding.x == 0.25f &&
            counter.shadow_scale() == 1.0f,
            "Unscaled counter no longer leaves exactly a quarter pixel for the adjacent record");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            edge(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(1031.0f);
        }, 10, 0);
        const auto many = variants.load([&](rapidjson::Document& d) { navigation(d); });
        for (size_t total : {size_t{1}, size_t{9}, size_t{10}, size_t{99}, size_t{100}})
        {
            many.validate_stage_counts(world.render, total, 0);
        }
        rejects_counts("", [](rapidjson::Document&) {}, 0, 0);
        rejects_counts("", [](rapidjson::Document&) {}, 10, 11);
        const auto negative = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(53.5f);
        };
        const auto left = variants.load(negative);
        left.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow shadowed("count_negative_shadow", L"8/10", "menu_stage_count_proportional",
            left.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), left.menu().detail_shadow_offset);
        require(shadowed.bounds().x == 52.0f && shadowed.bounds().width == 105.5f,
            "Concrete counter lost its negative shadow at the outer paper's inclusive left edge");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            negative(d);
            d["main_menu"]["stage_select"]["navigation_position"]["x"].SetFloat(53.25f);
        }, 10, 0);
        const auto bottom = [&](rapidjson::Document& d)
        {
            navigation(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(905.0f);
        };
        const auto bottom_contact = variants.load(bottom);
        bottom_contact.validate_stage_counts(world.render, 10, 0);
        UiTextDropShadow bottom_label("count_bottom", L"8/10", "menu_stage_count_proportional",
            bottom_contact.main_menu().stage_select.navigation_position, &world.render, Colour::black,
            bottom_contact.menu().shadow_colour, bottom_contact.menu().detail_shadow_offset);
        require(bottom_label.bounds().bottom() == 926.0f,
            "Concrete counter no longer touches the outer Stage paper before the footer");
        rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
        {
            bottom(d);
            d["main_menu"]["stage_select"]["navigation_position"]["y"].SetFloat(905.25f);
        }, 10, 0);
        for (char32_t digit = U'0'; digit <= U'9'; ++digit)
        {
            const std::string name = "menu_stage_count_missing_" + std::to_string(static_cast<unsigned int>(digit - U'0'));
            add_font(name, 8, 8, 16, 0.0f, digit);
            rejects_counts("numbered_navigation", [&](rapidjson::Document& d)
            {
                navigation(d, name.c_str());
            }, 100, 0);
            rejects_counts("random_hint", [&](rapidjson::Document& d)
            {
                navigation(d, "menu_text_exact");
                d["menu_style"]["chrome"]["brand_font"].SetString(name.c_str(), d.GetAllocator());
                d["menu_text"]["stage"]["random_hint"].SetString("{count}");
            }, 100, digit == U'0' ? size_t{10} : static_cast<size_t>(digit - U'0'));
        }
        // The shared detail font reaches the existing 0-15 countdown guard
        // first. Preserve that diagnostic; the brand font reaches the new
        // Random all-digit guard even though its standalone sample is only 1.
        for (bool hint_font : {false, true})
        {
            variants.rejects(hint_font ? "stage.random_hint" : "display_trial_countdown", [&](rapidjson::Document& d)
            {
                if (hint_font) { d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_count_missing_6"); }
                else { d["menu_style"]["fonts"]["detail"].SetString("menu_stage_count_missing_6"); }
            }, &world);
        }
        add_font("menu_stage_count_hint_edge", 8, 943, 223);
        add_font("menu_stage_count_hint_over", 8, 943, 223, 0.5f);
        const auto hint_source = [&](rapidjson::Document& d, const char* name = "menu_stage_count_hint_edge")
        {
            navigation(d, "menu_text_exact");
            d["menu_style"]["chrome"]["brand_font"].SetString(name, d.GetAllocator());
            d["menu_text"]["stage"]["random_hint"].SetString("~{count}\nF");
        };
        const auto hint_edge = variants.load([&](rapidjson::Document& d) { hint_source(d); });
        hint_edge.validate_stage_counts(world.render, 10, 1);
        const auto& stage = hint_edge.main_menu().stage_select;
        const auto point = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f;
        UiTextDropShadow hint("count_hint", hint_edge.menu_text().stage_random_hint(1), "menu_stage_count_hint_edge",
            point, &world.render, Colour::black, Colour(0.0f, 0.0f, 0.0f, 0.0f), hint_edge.menu().detail_shadow_offset);
        const auto fit_hint = [&](const mattmath::Vector2F& shadow)
        {
            const auto raw = mattmath::RectangleF::union_of(hint.text_bounds_at(mattmath::Vector2F::ZERO, 1.0f),
                hint.text_bounds_at(shadow, 1.0f));
            const float scale = std::min(1.0f, 476.0f / raw.width);
            hint.set_scale(scale);
            hint.set_shadow_scale(scale);
            hint.set_shadow_offset(shadow * scale);
            return scale;
        };
        require(fit_hint(hint_edge.menu().detail_shadow_offset) == 0.5f && hint.text() == L"~1\nF" &&
            hint.bounds().width == 476.0f && hint.bounds().bottom() == 910.0f,
            "Random count hint no longer uses only width to fit its multiline text and scaled shadow");
        rejects_counts("random_hint", [&](rapidjson::Document& d) { hint_source(d, "menu_stage_count_hint_over"); }, 10, 1);
        const auto no_random = variants.load([&](rapidjson::Document& d) { hint_source(d, "menu_stage_count_hint_over"); });
        no_random.validate_stage_counts(world.render, 10, 0);
        const auto reversed = variants.load([&](rapidjson::Document& d)
        {
            hint_source(d);
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.5f);
            d["menu_style"]["shadow"]["detail_offset"]["y"].SetFloat(-1.5f);
        });
        reversed.validate_stage_counts(world.render, 10, 1);
        const float reversed_scale = fit_hint(reversed.menu().detail_shadow_offset);
        require(reversed_scale > 0.0f && reversed_scale < 1.0f && hint.bounds().width <= 476.01f &&
            hint.shadow_offset() == reversed.menu().detail_shadow_offset * reversed_scale,
            "Random count width fitting discarded a negative shadow");
        MenuText short_copy = hint_edge.menu_text();
        short_copy.stage.random_hint = L"{count}";
        hint.set_text(short_copy.stage_random_hint(1));
        require(fit_hint(hint_edge.menu().detail_shadow_offset) == 1.0f && hint.text() == L"1" &&
            hint.shadow_scale() == 1.0f && hint.shadow_offset() == hint_edge.menu().detail_shadow_offset &&
            hint.bounds().width == 9.0f,
            "Short Random count replacement retained a stale shrink or scaled shadow");
        add_font("menu_stage_count_card_edge", 8, 943, 227);
        add_font("menu_stage_count_card_over", 8, 943, 227, 0.5f);
        const auto card_contact = [&](rapidjson::Document& d)
        {
            hint_source(d, "menu_stage_count_card_edge");
            d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(667.0f);
        };
        const auto card_edge = variants.load(card_contact);
        card_edge.validate_stage_counts(world.render, 10, 1);
        UiTextDropShadow card_label("count_hint_card", card_edge.menu_text().stage_random_hint(1),
            "menu_stage_count_card_edge", point, &world.render, Colour::black,
            Colour(0.0f, 0.0f, 0.0f, 0.0f), card_edge.menu().detail_shadow_offset);
        card_label.set_scale(0.5f);
        card_label.set_shadow_scale(0.5f);
        card_label.set_shadow_offset(card_edge.menu().detail_shadow_offset * 0.5f);
        require(card_label.text() == L"~1\nF" && card_label.bounds().width == 476.0f &&
            card_label.bounds().bottom() == 912.0f && card_label.bounds().bottom() + 14.0f == 926.0f,
            "Random count card no longer reaches the outer paper with its retained artwork inset");
        rejects_counts("random_hint", [&](rapidjson::Document& d)
        {
            card_contact(d);
            d["menu_style"]["chrome"]["brand_font"].SetString("menu_stage_count_card_over");
            d["main_menu"]["stage_select"]["briefing"]["size"]["y"].SetFloat(667.25f);
        }, 10, 1);
    }
}
