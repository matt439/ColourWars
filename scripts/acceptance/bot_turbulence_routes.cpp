#include "game/pch.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>
#include <sstream>

namespace
{
    using namespace labrador;
    using namespace bot_navigation_acceptance;

    constexpr int trial_steps = 3600;

    enum class Route { upper_crossing, continuous_ascent, divider_recovery };

    const char* route_name(Route route)
    {
        switch (route)
        {
        case Route::upper_crossing: return "upper_crossing";
        case Route::continuous_ascent: return "continuous_ascent";
        case Route::divider_recovery: return "divider_recovery";
        }
        throw std::runtime_error("Unknown bot Turbulence route");
    }

    struct Trial
    {
        Route route;
        PlayerTeam team;
        WeaponType tool;
        BotDifficulty difficulty;
    };

    struct Contact
    {
        std::vector<const RectangleF*> alternatives;
        const char* name;

        bool reached(const Player& player) const
        {
            for (const RectangleF* support : alternatives)
            { if (on_support(player, *support)) { return true; } }
            return false;
        }
    };

    struct Endpoints
    {
        std::vector<Contact> contacts;
        float start_x;
        int minimum_jumps;
    };

    Endpoints endpoints(const World& world, const Trial& trial)
    {
        const bool left = trial.team == PlayerTeam::a;
        Endpoints result;
        result.start_x = left ? 2650.0f : 3350.0f;
        result.minimum_jumps = 4;
        if (trial.route != Route::upper_crossing)
        {
            const bool recovery = trial.route == Route::divider_recovery;
            result.start_x = recovery ? (left ? 2825.0f : 3175.0f) : (left ? 825.0f : 5385.0f);
            result.minimum_jumps = 10;
            result.contacts.push_back({ { &world.named("boundary_floor") }, "boundary_floor" });
            result.contacts.push_back({ { &world.named(left ?
                (recovery ? "left_bottom_right_jump" : "left_bottom_left_jump") :
                (recovery ? "right_bottom_left_jump" : "right_bottom_right_jump")) }, "bottom_jump" });
            result.contacts.push_back({ { &world.named(left ? "left_lower_paint" : "right_lower_paint") }, "lower_paint" });
            // Either authored mid step supplies a legitimate continuation
            // from the same scoring deck. Do not prescribe the driver's path.
            result.contacts.push_back({ { &world.named(left ? "left_mid_left_jump" : "right_mid_left_jump"),
                &world.named(left ? "left_mid_right_jump" : "right_mid_right_jump") }, "mid_jump" });
            result.contacts.push_back({ { &world.named(left ? "left_mid_paint" : "right_mid_paint") }, "mid_paint" });
            result.contacts.push_back({ { &world.named(left ? "left_upper_mid_right_jump" : "right_upper_mid_right_jump") },
                "upper_mid_jump" });
        }
        result.contacts.push_back({ { &world.named("central_divider_ledge_paint") }, "divider_ledge_paint" });
        result.contacts.push_back({ { &world.named("upper_jump_mid") }, "upper_jump_mid" });
        // Once above the divider, either upper deck and either outer step
        // may complete the crossing. Both starts still have independent gates.
        result.contacts.push_back({ { &world.named("upper_left"), &world.named("upper_right") }, "upper_deck" });
        result.contacts.push_back({ { &world.named("upper_jump_left"), &world.named("upper_jump_right") }, "upper_outer_jump" });
        result.contacts.push_back({ { &world.named("upper_top_paint") }, "upper_top_paint" });
        return result;
    }

    // Injection ends at initial placement. No route, destination, input,
    // resources, paint or enemy-information override reaches the bot policy.
    void prepare(Level& level, const World& world, const Trial& trial)
    {
        const std::vector<PlayerInputData> physical = physical_inputs();
        for (int tick = 0; tick < 900 && level.state() != LevelState::active; ++tick)
        { level.update(physical, dt); }
        require(level.state() == LevelState::active, "Turbulence-route trial did not finish its ordinary countdown");
        const Endpoints route = endpoints(world, trial);
        const Vector2F size = world.assets.tuning.player().size;
        const RectangleF& start = *route.contacts.front().alternatives.front();
        require(level.place_player(1, Vector2F(route.start_x - size.x * 0.5f, start.top() - size.y)),
            "Turbulence-route trial could not place its initial endpoint");
        require(std::fabs(level.player(1).rectangle().bottom() - start.top()) <= endpoint_tolerance,
            "Turbulence-route initial feet are not at the named support height");
    }

    struct TrialResult
    {
        bool route_passed;
        bool owns_top_paint;
    };

