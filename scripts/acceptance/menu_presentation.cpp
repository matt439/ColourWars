#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/menu_panel.h"
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
            layout["position"]["x"].SetInt(90); layout["position"]["y"].SetInt(120);
            layout["size"]["x"].SetInt(450); layout["size"]["y"].SetInt(90);
            layout["spacing"]["x"].SetInt(510); layout["spacing"]["y"].SetInt(180);
        });
        custom.validate_resources(world.render, world.audio);
        require(custom.menu().layout.position == Vector2F(90.0f, 120.0f) &&
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
