#include "game/pch.h"
#include "scripts/acceptance/menu_text/fixtures.h"

// The title and home screens.
namespace menu_text_acceptance
{
    namespace
    {
    void select_title_font(rapidjson::Document& d, const char* key, const char* font)
    {
        if (std::string(key) == "wordmark")
        {
            d["main_menu"]["fonts"]["title"].SetString(font, d.GetAllocator());
        }
        else
        {
            d["menu_style"]["fonts"][std::string(key) == "subtitle" ? "detail" : "item"].SetString(font, d.GetAllocator());
        }
    }
    }

    void check_title_and_home_text(Variants& variants, World& world, const Presentation& defaults)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& entry : title_text)
            {
                d["menu_text"]["title"][entry.key].SetString(" A\xC2\xA0\xE2\x80\x94" "B ");
            }
            d["menu_text"]["title"]["wordmark"].SetString(" A\xC2\xA0\xE2\x80\x94" "B \n C ");
            d["menu_text"]["home_subtitle"].SetString(" A\xC2\xA0\xE2\x80\x94" "B ");
        });
        require(normalized.menu_text().title.wordmark == L" A -B \n C " &&
            normalized.menu_text().title.subtitle == L" A -B " && normalized.menu_text().title.start == L" A -B " &&
            normalized.menu_text().title.no_controller == L" A -B " && normalized.menu_text().home_subtitle == L" A -B ",
            "Title/Home text did not preserve authored spacing/LF and normalize punctuation/NBSP");
        normalized.validate_resources(world.render, world.audio);
        for (const auto& entry : title_text)
        {
            const std::string field = std::string("title.") + entry.key;
            for (const char* invalid : { "   ", " \xC2\xA0 ", "line\rbreak", "line\r\nbreak", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3" })
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["title"][entry.key].SetString(invalid, d.GetAllocator());
                });
            }
            if (std::string(entry.key) != "wordmark")
            {
                variants.rejects(field, [&](rapidjson::Document& d)
                {
                    d["menu_text"]["title"][entry.key].SetString("line\nbreak");
                });
            }
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                select_title_font(d, entry.key, "menu_text_limited");
                d["menu_text"]["title"][entry.key].SetString("Glyph~");
            }, &world);
            variants.rejects(field, [&](rapidjson::Document& d)
            {
                d["menu_text"]["title"][entry.key].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
            }, &world);
        }
        for (const char* invalid : { "\nA", "A\n", "A\n\nB", "A\n \nB", " \nA", "A\n \xC2\xA0 " })
        {
            variants.rejects("title.wordmark", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["title"]["wordmark"].SetString(invalid, d.GetAllocator());
            });
        }
        variants.rejects("title.wordmark", [](rapidjson::Document& d)
        {
            d["menu_text"]["title"]["wordmark"].SetString("COLOUR\nWARS\nAGAIN");
        }, &world);
        variants.rejects("title.wordmark", [](rapidjson::Document& d)
        {
            d["main_menu"]["fonts"]["title"].SetString("menu_text_tall");
        }, &world);
        for (const char* invalid : { "   ", "line\nbreak", "line\rbreak", "tab\ttext", "\xC3", "\xC3\xA9" })
        {
            variants.rejects("home_subtitle", [invalid](rapidjson::Document& d)
            {
                d["menu_text"]["home_subtitle"].SetString(invalid, d.GetAllocator());
            });
        }
        variants.rejects("home_subtitle", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("menu_text_limited");
            d["menu_text"]["home_subtitle"].SetString("Glyph~");
        }, &world);
        variants.rejects("home_subtitle", [](rapidjson::Document& d)
        {
            d["menu_text"]["home_subtitle"].SetString(std::string(200, 'W').c_str(), d.GetAllocator());
        }, &world);

        const auto font = world.render.resolve_sprite_font(defaults.main_menu().title_font);
        const auto measured = world.render.measure_text(font, defaults.menu_text().title.wordmark);
        const auto first_line = world.render.measure_text(font, L"COLOUR");
        const auto second_line = world.render.measure_text(font, L"WARS");
        require(measured.x == std::max(first_line.x, second_line.x) &&
            measured.y > std::max(first_line.y, second_line.y),
            "The authored wordmark no longer measures as two complete unscaled lines");
        const mattmath::Vector2F position((1920.0f - measured.x) / 2.0f, defaults.main_menu().title_y);
        UiText wordmark("wordmark", defaults.menu_text().title.wordmark, defaults.main_menu().title_font,
            position, &world.render, defaults.main_menu().art.ink);
        require(wordmark.text() == L"COLOUR\nWARS" &&
            wordmark.bounds() == mattmath::RectangleF(position, measured),
            "Actual UiText changed the wordmark's authored lines, centering or unscaled font bounds");
        wordmark.set_hidden(true);
        DrawList no_device;
        wordmark.draw(no_device);
    }

    void check_title_and_home_bounds(Variants& variants, World& world)
    {
        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("menu_text_exact");
            }
            d["main_menu"]["fonts"]["title"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
            for (const auto& entry : title_text) { d["menu_text"]["title"][entry.key].SetString("Fit"); }
            d["menu_text"]["title"]["wordmark"].SetString("Fit\nFit");
            d["menu_text"]["home_subtitle"].SetString("Fit");
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
        };
        const auto multiline = variants.load([&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_text"]["title"]["wordmark"].SetString("Fit\nFit\nFit");
        });
        multiline.validate_resources(world.render, world.audio);
        require(multiline.menu_text().title.wordmark == L"Fit\nFit\nFit",
            "Wordmark line breaks became a fixed two-line replacement instead of authored text");
        for (const auto& entry : title_text)
        {
            const size_t fits_count = std::string(entry.key) == "start" ? 158 : 165;
            const auto edge = [&](rapidjson::Document& d)
            {
                exact(d);
                d["menu_text"]["title"][entry.key].SetString(std::string(fits_count, 'F').c_str(), d.GetAllocator());
            };
            const auto fits = variants.load(edge);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(std::string("title.") + entry.key, [&](rapidjson::Document& d)
            {
                edge(d);
                d["menu_text"]["title"][entry.key].SetString(std::string(fits_count + 1, 'F').c_str(), d.GetAllocator());
            }, &world);
        }
        struct OrderEdge { const char* position; float value; float outside; const char* field; };
        constexpr OrderEdge title_edges[]{
            { "title_subtitle_y", 130.0f, 129.75f, "title.wordmark" },
            { "title_start_y", 628.0f, 627.75f, "title.subtitle" },
            { "title_no_pad_y", 718.0f, 717.75f, "title.start" },
            { "build_id_y", 820.0f, 819.75f, "title.no_controller" }
        };
        for (const auto& edge : title_edges)
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"][edge.position].SetFloat(edge.value);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects(edge.field, [&](rapidjson::Document& d)
            {
                edit(d);
                d["main_menu"][edge.position].SetFloat(edge.outside);
            }, &world);
        }
        for (const auto& edge : { std::pair<float, float>{ 200.0f, 199.75f }, { 382.0f, 382.25f } })
        {
            const auto edit = [&](rapidjson::Document& d)
            {
                exact(d);
                d["main_menu"]["home_subtitle_position"]["y"].SetFloat(edge.first);
            };
            const auto fits = variants.load(edit);
            fits.validate_resources(world.render, world.audio);
            variants.rejects("home_subtitle", [&](rapidjson::Document& d)
            {
                edit(d);
                d["main_menu"]["home_subtitle_position"]["y"].SetFloat(edge.second);
            }, &world);
        }
        const auto home_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["home_subtitle_position"]["x"].SetFloat(1596.0f);
        };
        const auto fits_home = variants.load(home_edge);
        fits_home.validate_resources(world.render, world.audio);
        variants.rejects("home_subtitle", [&](rapidjson::Document& d)
        {
            home_edge(d);
            d["main_menu"]["home_subtitle_position"]["x"].SetFloat(1596.25f);
        }, &world);
    }
}
