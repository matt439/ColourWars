#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        float reflected_x(const Run& run, bool right, float x)
        {
            const RectangleF bounds = run.services.map.info.out_of_bounds_rectangle();
            return right ? x : bounds.left() + bounds.right() - x;
        }

        void require_support(const Run& run, const std::string& name, const char* context)
        {
            require(supported(*run.player) && run.support() == name,
                std::string("Crossfeed ") + context + " missed support '" + name + "'");
        }

        void lower_departure(Run& run, bool right)
        {
            const int takeoffs = run.takeoffs;
            // The compulsory departure is a drop followed by walking. Holding
            // Down clears the one-way home floor without requesting a jump.
            for (int tick = 0; tick < 90; ++tick)
            {
                run.tick(input(0.0f, false, false, true));
            }
            const float floor_y = run.services.map.named("floor_paint_a").rectangle.top();
            require(supported(*run.player) && approximately(run.player->rectangle().bottom(), floor_y),
                "Crossfeed home drop did not recover on the continuous lower floor");
            drive_to(run, reflected_x(run, right, 1150.0f));
            require_support(run, right ? "floor_paint_a" : "floor_paint_b", "lower departure");
            require(run.takeoffs == takeoffs, "Crossfeed required departure requested a jump");
        }

        void upper_traverse(Run& run, bool right, int delay)
        {
            const auto x = [&](float value) { return reflected_x(run, right, value); };
            const auto side = [&](const char* a, const char* b) { return right ? a : b; };
            require_support(run, side("spawn_a_floor", "spawn_b_floor"), "upper route start");
            drive_to(run, x(550.0f));
            // These are optional inputs. A single-tick press is retained for
            // each 80-unit ascent; the 100-unit baffle rise uses a held jump.
            jump_to(run, run.services.map.named(side("spawn_a_step", "spawn_b_step")), x(710.0f), delay, 1);
            jump_to(run, run.services.map.named(side("spawn_a_baffle", "spawn_b_baffle")), x(920.0f), delay);
            drive_to(run, x(1200.0f));
            require_support(run, side("side_paint_a", "side_paint_b"), "upper departure landing");
            drive_to(run, x(1310.0f));
            jump_to(run, run.services.map.named(side("upper_step_a", "upper_step_b")), x(1450.0f), delay, 1);
            drive_to(run, x(1610.0f));
            jump_to(run, run.services.map.named("centre_mixer"), x(1750.0f), delay, 1);
            drive_to(run, x(2050.0f));
            require_support(run, side("upper_step_b", "upper_step_a"), "far upper landing");
            // A short descending tap crosses the far ruler's 40-unit gap.
            // Requiring a controlled stop on the side block also checks the
            // landing rather than accepting a later fall onto the lower route.
            jump_to(run, run.services.map.named(side("side_paint_b", "side_paint_a")), x(2390.0f), delay, 1);
            drive_to(run, x(2490.0f));
            jump_to(run, run.services.map.named(side("spawn_b_baffle", "spawn_a_baffle")), x(2680.0f), delay);
            drive_to(run, x(2890.0f));
            require_support(run, side("spawn_b_step", "spawn_a_step"), "far home step");
            drive_to(run, x(3050.0f));
            require_support(run, side("spawn_b_floor", "spawn_a_floor"), "far home arrival");
        }
    }

    void spawn_checks(Services& s)
    {
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const bool right = team == PlayerTeam::a;
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            const std::string home = right ? "spawn_a_floor" : "spawn_b_floor";
            require(points.size() == 4, "Crossfeed needs four tested spawn positions per team");
            for (size_t index = 0; index < points.size(); ++index)
            {
                const RectangleF body(points[index], s.tuning.player().size);
                for (size_t other = index + 1; other < points.size(); ++other)
                {
                    require(!narrow_phase(body, RectangleF(points[other], s.tuning.player().size)),
                        "Crossfeed same-team full-body spawns overlap");
                }
                Run run(s, points[index], team);
                run.settle();
                require_support(run, home, "spawn settling");
                for (int death = 0; death < 2; ++death)
                {
                    run.kill();
                    run.tick();
                    require(run.player->state() == PlayerState::dead, "Crossfeed damage fixture missed death");
                    int elapsed = 0;
                    while (run.player->state() == PlayerState::dead && elapsed++ < 600)
                    {
                        run.tick(input(right ? 1.0f : -1.0f, true, true, true));
                    }
                    require(run.player->state() == PlayerState::alive, "Crossfeed player never respawned");
                    run.death_expected = false;
                    const int takeoffs = run.takeoffs;
                    for (int tick = 0; tick < 30; ++tick) { run.tick(input(0.0f, false, true)); }
                    require_support(run, home, "respawn support");
                    require(run.takeoffs == takeoffs &&
                        approximately(run.player->rectangle().x, points[index].x) &&
                        approximately(run.player->rectangle().y, points[index].y),
                        "Crossfeed respawn moved or jumped with carried held input");
                    run.tick();
                }
                const int departure_start = run.ticks;
                lower_departure(run, right);
                std::cout << "MEASURE Crossfeed spawn team=" << (right ? 'a' : 'b') << " index=" << index <<
                    ": two damage deaths, held-input-safe respawn; required_drop_walk_to_scoring_ticks=" <<
                    run.ticks - departure_start << '\n';
            }
        }
        std::cout << "PASS Crossfeed all eight full-body spawns, two deaths/respawns each, "
            "and jump-free drop/walk departures to shared scoring flats\n";
    }

    void route_checks(Services& s)
    {
        const Surface& plinth = s.map.named("centre_plinth");
        for (const bool right : { true, false })
        {
            for (const float magnitude : { 0.25f, 0.5f, 1.0f })
            {
                // A controlled crest-start fixture, not a driven arrival:
                // the supporting corner begins exactly at the ramp's crest.
                Run crest(s, standing_on(s, plinth, right ? 1594.0f : 2006.0f),
                    right ? PlayerTeam::a : PlayerTeam::b);
                crest.settle();
                const int start = crest.ticks;
                bool entered = false;
                for (int tick = 0; tick < 180 && !entered; ++tick)
                {
                    crest.tick(input(right ? magnitude : -magnitude));
                    const RectangleF& body = crest.player->rectangle();
                    entered = crest.support() == plinth.name && (right ?
                        body.left() >= plinth.rectangle.left() + tolerance :
                        body.right() <= plinth.rectangle.right() - tolerance);
                }
                std::cout << "MEASURE Crossfeed controlled_crest_restart direction=" << (right ? 1 : -1) <<
                    " post_deadzone_input=" << fixed(magnitude) << " ticks=" << crest.ticks - start <<
                    " centre=" << fixed(crest.player->center().x) << " feet=" << fixed(crest.player->rectangle().bottom()) <<
                    " vx=" << fixed(crest.player->velocity().x) << " entered_plinth=" << entered << '\n';
                require(entered && crest.takeoffs == 0, "Crossfeed controlled crest restart failed at input=" +
                    fixed(magnitude) + " direction=" + std::to_string(right ? 1 : -1));
            }
        }
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const bool right = team == PlayerTeam::a;
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            require(!points.empty(), "Crossfeed route requires a representative home spawn");
            Run required(s, points.front(), team);
            required.settle();
            const auto x = [&](float value) { return reflected_x(required, right, value); };
            const int departure_start = required.ticks;
            lower_departure(required, right);
            const int outward_start = required.ticks;
            const int takeoffs = required.takeoffs;
            // The required attack/retreat connection is continuous ground:
            // both ramps and the central plinth must work without Jump.
            drive_to(required, x(1800.0f));
            require_support(required, "centre_plinth", "required central arrival");
            const int centre_ticks = required.ticks - outward_start;
            drive_to(required, x(2450.0f));
            require_support(required, right ? "floor_paint_b" : "floor_paint_a", "required far arrival");
            const int outward_ticks = required.ticks - outward_start;
            drive_to(required, x(1800.0f));
            require_support(required, "centre_plinth", "required return through centre");
            drive_to(required, x(1150.0f));
            require_support(required, right ? "floor_paint_a" : "floor_paint_b", "required near return");
            require(required.takeoffs == takeoffs,
                "Crossfeed required ramp/plinth route or its return needed a jump");
            std::cout << "MEASURE Crossfeed required_lower team=" << (right ? 'a' : 'b') <<
                " home_to_scoring_ticks=" << outward_start - departure_start <<
                " scoring_to_plinth_ticks=" << centre_ticks << " scoring_to_far_ticks=" << outward_ticks <<
                " lower_ground_round_trip_ticks=" << required.ticks - outward_start << " crossing_takeoffs=0\n";
            // Re-entry is a separate recovery edge: ground travel remains
            // jump-free, then one held 160-unit jump returns to the home floor.
            const int home_return_start = required.ticks;
            drive_to(required, x(500.0f));
            jump_to(required, s.map.named(right ? "spawn_a_floor" : "spawn_b_floor"), x(500.0f), 0);
            require_support(required, right ? "spawn_a_floor" : "spawn_b_floor", "lower home recovery");
            require(required.takeoffs == takeoffs + 1,
                "Crossfeed lower home recovery did not use exactly one additional held jump");
            std::cout << "MEASURE Crossfeed home_return team=" << (right ? 'a' : 'b') <<
                " home_return_ticks=" << required.ticks - home_return_start << " input=held rise=160\n";

            Run roof(s, points.front(), team);
            roof.settle();
            const int roof_start = roof.ticks;
            jump_to(roof, s.map.named(right ? "spawn_a_roof" : "spawn_b_roof"), x(550.0f), 0);
            require_support(roof, right ? "spawn_a_roof" : "spawn_b_roof", "held home-to-roof shortcut");
            jump_to(roof, s.map.named(right ? "spawn_a_baffle" : "spawn_b_baffle"), x(900.0f), 0);
            require_support(roof, right ? "spawn_a_baffle" : "spawn_b_baffle", "roof-to-baffle shortcut");
            require(roof.takeoffs == 2, "Crossfeed roof shortcut did not use its two declared held jumps");
            std::cout << "MEASURE Crossfeed optional_roof team=" << (right ? 'a' : 'b') <<
                " ticks=" << roof.ticks - roof_start << " home_rise=160 roof_gap=180 baffle_rise=20 input=held\n";

            Run recovery(s, points.front(), team);
            recovery.settle();
            drive_to(recovery, x(550.0f));
            jump_to(recovery, s.map.named(right ? "spawn_a_step" : "spawn_b_step"), x(710.0f), 0, 1);
            jump_to(recovery, s.map.named(right ? "spawn_a_baffle" : "spawn_b_baffle"), x(920.0f), 0);
            drive_to(recovery, x(1200.0f));
            drive_to(recovery, x(1310.0f));
            jump_to(recovery, s.map.named(right ? "upper_step_a" : "upper_step_b"), x(1450.0f), 0, 1);
            drive_to(recovery, x(1530.0f));
            require_support(recovery, right ? "upper_step_a" : "upper_step_b", "intentional drop start");
            const int recovery_takeoffs = recovery.takeoffs;
            const int recovery_start = recovery.ticks;
            for (int tick = 0; tick < 90; ++tick)
            {
                recovery.tick(input(0.0f, false, false, true));
            }
            drive_to(recovery, x(1150.0f));
            require_support(recovery, right ? "floor_paint_a" : "floor_paint_b", "upper drop recovery");
            require(recovery.takeoffs == recovery_takeoffs,
                "Crossfeed intentional upper drop needed an additional jump to rejoin lower paint");
            std::cout << "MEASURE Crossfeed intentional_upper_drop team=" << (right ? 'a' : 'b') <<
                " recovery_ticks=" << recovery.ticks - recovery_start << " support=" << recovery.support() <<
                " support_y=" << fixed(recovery.player->rectangle().bottom()) << " recovery_takeoffs=0\n";

            for (const int delay : { -2, 0, 2 })
            {
                Run optional(s, points.front(), team);
                optional.settle();
                const int start = optional.ticks;
                upper_traverse(optional, right, delay);
                const int outward_ticks_upper = optional.ticks - start;
                upper_traverse(optional, !right, delay);
                require_support(optional, right ? "spawn_a_floor" : "spawn_b_floor", "optional upper return");
                std::cout << "MEASURE Crossfeed optional_upper team=" << (right ? 'a' : 'b') <<
                    " press_delay=" << delay << " tap_hold_ticks=1 baffle_input=held outward_ticks=" <<
                    outward_ticks_upper << " round_trip_ticks=" << optional.ticks - start <<
                    " takeoffs=" << optional.takeoffs << '\n';
            }
        }
        std::cout << "PASS Crossfeed required lower ground crossings/returns use no jumps; "
            "home re-entry uses a separate held recovery, and optional roof/drop edges reconnect; "
            "optional upper crossings return the same player to its home in both directions with +/-2 press ticks. "
            "Scripted stops and input families establish route feasibility, not pressure or controller comfort\n";
    }
}
