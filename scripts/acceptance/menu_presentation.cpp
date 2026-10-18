#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/menu_panel.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

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

    void write_document(const rapidjson::Document& document, const std::filesystem::path& path)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        require(document.Accept(writer), "Cannot encode menu presentation fixture");
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        output.close();
        require(output.good(), "Cannot write menu presentation fixture");
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;

        explicit World(const std::filesystem::path& content)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
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
            const auto path = scratch / ("menu-presentation-" + std::to_string(next++) + ".json");
            write_document(document, path);
            return path;
        }

        template<class Edit>
        Presentation load(Edit edit)
        {
            const auto path = write(edit);
            return Presentation(path.string().c_str());
        }

        template<class Edit>
        void rejects(const char* owner, const char* field, Edit edit, World* resources = nullptr)
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
                const std::string message = error.what();
                diagnostic = message;
                contextual = message.find(path.filename().string()) != std::string::npos &&
                    message.find(owner) != std::string::npos && message.find(field) != std::string::npos;
            }
            require(contextual, std::string("Invalid ") + owner + '.' + field +
                " did not identify its source file and field: " + diagnostic);
        }
    };

    void check_defaults(const Presentation& look)
    {
        const MenuStyle& menu = look.menu();
        require(menu.layout.position == Vector2F(150.0f, 150.0f) &&
            menu.layout.size == Vector2F(300.0f, 75.0f) &&
            menu.layout.spacing == Vector2F(250.0f, 150.0f) &&
            menu.sprite_sheet == "sprite_sheet_1" && menu.fill_frame == "pixel",
            "Authored menu defaults changed the previous base layout or fill artwork");
        const auto& main = look.main_menu();
        require(main.setup_box_size == Vector2F(1760.0f, 920.0f) && main.setup_focus_padding == Vector2F(28.0f, 7.0f) &&
            main.setup_shadow_offset == 2.0f &&
            (Vector2F(1920.0f, menu.footer_y) - main.setup_box_size) / 2.0f == Vector2F(80.0f, 20.0f),
            "Authored setup paper changed its reviewed size or placement above the shared footer");
        require(main.team_box_size == Vector2F(1840.0f, 920.0f) &&
            main.team_warning_position == Vector2F(150.0f, 860.0f) &&
            main.team_select.spacing == Vector2F(250.0f, 125.0f) && main.team_select.size == Vector2F(50.0f, 50.0f),
            "Authored Team paper changed its reviewed warning or existing selection geometry");
        require(main.weapon_box_size == Vector2F(1840.0f, 920.0f) &&
            main.weapon_select.spacing == Vector2F(250.0f, 125.0f) && main.weapon_select.size == Vector2F(120.0f, 64.0f) &&
            main.weapon_description_colour == main.art.ink,
            "Authored Weapon paper changed its reviewed ink or existing selection geometry");
        require(main.stage_box_size == Vector2F(1840.0f, 920.0f) &&
            main.stage_select.selected_ink == main.art.focus_ink && main.stage_select.unselected == main.art.ink &&
            main.stage_select.selected == menu.highlight,
            "Authored Stage paper changed its reviewed ink or selected preview accent");
        require(main.team_select_sheet == "sprite_sheet_1" &&
            main.team_select_frames == std::array<std::string, 3>{ "team_select_a", "team_select_center", "team_select_b" } &&
            main.team_select_selected_frames == std::array<std::string, 3>{ "team_select_a_selected",
                "team_select_center_selected", "team_select_b_selected" },
            "Authored team assets changed their A/centre/B order or readiness defaults");
    }

    void check_layout(Variants& variants, World& world)
    {
        const auto custom = variants.load([](rapidjson::Document& d)
        {
            auto& layout = d["menu_style"]["layout"];
            layout["position"]["x"].SetInt(150); layout["position"]["y"].SetInt(120);
            layout["size"]["x"].SetInt(450); layout["size"]["y"].SetInt(90);
            layout["spacing"]["x"].SetInt(510); layout["spacing"]["y"].SetInt(180);
        });
        custom.validate_resources(world.render, world.audio);
        require(custom.menu().layout.position == Vector2F(150.0f, 120.0f) &&
            custom.menu().layout.size == Vector2F(450.0f, 90.0f) &&
            custom.menu().layout.spacing == Vector2F(510.0f, 180.0f), "Custom base menu layout was not loaded exactly");
        const auto boundary = variants.load([](rapidjson::Document& d)
        {
            auto& layout = d["menu_style"]["layout"];
            layout["position"]["x"].SetInt(0); layout["position"]["y"].SetInt(0);
            layout["size"]["x"].SetInt(1920); layout["size"]["y"].SetInt(1080);
            layout["spacing"]["x"].SetInt(1920); layout["spacing"]["y"].SetInt(1080);
        });
        require(boundary.menu().layout.size == Vector2F(1920.0f, 1080.0f) &&
            boundary.menu().layout.spacing == Vector2F(1920.0f, 1080.0f), "Inclusive base layout limits were rejected");
        const auto one_item = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["layout"]["spacing"]["x"].SetInt(0);
            d["menu_style"]["layout"]["spacing"]["y"].SetInt(0);
        });
        require(one_item.menu().layout.spacing == Vector2F::ZERO, "A one-item base layout cannot use zero spacing");
        variants.rejects("menu_style", "layout", [](rapidjson::Document& d) { d["menu_style"].RemoveMember("layout"); });
        for (const char* section : { "position", "size", "spacing" })
        {
            variants.rejects("menu_style", "layout", [section](rapidjson::Document& d)
                { d["menu_style"]["layout"].RemoveMember(section); });
            variants.rejects("menu_style", "layout", [section](rapidjson::Document& d)
                { d["menu_style"]["layout"][section].SetArray(); });
            for (const char* axis : { "x", "y" })
            {
                variants.rejects("menu_style", "layout", [section, axis](rapidjson::Document& d)
                    { d["menu_style"]["layout"][section].RemoveMember(axis); });
                variants.rejects("menu_style", "layout", [section, axis](rapidjson::Document& d)
                    { d["menu_style"]["layout"][section][axis].SetBool(true); });
                variants.rejects("menu_style", "layout", [section, axis](rapidjson::Document& d)
                    { d["menu_style"]["layout"][section][axis].SetDouble(1e100); });
                variants.rejects("menu_style", "layout", [section, axis](rapidjson::Document& d)
                    { d["menu_style"]["layout"][section][axis].SetDouble(-0.01); });
            }
        }
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("menu_style", "size", [axis](rapidjson::Document& d)
                { d["menu_style"]["layout"]["size"][axis].SetInt(0); });
            const int limit = std::string(axis) == "x" ? 1920 : 1080;
            variants.rejects("menu_style", "position", [axis, limit](rapidjson::Document& d)
                { d["menu_style"]["layout"]["position"][axis].SetInt(limit); });
            variants.rejects("menu_style", "spacing", [axis, limit](rapidjson::Document& d)
                { d["menu_style"]["layout"]["spacing"][axis].SetInt(limit + 1); });
        }
    }

    void check_setup_panel(Variants& variants, World& world)
    {
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_box_size"); });
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"].SetArray(); });
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"].SetNull(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"].RemoveMember(axis); });
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"][axis].SetBool(true); });
            variants.rejects("main_menu", "setup_box_size", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_box_size"][axis].SetString("920"); });
            for (const double invalid : { 0.0, -1.0, 1e100 })
            {
                variants.rejects("main_menu", "setup_box_size", [axis, invalid](rapidjson::Document& d)
                    { d["main_menu"]["setup_box_size"][axis].SetDouble(invalid); });
            }
            const float maximum = std::string(axis) == "x" ? 1760.0f : 920.0f;
            variants.rejects("main_menu", "setup_box_size", [axis, maximum](rapidjson::Document& d)
            {
                d["main_menu"]["setup_box_size"][axis].SetFloat(std::nextafter(maximum,
                    std::numeric_limits<float>::infinity()));
            });
        }
        const auto custom = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_box_size"]["x"].SetInt(1720);
            d["main_menu"]["setup_box_size"]["y"].SetInt(900);
            d["menu_style"]["layout"]["position"]["x"].SetInt(170);
        });
        custom.validate_resources(world.render, world.audio);
        require(custom.main_menu().setup_box_size == Vector2F(1720.0f, 900.0f),
            "Custom setup paper size was not loaded exactly");
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetInt(1000);
            d["main_menu"]["setup_box_size"]["y"].SetInt(960);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require((Vector2F(1920.0f, footer_boundary.menu().footer_y) -
            footer_boundary.main_menu().setup_box_size) / 2.0f == Vector2F(80.0f, 20.0f),
            "Setup paper no longer derives its vertical center from the authored footer");
        variants.rejects("main_menu", "setup_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["setup_box_size"]["y"].SetInt(960);
        });
        variants.rejects("main_menu", "setup_focus_padding", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_focus_padding"); });
        variants.rejects("main_menu", "setup_focus_padding", [](rapidjson::Document& d)
            { d["main_menu"]["setup_focus_padding"].SetArray(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects("main_menu", "setup_focus_padding", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"].RemoveMember(axis); });
            variants.rejects("main_menu", "setup_focus_padding", [axis](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"][axis].SetBool(false); });
            for (const double invalid : { 0.0, -1.0, 6.0, 1e100 })
            {
                variants.rejects("main_menu", "setup_focus_padding", [axis, invalid](rapidjson::Document& d)
                    { d["main_menu"]["setup_focus_padding"][axis].SetDouble(invalid); });
            }
            const float limit = std::string(axis) == "x" ? 48.0f : 24.0f;
            variants.rejects("main_menu", "setup_focus_padding", [axis, limit](rapidjson::Document& d)
                { d["main_menu"]["setup_focus_padding"][axis].SetFloat(limit + 0.25f); });
        }
        const auto padding_boundary = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_focus_padding"]["x"].SetInt(48);
            d["main_menu"]["setup_focus_padding"]["y"].SetInt(24);
        });
        require(padding_boundary.main_menu().setup_focus_padding == Vector2F(48.0f, 24.0f),
            "Inclusive setup focus-padding definition limits were rejected");
        const auto padding_minimum = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["setup_focus_padding"]["x"].SetFloat(6.25f);
            d["main_menu"]["setup_focus_padding"]["y"].SetFloat(6.25f);
        });
        require(padding_minimum.main_menu().setup_focus_padding == Vector2F(6.25f, 6.25f),
            "Setup focus padding just above the shade-plus-keyline threshold was rejected");
        variants.rejects("main_menu", "setup_shadow_offset", [](rapidjson::Document& d)
            { d["main_menu"].RemoveMember("setup_shadow_offset"); });
        variants.rejects("main_menu", "setup_shadow_offset", [](rapidjson::Document& d)
            { d["main_menu"]["setup_shadow_offset"].SetBool(false); });
        for (const double invalid : { -0.25, 24.25, 1e100 })
        {
            variants.rejects("main_menu", "setup_shadow_offset", [invalid](rapidjson::Document& d)
                { d["main_menu"]["setup_shadow_offset"].SetDouble(invalid); });
        }
        const auto no_shade = variants.load([](rapidjson::Document& d)
            { d["main_menu"]["setup_shadow_offset"].SetFloat(0.0f); });
        no_shade.validate_resources(world.render, world.audio);
        require(no_shade.main_menu().setup_shadow_offset == 0.0f,
            "Inclusive zero setup-shadow limit was rejected");
    }

    void check_setup_geometry(Variants& variants, World& world)
    {
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, RectangleI(0, 0, 8, 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("setup_exact", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("setup_exact");
            }
            for (auto member = d["menu_text"]["items"].MemberBegin(); member != d["menu_text"]["items"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
        };
        const auto check_edge = [&](const char* owner, const char* field, auto edge, auto outside)
        {
            const auto fits = variants.load([&](rapidjson::Document& d) { exact(d); edge(d); });
            fits.validate_resources(world.render, world.audio);
            variants.rejects(owner, field, [&](rapidjson::Document& d)
            {
                exact(d);
                edge(d);
                outside(d);
            }, &world);
        };
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["main_menu"]["setup_box_size"]["x"].SetFloat(1756.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["setup_box_size"]["x"].SetFloat(1755.75f); });
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["menu_style"]["layout"]["position"]["x"].SetFloat(148.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["x"].SetFloat(147.75f); });
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
        {
            d["menu_style"]["layout"]["position"]["x"].SetFloat(149.5f);
            d["menu_style"]["shadow"]["item_offset"]["x"].SetFloat(-1.5f);
        }, [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["x"].SetFloat(149.25f); });
        check_edge("menu_text.headings", "mode", [](rapidjson::Document& d)
            { d["menu_style"]["layout"]["position"]["y"].SetFloat(32.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["y"].SetFloat(31.75f); });
        check_edge("menu_text.items", "single_match", [](rapidjson::Document& d)
            { d["menu_style"]["shadow"]["heading_offset"]["y"].SetFloat(173.0f); },
            [](rapidjson::Document& d) { d["menu_style"]["shadow"]["heading_offset"]["y"].SetFloat(173.25f); });
        check_edge("menu_text", "solo_practice_hint", [](rapidjson::Document& d)
            { d["menu_style"]["layout"]["position"]["y"].SetFloat(204.5f); },
            [](rapidjson::Document& d) { d["menu_style"]["layout"]["position"]["y"].SetFloat(204.75f); });
        // Join shares Count's spacing and adds a status at row seven. Its
        // complete 20-high text plus one-unit shadow reaches the inset bottom
        // at 65 + 7*120 + 21 = 926. A Count Back card at that bottom would
        // leave no space for this later status, so retain its old overflow
        // rejection separately instead of calling that layout a valid page.
        check_edge("menu_text.connection", "join_ready", [](rapidjson::Document& d)
        {
            d["menu_style"]["layout"]["position"]["y"].SetFloat(65.0f);
            d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.25f); });
        variants.rejects("menu_text.items", "back", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["layout"]["position"]["y"].SetFloat(175.5f);
            d["main_menu"]["player_count_spacing"]["y"].SetFloat(120.25f);
        }, &world);
        for (const char* spacing : { "mode_select_spacing", "player_count_spacing" })
        {
            const char* field = std::string(spacing) == "mode_select_spacing" ? "single_match" : "one_player_practice";
            check_edge("menu_text.items", field, [spacing](rapidjson::Document& d)
                { d["main_menu"][spacing]["y"].SetFloat(37.5f); },
                [spacing](rapidjson::Document& d) { d["main_menu"][spacing]["y"].SetFloat(37.25f); });
        }
        variants.rejects("menu_text.items", "match_length", [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["mode_select_spacing"]["x"].SetFloat(900.0f);
        }, &world);

        // Keep Mode/Count and their detached arrows comfortably inside this
        // narrower shared paper. Only Join's authored heading reaches its edge;
        // both widths still fit the old generic design-area/right-margin check.
        check_edge("menu_text.headings", "join_players", [](rapidjson::Document& d)
        {
            d["main_menu"]["setup_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["join_players"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        }, [](rapidjson::Document& d)
        {
            d["menu_text"]["headings"]["join_players"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        });

        std::vector<Glyph> join_glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            join_glyphs.push_back({ character, RectangleI(0, 0, 8, character == U'~' ? 780 : 16),
                0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("join_heading_tall", std::make_unique<Font>(TextureHandle{}, std::move(join_glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("join_heading_tall"), L"~") ==
            Vector2F(8.0f, 780.0f), "Join paper fixture did not retain its tall glyph beyond the font's line spacing");
        const auto ordinary_join = variants.load([&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
        });
        ordinary_join.validate_resources(world.render, world.audio);
        // A glyph taller than line spacing still fits the generic footer area,
        // but its complete shadow bounds cross Join's paper at y=926.
        variants.rejects("menu_text.headings", "join_players", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["join_players"].SetString("~");
        }, &world);
    }

    template<class Container>
    void check_vector_definition(Variants& variants, const char* owner, const char* key, Container container, bool positive)
    {
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d).RemoveMember(key); });
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetNull(); });
        variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetArray(); });
        for (const char* axis : { "x", "y" })
        {
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].RemoveMember(axis); });
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetBool(false); });
            for (const double invalid : { -0.25, 1e100 })
            {
                variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetDouble(invalid); });
            }
            if (positive)
            {
                variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key][axis].SetInt(0); });
            }
        }
    }

    void check_team_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "team_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* key : { "warning_position", "spacing", "size" })
        {
            check_vector_definition(variants, "main_menu.team_select", key, [](rapidjson::Document& d) -> rapidjson::Value&
                { return d["main_menu"]["team_select"]; }, std::string(key) != "warning_position");
        }
        for (const char* axis : { "x", "y" })
        {
            const float panel_limit = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "team_box_size", [axis, panel_limit](rapidjson::Document& d)
                { d["main_menu"]["team_box_size"][axis].SetFloat(panel_limit + 0.25f); });
            const float position_limit = std::string(axis) == "x" ? 1920.0f : 960.0f;
            variants.rejects("main_menu.team_select", "warning_position", [axis, position_limit](rapidjson::Document& d)
                { d["main_menu"]["team_select"]["warning_position"][axis].SetFloat(position_limit + 0.25f); });
        }
        const auto warning_definition_edges = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1920.0f);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(960.0f);
        });
        require(warning_definition_edges.main_menu().team_warning_position == Vector2F(1920.0f, 960.0f),
            "Inclusive Team warning position definition limits were rejected");
        const auto warning_definition_origin = variants.load([](rapidjson::Document& d)
        {
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(0.0f);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(0.0f);
        });
        require(warning_definition_origin.main_menu().team_warning_position == Vector2F::ZERO,
            "Nonnegative Team warning position definitions rejected zero before resource bounds were checked");
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["team_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().team_box_size == Vector2F(1840.0f, 960.0f),
            "Team panel height did not follow its authored footer boundary");
        variants.rejects("main_menu", "team_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["team_box_size"]["y"].SetFloat(960.0f);
        });

        const auto exact = [](rapidjson::Document& d)
        {
            for (const char* font : { "heading", "item", "detail" })
            {
                d["menu_style"]["fonts"][font].SetString("setup_exact");
            }
            for (const char* group : { "headings", "items" })
            {
                for (auto member = d["menu_text"][group].MemberBegin(); member != d["menu_text"][group].MemberEnd(); ++member)
                {
                    member->value.SetString("Fit");
                }
            }
            d["menu_text"]["solo_practice_hint"].SetString("Fit");
        };
        const auto check_edge = [&](const char* owner, const char* field, auto edge, auto outside)
        {
            const auto fits = variants.load([&](rapidjson::Document& d) { exact(d); edge(d); });
            fits.validate_resources(world.render, world.audio);
            variants.rejects(owner, field, [&](rapidjson::Document& d)
            {
                exact(d);
                edge(d);
                outside(d);
            }, &world);
        };
        // The complete fixed warning is 192x20 in this font; its retained
        // one-pixel shadow must stay inside Team's (52,32)..(1866,926) interior.
        require(world.render.measure_text(world.render.resolve_sprite_font("setup_exact"), L"Both teams need a player") ==
            Vector2F(192.0f, 20.0f), "Team warning fixture no longer measures the complete shipped wording");
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(52.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(51.75f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1673.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(1673.25f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
            { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(905.0f); },
            [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(905.25f); });
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
        {
            d["menu_style"]["shadow"]["detail_offset"]["x"].SetFloat(-1.0f);
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(53.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(52.75f); });
        variants.rejects("main_menu.team_select", "warning_position", [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(100.0f);
        }, &world);
        check_edge("menu_text.headings", "teams", [](rapidjson::Document& d)
        {
            d["main_menu"]["team_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["main_menu"]["team_select"]["warning_position"]["x"].SetFloat(500.0f);
            d["menu_text"]["headings"]["teams"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        }, [](rapidjson::Document& d)
        {
            d["menu_text"]["headings"]["teams"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        });
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character <= U'~'; ++character)
        {
            glyphs.push_back({ character, RectangleI(0, 0, 8, character == U'y' ? 70 : 16), 0.0f, 0.0f, 0.0f });
        }
        world.render.add_font("team_warning_descender", std::make_unique<Font>(TextureHandle{}, std::move(glyphs), 20.0f));
        require(world.render.measure_text(world.render.resolve_sprite_font("team_warning_descender"), L"Both teams need a player") ==
            Vector2F(192.0f, 70.0f), "Team warning did not measure its descender beyond the font's line spacing");
        check_edge("main_menu.team_select", "warning_position", [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["detail"].SetString("team_warning_descender");
            d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(855.0f);
        }, [](rapidjson::Document& d) { d["main_menu"]["team_select"]["warning_position"]["y"].SetFloat(855.25f); });
    }

    void check_weapon_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "weapon_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* key : { "spacing", "size" })
        {
            check_vector_definition(variants, "main_menu.weapon_select", key, [](rapidjson::Document& d) -> rapidjson::Value&
                { return d["main_menu"]["weapon_select"]; }, true);
        }
        for (const char* axis : { "x", "y" })
        {
            const float maximum = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "weapon_box_size", [axis, maximum](rapidjson::Document& d)
                { d["main_menu"]["weapon_box_size"][axis].SetFloat(maximum + 0.25f); });
        }
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["weapon_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().weapon_box_size == Vector2F(1840.0f, 960.0f),
            "Weapon paper height did not follow its authored footer boundary");
        variants.rejects("main_menu", "weapon_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["weapon_box_size"]["y"].SetFloat(960.0f);
        });
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("setup_exact");
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
        };
        const auto width_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["weapon_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            // Moving the shared origin also moves Join's unfitted legend.
            // Keep it short so Weapon's heading owns this paper-edge case.
            d["menu_text"]["connection"]["join_waiting"].SetString("Fit");
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["weapons"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        };
        const auto width = variants.load(width_edge);
        width.validate_resources(world.render, world.audio);
        // Generic design bounds and the wider Setup/Team papers still accept
        // this text; the new Weapon paper alone owns the exact right edge.
        variants.rejects("menu_text.headings", "weapons", [&](rapidjson::Document& d)
        {
            width_edge(d);
            d["menu_text"]["headings"]["weapons"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("menu_text.headings", "weapons", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["weapons"].SetString("~");
        }, &world);
    }

    void check_stage_paper(Variants& variants, World& world)
    {
        check_vector_definition(variants, "main_menu", "stage_box_size", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]; }, true);
        for (const char* axis : { "x", "y" })
        {
            const float maximum = std::string(axis) == "x" ? 1840.0f : 920.0f;
            variants.rejects("main_menu", "stage_box_size", [axis, maximum](rapidjson::Document& d)
                { d["main_menu"]["stage_box_size"][axis].SetFloat(maximum + 0.25f); });
        }
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"].RemoveMember("selected_ink"); });
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"]["selected_ink"].SetBool(true); });
        variants.rejects("main_menu.stage_select", "selected_ink", [](rapidjson::Document& d)
            { d["main_menu"]["stage_select"]["selected_ink"].SetNull(); });
        const auto footer_boundary = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(1000.0f);
            d["main_menu"]["stage_box_size"]["y"].SetFloat(960.0f);
        });
        footer_boundary.validate_resources(world.render, world.audio);
        require(footer_boundary.main_menu().stage_box_size == Vector2F(1840.0f, 960.0f),
            "Stage paper height did not follow its authored footer boundary");
        variants.rejects("main_menu", "stage_box_size", [](rapidjson::Document& d)
        {
            d["menu_style"]["chrome"]["footer_y"].SetFloat(999.75f);
            d["main_menu"]["stage_box_size"]["y"].SetFloat(960.0f);
        });
        const auto exact = [](rapidjson::Document& d)
        {
            d["menu_style"]["fonts"]["heading"].SetString("setup_exact");
            for (auto member = d["menu_text"]["headings"].MemberBegin(); member != d["menu_text"]["headings"].MemberEnd(); ++member)
            {
                member->value.SetString("Fit");
            }
        };
        const auto width_edge = [&](rapidjson::Document& d)
        {
            exact(d);
            d["main_menu"]["stage_box_size"]["x"].SetFloat(1200.0f);
            d["menu_style"]["layout"]["position"]["x"].SetFloat(500.0f);
            // The shared origin must not make Join's separate legend the
            // failing role in this Stage heading boundary fixture.
            d["menu_text"]["connection"]["join_waiting"].SetString("Fit");
            d["menu_style"]["shadow"]["heading_offset"]["x"].SetFloat(6.0f);
            d["menu_text"]["headings"]["arena"].SetString(std::string(130, 'F').c_str(), d.GetAllocator());
        };
        const auto width = variants.load(width_edge);
        width.validate_resources(world.render, world.audio);
        // These headings fit every prior design-area and paper check. Stage's
        // own paper owns this exact shadow edge and the tall-glyph rejection.
        variants.rejects("menu_text.headings", "arena", [&](rapidjson::Document& d)
        {
            width_edge(d);
            d["menu_text"]["headings"]["arena"].SetString(std::string(131, 'F').c_str(), d.GetAllocator());
        }, &world);
        variants.rejects("menu_text.headings", "arena", [&](rapidjson::Document& d)
        {
            exact(d);
            d["menu_style"]["fonts"]["heading"].SetString("join_heading_tall");
            d["menu_text"]["headings"]["arena"].SetString("~");
        }, &world);
    }

    std::unique_ptr<SpriteSheet> sparse_sheet(std::initializer_list<const char*> names)
    {
        NameTable<SpriteFrame> frames("sprite frame");
        for (const char* name : names) { frames.add(name, SpriteFrame(RectangleI(0, 0, 4, 4))); }
        return std::make_unique<SpriteSheet>(TextureHandle{}, std::move(frames), NameTable<AnimationStrip>("animation strip"));
    }

    void check_assets(Variants& variants, World& world)
    {
        world.render.add_sprite_sheet("authored_menu_fill", sparse_sheet({ "authored_fill" }));
        world.render.add_sprite_sheet("authored_team_choices", sparse_sheet({ "choice_a", "choice_center", "choice_b",
            "ready_a", "ready_center", "ready_b" }));
        const auto custom = variants.load([](rapidjson::Document& d)
        {
            d["menu_style"]["sprite_sheet"].SetString("authored_menu_fill");
            d["menu_style"]["fill_frame"].SetString("authored_fill");
            auto& team = d["main_menu"]["team_select"];
            team["sheet"].SetString("authored_team_choices");
            team["frames"]["a"].SetString("choice_a"); team["frames"]["center"].SetString("choice_center");
            team["frames"]["b"].SetString("choice_b");
            team["selected_frames"]["a"].SetString("ready_a"); team["selected_frames"]["center"].SetString("ready_center");
            team["selected_frames"]["b"].SetString("ready_b");
        });
        custom.validate_resources(world.render, world.audio);
        require(custom.menu().sprite_sheet == "authored_menu_fill" && custom.menu().fill_frame == "authored_fill" &&
            custom.main_menu().team_select_sheet == "authored_team_choices" &&
            custom.main_menu().team_select_frames == std::array<std::string, 3>{ "choice_a", "choice_center", "choice_b" } &&
            custom.main_menu().team_select_selected_frames == std::array<std::string, 3>{ "ready_a", "ready_center", "ready_b" },
            "Custom menu/team artwork names or role ordering were lost");

        // There is no legacy atlas or 'pixel' frame in this resource bag. Real
        // paper construction must resolve both authored names successfully.
        RenderResources sparse;
        sparse.add_sprite_sheet("authored_menu_fill", sparse_sheet({ "authored_fill" }));
        const FocusGroup focus;
        const RectangleF rectangle(100.0f, 120.0f, 800.0f, 600.0f);
        MenuPaperArtwork paper(rectangle, &sparse, &custom.menu(), &custom.main_menu().art, &focus);
        require(paper.bounds() == rectangle, "Authored paper construction changed its panel geometry");
        paper.set_hidden(true);
        DrawList no_device;
        paper.draw(no_device); // Hidden artwork may never touch a renderer.

        const auto string_field = [&](const char* owner, const char* key, auto container)
        {
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d).RemoveMember(key); });
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetString(""); });
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetInt(7); });
            variants.rejects(owner, key, [&](rapidjson::Document& d) { container(d)[key].SetNull(); });
            variants.rejects(owner, key, [&](rapidjson::Document& d)
                { container(d)[key].SetString("nul\0identifier", 14, d.GetAllocator()); });
            const char* resource_key = std::string(key) == "sprite_sheet" ? "fill_frame" :
                (std::string(key) == "sheet" ? "frames.a" : key);
            variants.rejects(owner, resource_key, [&](rapidjson::Document& d)
                { container(d)[key].SetString("missing_menu_asset"); }, &world);
        };
        for (const char* key : { "sprite_sheet", "fill_frame" })
        {
            string_field("menu_style", key, [](rapidjson::Document& d) -> rapidjson::Value& { return d["menu_style"]; });
        }
        string_field("main_menu.team_select", "sheet", [](rapidjson::Document& d) -> rapidjson::Value&
            { return d["main_menu"]["team_select"]; });
        for (const char* group : { "frames", "selected_frames" })
        {
            variants.rejects("main_menu.team_select", group, [group](rapidjson::Document& d)
                { d["main_menu"]["team_select"].RemoveMember(group); });
            for (const char* role : { "a", "center", "b" })
            {
                string_field("main_menu.team_select", role, [group](rapidjson::Document& d) -> rapidjson::Value&
                    { return d["main_menu"]["team_select"][group]; });
            }
        }
    }

    void replace_builtin_pixel_references(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator)
    {
        if (value.IsObject())
        {
            // Other atlases also own a frame called pixel (notably painter's
            // redraw pen). Only references to the removed atlas may change.
            const bool builtin_sheet = value.HasMember("sprite_sheet") && value["sprite_sheet"].IsString() &&
                std::string(value["sprite_sheet"].GetString()) == "sprite_sheet_1";
            for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
            {
                if (builtin_sheet && member->value.IsString() &&
                    std::string(member->value.GetString(), member->value.GetStringLength()) == "pixel")
                {
                    member->value.SetString("square_white_4", allocator);
                }
                else { replace_builtin_pixel_references(member->value, allocator); }
            }
        }
        else if (value.IsArray())
        {
            for (auto& item : value.GetArray()) { replace_builtin_pixel_references(item, allocator); }
        }
    }

    void check_required_primitive(Variants& variants, World& world, const std::filesystem::path& content)
    {
        auto sheet = read_document(content / "textures/sprite_sheet_1.json");
        auto& frames = sheet["sprite_frames"];
        bool removed = false;
        for (auto frame = frames.Begin(); frame != frames.End(); ++frame)
        {
            if (std::string((*frame)["name"].GetString()) == "pixel")
            {
                frames.Erase(frame);
                removed = true;
                break;
            }
        }
        require(removed, "Required primitive regression fixture has no authored pixel frame");
        const auto path = variants.scratch / "sheet-without-pixel.json";
        write_document(sheet, path);
        world.render.add_sprite_sheet("sprite_sheet_1", read_sprite_sheet(path.string().c_str(), TextureHandle{}));
        // Point data-owned sprite_sheet_1 pixel references at a substitute,
        // including the custom menu atlas, so only the explicit guard for
        // non-menu C++ primitive users can catch the missing built-in frame.
        variants.rejects("built-in primitive frame", "pixel", [](rapidjson::Document& d)
        {
            replace_builtin_pixel_references(d, d.GetAllocator());
            d["menu_style"]["sprite_sheet"].SetString("authored_menu_fill");
            d["menu_style"]["fill_frame"].SetString("authored_fill");
        }, &world);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: menu_presentation.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "menu-presentation-fixtures");
        World world(content);
        const Presentation defaults((content / "presentation.json").string().c_str());
        check_defaults(defaults);
        defaults.validate_resources(world.render, world.audio);
        check_layout(variants, world);
        check_setup_panel(variants, world);
        check_setup_geometry(variants, world);
        check_team_paper(variants, world);
        check_weapon_paper(variants, world);
        check_stage_paper(variants, world);
        check_assets(variants, world);
        check_required_primitive(variants, world, content);
        std::cout << "PASS menu presentation: unchanged defaults, authored base layout/artwork, contextual rejection, "
            "sparse paper construction and retained built-in primitive validation.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL menu presentation: " << error.what() << '\n';
        return 1;
    }
}
