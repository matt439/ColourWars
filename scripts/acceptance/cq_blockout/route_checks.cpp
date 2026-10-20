#include "game/pch.h"
#include "scripts/acceptance/cq_blockout/fixtures.h"

namespace cq_blockout_acceptance
{
    void spawn_checks(Services& s)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const bool right = team == PlayerTeam::a;
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            const std::string home = right ? "spawn_a_floor" : "spawn_b_floor";
            for (size_t index = 0; index < points.size(); ++index)
            {
                const RectangleF body(points[index], s.tuning.player().size);
                for (size_t other = index + 1; other < points.size(); ++other)
                {
                    const auto overlap = narrow_phase(body, RectangleF(points[other], s.tuning.player().size));
                    require(!overlap, "CQ same-team spawn bodies overlap");
                }
                Run run(s, points[index], team);
                run.settle();
                require(run.support() == home, "CQ spawn did not settle on its authored home floor");
                for (int death = 0; death < 2; ++death)
                {
                    run.kill();
                    run.tick();
                    require(run.player->state() == PlayerState::dead, "CQ damage fixture missed death");
                    int elapsed = 0;
                    while (run.player->state() == PlayerState::dead && elapsed++ < 600)
                    {
                        run.tick(input(right ? 1.0f : -1.0f, true, true, true));
                    }
                    require(run.player->state() == PlayerState::alive, "CQ spawn never respawned");
                    run.death_expected = false;
                    const int takeoffs = run.takeoffs;
                    for (int tick = 0; tick < 30; ++tick) { run.tick(input(0.0f, false, true)); }
                    require(run.support() == home && run.takeoffs == takeoffs &&
                        approximately(run.player->rectangle().x, points[index].x) &&
                        approximately(run.player->rectangle().y, points[index].y),
                        "CQ respawn moved, jumped with carried input or lost its home support");
                    run.tick();
                }
                // The lower exit has no jump requirement: release into the
                // ground route, then walk under the protective baffle/deck.
                for (int tick = 0; tick < 90; ++tick) { run.tick(input(0.0f, false, false, true)); }
                require(approximately(run.player->rectangle().bottom(), s.map.named("boundary_floor").rectangle.top()),
                    "CQ spawn drop did not reach the lower walking route");
                const int exit_start = run.ticks;
                drive_to(run, right ? 1800.0f : bounds.left() + bounds.right() - 1800.0f);
                require(run.support() == "boundary_floor", "CQ lower spawn exit did not reconnect to scoring ground");
                std::cout << "MEASURE CQ spawn team=" << (right ? 'a' : 'b') << " index=" << index <<
                    ": two deaths, held-input-safe respawn, lower exit walk_ticks=" << run.ticks - exit_start << '\n';
                if (index == 0)
                {
                    // Return with the same player that just escaped. The
                    // recovery platform supplies the two ordinary rises;
                    // no position injection restarts the return trial.
                    const auto x = [&](float value)
                        { return right ? value : bounds.left() + bounds.right() - value; };
                    const int return_start = run.ticks;
                    drive_to(run, x(700.0f));
                    jump_to(run, s.map.named(right ? "spawn_a_recovery" : "spawn_b_recovery"), x(500.0f), 0);
                    jump_to(run, s.map.named(home), x(500.0f), 0);
                    require(run.support() == home, "CQ lower recovery route did not return to its home room");
                    std::cout << "MEASURE CQ lower home return team=" << (right ? 'a' : 'b') <<
                        " ticks=" << run.ticks - return_start << " support=" << run.support() << '\n';
                }
            }

