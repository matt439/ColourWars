// REV#1: a player whose box leaves the stage's out_of_bounds rectangle is
// eliminated with nobody credited and respawns at its spawn, instead of the
// match ending with a failure report. Everything here runs a real Level built
// from movement_course against CPU-only assets: no graphics or audio device,
// window or player save.
#include "game/pch.h"
#include "game/input/input_slots.h"
#include "game/contracts/simulation.h"
#include "game/objects/level_builder.h"
#include "game/config/presentation/presentation.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/slot_seats.h"

#include <filesystem>
#include <iostream>
#include <tuple>
#include <vector>

using namespace labrador;

namespace
{
    using acceptance::require;

    const float dt = simulation::seconds_per_step;

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
        RectangleF bounds;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            acceptance::load_cpu_assets(content, this->render, this->audio);
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->bounds = info->out_of_bounds_rectangle();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        // Two players on opposite teams; the match stays active throughout.
        std::unique_ptr<Level> build()
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(2);
            settings.set_screen_layout(ScreenLayout::two_player);
            settings.set_duration(300.0f);
            settings.set_player_team(0, PlayerTeam::a);
            settings.set_player_team(1, PlayerTeam::b);
            settings.set_player_weapon(0, WeaponType::sprayer);
            settings.set_player_weapon(1, WeaponType::sprayer);
            settings.begin_input_claims(0);
            require(settings.claim_input_slot(1), "Cannot claim the second seat's input");
            this->viewports.set_layout(settings.screen_layout());
            auto level = this->builder->build_level(settings, TeamColourPair::blue_orange,
                OpeningMode::off, MatchRandom());
            const std::vector<PlayerInputData> neutral(input_slots::count);
            for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick)
            {
                level->update(acceptance::slot_seats(*level, neutral), dt);
            }
            require(level->state() == LevelState::active, "Out-of-bounds fixture did not reach active play");
            return level;
        }

        // Well below the bounds, so the whole box is outside them.
        Vector2F outside() const
        {
            return { this->bounds.x + this->bounds.width / 2.0f,
                this->bounds.y + this->bounds.height + 4.0f * this->tuning.player().size.y };
        }
    };

    void step(Level& level)
    {
        const std::vector<PlayerInputData> neutral(input_slots::count);
        level.update(acceptance::slot_seats(level, neutral), dt);
    }

    bool no_splats(const Level& level)
    {
        for (std::uint64_t count : level.level_end_info().splats) { if (count != 0) { return false; } }
        return true;
    }

    // Everything a tick of the out-of-bounds sequence is observed by.
    using Record = std::tuple<RectangleF, PlayerState, float, float, RectangleF, PlayerState, float>;
    Record observe(const Level& level)
    {
        const Player& left = level.player(0);
        const Player& other = level.player(1);
        return { left.rectangle(), left.state(), left.health(), left.respawn_timer(),
            other.rectangle(), other.state(), other.health() };
    }

    // Leaves the bounds, dies, waits out the respawn and returns to the spawn.
    std::vector<Record> leave_and_return(World& world, bool check)
    {
        std::vector<Record> trace;
        auto level = world.build();
        const float starting = world.tuning.player().starting_health;
        const Player& left = level->player(0);
        const Player& other = level->player(1);
        const RectangleF spawn = left.spawn_rectangle();
        require(world.bounds.intersects(spawn), "Fixture spawn is not inside the stage's bounds");

        require(level->place_player(0, world.outside()), "Cannot place the fixture player outside the bounds");
        require(!world.bounds.intersects(left.rectangle()), "Fixture placement is not wholly outside the bounds");
        step(*level); // Used to throw PlayerOutOfBounds here.
        trace.push_back(observe(*level));
        if (check)
        {
            require(left.state() == PlayerState::alive && left.health() == 0.0f,
                "Leaving the bounds did not zero the player's health on the same tick");
            std::cout << "PASS out of bounds: leaving the bounds zeroes health instead of ending the match\n";
        }

        step(*level);
        trace.push_back(observe(*level));
        if (check)
        {
            require(left.state() == PlayerState::dead &&
                left.animation_state() == PlayerAnimationState::elimination,
                "Out-of-bounds player did not take the normal elimination on its next update");
            require(no_splats(*level) && left.lethal_projectiles() == PlayerLethalProjectiles{} &&
                other.lethal_projectiles() == PlayerLethalProjectiles{},
                "An out-of-bounds elimination credited a splat or counted an uncredited projectile");
            std::cout << "PASS out of bounds: the next update is the normal elimination, credited to nobody\n";
        }

        // Dead outside the bounds: it is not killed again, so the respawn timer
        // counts down once, and the respawn puts it back at its own spawn.
        float timer = left.respawn_timer();
        int ticks = 0;
        const int limit = static_cast<int>(world.tuning.player().respawn_delay / dt) + 10;
        while (left.state() == PlayerState::dead && ticks < limit)
        {
            if (check)
            {
                require(!world.bounds.intersects(left.rectangle()), "A dead player moved back into the bounds");
            }
            step(*level);
            trace.push_back(observe(*level));
            ++ticks;
            if (check && left.state() == PlayerState::dead)
            {
                require(left.respawn_timer() < timer, "The dead player's respawn timer did not count down");
                timer = left.respawn_timer();
            }
        }
        if (check)
        {
            require(left.state() == PlayerState::alive, "Out-of-bounds player never respawned");
            require(ticks >= static_cast<int>(world.tuning.player().respawn_delay / dt) - 1,
                "Out-of-bounds player respawned before the normal respawn delay");
            require(left.rectangle().top_left() == spawn.top_left() && left.health() == starting,
                "Out-of-bounds player did not respawn at its spawn with full health");
            require(no_splats(*level) && left.lethal_projectiles() == PlayerLethalProjectiles{},
                "The respawn gave an out-of-bounds elimination credit");
            std::cout << "PASS out of bounds: dead player stays down for the respawn delay, then respawns at its spawn\n";
        }

        // The respawned player plays on; the other seat never noticed.
        for (int tick = 0; tick < 30; ++tick) { step(*level); trace.push_back(observe(*level)); }
        if (check)
        {
            require(left.state() == PlayerState::alive && left.health() > 0.0f,
                "Respawned player was eliminated again inside the bounds");
            for (const Record& record : trace)
            {
                require(std::get<5>(record) == PlayerState::alive && std::get<6>(record) == starting,
                    "The other player was affected by an out-of-bounds elimination");
            }
            require(level->state() == LevelState::active, "The match did not carry on");
            std::cout << "PASS out of bounds: the match and the other seat carry on\n";
        }
        return trace;
    }

    void edge_is_inside(World& world)
    {
        auto level = world.build();
        const Player& left = level->player(0);
        const Vector2F size = world.tuning.player().size;
        // Half the box past the left edge, at the first spawn's height.
        const Vector2F edge{ world.bounds.x - size.x / 2.0f, left.spawn_rectangle().y };
        require(level->place_player(0, edge), "Cannot place the fixture player at the edge");
        const float health = left.health();
        for (int tick = 0; tick < 10; ++tick)
        {
            step(*level);
            require(world.bounds.intersects(left.rectangle()), "Edge fixture left the bounds while being checked");
            require(left.state() == PlayerState::alive && left.health() == health,
                "A player whose box still touches the bounds was eliminated");
        }
        std::cout << "PASS out of bounds: a box partly inside the bounds is still in play\n";
    }

    void deterministic(World& world)
    {
        require(leave_and_return(world, false) == leave_and_return(world, false),
            "Two identical out-of-bounds sequences diverged");
        std::cout << "PASS out of bounds: the sequence replays identically\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("out of bounds", [&]
    {
        require(argc == 2, "Usage: out_of_bounds.exe <game/content directory>");
        World world{ std::filesystem::path(argv[1]) };
        leave_and_return(world, true);
        edge_is_inside(world);
        deterministic(world);
        return 0;
    });
}
