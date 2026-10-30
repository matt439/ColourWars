#include "game/pch.h"
#include "game/presentation.h"
#include "game/states/menu_panel.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/math/vector2i.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

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
    using acceptance::require;

    constexpr const char* keys[]{ "sprayer", "sniper", "roller", "mister", "bucket", "random" };
    constexpr const wchar_t* names[]{ L"Sprayer", L"Sniper", L"Roller", L"Mister", L"Bucket", L"Random" };
    constexpr WeaponType types[]{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
        WeaponType::mister, WeaponType::bucket, WeaponType::random_primary };

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
        require(document.Accept(writer), "Cannot encode weapon catalog fixture");
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
        output.close();
        require(output.good(), "Cannot write weapon catalog fixture");
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;

        explicit World(const std::filesystem::path& content, bool legacy_weapon_sheet = true)
        {
            for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                if (!legacy_weapon_sheet && std::string(name) == "weapon_paint") { continue; }
                render.add_sprite_sheet(name, read_sprite_sheet(
                    (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* name : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
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
            const auto path = scratch / ("weapon-catalog-" + std::to_string(next++) + ".json");
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
        void rejects(const std::string& field, Edit edit, World* resources = nullptr,
            const char* owner = "main_menu.weapon_select")
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
                    diagnostic.find(owner) != std::string::npos;
                // Checked JSON access separates the object path and field with
                // prose; resource validation spells the same location dotted.
                size_t begin = 0;
                while (begin < field.size())
                {
                    const size_t end = field.find('.', begin);
                    contextual = contextual && diagnostic.find(field.substr(begin, end - begin)) != std::string::npos;
                    if (end == std::string::npos) { break; }
                    begin = end + 1;
                }
            }
            require(contextual, "Invalid weapon catalog " + field + " lost source/key context: " + diagnostic);
        }
    };

    rapidjson::Value& catalog(rapidjson::Document& d) { return d["main_menu"]["weapon_select"]; }
    rapidjson::Value& choice(rapidjson::Document& d, const char* key) { return catalog(d)["choices"][key]; }

    void check_defaults(const Presentation& look)
    {
        const auto& art = look.main_menu().weapon_select_art;
        require(WeaponSelectArtworkPresentation::CHOICES == std::size(keys) && art.sheet == "weapon_paint" &&
            art.fill_frame == "pixel", "Weapon catalog changed the six existing choices or atlas defaults");
        for (size_t index = 0; index < std::size(keys); ++index)
        {
            const auto& entry = art.choice(types[index]);
            const bool concrete = types[index] != WeaponType::random_primary;
            const bool supply = types[index] == WeaponType::sprayer || types[index] == WeaponType::mister;
            require(&entry == &art.choices[index] && entry.name == names[index] && entry.frame == keys[index] &&
                entry.paint_frame == (concrete ? std::string(keys[index]) + "_paint" : std::string()) &&
                entry.supply_strip == (supply ? std::string(keys[index]) + "_supply" : std::string()),
                "Authored catalog differs from the previous name/base/paint/supply choices");
        }
        for (const int invalid : { -1, 6, 7, 8, 99 })
        {
            bool rejected = false;
            try { art.choice(static_cast<WeaponType>(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Unsupported weapon catalog enum silently resolved a choice");
        }
    }

    void check_keyed_choices(Variants& variants, const Presentation& defaults)
    {
        const auto reversed = variants.load([](rapidjson::Document& d)
        {
            rapidjson::Value ordered(rapidjson::kObjectType);
            for (size_t count = std::size(keys); count > 0; --count)
            {
                const char* key = keys[count - 1];
                rapidjson::Value name;
                name.SetString(key, d.GetAllocator());
                rapidjson::Value entry;
                entry.CopyFrom(choice(d, key), d.GetAllocator());
                ordered.AddMember(name, entry, d.GetAllocator());
            }
            catalog(d)["choices"].Swap(ordered);
        });
        check_defaults(reversed);
        require(reversed.main_menu().weapon_descriptions == defaults.main_menu().weapon_descriptions,
            "Weapon catalog parsing changed the existing authored descriptions");
        variants.rejects("choices", [](rapidjson::Document& d) { catalog(d).RemoveMember("choices"); });
        variants.rejects("choices", [](rapidjson::Document& d) { catalog(d)["choices"].SetArray(); });
        for (const char* key : keys)
        {
            variants.rejects(key, [key](rapidjson::Document& d) { catalog(d)["choices"].RemoveMember(key); });
            variants.rejects(key, [key](rapidjson::Document& d) { choice(d, key).SetArray(); });
            for (const char* field : { "name", "frame", "paint_frame", "supply_strip" })
            {
                variants.rejects(std::string(key) + '.' + field,
                    [key, field](rapidjson::Document& d) { choice(d, key).RemoveMember(field); });
                variants.rejects(std::string(key) + '.' + field,
                    [key, field](rapidjson::Document& d) { choice(d, key)[field].SetBool(true); });
                variants.rejects(std::string(key) + '.' + field,
                    [key, field](rapidjson::Document& d) { choice(d, key)[field].SetNull(); });
                variants.rejects(std::string(key) + '.' + field, [key, field](rapidjson::Document& d)
                    { choice(d, key)[field].SetString("bad\0value", 9, d.GetAllocator()); });
            }
        }
        for (const char* field : { "sheet", "fill_frame" })
        {
            variants.rejects(field, [field](rapidjson::Document& d) { catalog(d).RemoveMember(field); });
            variants.rejects(field, [field](rapidjson::Document& d) { catalog(d)[field].SetString(""); });
            variants.rejects(field, [field](rapidjson::Document& d) { catalog(d)[field].SetInt(7); });
            variants.rejects(field, [field](rapidjson::Document& d)
                { catalog(d)[field].SetString("bad\0value", 9, d.GetAllocator()); });
        }
    }

    void check_names(Variants& variants, World& world, const std::filesystem::path& content)
    {
        const auto normalized = variants.load([](rapidjson::Document& d)
        {
            choice(d, "sprayer")["name"].SetString("  A\xC2\xA0\xE2\x80\x9C" "B" "\xE2\x80\x9D\xE2\x80\x94" "C\xE2\x80\xA6  ");
        });
        require(normalized.main_menu().weapon_select_art.choice(WeaponType::sprayer).name == L"  A \"B\"-C...  ",
            "Catalog names did not normalize punctuation/NBSP while preserving authored spaces");
        normalized.validate_resources(world.render, world.audio);
        for (const char* text : { "", "   ", " \xC2\xA0 ", "first\nsecond", "first\rsecond", "tab\tname", "control\x01",
            "delete\x7F", "caf\xC3\xA9", "\xF0\x9F\x8E\xA8", "\xC3\x28" })
        {
            variants.rejects("sprayer.name", [text](rapidjson::Document& d)
                { choice(d, "sprayer")["name"].SetString(text, d.GetAllocator()); });
        }
        variants.rejects("sprayer.frame", [](rapidjson::Document& d) { choice(d, "sprayer")["frame"].SetString(""); });
        variants.rejects("sprayer.name", [](rapidjson::Document& d)
            { choice(d, "sprayer")["name"].SetString(std::string(100, 'W').c_str(), d.GetAllocator()); }, &world);
        for (const double width : { 0.0, -1.0, 1e100 })
        {
            variants.rejects("x_offset", [width](rapidjson::Document& d)
                { d["main_menu"]["weapon_description"]["x_offset"].SetDouble(width); },
                &world, "main_menu.weapon_description");
        }
        const auto baseline = variants.load([](rapidjson::Document&) {});
        const float exact_width = world.render.measure_text(world.render.resolve_sprite_font(
            baseline.main_menu().weapon_description_font), L"Name").x + std::abs(baseline.menu().detail_shadow_offset.x);
        const auto fits_exactly = variants.load([exact_width](rapidjson::Document& d)
        {
            d["main_menu"]["weapon_description"]["x_offset"].SetFloat(exact_width);
            for (const char* key : keys) { choice(d, key)["name"].SetString("Name"); }
        });
        fits_exactly.validate_resources(world.render, world.audio);
        variants.rejects("sprayer.name", [exact_width](rapidjson::Document& d)
        {
            d["main_menu"]["weapon_description"]["x_offset"].SetFloat(std::nextafter(exact_width, 0.0f));
            for (const char* key : keys) { choice(d, key)["name"].SetString("Name"); }
        }, &world);

        const SpriteFontFile font_file = read_sprite_font_file(
            (content / "fonts/lato_black_36.spritefont").string());
        auto glyphs = font_file.glyphs;
        const auto removed = std::remove_if(glyphs.begin(), glyphs.end(),
            [](const Glyph& glyph) { return glyph.character == U'~'; });
        require(removed != glyphs.end(), "Subset-font fixture requires the shipped tilde glyph");
        glyphs.erase(removed, glyphs.end());
        auto limited = std::make_unique<Font>(TextureHandle{}, std::move(glyphs), font_file.line_spacing);
        limited->set_stand_in(U'?');
        world.render.add_font("catalog_limited_font", std::move(limited));
        variants.rejects("sprayer.name", [](rapidjson::Document& d)
        {
            d["main_menu"]["fonts"]["weapon_description"].SetString("catalog_limited_font");
            choice(d, "sprayer")["name"].SetString("Catalog~");
        }, &world);
    }

    std::unique_ptr<SpriteSheet> make_catalog_sheet(const Vector2F& body_origin = Vector2F::ZERO,
        const Vector2F& paint_origin = Vector2F::ZERO, bool wrong_paint_size = false,
        int supply_count = 2, bool wrong_supply_size = false, int body_width = 40, int body_height = 24)
    {
        NameTable<SpriteFrame> frames("sprite frame");
        frames.add("catalog_fill", SpriteFrame(RectangleI(0, 0, 1, 1)));
        frames.add("catalog_body", SpriteFrame(RectangleI(0, 4, body_width, body_height), body_origin));
        frames.add("catalog_paint", SpriteFrame(RectangleI(50, 4, wrong_paint_size ? 41 : 40, 24), paint_origin));
        NameTable<AnimationStrip> strips("animation strip");
        strips.add("catalog_supply", AnimationStrip(RectangleI(0, 40, wrong_supply_size ? 41 : 40, 24),
            supply_count, 1.0f, false));
        return std::make_unique<SpriteSheet>(TextureHandle{}, std::move(frames), std::move(strips));
    }

    void custom_catalog(rapidjson::Document& d, bool use_supply = true)
    {
        catalog(d)["sheet"].SetString("catalog_atlas");
        catalog(d)["fill_frame"].SetString("catalog_fill");
        for (const char* key : keys)
        {
            auto& entry = choice(d, key);
            entry["name"].SetString((std::string("Alt ") + key).c_str(), d.GetAllocator());
            entry["frame"].SetString("catalog_body");
            entry["paint_frame"].SetString("catalog_paint");
            entry["supply_strip"].SetString(use_supply ? "catalog_supply" : "", d.GetAllocator());
        }
    }

    void check_sparse_catalog(Variants& variants, const std::filesystem::path& content)
    {
        World world(content, false); // No legacy weapon atlas or error frame.
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet());
        const auto custom = variants.load([](rapidjson::Document& d) { custom_catalog(d); });
        custom.validate_resources(world.render, world.audio);
        const auto& art = custom.main_menu().weapon_select_art;
        require(art.sheet == "catalog_atlas" && art.fill_frame == "catalog_fill", "Custom catalog atlas/fill was lost");
        for (size_t index = 0; index < std::size(keys); ++index)
        {
            const std::string label = std::string("Alt ") + keys[index];
            const auto& entry = art.choice(types[index]);
            require(entry.name == std::wstring(label.begin(), label.end()) && entry.frame == "catalog_body" &&
                entry.paint_frame == "catalog_paint" && entry.supply_strip == "catalog_supply",
                "A custom catalog choice lost its authored name or artwork bindings");
        }

        // Only the authored body/paint/fill/strip names exist. Construction,
        // selection updates and hidden draws must not request pixel or error.
        RenderResources sparse;
        sparse.add_sprite_sheet("catalog_atlas", make_catalog_sheet());
        const RectangleF bounds(100.0f, 120.0f, 120.0f, 64.0f);
        MenuIcon icon("catalog_icon", art.choice(WeaponType::sprayer).frame, bounds, &sparse,
            Colour::white, &custom.menu(), art.sheet, art.fill_frame);
        icon.set_hidden(true);
        DrawList no_device;
        for (const WeaponType type : types)
        {
            const auto& entry = art.choice(type);
            icon.set_sprite_frame(entry.frame, entry.paint_frame, entry.supply_strip);
            icon.set_paint_colour(Colour::blue);
            icon.draw(no_device);
            require(icon.bounds() == bounds, "Catalog choice changed the icon's authored widget bounds");
        }

        const auto rejects_asset = [&](const char* key, const char* field)
        {
            variants.rejects(std::string(key) + '.' + field, [key, field](rapidjson::Document& d)
            {
                custom_catalog(d);
                choice(d, key)[field].SetString("missing_catalog_asset");
            }, &world);
        };
        for (const char* key : keys)
        {
            for (const char* field : { "frame", "paint_frame", "supply_strip" }) { rejects_asset(key, field); }
        }
        variants.rejects("fill_frame", [](rapidjson::Document& d)
        {
            custom_catalog(d);
            catalog(d)["fill_frame"].SetString("missing_catalog_fill");
        }, &world);
        variants.rejects("fill_frame", [](rapidjson::Document& d)
        {
            custom_catalog(d);
            catalog(d)["sheet"].SetString("missing_catalog_sheet");
        }, &world);

        const auto no_overlays = variants.load([](rapidjson::Document& d)
        {
            custom_catalog(d, false);
            for (const char* key : keys) { choice(d, key)["paint_frame"].SetString(""); }
        });
        no_overlays.validate_resources(world.render, world.audio);
        const auto geometry_rejects = [&](const char* field)
        {
            variants.rejects(std::string("sprayer.") + field,
                [](rapidjson::Document& d) { custom_catalog(d); }, &world);
        };
        for (const Vector2I size : { Vector2I(0, 24), Vector2I(-1, 24), Vector2I(40, 0), Vector2I(40, -1) })
        {
            world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {}, false, 2, false, size.x, size.y));
            geometry_rejects("frame");
        }
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {}, true));
        geometry_rejects("paint_frame");
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {1.0f, 2.0f}));
        geometry_rejects("paint_frame");
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {}, false, 0));
        geometry_rejects("supply_strip");
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {}, false, 2, true));
        geometry_rejects("supply_strip");
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({1.0f, 2.0f}, {1.0f, 2.0f}));
        geometry_rejects("supply_strip");
        const auto no_supply = variants.load([](rapidjson::Document& d) { custom_catalog(d, false); });
        no_supply.validate_resources(world.render, world.audio); // Matching nonzero origins are valid without a strip.
        world.render.add_sprite_sheet("catalog_atlas", make_catalog_sheet({}, {}, false, 1));
        custom.validate_resources(world.render, world.audio); // A strip displaying one final full frame is valid.
    }

    void check_no_legacy_error(Variants& variants, World& world, const std::filesystem::path& content)
    {
        auto sheet = read_document(content / "textures/weapon_paint.json");
        auto& frames = sheet["sprite_frames"];
        bool removed = false;
        for (auto frame = frames.Begin(); frame != frames.End(); ++frame)
        {
            if (std::string((*frame)["name"].GetString()) == "error")
            {
                frames.Erase(frame);
                removed = true;
                break;
            }
        }
        require(removed, "Legacy error-frame fixture disappeared from the authored weapon atlas");
        const auto path = variants.scratch / "weapon-sheet-without-error.json";
        write_document(sheet, path);
        world.render.add_sprite_sheet("weapon_paint", read_sprite_sheet(path.string().c_str(), TextureHandle{}));
        const auto defaults = variants.load([](rapidjson::Document&) {});
        defaults.validate_resources(world.render, world.audio);
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("weapon catalog", [&]
    {
        require(argc == 2, "Usage: weapon_catalog.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Variants variants(content, std::filesystem::absolute(argv[0]).parent_path() / "weapon-catalog-fixtures");
        World world(content);
        const Presentation defaults((content / "presentation.json").string().c_str());
        check_defaults(defaults);
        defaults.validate_resources(world.render, world.audio);
        check_keyed_choices(variants, defaults);
        check_names(variants, world, content);
        check_sparse_catalog(variants, content);
        check_no_legacy_error(variants, world, content);
        std::cout << "PASS weapon catalog: six keyed defaults, normalized names, enum/content/resource guards, "
            "custom sparse icons, overlay/strip geometry and optional legacy error frame.\n";
        return 0;
    });
}