            // One representative from each team also drives the upper exit;
            // all initial positions above are independently checked.
            Run upper(s, points.front(), team);
            upper.settle();
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            drive_to(upper, x(550.0f));
            jump_to(upper, s.map.named(right ? "spawn_a_inner_step" : "spawn_b_inner_step"), x(750.0f), 0);
            jump_to(upper, s.map.named(right ? "spawn_a_baffle" : "spawn_b_baffle"), x(900.0f), 0);
            drive_to(upper, x(1050.0f));
            require(upper.support() == (right ? "spawn_a_outer_step" : "spawn_b_outer_step"),
                "CQ upper exit skipped its outer landing");
            drive_to(upper, x(1280.0f));
            require(upper.support() == (right ? "approach_a_bridge" : "approach_b_bridge"),
                "CQ upper exit did not reach the approach bridge");
            drive_to(upper, x(1680.0f));
            require(upper.support() == "lower_paint", "CQ upper spawn exit did not reconnect to the main deck");
            std::cout << "MEASURE CQ upper spawn exit team=" << (right ? 'a' : 'b') << " total_ticks=" <<
                upper.ticks << " support=" << upper.support() << '\n';
            const int return_start = upper.ticks;
            jump_to(upper, s.map.named(right ? "approach_a_bridge" : "approach_b_bridge"), x(1280.0f), 0);
            jump_to(upper, s.map.named(right ? "spawn_a_outer_step" : "spawn_b_outer_step"), x(1050.0f), 0);
            jump_to(upper, s.map.named(right ? "spawn_a_baffle" : "spawn_b_baffle"), x(900.0f), 0);
            drive_to(upper, x(750.0f));
            require(upper.support() == (right ? "spawn_a_inner_step" : "spawn_b_inner_step"),
                "CQ upper home return skipped the inner landing");
            drive_to(upper, x(550.0f));
            require(upper.support() == home, "CQ upper approach did not return to its home room");
            std::cout << "MEASURE CQ upper home return team=" << (right ? 'a' : 'b') <<
                " ticks=" << upper.ticks - return_start << " support=" << upper.support() << '\n';
        }
        std::cout << "PASS CQ all eight full-body spawns and repeated respawns, all lower exits, paired upper exits "
            "and lower/upper home returns with the same driven players\n";
    }

    void route_checks(Services& s)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        for (const float direction : { 1.0f, -1.0f })
        {
            const bool right = direction > 0.0f;
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            const Surface& start = s.map.named(right ? "boundary_floor_left" : "boundary_floor_right");
            {
                Run run(s, standing_on(s, start, x(1100.0f)), right ? PlayerTeam::a : PlayerTeam::b);
                run.settle();
                const int before = run.ticks;
                drive_to(run, x(4900.0f));
                require(run.takeoffs == 0 && approximately(run.player->rectangle().bottom(), start.rectangle.top()),
                    "CQ under-deck bypass requires a jump or left its level ground");
                std::cout << "MEASURE CQ lower bypass dir=" << direction << " ticks=" << run.ticks - before <<
                    " seconds=" << fixed(static_cast<float>(run.ticks - before) * dt) << '\n';
            }
            for (const int delay : { -2, 0, 2 })
            {
                Run run(s, standing_on(s, start, x(1100.0f)), right ? PlayerTeam::a : PlayerTeam::b);
                run.settle();
                const int before = run.ticks;
                jump_to(run, s.map.named(right ? "lower_left_platform" : "lower_right_platform"), x(1350.0f), delay);
                jump_to(run, s.map.named("lower_paint"), x(1650.0f), delay);
                drive_to(run, x(2800.0f));
                jump_to(run, s.map.named("lower_paint"), x(3200.0f), delay);
                drive_to(run, x(4350.0f));
                drive_to(run, x(4650.0f));
                require(run.support() == (right ? "lower_right_platform" : "lower_left_platform"),
                    "CQ main route missed the far lower step");
                drive_to(run, x(4900.0f));
                require(approximately(run.player->rectangle().bottom(), start.rectangle.top()),
                    "CQ main route did not reconnect to the lower loop");
                std::cout << "MEASURE CQ main loop dir=" << direction << " press_delay=" << delay <<
                    " ticks=" << run.ticks - before << " takeoffs=" << run.takeoffs << '\n';

                Run upper(s, standing_on(s, start, x(1100.0f)), right ? PlayerTeam::a : PlayerTeam::b);
                upper.settle();
                const int upper_before = upper.ticks;
                jump_to(upper, s.map.named(right ? "lower_left_platform" : "lower_right_platform"), x(1350.0f), delay);
                jump_to(upper, s.map.named("lower_paint"), x(1650.0f), delay);
                drive_to(upper, x(2100.0f));
                jump_to(upper, s.map.named(right ? "upper_left_platform" : "upper_right_platform"), x(2350.0f), delay);
                jump_to(upper, s.map.named("upper_paint"), x(2650.0f), delay);
                drive_to(upper, x(3350.0f));
                drive_to(upper, x(3650.0f));
                require(upper.support() == (right ? "upper_right_platform" : "upper_left_platform"),
                    "CQ upper bypass missed the far upper step");
                drive_to(upper, x(3900.0f));
                require(upper.support() == "lower_paint", "CQ upper bypass did not reconnect to the main deck");
                drive_to(upper, x(4650.0f));
                drive_to(upper, x(4900.0f));
                require(approximately(upper.player->rectangle().bottom(), start.rectangle.top()),
                    "CQ upper bypass did not reconnect to the lower loop");
                std::cout << "MEASURE CQ upper loop dir=" << direction << " press_delay=" << delay <<
                    " ticks=" << upper.ticks - upper_before << " takeoffs=" << upper.takeoffs << '\n';
            }
        }
        std::cout << "PASS CQ driven lower, main and upper routes in both directions; ordinary jumps tolerate +/-2 press ticks\n";
    }
}
