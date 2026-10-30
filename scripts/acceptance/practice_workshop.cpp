#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// POL#19: untimed solo practice on the Movement Course. The lessons the
// stations carry, the builder's choice of who gets them, the clock that never
// runs out, the lesson that follows the player, the card's layout and the
// content rules that reject a bad lesson. Real objects, no device or save.

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    PlayerInputData input()
    {
        PlayerInputData result;
        result.connected = true;
        return result;
    }

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
            this->resolution.set_resolution(Vector2F(1920.0f, 1080.0f));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        const LevelLoadedInfo& course() const { return *this->levels.get("movement_course"); }

        std::unique_ptr<Level> build(bool practice, int humans = 1, float duration = 1.0f,
            WeaponType tool = WeaponType::sprayer)
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(humans);
            settings.set_screen_layout(humans == 1 ? ScreenLayout::one_player : ScreenLayout::two_player);
            settings.set_duration(duration);
            settings.begin_input_claims(0);
            for (int slot = 1; slot < humans; ++slot)
            {
                require(settings.claim_input_slot(slot), "Could not claim a CPU pad slot");
            }
            for (int index = 0; index < humans; ++index)
            {
                settings.set_player_team(index, PlayerTeam::a);
                settings.set_player_weapon(index, tool);
            }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange,
                OpeningMode::off, practice);
        }
    };

    void step(Level& level, float seconds)
    {
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        for (int tick = 0; tick < static_cast<int>(std::ceil(seconds / dt)); ++tick)
        {
            level.update(inputs, dt);
        }
    }

    int lesson_index(const Level& level, const std::string& station)
    {
        for (int index = 0; index < level.lesson_count(); ++index)
        {
            if (level.lesson(index).station == station) { return index; }
        }
        throw std::runtime_error("no lesson at station " + station);
    }

    // Every station teaches, `goto` lands in its own lesson, and the areas
    // only touch, so the card the player sees never depends on file order.
    void check_authored_lessons(const World& world)
    {
        const LevelLoadedInfo& course = world.course();
        const std::vector<LevelLesson> lessons = course.lessons();
        const std::vector<LevelStation> stations = course.stations();
        require(lessons.size() == 10 && stations.size() == 12,
            "The course's ten stations each carry a lesson; the spawns do not");
        const RectangleF camera = course.camera_bounds_rectangle();
        for (size_t index = 0; index < lessons.size(); ++index)
        {
            const LevelLesson& lesson = lessons[index];
            require(lesson.station == stations[index].name, "Lessons follow station order");
            const RectangleF body(stations[index].position, world.tuning.player().size);
            const Vector2F centre = body.center();
            require(lesson.area.left() <= centre.x && centre.x <= lesson.area.right() &&
                lesson.area.top() <= centre.y && centre.y <= lesson.area.bottom(),
                lesson.station + ": a player at the station stands in its lesson");
            require(camera.contains(lesson.area), lesson.station + ": the area is inside the camera bounds");
            require(!lesson.title.empty() && !lesson.text.empty(), lesson.station + ": title and text");
            for (size_t other = index + 1; other < lessons.size(); ++other)
            {
                const RectangleF& a = lesson.area;
                const RectangleF& b = lessons[other].area;
                const float width = std::min(a.right(), b.right()) - std::max(a.left(), b.left());
                const float height = std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top());
                require(width <= 0.0f || height <= 0.0f,
                    lesson.station + " and " + lessons[other].station + " overlap");
            }
        }
        // The main route, from the left spawn to the right runway, is taught
        // end to end: every point along it at standing height is in a lesson.
        const float standing = 1000.0f - world.tuning.player().size.y / 2.0f;
        for (float x = camera.left() + 1.0f; x < camera.right(); x += 10.0f)
        {
            bool taught = false;
            for (const LevelLesson& lesson : lessons)
            {
                taught = taught || (lesson.area.left() <= x && x <= lesson.area.right() &&
                    lesson.area.top() <= standing && standing <= lesson.area.bottom());
            }
            require(taught, "x=" + std::to_string(x) + " along the course has no lesson");
        }
    }

    void check_practice_level(World& world)
    {
        std::unique_ptr<Level> level = world.build(true);
        require(level->practice() && level->lesson_count() == 10, "Solo practice gets the lessons");
        require(level->active_lesson() == -1, "No lesson before play");
        step(*level, world.tuning.match().countdown + 0.1f);
        require(level->state() == LevelState::active, "Practice starts after the ordinary countdown");
        require(level->active_lesson() == lesson_index(*level, "workshop"),
            "The left spawn stands in the workshop lesson");

        // Three times a one-second duration: the clock never runs out.
        step(*level, 3.0f);
        require(level->state() == LevelState::active, "Practice has no clock to run out");

        const RectangleF card = level->lesson_card_bounds();
        const Vector2F screen = world.resolution.resolution_vec();
        require(card.width > 0.0f && card.height > 0.0f && card.left() >= 0.0f &&
            card.right() <= screen.x && card.bottom() <= screen.y && card.top() > screen.y / 2.0f,
            "The lesson card sits in the lower half of the window");

        for (const LevelStation& station : world.course().stations())
        {
            if (station.name == "spawn_a" || station.name == "spawn_b") { continue; }
            require(level->place_player(0, station.position), "goto " + station.name);
            step(*level, dt);
            require(level->active_lesson() == lesson_index(*level, station.name),
                "At " + station.name + " its own lesson shows");
            const RectangleF shown = level->lesson_card_bounds();
            require(shown.width > 0.0f && shown.left() >= 0.0f && shown.right() <= screen.x &&
                shown.bottom() <= screen.y, station.name + "'s card fits the window");
        }
        require(level->kill_player(0), "kill");
        step(*level, dt);
        require(level->active_lesson() == -1 && level->lesson_card_bounds().width == 0.0f,
            "A dead player sees no lesson");
    }

    void check_who_gets_practice(World& world)
    {
        std::unique_ptr<Level> timed = world.build(false);
        require(!timed->practice() && timed->lesson_count() == 0, "A timed build has no lessons");
        step(*timed, world.tuning.match().countdown + 1.5f);
        require(timed->state() != LevelState::active, "A timed build still runs out");

        std::unique_ptr<Level> pair = world.build(true, 2);
        require(!pair->practice() && pair->lesson_count() == 0,
            "Two humans are a match, never the solo workshop");
    }

    void check_tool_change_start(World& world)
    {
        std::unique_ptr<Level> level = world.build(true, 1, 1.0f, WeaponType::bucket);
        level->start_without_countdown();
        step(*level, dt);
        require(level->state() == LevelState::active && level->match_started(),
            "A tool change goes straight to GO");
        require(level->player(0).primary_weapon().type() == WeaponType::bucket,
            "The rebuilt player holds the chosen tool");
    }

    void check_tool_label(const World& world)
    {
        const MenuText& text = world.presentation.menu_text();
        const std::wstring label = text.practice_tool_label(L"Sprayer");
        require(label.find(L"Sprayer") != std::wstring::npos && label.find(L"{tool}") == std::wstring::npos,
            "The Tool row names the tool");
    }

    // A bad lesson is a content bug that fails at startup.
    void check_rejected_lessons(const std::filesystem::path& content)
    {
        std::string source;
        {
            std::ifstream stream(content / "levels/movement_course.json", std::ios::binary);
            source.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }
        const std::filesystem::path path = std::filesystem::temp_directory_path() /
            L"colourwars_practice_lessons.json";
        const auto rejects = [&](const char* phrase, const auto& edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse the course");
            edit(json["stations"][0]["lesson"], json.GetAllocator());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            {
                std::ofstream stream(path, std::ios::binary | std::ios::trunc);
                stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            }
            try
            {
                LevelLoadedInfo(path.string().c_str()).validate();
            }
            catch (const std::exception& error)
            {
                require(std::string(error.what()).find(phrase) != std::string::npos,
                    std::string("A bad lesson failed for another reason: ") + error.what());
                return;
            }
            throw std::runtime_error(std::string("A bad lesson was accepted: ") + phrase);
        };
        rejects("leaves the camera bounds", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["x"].SetFloat(-50.0f); });
        rejects("does not contain station", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["x"].SetFloat(3000.0f); });
        rejects("positive width and height", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["width"].SetFloat(0.0f); });
        rejects("is empty", [](rapidjson::Value& lesson, auto& allocator)
            { lesson["title"].SetString("  ", allocator); });
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("practice workshop", [&]
    {
        require(argc == 2, "Usage: practice_workshop.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_authored_lessons(world);
        check_practice_level(world);
        check_who_gets_practice(world);
        check_tool_change_start(world);
        check_tool_label(world);
        check_rejected_lessons(content);
        std::cout << "PASS practice workshop\n";
        return 0;
    });
}
