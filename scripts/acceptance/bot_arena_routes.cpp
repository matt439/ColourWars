#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
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

    constexpr int trial_steps = 900;

    struct Route
    {
        const char* stage;
        const char* name;
        const char* start_a;
        const char* start_b;
        const char* intermediate_a;
        const char* intermediate_b;
        const char* finish_a;
        const char* finish_b;
        float start_x_a;
        float start_x_b;
    };

    const std::array<Route, 5> routes = {{
        { "crossfeed", "home_return", "boundary_floor_a", "boundary_floor_b", nullptr, nullptr,
            "spawn_a_floor", "spawn_b_floor", 300.0f, 3300.0f },
        { "crossfeed", "mixer", "centre_plinth", "centre_plinth", "upper_step_a", "upper_step_b",
            "centre_mixer", "centre_mixer", 1700.0f, 1900.0f },
        { "king_of_the_hill", "terraces", "mountain_level_5", "mountain_level_5",
            "mountain_level_6", "mountain_level_6", "mountain_level_7", "mountain_level_7", 2000.0f, 4000.0f },
        { "turbulence", "lower_deck", "boundary_floor", "boundary_floor",
            "left_bottom_left_jump", "right_bottom_right_jump", "left_lower_paint", "right_lower_paint",
            825.0f, 5385.0f },
        { "turbulence", "mid_deck", "left_lower_paint", "right_lower_paint",
            "left_mid_left_jump", "right_mid_right_jump", "left_mid_paint", "right_mid_paint", 1190.0f, 4810.0f }
    }};

    struct Trial
    {
        const Route& route;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct Endpoints
    {
        const RectangleF& start;
        const RectangleF* intermediate;
        const RectangleF& finish;
        float start_x;
    };

    Endpoints endpoints(const World& world, const Trial& trial)
    {
        const bool left = trial.team == PlayerTeam::a;
        const Route& route = trial.route;
        const char* middle = left ? route.intermediate_a : route.intermediate_b;
        return { world.named(left ? route.start_a : route.start_b),
            middle == nullptr ? nullptr : &world.named(middle),
            world.named(left ? route.finish_a : route.finish_b), left ? route.start_x_a : route.start_x_b };
    }

    MenuLevelSettings roster(const Trial& trial)
    {
        MenuLevelSettings settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage(trial.route.stage); settings.set_duration(30.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // Injection ends at initial placement. Goals, observations, resources,
    // subsequent movement and paint come from the ordinary shipping path.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Arena-route trial did not finish its ordinary countdown");
        const Endpoints route = endpoints(world, trial);
        const Vector2F size = world.assets.tuning.player().size;
        require(level.place_player(1, Vector2F(route.start_x - size.x * 0.5f, route.start.top() - size.y)),
            "Arena-route trial could not place its initial endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - route.start.top()) <= endpoint_tolerance,
            "Arena-route initial feet are not at the named support height");
    }

    float owned_top_paint(const Level& level, const World& world, const RectangleF& support)
    {
        const float thickness = world.assets.tuning.paint().thickness;
        const int owner = paint_owners::index(-1, 1);
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != owner) { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                belongs = belongs && corner.x >= support.left() - endpoint_tolerance &&
                    corner.x <= support.right() + endpoint_tolerance &&
                    std::fabs(corner.y - support.top()) <= thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    struct TrialResult
    {
        bool route_passed;
        bool owns_finish_paint;
    };

    TrialResult run(World& world, const Trial& trial, bool replay_check)
    {
        const MenuLevelSettings settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const Endpoints route = endpoints(world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        int first_start = -1, first_intermediate = -1, first_finish = -1, jumps = 0;
        Tape tape("Arena-route playback exhausted its finite final-input tape");
        std::ostringstream trace;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Arena-route trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Arena-route trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Arena-route bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Arena-route driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.state() == PlayerState::alive && bot.player_num() == -1 &&
                bot.primary_weapon().type() == trial.tool && std::isfinite(bot.center().x) &&
                std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) && bot.weapon_ammo() >= 0.0f,
                "Arena-route bot died, changed identity/tool or produced invalid state");
            if (on_support(bot, route.start) && first_start < 0) { first_start = tick; }
            if (first_start >= 0 && route.intermediate != nullptr && on_support(bot, *route.intermediate) &&
                first_intermediate < 0) { first_intermediate = tick; }
            const bool previous_contact = route.intermediate == nullptr ? first_start >= 0 : first_intermediate >= 0;
            if (previous_contact && on_support(bot, route.finish) && first_finish < 0) { first_finish = tick; }
            jumps += input.jump_pressed ? 1 : 0;
            if (static_cast<int>(trial.tool) == 0 && trial.difficulty == BotDifficulty::normal &&
                (tick < 180 && (tick % 5 == 0 || input.jump_pressed)))
            {
                trace << "TRACE step=" << tick << " x=" << bot.center().x << " feet=" << bot.rectangle().bottom()
                    << " vx=" << bot.velocity().x << " vy=" << bot.velocity().y << " ground=" << grounded(bot)
                    << " move=" << input.x_movement << " jump=" << input.jump_pressed << " drop=" << input.drop_through << '\n';
            }
        }
        const float paint = owned_top_paint(*level, world, route.finish);
        // Named grounded contacts qualify the route. Retained scoring paint
        // is reported separately, since these local starts do not prove value.
        const bool passed = first_start >= 0 && first_finish > first_start &&
            (route.intermediate == nullptr || (first_intermediate > first_start && first_finish > first_intermediate)) &&
            jumps >= (route.intermediate == nullptr ? 1 : 2);
        std::cout << "MEASURE bot arena route stage=" << trial.route.stage << " route=" << trial.route.name <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " start=" << first_start <<
            " intermediate=" << first_intermediate << " finish=" << first_finish << " jumps=" << jumps <<
            " top_paint=" << paint << " final_x=" << level->player(1).center().x <<
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
                "Arena-route final-input replay changed public actors, projectiles or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Arena-route final-input replay changed the complete Debug fingerprint");
#endif
        }
        return { passed, paint > 0.0f };
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_arena_routes <content>");
        int trials = 0, passed = 0, paint_trials = 0;
        for (const Route& route : routes)
        {
            World world(std::filesystem::absolute(argv[1]), "bot-arena-routes",
                "Bot arena route needs authored support '", route.stage);
            int route_passed = 0;
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const TrialResult result = run(world, { route, team, static_cast<WeaponType>(tool), difficulty },
                            tool == 0 && difficulty == BotDifficulty::normal);
                        route_passed += result.route_passed ? 1 : 0;
                        paint_trials += result.owns_finish_paint ? 1 : 0;
                        ++trials;
                    }
                }
            }
            passed += route_passed;
            std::cout << "MEASURE bot arena route completed stage=" << route.stage << " route=" << route.name <<
                " route_passed=" << route_passed << "/30\n";
        }
        std::cout << "MEASURE bot arena routes completed=" << trials << " route_passed=" << passed <<
            " finish_paint_trials=" << paint_trials << "/" << trials << '\n';
        require(trials == 150 && passed == trials,
            "A bot did not complete every named static arena route within 900 active steps");
        std::cout << "PASS 150 bounded actual-driver Crossfeed, King of the Hill and Turbulence routes, ordered named "
            "contacts and ordinary jumps; ten 900-step final-input replays; finish paint is measured, "
            "territory usefulness remains unqualified\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot arena routes: " << error.what() << '\n'; return 1; }
}
