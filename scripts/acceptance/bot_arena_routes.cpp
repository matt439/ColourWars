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
        // The fewest takeoffs any path between the named contacts needs.
        int min_jumps;
    };

    const std::array<Route, 5> routes = {{
        { "crossfeed", "home_return", "boundary_floor_a", "boundary_floor_b", nullptr, nullptr,
            "spawn_a_floor", "spawn_b_floor", 300.0f, 3300.0f, 1 },
        { "crossfeed", "mixer", "centre_plinth", "centre_plinth", "upper_step_a", "upper_step_b",
            "centre_mixer", "centre_mixer", 1700.0f, 1900.0f, 2 },
        { "king_of_the_hill", "terraces", "mountain_level_5", "mountain_level_5",
            "mountain_level_6", "mountain_level_6", "mountain_level_7", "mountain_level_7", 2000.0f, 4000.0f, 2 },
        { "turbulence", "lower_deck", "boundary_floor", "boundary_floor",
            "left_bottom_left_jump", "right_bottom_right_jump", "left_lower_paint", "right_lower_paint",
            825.0f, 5385.0f, 2 },
        { "turbulence", "mid_deck", "left_lower_paint", "right_lower_paint",
            "left_mid_left_jump", "right_mid_right_jump", "left_mid_paint", "right_mid_paint", 1190.0f, 4810.0f, 2 }
    }};

    // Press Tower is measured, not required: an unfinished route is a limit
    // with its trace (docs/polish-press-tower.md, POL#41). R2 walks the ramps
    // and bed. A held jump rises 207 at most, and R3 rises 240 and R4 320, so each
    // needs two takeoffs by any path; the summit leg's 80 needs one.
    const std::array<Route, 4> press_tower_routes = {{
        { "press_tower", "r2", "floor_yard_a", "floor_yard_b", "press_bed", "press_bed",
            "floor_yard_b", "floor_yard_a", 1200.0f, 2400.0f, 0 },
        { "press_tower", "r3", "catwalk_a", "catwalk_b", "stair_3_a", "stair_3_b",
            "shoulder_a", "shoulder_b", 900.0f, 2700.0f, 2 },
        { "press_tower", "r4", "press_bed", "press_bed", "frame_a", "frame_b",
            "shoulder_a", "shoulder_b", 1560.0f, 2040.0f, 2 },
        { "press_tower", "summit", "shoulder_a", "shoulder_b", nullptr, nullptr,
            "press_head", "press_head", 1560.0f, 2040.0f, 1 }
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
        int finish_steps = 0, finish_fire = 0, finish_paid = 0;
        Tape tape("Arena-route playback exhausted its finite final-input tape");
        std::ostringstream trace;
        std::ostringstream paint_trace;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            const float previous_ammo = level->player(1).weapon_ammo();
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
            // Each contact counts from the step after the one before it: a body
            // straddling two level neighbours (Press Tower's stair_3 and
            // shoulder) touches both at once without having crossed.
            if (first_start >= 0 && tick > first_start && route.intermediate != nullptr &&
                on_support(bot, *route.intermediate) && first_intermediate < 0) { first_intermediate = tick; }
            const int previous_contact = route.intermediate == nullptr ? first_start : first_intermediate;
            if (previous_contact >= 0 && tick > previous_contact && on_support(bot, route.finish) && first_finish < 0)
            { first_finish = tick; }
            if (on_support(bot, route.finish))
            {
                // An observed net ammunition decrease establishes paid fire
                // during this step, not a count of emitted projectiles.
                const bool paid_fire = bot.weapon_ammo() < previous_ammo;
                ++finish_steps;
                finish_fire += input.primary_shoot ? 1 : 0;
                finish_paid += paid_fire ? 1 : 0;
                if (finish_steps <= 20)
                {
                    paint_trace << "PAINT_TRACE step=" << tick << " x=" << bot.center().x <<
                        " y=" << bot.center().y << " feet=" << bot.rectangle().bottom() <<
                        " angle=" << input.shoot_angle << " fire=" << input.primary_shoot <<
                        " paid_fire=" << paid_fire <<
                        " readiness=" << static_cast<int>(bot.primary_weapon().readiness()) <<
                        " ammo=" << bot.weapon_ammo() << " nozzle_x=" << bot.primary_weapon().nozzle_pos().x <<
                        " nozzle_y=" << bot.primary_weapon().nozzle_pos().y << '\n';
                }
            }
            jumps += input.jump_pressed ? 1 : 0;
            // Press Tower may leave a route unfinished, so every one of its
            // trials keeps a sparse whole-horizon trace for the limit record.
            const bool tower = std::string_view(trial.route.stage) == "press_tower";
            if ((static_cast<int>(trial.tool) == 0 && trial.difficulty == BotDifficulty::normal &&
                (tick < 180 && (tick % 5 == 0 || input.jump_pressed))) ||
                (tower && (tick % 30 == 0 || input.jump_pressed)))
            {
                trace << "TRACE step=" << tick << " x=" << bot.center().x << " feet=" << bot.rectangle().bottom()
                    << " vx=" << bot.velocity().x << " vy=" << bot.velocity().y << " ground=" << grounded(bot)
                    << " move=" << input.x_movement << " jump=" << input.jump_pressed << " drop=" << input.drop_through << '\n';
            }
        }
        const float paint = owned_top_paint(*level, world, route.finish);
        if (std::string_view(trial.route.stage) == "crossfeed" && std::string_view(trial.route.name) == "mixer" &&
            trial.tool == WeaponType::sniper && (trial.difficulty == BotDifficulty::normal ? trial.team == PlayerTeam::b :
                (trial.team == PlayerTeam::a) == (trial.difficulty == BotDifficulty::easy)))
        {
            // Normal b, Easy a and Hard b. The latter two spent their shot on
            // a stale or overshooting angle before the deliberate support gate.
            require(paint > 0.0f, "The retained Sniper mixer landing again failed to paint above its long muzzle");
        }
        // Named grounded contacts qualify the route. Retained scoring paint
        // is reported separately, since these local starts do not prove value.
        const bool passed = first_start >= 0 && first_finish > first_start &&
            (route.intermediate == nullptr || (first_intermediate > first_start && first_finish > first_intermediate)) &&
            jumps >= trial.route.min_jumps;
        std::cout << "MEASURE bot arena route stage=" << trial.route.stage << " route=" << trial.route.name <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " start=" << first_start <<
            " intermediate=" << first_intermediate << " finish=" << first_finish << " jumps=" << jumps <<
            " top_paint=" << paint << " finish_grounded_steps=" << finish_steps <<
            " finish_fire_requests=" << finish_fire << " finish_paid_fire_steps=" << finish_paid <<
            " final_x=" << level->player(1).center().x <<
            " final_y=" << level->player(1).center().y << " route_passed=" << passed << '\n';
        if (!passed) { std::cout << trace.str(); }
        if (!passed || paint <= 0.0f) { std::cout << paint_trace.str(); }
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

        // Press Tower's own totals, after the unchanged contract above. Its
        // unfinished routes are measured limits, never a failure.
        World tower(std::filesystem::absolute(argv[1]), "bot-arena-routes",
            "Bot arena route needs authored support '", "press_tower");
        int tower_trials = 0, tower_passed = 0, tower_paint = 0;
        std::array<int, 3> by_difficulty{};
        std::array<int, 2> by_team{};
        for (const Route& route : press_tower_routes)
        {
            int route_passed = 0;
            std::array<int, 3> route_difficulty{};
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const TrialResult result = run(tower, { route, team, static_cast<WeaponType>(tool), difficulty },
                            tool == 0 && difficulty == BotDifficulty::normal);
                        const int pass = result.route_passed ? 1 : 0;
                        route_passed += pass;
                        route_difficulty[static_cast<std::size_t>(difficulty)] += pass;
                        by_difficulty[static_cast<std::size_t>(difficulty)] += pass;
                        by_team[team == PlayerTeam::a ? 0 : 1] += pass;
                        tower_paint += result.owns_finish_paint ? 1 : 0;
                        ++tower_trials;
                    }
                }
            }
            tower_passed += route_passed;
            std::cout << "MEASURE bot arena route completed stage=press_tower route=" << route.name <<
                " route_passed=" << route_passed << "/30 easy=" << route_difficulty[0] << "/10 normal=" <<
                route_difficulty[1] << "/10 hard=" << route_difficulty[2] << "/10\n";
        }
        std::cout << "MEASURE bot arena press_tower routes completed=" << tower_trials << " route_passed=" <<
            tower_passed << " finish_paint_trials=" << tower_paint << "/" << tower_trials << " easy=" <<
            by_difficulty[0] << "/40 normal=" << by_difficulty[1] << "/40 hard=" << by_difficulty[2] <<
            "/40 team_a=" << by_team[0] << "/60 team_b=" << by_team[1] << "/60\n";
        require(tower_trials == 120, "Press Tower route matrix changed size");
        std::cout << "PASS 150 bounded actual-driver Crossfeed, King of the Hill and Turbulence routes, ordered named "
            "contacts and ordinary jumps; ten 900-step final-input replays; finish paint is measured, "
            "territory usefulness remains unqualified\n";
        std::cout << "PASS 120 bounded actual-driver Press Tower R2/R3/R4/summit measurements and eight final-input "
            "replays; unfinished routes are measured limits, not failures\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot arena routes: " << error.what() << '\n'; return 1; }
}
