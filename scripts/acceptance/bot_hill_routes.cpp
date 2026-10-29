#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/support/probe.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <iostream>
#include <sstream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 1800;

    enum class Route { ramp_approach, summit_entrance, castle_interior };

    const char* route_name(Route route)
    {
        switch (route)
        {
        case Route::ramp_approach: return "ramp_approach";
        case Route::summit_entrance: return "summit_entrance";
        case Route::castle_interior: return "castle_interior";
        }
        throw std::runtime_error("Unknown bot hill route");
    }

    struct Trial
    {
        Route route;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct Ramp
    {
        Vector2F toe;
        Vector2F crest;
        PlayerMoveState state;

        bool contact(const Player& player) const
        {
            const RectangleF& body = player.rectangle();
            const float foot_x = crest.x > toe.x ? body.right() : body.left();
            if (player.move_state() != state || foot_x < std::min(toe.x, crest.x) ||
                foot_x > std::max(toe.x, crest.x)) { return false; }
            const float surface_y = toe.y + (crest.y - toe.y) * (foot_x - toe.x) / (crest.x - toe.x);
            return std::fabs(body.bottom() - surface_y) <= endpoint_tolerance;
        }
    };

    Ramp read_ramp(const std::filesystem::path& content, bool left)
    {
        LevelLoadedInfo info(path_to_utf8(content / "levels" / "king_of_the_hill.json").c_str());
        info.validate();
        const JsonValue entries = info.collision_objects_json();
        const char* name = left ? "mountain_level_1_ramp_east" : "mountain_level_1_ramp_west";
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const JsonValue entry = entries.at(index);
            if (!entry.has("name") || entry.string("name") != name) { continue; }
            const JsonValue triangle = entry.object("triangle");
            const Ramp ramp{ { triangle.number("x1"), triangle.number("y1") },
                { triangle.number("x3"), triangle.number("y3") },
                left ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left };
            require(ramp.toe.y > ramp.crest.y && ramp.toe.x != ramp.crest.x,
                "Hill-route ramp does not have the expected authored toe and crest");
            return ramp;
        }
        throw std::runtime_error("Hill route needs authored ramp '" + std::string(name) + "'");
    }

    struct Endpoints
    {
        std::vector<const RectangleF*> supports;
        float start_x;
        int minimum_jumps;
    };

    Endpoints endpoints(const World& world, const Trial& trial)
    {
        const bool left = trial.team == PlayerTeam::a;
        switch (trial.route)
        {
        case Route::ramp_approach:
            return { { &world.named("boundary_floor"), &world.named("mountain_level_1_centre"),
                &world.named("mountain_level_5") }, left ? 200.0f : 5800.0f, 1 };
        case Route::summit_entrance:
            return { { &world.named("mountain_level_7"), &world.named("mountain_level_8"),
                &world.named("mountain_level_9"), &world.named("mountain_level_10"),
                &world.named("mountain_level_11"), &world.named("castle_entrance") },
                left ? 2350.0f : 3650.0f, 5 };
        case Route::castle_interior:
            return { { &world.named(left ? "castle_floor_left" : "castle_floor_right"),
                &world.named(left ? "castle_platform_left" : "castle_platform_right"),
                &world.named("castle_platform_mid"), &world.named("castle_room_entrance") },
                left ? 2775.0f : 3225.0f, 3 };
        }
        throw std::runtime_error("Unknown bot hill endpoints");
    }

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("king_of_the_hill"); settings.set_duration(60.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // Only the initial position is injected. The shipping driver receives no
    // route, goal, movement, paint, resource or enemy-information override.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Hill-route trial did not finish its ordinary countdown");
        const Endpoints route = endpoints(world, trial);
        const Vector2F size = world.assets.tuning.player().size;
        require(level.place_player(1, Vector2F(route.start_x - size.x * 0.5f,
            route.supports.front()->top() - size.y)), "Hill-route trial could not place its initial endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - route.supports.front()->top()) <= endpoint_tolerance,
            "Hill-route initial feet are not at the named support height");
    }

    bool run(World& world, const Trial& trial, const Ramp& ramp, bool replay_check)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const Endpoints route = endpoints(world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        std::vector<int> contacts(route.supports.size(), -1);
        std::size_t next_contact = 0;
        int first_ramp = -1, jumps = 0;
        Tape tape("Hill-route playback exhausted its finite final-input tape");
        std::ostringstream trace;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Hill-route trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Hill-route trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Hill-route bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Hill-route driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.state() == PlayerState::alive && bot.player_num() == -1 &&
                bot.primary_weapon().type() == trial.tool && std::isfinite(bot.center().x) &&
                std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) && bot.weapon_ammo() >= 0.0f,
                "Hill-route bot died, changed identity/tool or produced invalid state");
            if (next_contact > 0 && first_ramp < 0 && ramp.contact(bot)) { first_ramp = tick; }
            if (next_contact < contacts.size() && on_support(bot, *route.supports[next_contact]) &&
                (trial.route != Route::ramp_approach || next_contact == 0 || first_ramp >= 0))
            { contacts[next_contact++] = tick; }
            jumps += input.jump_pressed ? 1 : 0;
            if (static_cast<int>(trial.tool) == 0 && trial.difficulty == BotDifficulty::normal &&
                (tick % 10 == 0 || input.jump_pressed))
            {
                trace << "TRACE step=" << tick << " x=" << bot.center().x << " feet=" << bot.rectangle().bottom()
                    << " vx=" << bot.velocity().x << " vy=" << bot.velocity().y << " ground=" << grounded(bot)
                    << " move=" << input.x_movement << " jump=" << input.jump_pressed << " drop=" << input.drop_through
                    << " contacts=" << next_contact << '\n';
            }
        }
        const bool passed = next_contact == contacts.size() && jumps >= route.minimum_jumps;
        std::cout << "MEASURE bot hill route=" << route_name(trial.route) <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " ramp=" << first_ramp << " contacts=";
        for (const int contact : contacts) { std::cout << contact << ','; }
        std::cout << " jumps=" << jumps << " final_x=" << level->player(1).center().x <<
            " final_y=" << level->player(1).center().y << " route_passed=" << passed << '\n';
        if (!passed) { std::cout << trace.str(); }
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, trial_steps);
#endif
            level.reset();
            level = world.build(settings); prepare(*level, world, trial);
            tape.playback = true;
            for (int tick = 0; tick < trial_steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == trial_steps && snapshot(*level) == expected,
                "Hill-route final-input replay changed public players, projectile count or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Hill-route final-input replay changed the complete Debug fingerprint");
