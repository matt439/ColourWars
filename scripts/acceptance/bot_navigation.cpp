#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 600;

    struct Trial
    {
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    MatchSetup roster(const Trial& trial)
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_player_bot(1, trial.difficulty);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_stage("close_quarters"); settings.set_duration(20.0f);
        settings.set_player_team(0, trial.team == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
        settings.set_player_team(1, trial.team);
        settings.set_player_weapon(0, trial.tool); settings.set_player_weapon(1, trial.tool);
        return settings;
    }

    // This separate capability trial places the bot once at its home-floor
    // endpoint. All subsequent movement, drops and firing come from the real
    // shipping driver; the opposite human starts at its natural spawn and
    // receives neutral physical inputs throughout.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Navigation trial did not finish the ordinary countdown");
        const RectangleF& home = world.named(trial.team == PlayerTeam::a ? "spawn_a_floor" : "spawn_b_floor");
        const Vector2F size = world.assets.tuning.player().size;
        const float center_x = trial.team == PlayerTeam::a ? 500.0f : 5500.0f;
        require(level.place_player(1, Vector2F(center_x - size.x * 0.5f, home.top() - size.y)),
            "Navigation trial could not place the initial home-floor endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - home.top()) <= endpoint_tolerance,
            "Navigation trial initial body does not stand at the named home-floor height");
    }

    float owned_floor_paint(const Level& level, const World& world)
    {
        const RectangleF& floor = world.named("boundary_floor");
        const float thickness = world.assets.tuning.paint().thickness;
        const int owner = paint_owners::index(-1, 1);
        float area = 0.0f;
        for (const PaintSurfaceSample& surface : level.paint_surfaces())
        {
            if (surface.face != PaintSurfaceFace::top || surface.paint_owner != owner) { continue; }
            bool belongs = true;
            for (const Vector2F& corner : surface.corners)
            {
                belongs = belongs && corner.x >= floor.left() - endpoint_tolerance &&
                    corner.x <= floor.right() + endpoint_tolerance &&
                    std::fabs(corner.y - floor.top()) <= thickness + endpoint_tolerance;
            }
            if (belongs) { area += surface.scored_area; }
        }
        return area;
    }

    void run(World& world, const Trial& trial, bool replay_check)
    {
        const MatchSetup settings = roster(trial);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        const RectangleF& home = world.named(trial.team == PlayerTeam::a ?
            "spawn_a_floor" : "spawn_b_floor");
        const RectangleF& recovery = world.named(trial.team == PlayerTeam::a ?
            "spawn_a_recovery" : "spawn_b_recovery");
        const RectangleF& scoring = world.named("boundary_floor");
        const RectangleF& near_floor = world.named(trial.team == PlayerTeam::a ?
            "boundary_floor_left" : "boundary_floor_right");
        const float initial_x = level->player(1).center().x;
        int first_drop = -1, first_landing = -1, first_scoring = -1, firing_steps = 0;
        float maximum_progress = 0.0f;
        Tape tape;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            const bool was_on_home = on_support(level->player(1), home);
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Navigation trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Navigation trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                "Navigation bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Navigation driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.state() == PlayerState::alive && bot.player_num() == -1 &&
                bot.primary_weapon().type() == trial.tool && std::isfinite(bot.center().x) &&
                std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) && bot.weapon_ammo() >= 0.0f,
                "Navigation bot died, changed identity/tool or produced invalid state");
            if (was_on_home && input.drop_through && first_drop < 0) { first_drop = tick; }
            if (first_drop >= 0 && first_landing < 0 &&
                (on_support(bot, recovery) || on_support(bot, near_floor) || on_support(bot, scoring)))
            { first_landing = tick; }
            if (on_support(bot, scoring) && first_scoring < 0) { first_scoring = tick; }
            firing_steps += bot.primary_weapon().shooting_this_update() ? 1 : 0;
            maximum_progress = std::max(maximum_progress,
                (bot.center().x - initial_x) * (trial.team == PlayerTeam::a ? 1.0f : -1.0f));
        }
        const float paint = owned_floor_paint(*level, world);
        std::cout << "MEASURE CQ bot home drop team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') <<
            " tool=" << static_cast<int>(trial.tool) << " difficulty=" << static_cast<int>(trial.difficulty) <<
            " drop=" << first_drop << " lower_landing=" << first_landing << " scoring_landing=" << first_scoring <<
            " progress=" << maximum_progress << " firing_steps=" << firing_steps << " floor_paint=" << paint << '\n';
        require(first_drop >= 0 && first_landing >= first_drop,
            "CQ bot did not drop through its home platform and land on a named lower support");
        require(first_scoring >= first_landing && firing_steps > 0 && paint > 0.0f,
            "CQ bot did not reach the named scored bypass and own paid floor paint within 600 steps");
        if (replay_check)
        {
            const std::vector<std::uint64_t> expected = snapshot(*level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint expected_full = MatchFingerprint::of(*level, trial_steps);
#endif
            // Both levels borrow the same partitioner, so destroy before rebuilding.
            level.reset();
            level = world.build(settings); prepare(*level, world, trial);
            tape.playback = true;
            for (int tick = 0; tick < trial_steps; ++tick) { level->update(physical, dt, &tape); }
            require(tape.next == trial_steps && snapshot(*level) == expected,
                "Navigation final-input replay changed actors, projectiles or exact paint state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(expected_full, MatchFingerprint::of(*level, trial_steps)).empty(),
                "Navigation final-input replay changed the complete Debug fingerprint");
#endif
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_navigation <content>");
        World world(std::filesystem::absolute(argv[1]));
        int trials = 0;
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            for (int tool = 0; tool < 5; ++tool)
            {
                for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
                {
                    run(world, { team, static_cast<WeaponType>(tool), difficulty },
                        tool == 0 && difficulty == BotDifficulty::normal);
                    ++trials;
                }
            }
        }
        require(trials == 30, "Navigation matrix missed a mirrored tool/difficulty configuration");
        std::cout << "PASS 30 bounded actual-driver CQ home drops, named lower/scoring landings and owned floor paint; "
            "two 600-step final-input replays; platform ascent and general navigation remain separate qualification\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL bot navigation: " << error.what() << '\n'; return 1; }
}
