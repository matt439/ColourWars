#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"

namespace press_tower_acceptance
{
    namespace
    {
        constexpr std::array<int, 3> delays{ -2, 0, 2 };

        // Team A's authored names and X; `right == false` uses the mirror.
        struct Side
        {
            const Services& s;
            bool right;
            float x(float value) const { return reflected_x(this->s, this->right, value); }
            std::string name(const std::string& base) const { return base + (this->right ? "_a" : "_b"); }
            const Surface& surface(const std::string& base) const { return this->s.map.named(this->name(base)); }
            PlayerTeam team() const { return this->right ? PlayerTeam::a : PlayerTeam::b; }
            char letter() const { return this->right ? 'a' : 'b'; }
        };

        // R3: catwalk, three stair risers, a level walk onto the shoulder,
        // then the press-head riser. hold_ticks 1 is the tap family.
        void stair_climb(Services& s, const Side& side, int delay, int hold_ticks)
        {
            Run run(s, standing_on(s, side.surface("catwalk"), side.x(900.0f)), side.team());
            run.settle();
            const int start = run.ticks;
            jump_to(run, side.surface("stair_1"), side.x(1080.0f), delay, hold_ticks);
            jump_to(run, side.surface("stair_2"), side.x(1240.0f), delay, hold_ticks);
            jump_to(run, side.surface("stair_3"), side.x(1400.0f), delay, hold_ticks);
            const int takeoffs = run.takeoffs;
            drive_to(run, side.x(1560.0f));
            require_support(run, side.name("shoulder"), "R3 level stair_3-to-shoulder walk");
            require(run.takeoffs == takeoffs, "Press Tower R3 stair_3-to-shoulder walk needed a jump");
            jump_to(run, s.map.named("press_head"), side.x(1710.0f), delay, hold_ticks);
            require(run.takeoffs == 4, "Press Tower R3 did not use exactly its four declared rises");
            std::cout << "MEASURE Press Tower R3 stair_climb team=" << side.letter() << " delay=" << delay <<
                " hold_ticks=" << hold_ticks << " ticks=" << run.ticks - start << " takeoffs=" << run.takeoffs << '\n';
        }
    }

    void spawn_checks(Services& s)
    {
        for (const bool right : { true, false })
        {
            const Side side{ s, right };
            const auto points = right ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            const std::string home = side.name("home_floor");
            require(points.size() == 4, "Press Tower needs four tested spawn positions per team");
            for (size_t index = 0; index < points.size(); ++index)
            {
                Run run(s, points[index], side.team());
                run.settle();
                require_support(run, home, "spawn settling");
                for (int death = 0; death < 2; ++death)
                {
                    run.kill();
                    run.tick();
                    require(run.player->state() == PlayerState::dead, "Press Tower damage fixture missed death");
                    // Counted from before the dead ticks, so a press carried
                    // into the respawn tick itself is seen too.
                    const int takeoffs = run.takeoffs;
                    int elapsed = 0;
                    while (run.player->state() == PlayerState::dead && elapsed++ < 600)
                    {
                        run.tick(input(right ? 1.0f : -1.0f, true, true, true));
                    }
                    require(run.player->state() == PlayerState::alive, "Press Tower player never respawned");
                    run.death_expected = false;
                    for (int tick = 0; tick < 30; ++tick) { run.tick(input(0.0f, false, true)); }
                    require_support(run, home, "respawn support");
                    require(run.takeoffs == takeoffs &&
                        approximately(run.player->rectangle().x, points[index].x) &&
                        approximately(run.player->rectangle().y, points[index].y),
                        "Press Tower respawn moved or jumped with carried held input");
                    run.tick();
                }
                // R1, required: the departure is a drop, never a jump.
                const int takeoffs = run.takeoffs;
                const int departure_start = run.ticks;
                for (int tick = 0; tick < 90; ++tick) { run.tick(input(0.0f, false, false, true)); }
                require_support(run, side.name("floor_home"), "R1 required home drop");
                require(run.takeoffs == takeoffs, "Press Tower R1 required departure requested a jump");
                std::cout << "MEASURE Press Tower spawn team=" << side.letter() << " index=" << index <<
                    ": two damage deaths, held-input-safe respawn; R1 drop_ticks=" << run.ticks - departure_start << '\n';
            }
        }
        std::cout << "PASS Press Tower all eight full-body spawns, two deaths/respawns each, "
            "and jump-free R1 drops onto the home ground\n";
    }