#endif
        }
        return passed;
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot hill routes", [&]
    {
        require(argc == 2, "usage: bot_hill_routes <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        World world(content, "bot-hill-routes", "Bot hill route needs authored support '", "king_of_the_hill");
        const Ramp left_ramp = read_ramp(content, true), right_ramp = read_ramp(content, false);
        int trials = 0, passed = 0;
        for (const Route route : { Route::ramp_approach, Route::summit_entrance, Route::castle_interior })
        {
            int route_passed = 0;
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        route_passed += run(world, { route, team, static_cast<WeaponType>(tool), difficulty },
                            team == PlayerTeam::a ? left_ramp : right_ramp,
                            tool == 0 && difficulty == BotDifficulty::normal) ? 1 : 0;
                        ++trials;
                    }
                }
            }
            passed += route_passed;
            std::cout << "MEASURE bot hill route completed route=" << route_name(route) <<
                " route_passed=" << route_passed << "/30\n";
        }
        std::cout << "MEASURE bot hill routes completed=" << trials << " route_passed=" << passed << '\n';
        require(trials == 90 && passed == trials,
            "A bot did not complete every named hill route within 1800 active steps");
        std::cout << "PASS 90 bounded actual-driver King of the Hill ramp, summit/entrance and castle-interior chains, "
            "ordered named grounded contacts and ordinary jumps; six 1800-step final-input replays; "
            "territory usefulness remains unqualified\n";
        return 0;
    });
}
