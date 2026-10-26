#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"

namespace turbulence_blockout_acceptance
{
    namespace
    {
        void drop(Run& run, const std::string& destination)
        {
            const int takeoffs = run.takeoffs;
            for (int tick = 0; tick < 90; ++tick) { run.tick(input(0.0f, false, false, true)); }
            support_is(run, destination);
            require(run.takeoffs == takeoffs, "Drop unexpectedly needed a jump");
        }
    }

    void routes(Services& services)
    {
        for (const bool right : { true, false })
        {
            const PlayerTeam team = right ? PlayerTeam::a : PlayerTeam::b;
            const auto x = [right](float value) { return reflected(right, value); };
            const auto name = [right](const std::string& value) { return value + (right ? "_a" : "_b"); };
            const float direction = right ? 1.0f : -1.0f;
            const auto spawns = right ? services.info.team_a_spawns() : services.info.team_b_spawns();
            require(spawns.size() == 4, "Each team must retain four qualified spawns");
            for (std::size_t index = 0; index < spawns.size(); ++index)
            {
                Run run(services, spawns[index], team); run.settle();
                support_is(run, name("home_floor"));
                for (std::size_t other = 0; other < index; ++other)
                {
                    require(std::fabs(spawns[index].x - spawns[other].x) >= services.tuning.player().size.x,
                        "Authored same-team spawn bodies overlap");
                }
                for (int death = 0; death < 2; ++death)
                {
                    run.death_expected = true; run.player->kill(); run.tick();
                    require(run.player->state() == PlayerState::dead, "Explicit lifecycle fixture missed death");
                    const int before = run.takeoffs;
                    int wait = 0;
                    while (run.player->state() == PlayerState::dead && wait++ < 600)
                    { run.tick(input(direction, true, true, true)); }
                    require(run.player->state() == PlayerState::alive, "Stage did not respawn a killed player");
                    run.death_expected = false;
                    for (int tick = 0; tick < 30; ++tick) { run.tick(input(0.0f, false, true)); }
                    support_is(run, name("home_floor"));
                    require(run.takeoffs == before && std::fabs(run.player->rectangle().x - spawns[index].x) < tolerance &&
                        std::fabs(run.player->rectangle().y - spawns[index].y) < tolerance,
                        "Carried death/respawn input moved a home body or invented a takeoff");
                    run.tick();
                }
                drop(run, "ground");
                std::cout << "MEASURE Turbulence spawn team=" << (right ? 'a' : 'b') << " index=" << index <<
                    " two lifecycle deaths/held-input-safe respawns and jump-free home drop\n";
            }
            {
                Run run(services, standing(services, services.named("ground"), x(300)), team); run.settle();
                const int start = run.ticks;
                drive(run, x(1420)); support_is(run, name("lower_bed"));
                const int bed = run.ticks;
                drive(run, 2000); support_is(run, "ground");
                const int centre = run.ticks;
                drive(run, x(3700)); support_is(run, "ground");
                require(run.takeoffs == 0, "Required Turbulence crossing requested a jump");
                std::cout << "MEASURE Turbulence lower crossing team=" << (right ? 'a' : 'b') << " near_bed=" << bed - start <<
                    " centre=" << centre - start << " far_home=" << run.ticks - start << " ticks takeoffs=0\n";
            }
            for (const int delay : { -2, 0, 2 })
            {
                // A cold optional departure and the longer static return to
                // height both use this side step. Later movers do not gate it.
                Run climb(services, standing(services, services.named(name("home_floor")), x(560)), team);
                climb.settle();
                jump(climb, name("side_step"), x(720), delay, 1);
                jump(climb, name("upper_approach"), x(1000), delay);
                require(climb.takeoffs == 2, "Upper departure changed its tap/held jump contract");
                const int departure_ticks = climb.ticks;
                // A static miss/retreat: walk back off the upper receiver, then
                // keep down held until the original ground catches the body.
                drive(climb, x(830)); support_is(climb, name("side_step"));
                drive(climb, x(540));
                for (int tick = 0; tick < 100; ++tick) { climb.tick(input(0.0f, false, false, true)); }
                support_is(climb, "ground");
                drive(climb, x(300));
                jump(climb, name("home_floor"), x(300), delay);
                drive(climb, x(560));
                jump(climb, name("side_step"), x(720), delay, 1);
                jump(climb, name("upper_approach"), x(1000), delay);
                std::cout << "MEASURE Turbulence departure/recovery team=" << (right ? 'a' : 'b') << " delay=" << delay <<
                    " upper_tick=" << departure_ticks << " recovered_tick=" << climb.ticks << '\n';

                Run roof(services, standing(services, services.named(name("home_floor")), x(250)), team);
                roof.settle(); jump(roof, name("home_roof"), x(250), delay);

                for (const float speed : { 0.0f, 400.0f, 800.0f })
                {
                    const float start = speed == 0 ? 1210.0f : speed == 400 ? 1190.0f : 1160.0f;
                    Run crossing(services, standing(services, services.named(name("upper_approach")), x(start)), team,
                        direction * speed);
                    crossing.tick(input(speed == 0 ? 0.0f : direction));
                    support_is(crossing, name("upper_approach"));
                    const float actual_speed = crossing.player->velocity().x;
                    jump(crossing, name("crossover"), x(1490), delay);
                    drive(crossing, x(1880));
                    // The 120-wide baffle is a controlled tap/reversal landing,
                    // never advertised as a full-speed braking destination.
                    jump(crossing, "centre_baffle", 2000, delay, 1);
                    drive(crossing, x(2200));
                    support_is(crossing, right ? "crossover_b" : "crossover_a");
                    require(crossing.takeoffs == 2, "Fixed crossover acquired an undeclared jump");
                    std::cout << "MEASURE Turbulence fixed crossing team=" << (right ? 'a' : 'b') << " delay=" << delay <<
                        " initial_vx=" << actual_speed << " ticks=" << crossing.ticks << " takeoffs=" << crossing.takeoffs << '\n';
                }
            }
        }
        std::cout << "PASS eight supported spawns, repeated respawns, two departures, both lower crossings, "
            "optional tap/held edges and longer home-loop recovery; no post-construction position injection\n";
    }
}