    TrialResult run(World& world, const Trial& trial, bool replay_check)
    {
        const MatchSetup settings = one_bot_roster("turbulence", 120.0f, trial.team, trial.tool, trial.difficulty);
        std::unique_ptr<Level> level = world.build(settings);
        prepare(*level, world, trial);
        const Endpoints route = endpoints(world, trial);
        const std::vector<PlayerInputData> physical = physical_inputs();
        std::vector<int> contacts(route.contacts.size(), -1);
        std::size_t next_contact = 0;
        int jumps = 0;
        int finish_steps = 0, finish_fire = 0, finish_paid = 0;
        float highest_feet = level->player(1).rectangle().bottom();
        Tape tape("Turbulence-route playback exhausted its finite final-input tape");
        std::ostringstream trace;
        std::ostringstream paint_trace;
        for (int tick = 0; tick < trial_steps; ++tick)
        {
            const float previous_ammo = level->player(1).weapon_ammo();
            level->update(physical, dt, &tape);
            require(level->state() == LevelState::active && tape.steps.size() == static_cast<std::size_t>(tick + 1),
                "Turbulence-route trial escaped its finite active-step budget");
            require(tape.steps.back().size() == 2, "Turbulence-route trial lost a roster participant");
            const PlayerInputData& input = tape.steps.back()[1];
            require(input.connected && !input.toggle_debug,
                "Turbulence-route bot emitted a device/menu/pause action");
            require(tape.steps.back()[0].x_movement == 0.0f && !tape.steps.back()[0].primary_shoot,
                "Turbulence-route driver changed the stationary human's input");
            const Player& bot = level->player(1);
            require(bot.state() == PlayerState::alive && bot.player_num() == -1 &&
                bot.primary_weapon().type() == trial.tool && std::isfinite(bot.center().x) &&
                std::isfinite(bot.center().y) && std::isfinite(bot.weapon_ammo()) && bot.weapon_ammo() >= 0.0f,
                "Turbulence-route bot died, changed identity/tool or produced invalid state");
            const bool contact = next_contact < contacts.size() && route.contacts[next_contact].reached(bot);
            if (contact) { contacts[next_contact++] = tick; }
            if (route.contacts.back().reached(bot))
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
            highest_feet = std::min(highest_feet, bot.rectangle().bottom());
            if (tick % 30 == 0 || input.jump_pressed || contact)
            {
                trace << "TRACE step=" << tick << " x=" << bot.center().x << " feet=" << bot.rectangle().bottom()
                    << " vx=" << bot.velocity().x << " vy=" << bot.velocity().y << " ground=" << grounded(bot)
                    << " move=" << input.x_movement << " jump=" << input.jump_pressed << " drop=" << input.drop_through
                    << " contacts=" << next_contact << '\n';
            }
        }
        const float paint = owned_top_paint(*level, world.named("upper_top_paint"), world.assets.tuning.paint().thickness);
        const bool passed = next_contact == contacts.size() && jumps >= route.minimum_jumps;
        std::cout << "MEASURE bot turbulence route=" << route_name(trial.route) <<
            " team=" << (trial.team == PlayerTeam::a ? 'a' : 'b') << " tool=" << static_cast<int>(trial.tool) <<
            " difficulty=" << static_cast<int>(trial.difficulty) << " contacts=";
        for (std::size_t index = 0; index < contacts.size(); ++index)
        { std::cout << route.contacts[index].name << ':' << contacts[index] << ','; }
        std::cout << " jumps=" << jumps << " highest_feet=" << highest_feet << " top_paint=" << paint <<
            " finish_grounded_steps=" << finish_steps << " finish_fire_requests=" << finish_fire <<
            " finish_paid_fire_steps=" << finish_paid <<
            " final_x=" << level->player(1).center().x << " final_y=" << level->player(1).center().y <<
            " route_passed=" << passed << '\n';
        if (!passed) { std::cout << trace.str(); }
        if (!passed || paint <= 0.0f) { std::cout << paint_trace.str(); }
        if (replay_check)
        {
            check_final_input_replay(level, tape, trial_steps, physical, [&]
            {
                auto replayed = world.build(settings); prepare(*replayed, world, trial);
                return replayed;
            }, "Turbulence-route");
        }
        return { passed, paint > 0.0f };
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("bot turbulence routes", [&]
    {
        require(argc == 2, "usage: bot_turbulence_routes <content>");
        World world(std::filesystem::absolute(argv[1]), "bot-turbulence-routes",
            "Bot Turbulence route needs authored support '", "turbulence");
        int trials = 0, passed = 0, paint_trials = 0;
        for (const Route route : { Route::upper_crossing, Route::continuous_ascent, Route::divider_recovery })
        {
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
                        paint_trials += result.owns_top_paint ? 1 : 0;
                        ++trials;
                    }
                }
            }
            passed += route_passed;
            std::cout << "MEASURE bot turbulence route completed route=" << route_name(route) <<
                " route_passed=" << route_passed << "/30\n";
        }
        std::cout << "MEASURE bot turbulence routes completed=" << trials << " route_passed=" << passed <<
            " top_paint_trials=" << paint_trials << "/" << trials << '\n';
        require(trials == 90 && passed == trials,
            "A bot did not complete every named Turbulence chain within 3600 active steps");
        // Each chain ends on upper_top_paint; the finishing window holds the
        // bot there long enough to refill normally and pass one cooldown.
        require(paint_trials == trials, "A bot left its final Turbulence scoring landing without owned top paint");
        std::cout << "PASS 90 bounded actual-driver Turbulence upper crossings, continuous ascents and local divider "
            "recovery chains, ordered named grounded contacts and ordinary jumps; six 3600-step final-input replays; "
            "every final landing retains owned top paint; territory usefulness remains unqualified\n";
        return 0;
    });
}
