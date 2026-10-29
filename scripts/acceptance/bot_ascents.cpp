#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/support/probe.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 900;

    enum class Route { home_return, lower_deck, upper_deck };

    const char* route_name(Route route)
    {
        switch (route)
        {
        case Route::home_return: return "home_return";
        case Route::lower_deck: return "lower_deck";
        case Route::upper_deck: return "upper_deck";
        }
        throw std::runtime_error("Unknown bot ascent route");
    }

    struct Trial
    {
        Route route;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct TrialResult
    {
        bool route_passed;
        bool owns_deck_paint;
    };

    struct Endpoints
    {
        const RectangleF& start;
        const RectangleF& intermediate;
        const RectangleF& finish;
        float start_x;
    };

    Endpoints endpoints(const World& world, const Trial& trial)
    {
        const bool left = trial.team == PlayerTeam::a;
        switch (trial.route)
        {
        case Route::home_return:
            return { world.named(left ? "boundary_floor_left" : "boundary_floor_right"),
                world.named(left ? "spawn_a_recovery" : "spawn_b_recovery"),
                world.named(left ? "spawn_a_floor" : "spawn_b_floor"), left ? 500.0f : 5500.0f };
        case Route::lower_deck:
            return { world.named(left ? "boundary_floor_left" : "boundary_floor_right"),
                world.named(left ? "lower_left_platform" : "lower_right_platform"),
                world.named("lower_paint"), left ? 1300.0f : 4700.0f };
        case Route::upper_deck:
            return { world.named("lower_paint"),
                world.named(left ? "upper_left_platform" : "upper_right_platform"),
                world.named("upper_paint"), left ? 2300.0f : 3700.0f };
        }
        throw std::runtime_error("Unknown bot ascent endpoints");
    }

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("close_quarters"); settings.set_duration(30.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // Only the initial position is injected. No goal, movement, paint,
    // resources or enemy information is supplied to the shipping policy.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Ascent trial did not finish its ordinary countdown");
        const Endpoints route = endpoints(world, trial);
        const Vector2F size = world.assets.tuning.player().size;
        require(level.place_player(1, Vector2F(route.start_x - size.x * 0.5f, route.start.top() - size.y)),
            "Ascent trial could not place the initial lower endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - route.start.top()) <= endpoint_tolerance,
            "Ascent initial body does not stand at the named support height");
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

    TrialResult run(World& world, const Trial& trial, bool replay_check)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const Endpoints route = endpoints(world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        int first_start = -1, first_intermediate = -1, first_finish = -1, jumps = 0;
        Tape tape("Ascent playback exhausted its finite final-input tape");
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Ascent trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Ascent trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Ascent bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Ascent driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.state() == PlayerState::alive && bot.player_num() == -1 &&
                bot.primary_weapon().type() == trial.tool && std::isfinite(bot.center().x) &&
                std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) && bot.weapon_ammo() >= 0.0f,
                "Ascent bot died, changed identity/tool or produced invalid state");
            if (on_support(bot, route.start) && first_start < 0) { first_start = tick; }
            if (first_start >= 0 && on_support(bot, route.intermediate) && first_intermediate < 0)
            { first_intermediate = tick; }
            if (first_intermediate >= 0 && on_support(bot, route.finish) && first_finish < 0)
            { first_finish = tick; }
            jumps += input.jump_pressed ? 1 : 0;
        }
        const float paint = owned_top_paint(*level, world, route.finish);
        // This case qualifies the ordered route. Retained deck paint is a
        // separate measurement for the subsequent territory-usefulness work.
        const bool passed = first_start >= 0 && first_intermediate > first_start && first_finish > first_intermediate &&
            jumps >= 2;
        std::cout << "MEASURE CQ bot ascent route=" << route_name(trial.route) <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " start=" << first_start <<
            " intermediate=" << first_intermediate << " finish=" << first_finish << " jumps=" << jumps <<
            " top_paint=" << paint << " final_x=" << level->player(1).center().x <<
            " final_y=" << level->player(1).center().y << " route_passed=" << passed << '\n';
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
                "Ascent final-input replay changed actors, projectiles or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Ascent final-input replay changed the complete Debug fingerprint");
#endif
        }
        return { passed, trial.route != Route::home_return && paint > 0.0f };
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot ascents", [&]
    {
        require(argc == 2, "usage: bot_ascents <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-ascents", "Bot ascent needs authored support '");
        int trials = 0, passed = 0, deck_paint_trials = 0;
        for (const Route route : { Route::home_return, Route::lower_deck, Route::upper_deck })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (int tool = 0; tool < 5; ++tool)
                {
                    for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                    {
                        const TrialResult result = run(world, { route, team, static_cast<WeaponType>(tool), difficulty },
                            tool == 0 && difficulty == BotDifficulty::normal);
                        passed += result.route_passed ? 1 : 0;
                        deck_paint_trials += result.owns_deck_paint ? 1 : 0;
                        ++trials;
                    }
                }
            }
        }
        std::cout << "MEASURE CQ bot ascents completed=" << trials << " route_passed=" << passed <<
            " deck_paint_trials=" << deck_paint_trials << "/60\n";
        require(trials == 90 && passed == trials, "CQ bot did not complete every named ascent within 900 active steps");
        std::cout << "PASS 90 bounded actual-driver CQ recovery/home and lower/upper deck ascents, ordered named "
            "contacts and ordinary jumps; six 900-step final-input replays; deck paint is measured, "
            "territory usefulness remains unqualified\n";
        return 0;
    });
}
