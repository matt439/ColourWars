#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// POL#44: the POL#39 marquee around the mouse pointer in the actual Level.
// Every visibility condition (presentation switch, focus, each window edge,
// who holds the keyboard/mouse seat), whole-pixel frames with an open centre
// at 720, 1080 and 1440 lines, and every presentation refusal. No device.

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    // A seat: a pad, the shared keyboard/mouse, or a bot (never P1).
    enum class Seat { pad, keyboard, bot };

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(font) + ".spritefont")).string());
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                this->render.add_font(font, std::move(loaded));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner, &this->mouse);
        }

        CursorMarkerPresentation& look()
        {
            return const_cast<CursorMarkerPresentation&>(this->presentation.hud().cursor_marker);
        }

        std::unique_ptr<Level> build(const std::vector<Seat>& plan)
        {
            require(!plan.empty() && plan[0] != Seat::bot, "P1 cannot be a bot");
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(static_cast<int>(plan.size()));
            settings.set_duration(300.0f);
            for (std::size_t seat = 1; seat < plan.size(); ++seat)
            {
                if (plan[seat] == Seat::bot)
                {
                    settings.set_player_bot(static_cast<int>(seat), BotDifficulty::normal);
                }
            }
            // Pads claim sparse slots, so no seat's slot is its roster index.
            int next_pad = input_slots::pad_count - 1;
            const auto slot = [&](Seat seat)
            {
                return seat == Seat::keyboard ? input_slots::keyboard_and_mouse : next_pad--;
            };
            settings.begin_input_claims(slot(plan[0]));
            for (std::size_t seat = 1; seat < plan.size(); ++seat)
            {
                if (plan[seat] != Seat::bot)
                {
                    require(settings.claim_input_slot(slot(plan[seat])), "Could not claim a cursor seat");
                }
            }
            require(settings.input_roster_complete(), "Cursor roster is incomplete");
            settings.set_screen_layout(static_cast<ScreenLayout>(settings.human_count() - 1));
            for (std::size_t seat = 0; seat < plan.size(); ++seat)
            {
                settings.set_player_team(static_cast<int>(seat), seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(static_cast<int>(seat), WeaponType::sprayer);
            }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }

        // The pointer as the window would report it, after the frame boundary.
        void point(bool focused, const Vector2I& at)
        {
            this->mouse.set_focused(focused);
            this->mouse.on_move(at);
            this->mouse.poll();
        }
    };

    std::string describe(const std::vector<Seat>& plan)
    {
        std::string result;
        for (const Seat seat : plan)
        {
            result += seat == Seat::pad ? 'P' : seat == Seat::keyboard ? 'K' : 'B';
        }
        return result;
    }

    int checked_visibility = 0;

    void expect(const Level& level, bool visible, const std::string& where)
    {
        require(level.cursor_marker_visible() == visible,
            where + (visible ? ": the marquee was hidden" : ": the marquee was shown"));
        ++checked_visibility;
    }

    void check_visibility(World& world)
    {
        for (const ScreenResolution size : { ScreenResolution::s_1280_720, ScreenResolution::s_1920_1080,
            ScreenResolution::s_2560_1440 })
        {
            world.resolution.set_resolution(size);
            const std::string at = world.resolution.resolution_string();
            const Vector2F window = world.resolution.resolution_vec();
            const int w = static_cast<int>(window.x);
            const int h = static_cast<int>(window.y);
            const Vector2I middle(w / 2, h / 2);

            // A keyboard/mouse human in each participant slot, beside pads and bots.
            for (const std::vector<Seat>& plan : std::vector<std::vector<Seat>>{
                { Seat::keyboard },
                { Seat::keyboard, Seat::bot, Seat::pad, Seat::bot },
                { Seat::pad, Seat::keyboard, Seat::bot, Seat::pad },
                { Seat::pad, Seat::bot, Seat::keyboard, Seat::bot },
                { Seat::pad, Seat::pad, Seat::bot, Seat::keyboard },
                { Seat::pad, Seat::bot, Seat::bot, Seat::keyboard } })
            {
                const std::string where = at + " " + describe(plan);
                auto level = world.build(plan);
                world.point(true, middle);
                expect(*level, true, where + " centred");
                // The window's own corner pixels are inside.
                for (const Vector2I inside : { Vector2I(0, 0), Vector2I(w - 1, 0), Vector2I(0, h - 1), Vector2I(w - 1, h - 1) })
                {
                    world.point(true, inside);
                    expect(*level, true, where + " at a corner pixel");
                }
                // One pixel past each edge is outside.
                for (const Vector2I outside : { Vector2I(-1, middle.y), Vector2I(w, middle.y),
                    Vector2I(middle.x, -1), Vector2I(middle.x, h) })
                {
                    world.point(true, outside);
                    expect(*level, false, where + " past an edge");
                }
                world.point(false, middle);
                expect(*level, false, where + " unfocused");
                world.point(true, middle);
                world.look().enabled = false;
                expect(*level, false, where + " disabled");
                world.look().enabled = true;
                expect(*level, true, where + " re-enabled");
            }

            // No keyboard/mouse human: only pads, or a pad with only bots.
            for (const std::vector<Seat>& plan : std::vector<std::vector<Seat>>{
                { Seat::pad },
                { Seat::pad, Seat::pad, Seat::pad, Seat::pad },
                { Seat::pad, Seat::bot, Seat::bot, Seat::bot },
                { Seat::pad, Seat::bot, Seat::pad } })
            {
                auto level = world.build(plan);
                world.point(true, middle);
                expect(*level, false, at + " " + describe(plan) + " without a keyboard/mouse seat");
            }
        }
        // A Level built with no mouse at all never shows the marquee.
        world.resolution.set_resolution(ScreenResolution::s_1920_1080);
        LevelBuilder mouseless(&world.viewports, &world.render, &world.audio, &world.preferences,
            &world.levels, &world.tuning, &world.presentation, &world.resolution, nullptr, &world.partitioner, nullptr);
        MatchSetup settings;
        settings.set_stage("movement_course");
        settings.set_player_count(1);
        settings.set_duration(300.0f);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        require(settings.input_roster_complete(), "Mouseless roster is incomplete");
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_weapon(0, WeaponType::sprayer);
        settings.set_screen_layout(ScreenLayout::one_player);
        world.viewports.set_layout(settings.screen_layout());
        auto level = mouseless.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        expect(*level, false, "no mouse");
        std::cout << "PASS cursor marker visibility: " << checked_visibility << " checks at 720/1080/1440 - "
            "keyboard/mouse in each participant slot, corners in, each edge out, unfocused, disabled, "
            "pads/bots only, no mouse\n";
    }

    bool whole(float value) { return std::round(value) == value; }

    int checked_frames = 0;

    // The three frames for `look` at `at` in a window `height` lines tall.
    void check_frames(const CursorMarkerPresentation& look, const Vector2F& at, float height, const std::string& where)
    {
        const std::array<CursorMarkerFrame, 3> frames = Level::cursor_marker_frames(look, at, height);
        require(frames[0].colour == look.ink && frames[1].colour == look.paper && frames[2].colour == look.ink,
            where + ": frames are not ink, paper, ink");
        for (std::size_t index = 0; index < frames.size(); ++index)
        {
            const CursorMarkerFrame& frame = frames[index];
            const RectangleF& box = frame.box;
            const std::string which = where + " frame " + std::to_string(index);
            require(whole(box.x) && whole(box.y) && whole(box.width) && whole(box.height) && whole(frame.width),
                which + ": an edge is not on a whole pixel");
            require(frame.width >= 1.0f, which + ": an edge is thinner than a pixel");
            require(box.width == box.height, which + ": frame is not square");
            require(box.x + box.width * 0.5f == at.x && box.y + box.height * 0.5f == at.y,
                which + ": frame is not centred on the hotspot");
            if (index > 0)
            {
                const CursorMarkerFrame& outer = frames[index - 1];
                require(box.x == outer.box.x + outer.width && box.y == outer.box.y + outer.width &&
                    box.right() == outer.box.right() - outer.width && box.bottom() == outer.box.bottom() - outer.width,
                    which + ": frame does not sit flush inside the one outside it");
            }
        }
        const CursorMarkerFrame& inner = frames[2];
        const RectangleF hole(inner.box.x + inner.width, inner.box.y + inner.width,
            inner.box.width - inner.width * 2.0f, inner.box.height - inner.width * 2.0f);
        require(hole.width >= 2.0f && hole.contains(RectangleF(at, Vector2F::ZERO)),
            where + ": the hollow centre closed over the hotspot");
        ++checked_frames;
    }

    void check_geometry(World& world)
    {
        const CursorMarkerPresentation shipped = world.look();
        // The largest looks the reader accepts, at each width bound.
        CursorMarkerPresentation wide_ink = shipped;
        wide_ink.size = 96.0f;
        wide_ink.ink_width = 8.0f;
        wide_ink.paper_width = 7.5f;
        CursorMarkerPresentation wide_paper = shipped;
        wide_paper.size = 96.0f;
        wide_paper.ink_width = 5.5f;
        wide_paper.paper_width = 8.0f;
        for (const auto& [look, name] : { std::pair{ shipped, std::string("shipped") },
            std::pair{ wide_ink, std::string("widest ink") }, std::pair{ wide_paper, std::string("widest paper") } })
        {
            for (const float height : { 720.0f, 1080.0f, 1440.0f })
            {
                for (const Vector2F at : { Vector2F(0.0f, 0.0f), Vector2F(640.0f, 360.0f), Vector2F(959.0f, 541.0f),
                    Vector2F(1.0f, 1439.0f), Vector2F(2559.0f, 7.0f) })
                {
                    check_frames(look, at, height, name + " at " + std::to_string(static_cast<int>(height)) + " lines");
                }
            }
        }
        // The shipped marquee's pixel sizes, so a rounding change shows.
        const std::array<CursorMarkerFrame, 3> at_1080 = Level::cursor_marker_frames(shipped, Vector2F(100.0f, 100.0f), 1080.0f);
        require(at_1080[0].box.width == std::round(shipped.size * 0.5f) * 2.0f &&
            at_1080[0].width == std::round(shipped.ink_width) && at_1080[1].width == std::round(shipped.paper_width),
            "The 1080-line marquee is not the presented size");
        std::cout << "PASS cursor marker frames: " << checked_frames << " marquees (shipped and widest accepted) at "
            "720/1080/1440 lines - whole pixels, square, centred, flush, centre open over the hotspot\n";
    }

    void check_validation(const std::filesystem::path& content, const std::filesystem::path& scratch)
    {
        std::ifstream input_file(content / "presentation.json");
        require(input_file.good(), "Cannot read cursor presentation");
        const std::string source(std::istreambuf_iterator<char>(input_file), {});
        const auto path = scratch / "cursor-marker-invalid.json";
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        using Edit = std::function<void(rapidjson::Value&, rapidjson::Document::AllocatorType&)>;
        // True when the variant loads; false when it is refused naming cursor_marker.
        const auto loads = [&](const Edit& edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse cursor presentation fixture");
            edit(json["hud"]["cursor_marker"], json.GetAllocator());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            { std::ofstream variant(path); variant << buffer.GetString(); require(variant.good(), "Cannot write cursor variant"); }
            try { const Presentation loaded(path.string().c_str()); return true; }
            catch (const std::exception& error)
            {
                require(std::string(error.what()).find("cursor_marker") != std::string::npos,
                    std::string("A cursor_marker refusal lost its section name: ") + error.what());
                return false;
            }
        };
        int rejected = 0;
        const auto reject = [&](const std::string& what, const Edit& edit)
        {
            require(!loads(edit), "Malformed cursor_marker was accepted: " + what);
            ++rejected;
        };
        const auto set = [](const char* field, float value)
        {
            return [field, value](rapidjson::Value& look, auto&) { look[field].SetFloat(value); };
        };
        for (const char* field : { "enabled", "ink", "paper", "size", "ink_width", "paper_width" })
        {
            reject(std::string("missing ") + field, [field](rapidjson::Value& look, auto&) { look.RemoveMember(field); });
        }
        for (const char* field : { "size", "ink_width", "paper_width" })
        {
            reject(std::string("text ") + field,
                [field](rapidjson::Value& look, auto& allocator) { look[field].SetString("small", allocator); });
            reject(std::string("zero ") + field, set(field, 0.0f));
            reject(std::string("negative ") + field, set(field, -1.0f));
        }
        reject("enabled as a number", [](rapidjson::Value& look, auto&) { look["enabled"].SetFloat(1.0f); });
        reject("size above 96", set("size", 96.5f));
        reject("ink_width above 8", [](rapidjson::Value& look, auto&)
            { look["size"].SetFloat(96.0f); look["ink_width"].SetFloat(8.5f); look["paper_width"].SetFloat(1.0f); });
        reject("paper_width above 8", [](rapidjson::Value& look, auto&)
            { look["size"].SetFloat(96.0f); look["ink_width"].SetFloat(1.0f); look["paper_width"].SetFloat(8.5f); });
        // (2 * ink + paper) * 2 must stay below half the size: 44 at 3/2 closes 16 of 22.
        reject("less than half open", set("paper_width", 6.0f));
        reject("exactly half open", [](rapidjson::Value& look, auto&)
            { look["size"].SetFloat(32.0f); look["ink_width"].SetFloat(2.0f); look["paper_width"].SetFloat(4.0f); });
        reject("translucent ink", [](rapidjson::Value& look, auto& allocator) { look["ink"].SetString("141414FE", allocator); });
        reject("translucent paper", [](rapidjson::Value& look, auto& allocator) { look["paper"].SetString("FFFFFF80", allocator); });
        reject("ink equal to paper", [](rapidjson::Value& look, auto& allocator) { look["paper"].SetString("141414", allocator); });
        reject("unreadable ink", [](rapidjson::Value& look, auto& allocator) { look["ink"].SetString("ink", allocator); });

        // The bounds themselves are accepted, so the refusals are not over-wide.
        require(loads([](rapidjson::Value&, auto&) {}), "The shipped cursor_marker was refused");
        require(loads(set("size", 96.0f)), "size 96 was refused");
        require(loads([](rapidjson::Value& look, auto&)
            { look["size"].SetFloat(96.0f); look["ink_width"].SetFloat(8.0f); look["paper_width"].SetFloat(7.5f); }),
            "ink_width 8 was refused");
        require(loads([](rapidjson::Value& look, auto&)
            { look["size"].SetFloat(96.0f); look["ink_width"].SetFloat(5.5f); look["paper_width"].SetFloat(8.0f); }),
            "paper_width 8 was refused");
        require(loads([](rapidjson::Value& look, auto&) { look["enabled"].SetBool(false); }), "enabled false was refused");
        std::cout << "PASS cursor marker content: " << rejected << " malformed cursor_marker variants refused by name, "
            "5 boundary variants accepted\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("cursor marker", [&]
    {
        // Unbuffered, so a crash still shows how far the case got.
        std::cout << std::unitbuf;
        require(argc == 2, "Usage: cursor_marker.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_visibility(world);
        check_geometry(world);
        check_validation(content, std::filesystem::absolute(argv[0]).parent_path());
        std::cout << "PASS cursor marker\n";
        return 0;
    });
}