    void route_checks(Services& s)
    {
        for (const bool right : { true, false })
        {
            const Side side{ s, right };
            const Side opposite{ s, !right };

            // R2, required: home ground to the far home ground over both
            // ramps and the bed, with no takeoff in either direction.
            {
                Run run(s, standing_on(s, side.surface("floor_home"), side.x(260.0f)), side.team());
                run.settle();
                const int start = run.ticks;
                drive_to(run, 1800.0f);
                require_support(run, "press_bed", "R2 required bed arrival");
                const int bed_ticks = run.ticks - start;
                drive_to(run, side.x(3340.0f), 1200);
                require_support(run, opposite.name("floor_home"), "R2 required far home arrival");
                require(run.takeoffs == 0, "Press Tower R2 required crossing needed a jump");
                std::cout << "MEASURE Press Tower R2 required_crossing team=" << side.letter() <<
                    " home_to_bed_ticks=" << bed_ticks << " home_to_far_home_ticks=" << run.ticks - start <<
                    " takeoffs=0\n";
            }

            for (const int delay : delays)
            {
                // R3, both input families.
                stair_climb(s, side, delay, 1);
                stair_climb(s, side, delay, 120);

                // R4, hoist: two held rises through the one-way frame and
                // shoulder, then the tap onto the press head.
                {
                    Run run(s, standing_on(s, s.map.named("press_bed"), side.x(1534.0f)), side.team());
                    run.settle();
                    const int start = run.ticks;
                    jump_to(run, side.surface("frame"), side.x(1534.0f), delay);
                    jump_to(run, side.surface("shoulder"), side.x(1534.0f), delay);
                    jump_to(run, s.map.named("press_head"), side.x(1710.0f), delay, 1);
                    require(run.takeoffs == 3, "Press Tower R4 did not use exactly its three declared rises");
                    std::cout << "MEASURE Press Tower R4 hoist team=" << side.letter() << " delay=" << delay <<
                        " ticks=" << run.ticks - start << " takeoffs=" << run.takeoffs << '\n';
                }

                // R5, roof shortcut: held to the roof, walk onto the gantry,
                // held running jump across the gap onto stair_2.
                {
                    Run run(s, standing_on(s, side.surface("home_floor"), side.x(260.0f)), side.team());
                    run.settle();
                    const int start = run.ticks;
                    jump_to(run, side.surface("home_roof"), side.x(260.0f), delay);
                    const int takeoffs = run.takeoffs;
                    drive_to(run, side.x(960.0f));
                    require_support(run, side.name("gantry"), "R5 roof-to-gantry walk");
                    require(run.takeoffs == takeoffs, "Press Tower R5 roof-to-gantry walk needed a jump");
                    jump_to(run, side.surface("stair_2"), side.x(1240.0f), delay);
                    require(run.takeoffs == 2, "Press Tower R5 did not use exactly its two declared jumps");
                    std::cout << "MEASURE Press Tower R5 roof_shortcut team=" << side.letter() << " delay=" << delay <<
                        " ticks=" << run.ticks - start << " takeoffs=" << run.takeoffs << '\n';
                }

                // R6-R11: single optional edges; jump_to requires each to
                // land from exactly one takeoff.
                {
                    Run run(s, standing_on(s, side.surface("stair_1"), side.x(1080.0f)), side.team());
                    run.settle();
                    jump_to(run, side.surface("gantry"), side.x(900.0f), delay, 1);
                }
                {
                    Run run(s, standing_on(s, side.surface("catwalk"), side.x(760.0f)), side.team());
                    run.settle();
                    jump_to(run, side.surface("gantry"), side.x(760.0f), delay);
                }
                {
                    Run run(s, standing_on(s, side.surface("frame"), side.x(1520.0f)), side.team());
                    run.settle();
                    jump_to(run, side.surface("stair_3"), side.x(1400.0f), delay);
                }
                {
                    Run run(s, standing_on(s, s.map.named("press_head"), side.x(1710.0f)), side.team());
                    run.settle();
                    jump_to(run, s.map.named("press_crown"), 1800.0f, delay, 1);
                }
                {
                    Run run(s, standing_on(s, s.map.named("press_head"), side.x(1710.0f)), side.team());
                    run.settle();
                    jump_to(run, s.map.named("press_head"), side.x(1890.0f), delay, 1);
                    require(std::fabs(run.player->center().x - side.x(1890.0f)) <= 8.0f,
                        "Press Tower R10 crown crossing did not stop on the far half");
                }
                {
                    Run run(s, standing_on(s, side.surface("floor_yard"), side.x(760.0f)), side.team());
                    run.settle();
                    jump_to(run, side.surface("catwalk"), side.x(760.0f), delay);
                }
                {
                    Run run(s, standing_on(s, side.surface("floor_home"), side.x(260.0f)), side.team());
                    run.settle();
                    jump_to(run, side.surface("home_floor"), side.x(260.0f), delay);
                }
                std::cout << "MEASURE Press Tower R6-R11 single edges team=" << side.letter() << " delay=" << delay <<
                    ": stair_1->gantry tap, catwalk->gantry held, frame->stair_3 held, head->crown tap, "
                    "head west->east tap, yard->catwalk held, home ground->home floor held\n";
            }

            // R12, recovery: partial retreat tier by tier, zero takeoffs.
            {
                Run run(s, standing_on(s, s.map.named("press_head"), side.x(1710.0f)), side.team());
                run.settle();
                const int start = run.ticks;
                drive_to(run, side.x(1560.0f));
                require_support(run, side.name("shoulder"), "R12 walk-off onto the shoulder");
                drop_once(run, side.name("frame"));
                drop_once(run, "press_bed");
                drive_to(run, side.x(900.0f));
                require_support(run, side.name("floor_yard"), "R12 ramp walk-down");
                require(run.takeoffs == 0, "Press Tower R12 retreat needed a jump");
                std::cout << "MEASURE Press Tower R12 retreat team=" << side.letter() <<
                    " ticks=" << run.ticks - start << " takeoffs=0\n";
            }

            // R13, recovery: walk down the stair, zero takeoffs.
            {
                Run run(s, standing_on(s, side.surface("shoulder"), side.x(1560.0f)), side.team());
                run.settle();
                const int start = run.ticks;
                drive_to(run, side.x(1240.0f));
                require_support(run, side.name("stair_2"), "R13 walk-down to stair_2");
                drive_to(run, side.x(1080.0f));
                require_support(run, side.name("stair_1"), "R13 walk-down to stair_1");
                drive_to(run, side.x(760.0f));
                require_support(run, side.name("catwalk"), "R13 walk-down to the catwalk");
                require(run.takeoffs == 0, "Press Tower R13 stair walk-down needed a jump");
                std::cout << "MEASURE Press Tower R13 stair_walk_down team=" << side.letter() <<
                    " ticks=" << run.ticks - start << " takeoffs=0\n";
            }
        }
        std::cout << "PASS Press Tower required R1/R2 crossings use no jumps in both directions; optional "
            "R3-R11 climbs land with +/-2 press ticks (R3 in both tap and held families); R12/R13 retreats "
            "reach named tiers with no takeoff. Scripted inputs establish feasibility, not comfort or pressure\n";
    }
}
