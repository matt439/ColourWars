#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/option_row.h"
#include "game/states/prompt_bar.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct DefaultText { const char* key; const wchar_t* text; };
    constexpr DefaultText headings[]{
        { "home", L"Make your mark." }, { "options", L"Options" }, { "credits", L"Credits" },
        { "controls_audio", L"Controls / audio" }, { "pad_sticks", L"Pad sticks" },
        { "players", L"Number of Players" }, { "join_players", L"Join Players" },
        { "mode", L"Mode Select" }, { "teams", L"Team Select" }, { "weapons", L"Weapon Select" },
        { "arena", L"Choose your arena" }
    };
    constexpr DefaultText prompts[]{
        { "title", L"A/Enter Start" }, { "select_back", L"A/Enter Select    B/Esc Back" },
        { "options", L"Left/Right Change    A/Enter Select    B/Esc Back" },
        { "controls", L"Left/Right Change    A/Enter Select    B/Esc Back (discard unapplied changes)" },
        { "sticks", L"D-pad/arrows Navigate / change    A/Enter Select    B/Esc Back (discard unapplied changes)" },
        { "mode", L"A/Enter Select    Left/Right Match length    B/Esc Back" },
        { "join", L"A/Enter Join / P1 continue    B/Esc Leave / P1 back" },
        { "team", L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel" },
        { "team_shuffle", L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams" },
        { "weapon", L"Left/Right Change weapon    A/Enter Confirm    B/Esc Cancel" },
        { "arena_browse", L"Left/Right Browse arenas    A/Enter Lock arena    B/Esc Back" },
        { "arena_locked", L"A/Enter Start match    B/Esc Unlock arena" },
        { "missing_player", L" Reconnect controller    B/Esc Change players" },
        { "pause", L"A/Enter Select    B/Start/Esc Resume" },
        { "pause_missing", L" Reconnect pads to resume    A/Enter Select" },
        { "select", L"A/Enter Select" }, { "credits", L"A/Enter Back    B/Esc Back" }
    };
    constexpr wchar_t brand[] = L"C O L O U R W A R S   /   LOCAL PAINT BATTLES";
    constexpr DefaultText items[]{
        { "play", L"Play" }, { "options", L"Options" }, { "credits", L"Credits" }, { "exit", L"Exit" },
        { "resolution", L"Resolution" }, { "fullscreen", L"Fullscreen" }, { "team_palette", L"Team palette" },
        { "controls_audio", L"Controls / audio" }, { "apply", L"Apply" }, { "back", L"Back" }
    };

    rapidjson::Document read_document(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        require(input.good(), "Cannot read " + path.string());
        const std::string text{ std::istreambuf_iterator<char>(input), {} };
        rapidjson::Document result;
        result.Parse(text.c_str());
        require(!result.HasParseError(), "Cannot parse " + path.string());
        return result;
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;

        explicit World(const std::filesystem::path& content)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(
                    (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* name : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
                "gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
                "gill_sans_mt_bold_144", "courier_new_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(name) + ".spritefont")).string());
                auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                render.add_font(name, std::move(font));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    struct Variants
    {
        rapidjson::Document source;
        std::filesystem::path scratch;
        size_t next = 0;

        Variants(const std::filesystem::path& content, const std::filesystem::path& folder) :
            source(read_document(content / "presentation.json")), scratch(folder)
        {
            std::filesystem::create_directories(scratch);
        }

        template<class Edit>
        std::filesystem::path write(Edit edit)
        {
            rapidjson::Document document;
            document.CopyFrom(source, document.GetAllocator());
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode menu text fixture");
            const auto path = scratch / ("menu-text-" + std::to_string(next++) + ".json");
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close();
            require(output.good(), "Cannot write menu text fixture");
            return path;
        }

        template<class Edit>
        Presentation load(Edit edit)
        {
            const auto path = write(edit);
            return Presentation(path.string().c_str());
        }

        template<class Edit>
        void rejects(const std::string& field, Edit edit, World* resources = nullptr)
        {
            const auto path = write(edit);
            bool contextual = false;
            std::string diagnostic = "accepted without exception";
            try
            {
                const Presentation invalid(path.string().c_str());
                if (resources != nullptr) { invalid.validate_resources(resources->render, resources->audio); }
            }
            catch (const std::runtime_error& error)
            {
                diagnostic = error.what();
                contextual = diagnostic.find(path.filename().string()) != std::string::npos &&
                    diagnostic.find("menu_text") != std::string::npos;
                size_t begin = 0;
                while (begin < field.size())
                {
                    const size_t end = field.find('.', begin);
                    contextual = contextual && diagnostic.find(field.substr(begin, end - begin)) != std::string::npos;
                    if (end == std::string::npos) { break; }
                    begin = end + 1;
                }
            }
            require(contextual, "Invalid menu text " + field + " lost source/key context: " + diagnostic);
        }
    };

    rapidjson::Value& text_field(rapidjson::Document& d, const char* group, const char* key)
    {
        return group == nullptr ? d["menu_text"][key] : d["menu_text"][group][key];
    }

    void check_defaults(const Presentation& presentation)
    {
        const MenuText& text = presentation.menu_text();
        require(text.brand == brand && text.headings.size() == std::size(headings) &&
            text.prompts.size() == std::size(prompts) && text.items.size() == std::size(items),
            "Shared menu text changed its brand or fixed role count");
        for (size_t index = 0; index < std::size(headings); ++index)
        {
            const auto& actual = text.heading(static_cast<MenuHeading>(index));
            require(actual == headings[index].text && &actual == &text.headings[index],
                std::string("Heading default changed: ") + headings[index].key);
        }
        for (size_t index = 0; index < std::size(prompts); ++index)
        {
            const auto& actual = text.prompt(static_cast<MenuPrompt>(index));
            require(actual == prompts[index].text && &actual == &text.prompts[index],
                std::string("Complete keyboard-expanded prompt default changed: ") + prompts[index].key);
        }
        for (const int invalid : { -1, static_cast<int>(MenuHeading::count), 999 })
        {
            bool rejected = false;
            try { text.heading(static_cast<MenuHeading>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared heading role resolved silently");
        }
        for (const int invalid : { -1, static_cast<int>(MenuPrompt::count), 999 })
        {
            bool rejected = false;
            try { text.prompt(static_cast<MenuPrompt>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared prompt role resolved silently");
        }
        for (size_t index = 0; index < std::size(items); ++index)
        {
            const auto& actual = text.item(static_cast<MenuItem>(index));
            require(actual == items[index].text && &actual == &text.items[index],
                std::string("Shared menu item default changed: ") + items[index].key);
        }
        for (const int invalid : { -1, static_cast<int>(MenuItem::count), 999 })
        {
            bool rejected = false;
            try { text.item(static_cast<MenuItem>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported shared item role resolved silently");
        }
    }

    void check_keyed_content(Variants& variants)
    {
        const auto reversed = variants.load([](rapidjson::Document& d)
        {
            for (const char* group : { "headings", "prompts", "items" })
            {
                auto& object = d["menu_text"][group];
                rapidjson::Value reordered(rapidjson::kObjectType);
                auto member = object.MemberEnd();
                while (member != object.MemberBegin())
                {
                    --member;
                    rapidjson::Value name;
                    name.CopyFrom(member->name, d.GetAllocator());
                    rapidjson::Value value;
                    value.CopyFrom(member->value, d.GetAllocator());
                    reordered.AddMember(name, value, d.GetAllocator());
                }
                object.Swap(reordered);
            }
        });
        check_defaults(reversed);
        variants.rejects("", [](rapidjson::Document& d) { d.RemoveMember("menu_text"); });
        for (const char* group : { "headings", "prompts", "items" })
        {
            variants.rejects(group, [group](rapidjson::Document& d) { d["menu_text"].RemoveMember(group); });
            variants.rejects(group, [group](rapidjson::Document& d) { d["menu_text"][group].SetArray(); });
        }
        const auto required = [&](const char* group, const char* key)
        {
            const std::string field = group == nullptr ? key : std::string(group) + '.' + key;
            variants.rejects(field, [group, key](rapidjson::Document& d)
                { (group == nullptr ? d["menu_text"] : d["menu_text"][group]).RemoveMember(key); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetBool(true); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetNull(); });
            variants.rejects(field, [group, key](rapidjson::Document& d) { text_field(d, group, key).SetString(""); });
            variants.rejects(field, [group, key](rapidjson::Document& d)
                { text_field(d, group, key).SetString("bad\0text", 8, d.GetAllocator()); });
        };
        required(nullptr, "brand");
        for (const auto& entry : headings) { required("headings", entry.key); }
        for (const auto& entry : prompts) { required("prompts", entry.key); }
        for (const auto& entry : items) { required("items", entry.key); }
    }

    struct TextRole { const char* group; const char* key; };
    constexpr TextRole categories[]{ {nullptr, "brand"}, {"headings", "home"}, {"prompts", "title"}, {"items", "play"} };

    void select_font(rapidjson::Document& d, const TextRole& role, const char* font)
    {
        if (role.group == nullptr) { d["menu_style"]["chrome"]["brand_font"].SetString(font, d.GetAllocator()); }
        else if (std::string(role.group) == "headings")
        {
            d["menu_style"]["fonts"]["heading"].SetString(font, d.GetAllocator());
        }
        else if (std::string(role.group) == "items")
        {
            d["menu_style"]["fonts"]["item"].SetString(font, d.GetAllocator());
        }
        else { d["prompt_bar"]["font"].SetString(font, d.GetAllocator()); }
    }

    void check_text_and_fonts(Variants& variants, World& world, const std::filesystem::path& content)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            for (const auto& role : categories)
            {
                text_field(d, role.group, role.key).SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
            }
        });
        const std::wstring expected = L"  A \"B\"-C...  ";
        require(normalized.menu_text().brand == expected && normalized.menu_text().heading(MenuHeading::home) == expected &&
            normalized.menu_text().prompt(MenuPrompt::title) == expected && normalized.menu_text().item(MenuItem::play) == expected,
            "Shared menu text did not normalize punctuation/NBSP while preserving authored outer spaces");
        normalized.validate_resources(world.render, world.audio);
        const SpriteFontFile font = read_sprite_font_file((content / "fonts/gill_sans_mt_bold_24.spritefont").string());
        auto glyphs = font.glyphs;
        const auto removed = std::remove_if(glyphs.begin(), glyphs.end(),
            [](const Glyph& glyph) { return glyph.character == U'~'; });
        require(removed != glyphs.end(), "Missing-glyph fixture requires the shipped tilde glyph");
        glyphs.erase(removed, glyphs.end());
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), font.line_spacing);
        limited->set_stand_in(U'?');
        world.render.add_font("menu_text_limited", std::move(limited));
        world.render.add_font("menu_text_tall", std::make_unique<Font>(TextureHandle{}, font.glyphs, 1081.0f));
        for (const auto& role : categories)
        {
            const std::string field = role.group == nullptr ? role.key : std::string(role.group) + '.' + role.key;
            for (const char* text : { "   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\ttext",
                "control\x01", "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
            {
                variants.rejects(field, [role, text](rapidjson::Document& d)
                    { text_field(d, role.group, role.key).SetString(text, d.GetAllocator()); });
            }
            variants.rejects(field, [role](rapidjson::Document& d)
                { text_field(d, role.group, role.key).SetString(std::string(200, 'W').c_str(), d.GetAllocator()); }, &world);
            variants.rejects(field, [role](rapidjson::Document& d)
            {
                select_font(d, role, "menu_text_limited");
                text_field(d, role.group, role.key).SetString("Glyph~");
            }, &world);
            variants.rejects(field, [role](rapidjson::Document& d)
                { select_font(d, role, "menu_text_tall"); }, &world);
        }
    }

    void check_prompt_bar(World& world, const Presentation& look)
    {
        const auto& style = look.prompt_bar();
        for (size_t index = 0; index < std::size(prompts); ++index)
        {
            const std::wstring& legend = look.menu_text().prompt(static_cast<MenuPrompt>(index));
            PromptBar bar("authored_prompt", legend, style, &world.render);
            UiTextDropShadow reference("measured_prompt", legend, style.font, style.position, &world.render,
                style.colour, style.shadow_colour, style.shadow_offset);
            require(bar.text() == legend && bar.text_bounds() == reference.text_bounds() && bar.bounds() == reference.bounds(),
                "Actual authored PromptBar changed the complete legend, font or measured geometry");
            bar.set_hidden(true);
            DrawList no_device;
            bar.draw(no_device);
        }
        std::wstring source = L"  X/R Authored legend  ";
        PromptBar first("first", source, style, &world.render);
        source = L"A/Enter New legend";
        PromptBar next("next", source, style, &world.render);
        require(first.text() == L"  X/R Authored legend  " && next.text() == source,
            "PromptBar retained a borrowed legend or discarded complete authored spacing");
        const std::vector<PromptBar::Prompt> legacy{ {L"A", L"Select"}, {L"B", L"Back"} };
        PromptBar old("legacy", legacy, style, &world.render);
        require(old.text() == L"A Select    B Back", "Legacy PromptBar vector formatting changed");
    }

    void check_inclusive_bounds(Variants& variants, World& world)
    {
        std::vector<Glyph> glyphs;
        for (const char32_t character : { U'F', U'i', U't' })
        {
            glyphs.push_back({ character, mattmath::RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("menu_text_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("menu_text_exact"), L"Fit") ==
            mattmath::Vector2F(24.0f, 20.0f), "Exact-boundary fixture font has unexpected metrics");
        const auto edge = [](rapidjson::Document& d)
        {
            d["prompt_bar"]["font"].SetString("menu_text_exact");
            for (const auto& entry : prompts) { d["menu_text"]["prompts"][entry.key].SetString("Fit"); }
            // 24x20 text plus a one-unit shadow, with the original 150-unit
            // right margin, exactly touches both inclusive design boundaries.
            d["prompt_bar"]["position"]["x"].SetInt(1745);
            d["prompt_bar"]["position"]["y"].SetInt(1059);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(1);
        };
        const auto fits = variants.load(edge);
        fits.validate_resources(world.render, world.audio);
        const auto reversed_shadow = variants.load([&](rapidjson::Document& d)
        {
            edge(d);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(-1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(-1);
        });
        reversed_shadow.validate_resources(world.render, world.audio);
        const auto top_left = [&](rapidjson::Document& d)
        {
            edge(d);
            d["prompt_bar"]["position"]["x"].SetInt(1);
            d["prompt_bar"]["position"]["y"].SetInt(1);
            d["prompt_bar"]["shadow_offset"]["x"].SetInt(-1);
            d["prompt_bar"]["shadow_offset"]["y"].SetInt(-1);
        };
        const auto shadow_touches_edges = variants.load(top_left);
        shadow_touches_edges.validate_resources(world.render, world.audio);
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("prompts.title", [&](rapidjson::Document& d)
            {
                top_left(d);
                d["prompt_bar"]["position"][axis].SetDouble(0.75);
            }, &world);
            variants.rejects("prompts.title", [&](rapidjson::Document& d)
            {
                edge(d);
                d["prompt_bar"]["position"][axis].SetDouble(std::string(axis) == "x" ? 1745.25 : 1059.25);
            }, &world);
            for (const double invalid : { -1.0, 1e100 })
            {
                variants.rejects("prompts.title", [&](rapidjson::Document& d)
                {
                    edge(d);
                    d["prompt_bar"]["position"][axis].SetDouble(invalid);
                }, &world);
            }
        }
    }

    UiText* row_text(OptionRow& row, const std::string& name)
    {
        for (const auto& child : row.children())
        {
            if (child.first == name) { return dynamic_cast<UiText*>(child.second); }
        }
        return nullptr;
    }

    void check_option_row_fitting(Variants& variants, World& world)
    {
        const std::string authored(200, 'W');
        const auto look = variants.load([&](rapidjson::Document& d)
        {
            for (const char* key : { "resolution", "fullscreen", "team_palette" })
            {
                d["menu_text"]["items"][key].SetString(authored.c_str(), d.GetAllocator());
            }
        });
        // These labels are not plain text actions. Resource validation checks
        // their glyphs and fitted geometry, preserving the actual OptionRow's
        // ability to fit a long, otherwise valid label.
        look.validate_resources(world.render, world.audio);
        for (const char* key : { "resolution", "fullscreen", "team_palette" })
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
            {
                d["menu_style"]["fonts"]["item"].SetString("menu_text_limited");
                d["menu_text"]["items"][key].SetString("Glyph~");
            }, &world);
        }
        const MenuStyle& style = look.menu();
        const auto& art = look.main_menu().art;
        const mattmath::Vector2F label_position(100.0f, 100.0f);
        const mattmath::Vector2F value_position(420.0f, 100.0f);
        const float label_column = value_position.x - label_position.x - art.focus_padding.x;
        const auto font = world.render.resolve_sprite_font(style.item_font);
        for (const MenuItem item : { MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette })
        {
            int adjustments = 0;
            const std::wstring& text = look.menu_text().item(item);
            const auto original = world.render.measure_text(font, text);
            require(original.x > label_column, "Long OptionRow fixture already fits without scaling");
            OptionRow row("fitted", text, label_position, value_position, style, art.ink, &world.render,
                [&](Direction direction) { adjustments += direction == Direction::right ? 1 : -1; },
                &art, {160.0f, 1000.0f});
            UiText* label = row_text(row, "fitted_label");
            UiText* value = row_text(row, "fitted_value");
            require(label != nullptr && value != nullptr && label->text() == text &&
                std::isfinite(label->bounds().width) && label->bounds().width > 0.0f &&
                label->bounds().width <= label_column + 0.01f && label->bounds().width < original.x,
                "Actual OptionRow truncated or failed to fit its long authored label to its column");
            const auto retained_label = label->bounds();
            row.set_value(L"Current value");
            require(value->text() == L"Current value" && value->bounds().width <= 160.01f &&
                label->bounds() == retained_label && label->text() == text,
                "OptionRow value updates changed its fitted label or escaped the supplied value width");
            row.adjust(Direction::right);
            require(adjustments == 1, "Fitted OptionRow lost its right adjustment action");
            row.adjust(Direction::left);
            require(adjustments == 0, "Fitting an authored OptionRow label changed its adjustment actions");
        }

        const std::wstring short_label = L"Height constrained";
        const auto original = world.render.measure_text(font, short_label);
        require(original.y > 10.0f, "OptionRow height fixture already fits without scaling");
        OptionRow height_row("height", short_label, label_position, {10000.0f, 100.0f}, style,
            art.ink, &world.render, {}, &art, {160.0f, 10.0f});
        const UiText* fitted = row_text(height_row, "height_label");
        require(fitted != nullptr && fitted->text() == short_label && fitted->bounds().height > 0.0f &&
            fitted->bounds().height <= 10.01f && fitted->bounds().height < original.y &&
            fitted->bounds().width < original.x,
            "Actual OptionRow did not preserve and shrink a height-constrained authored label");
    }

    void check_item_bounds(Variants& variants, World& world)
    {
        for (const char* key : { "play", "options", "credits", "exit", "controls_audio", "apply", "back" })
        {
            variants.rejects(std::string("items.") + key, [key](rapidjson::Document& d)
                { d["menu_text"]["items"][key].SetString(std::string(200, 'W').c_str(), d.GetAllocator()); }, &world);
        }
        const auto original = variants.load([](rapidjson::Document&) {});
        const float row_height = 20.0f + original.main_menu().art.focus_padding.y * 2.0f;
        const float paper_right = (1920.0f + original.main_menu().home_box_size.x) / 2.0f;
        const float right_position = paper_right - 24.0f - original.main_menu().art.focus_padding.x;
        const auto exact_items = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["item"].SetString("menu_text_exact");
            for (const auto& entry : items) { d["menu_text"]["items"][entry.key].SetString("Fit"); }
        };
        const auto row_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["main_menu"]["home_spacing"]["y"].SetFloat(row_height);
            d["main_menu"]["home_subtitle_position"]["y"].SetFloat(
                original.main_menu().home_position.y + row_height / 2.0f);
        };
        const auto fits_row = variants.load(row_edge);
        fits_row.validate_resources(world.render, world.audio);
        variants.rejects("items.play", [&](rapidjson::Document& d)
        {
            row_edge(d);
            d["main_menu"]["home_spacing"]["y"].SetFloat(row_height - 0.25f);
        }, &world);

        const auto paper_edge = [&](rapidjson::Document& d)
        {
            exact_items(d);
            d["menu_style"]["fonts"]["heading"].SetString("menu_text_exact");
            for (const auto& entry : headings) { d["menu_text"]["headings"][entry.key].SetString("Fit"); }
            d["main_menu"]["home_position"]["x"].SetFloat(right_position);
        };
        const auto fits_paper = variants.load(paper_edge);
        fits_paper.validate_resources(world.render, world.audio);
        variants.rejects("items.play", [&](rapidjson::Document& d)
        {
            paper_edge(d);
            d["main_menu"]["home_position"]["x"].SetFloat(right_position + 0.25f);
        }, &world);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: menu_text.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "menu-text-fixtures");
        World world(content);
        const Presentation defaults((content / "presentation.json").string().c_str());
        check_defaults(defaults);
        defaults.validate_resources(world.render, world.audio);
        check_keyed_content(variants);
        check_text_and_fonts(variants, world, content);
        check_inclusive_bounds(variants, world);
        check_item_bounds(variants, world);
        check_option_row_fitting(variants, world);
        check_prompt_bar(world, defaults);
        std::cout << "PASS shared menu text: exact headings/items/complete legends, keyed and enum guards, normalization, "
            "font/paper/row bounds, OptionRow fitting and actual PromptBar copy/measurement compatibility.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL shared menu text: " << error.what() << '\n';
        return 1;
    }
}
