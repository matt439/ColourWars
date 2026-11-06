#include "game/pch.h"
#include "game/input/input_slots.h"
#include "game/contracts/simulation.h"
#include "game/objects/level_builder.h"
#include "game/states/pause_menu.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// POL#19: untimed solo practice on the Movement Course. The lessons the
// stations carry, the builder's choice of who gets them, the clock that never
// runs out, the lesson that follows the player, the card's layout and the
// content rules that reject a bad lesson. Real objects, no device or save.

using namespace labrador;

namespace
{
    using acceptance::require;

    const float dt = SECONDS_PER_STEP;

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
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            acceptance::load_cpu_assets(content, this->render, this->audio);
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
                OpeningMode::off, MatchRandom(), practice);
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

    float painted(const Level& level)
    {
        const LevelEndInfo info = level.level_end_info();
        return info.team_a_area + info.team_b_area;
    }

    // S2: the Tool row's rebuild as GameLevel's change_tool handler runs it
    // (game_states.cpp): the old level's tool_change_place, a fresh build
    // from settings holding the chosen tool, then enter_after_tool_change.
    void check_tool_change_start(World& world)
    {
        const int lesson = lesson_index(*world.build(true), "long_gap");
        const Vector2F stood = world.course().stations()[static_cast<size_t>(lesson)].position;
        for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket })
        {
            const std::string name(weapon_name(tool));
            std::unique_ptr<Level> old = world.build(true);
            step(*old, world.tuning.match().countdown + 0.1f);
            require(old->place_player(old->participant_for_view(0), stood), name + ": goto long_gap");
            step(*old, 1.0f); // settled where they stand
            const std::optional<Vector2F> place = old->tool_change_place();
            require(place.has_value(), name + ": a living player has no place to return to");
            const Vector2F before = *place;

            std::unique_ptr<Level> level = world.build(true, 1, 1.0f, tool);
            require(level->state() == LevelState::start_countdown,
                name + ": the rebuilt course starts in its countdown");
            level->enter_after_tool_change(place);
            step(*level, dt);
            const int rebuilt = level->participant_for_view(0);
            require(level->state() == LevelState::active && level->match_started(),
                name + ": a tool change goes straight to GO");
            require(level->player(rebuilt).primary_weapon().type() == tool,
                name + ": the rebuilt player holds the chosen tool");
            require(painted(*level) == 0.0f, name + ": the rebuilt course is not clean");
            const Vector2F back = level->player(rebuilt).rectangle().position();
            require(std::abs(back.x - before.x) < 0.5f && std::abs(back.y - before.y) < 0.5f,
                name + ": the player is not back where they stood");
            require(level->active_lesson() == lesson, name + ": the lesson where they stood shows");
        }

        // A dead player has no place: the rebuilt course keeps the spawn.
        std::unique_ptr<Level> old = world.build(true);
        step(*old, world.tuning.match().countdown + 0.1f);
        require(old->place_player(old->participant_for_view(0), stood), "goto long_gap");
        require(old->kill_player(old->participant_for_view(0)), "kill");
        step(*old, dt);
        require(old->player(old->participant_for_view(0)).state() != PlayerState::alive,
            "The killed player is still alive");
        const std::optional<Vector2F> place = old->tool_change_place();
        require(!place.has_value(), "A dead player returns to where they died");
        std::unique_ptr<Level> fresh = world.build(true);
        fresh->start_without_countdown();
        step(*fresh, dt);
        const Vector2F spawn = fresh->player(fresh->participant_for_view(0)).rectangle().position();
        std::unique_ptr<Level> level = world.build(true, 1, 1.0f, WeaponType::roller);
        level->enter_after_tool_change(place);
        step(*level, dt);
        const int rebuilt = level->participant_for_view(0);
        const Vector2F back = level->player(rebuilt).rectangle().position();
        require(level->state() == LevelState::active && level->match_started(),
            "After a death the tool change still goes straight to GO");
        require(level->player(rebuilt).state() == PlayerState::alive &&
            std::abs(back.x - spawn.x) < 0.5f && std::abs(back.y - spawn.y) < 0.5f,
            "After a death the rebuilt player does not start at the spawn");
        require(level->active_lesson() == lesson_index(*level, "workshop"),
            "After a death the spawn's lesson does not show");
    }

    // S2: PauseMenuInitial::cycle_tool's step over the five real tools, both
    // ways, wrapping at each end and never landing on Random.
    void check_tool_row_wraps()
    {
        constexpr std::array<WeaponType, 5> tools{ WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket };
        for (size_t index = 0; index < tools.size(); ++index)
        {
            const std::string name(weapon_name(tools[index]));
            require(practice_tool_after(tools[index], Direction::right) ==
                tools[(index + 1) % tools.size()], name + ": right is not the next tool");
            require(practice_tool_after(tools[index], Direction::left) ==
                tools[(index + tools.size() - 1) % tools.size()], name + ": left is not the previous tool");
        }
        for (const Direction direction : { Direction::right, Direction::left })
        {
            WeaponType tool = WeaponType::sprayer;
            for (int press = 0; press < 5; ++press)
            {
                tool = practice_tool_after(tool, direction);
                require(tool != WeaponType::random_primary, "The Tool row offers Random");
            }
            require(tool == WeaponType::sprayer, "Five presses one way do not come back to the Sprayer");
        }
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
        const acceptance::JsonVariants variants(content / "levels/movement_course.json", L"practice-lessons");
        const auto rejects = [&](const char* phrase, const auto& edit)
        {
            acceptance::rejects_variant(variants,
                [&](rapidjson::Document& json) { edit(json["stations"][0]["lesson"], json.GetAllocator()); },
                [](const std::filesystem::path& path) { LevelLoadedInfo(path.string().c_str()).validate(); },
                { phrase }, std::string("A bad lesson was accepted or refused for another reason than: ") + phrase);
        };
        rejects("leaves the camera bounds", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["x"].SetFloat(-50.0f); });
        rejects("does not contain station", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["x"].SetFloat(3000.0f); });
        rejects("positive width and height", [](rapidjson::Value& lesson, auto&)
            { lesson["area"]["width"].SetFloat(0.0f); });
        rejects("is empty", [](rapidjson::Value& lesson, auto& allocator)
            { lesson["title"].SetString("  ", allocator); });
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
        check_tool_row_wraps();
        check_tool_label(world);
        check_rejected_lessons(content);
        std::cout << "PASS practice workshop\n";
        return 0;
    });
}
