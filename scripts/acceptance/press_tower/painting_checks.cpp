#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"

namespace press_tower_acceptance
{
    namespace
    {
        // drive_to with the trigger held and the aim fixed forward-down. The
        // walk itself is the same steering, so the route stays jump-free.
        void drive_firing(Run& run, float centre_x, float angle, int limit = 900)
        {
            for (int tick = 0; tick < limit; ++tick)
            {
                if (std::fabs(run.player->center().x - centre_x) <= 8.0f &&
                    run.player->velocity().x == 0.0f && supported(*run.player)) { return; }
                PlayerInputData actions = aimed(angle, input(steering(run, centre_x)));
                actions.primary_shoot = true;
                run.tick(actions);
            }
            throw std::runtime_error("Press Tower firing walk did not reach x=" + fixed(centre_x) +
                "; at " + fixed(run.player->center().x) + " on " + run.support());
        }

        // Stand still with the trigger released so the next leg starts with
        // ammunition and every shot already in flight has landed.
        void recover(Run& run, int count = 180)
        {
            for (int tick = 0; tick < count; ++tick) { run.tick(); }
        }

        void fire_in_place(Run& run, float angle, int count = 45)
        {
            for (int tick = 0; tick < count; ++tick)
            {
                PlayerInputData actions = aimed(angle);
                actions.primary_shoot = true;
                run.tick(actions);
            }
            recover(run, 120);
        }
    }

    void painting_checks(Services& s)
    {
        for (const bool right : { true, false })
        {
            const PlayerTeam team = right ? PlayerTeam::a : PlayerTeam::b;
            const PlayerTeam enemy = right ? PlayerTeam::b : PlayerTeam::a;
            const auto x = [&](float value) { return reflected_x(s, right, value); };
            const auto side = [&](const std::string& base, bool own) { return base + (own == right ? "_a" : "_b"); };
            // Forward and down along the direction of travel.
            const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;

            // R2 while firing: three legs, refilling between them, so the
            // far yard is reached with ammunition to spend on it.
            {
                Run run(s, standing_on(s, s.map.named(side("floor_home", true)), x(260.0f)), team);
                run.settle();
                drive_firing(run, x(920.0f), angle);
                recover(run);
                drive_firing(run, 1800.0f, angle);
                recover(run);
                drive_firing(run, x(2680.0f), angle);
                recover(run);
                require(run.takeoffs == 0, "Press Tower R2 firing crossing needed a jump");
                require(run.paid_events > 0, "Press Tower R2 firing crossing never paid for fire");
                for (const std::string& name : { side("floor_yard", true), std::string("press_bed"), side("floor_yard", false) })
                {
                    require(run.team_area(team, name) > 0.0f,
                        "Press Tower R2 firing crossing left '" + name + "' unpainted");
                }
                require(run.team_area(enemy) == 0.0f, "Press Tower R2 firing crossing credited the wrong team");
                std::cout << "MEASURE Press Tower painting R2 team=" << (right ? 'a' : 'b') <<
                    " near_yard=" << fixed(run.team_area(team, side("floor_yard", true))) <<
                    " bed=" << fixed(run.team_area(team, "press_bed")) <<
                    " far_yard=" << fixed(run.team_area(team, side("floor_yard", false))) <<
                    " total=" << fixed(run.team_area(team)) << " paid_events=" << run.paid_events << " takeoffs=0\n";
            }

            // R3 while firing: the stair route's taps, firing from each tread.
            {
                Run run(s, standing_on(s, s.map.named(side("catwalk", true)), x(900.0f)), team);
                run.settle();
                const std::array<std::pair<const char*, float>, 3> treads{ { { "stair_1", 1080.0f },
                    { "stair_2", 1240.0f }, { "stair_3", 1400.0f } } };
                float stairs = 0.0f;
                for (const auto& [base, centre] : treads)
                {
                    jump_to(run, s.map.named(side(base, true)), x(centre), 0, 1);
                    fire_in_place(run, angle);
                }
                for (const auto& [base, centre] : treads)
                {
                    (void)centre;
                    stairs += run.team_area(team, side(base, true));
                }
                require(run.paid_events > 0 && stairs > 0.0f, "Press Tower R3 firing climb painted no stair");
                require(run.takeoffs == 3, "Press Tower R3 firing climb did not use exactly its three stair taps");
                require(run.team_area(enemy) == 0.0f, "Press Tower R3 firing climb credited the wrong team");
                std::cout << "MEASURE Press Tower painting R3 team=" << (right ? 'a' : 'b') <<
                    " stair_area=" << fixed(stairs) << " total=" << fixed(run.team_area(team)) <<
                    " paid_events=" << run.paid_events << " takeoffs=" << run.takeoffs << '\n';
            }
        }
        std::cout << "PASS Press Tower sprayer paints both yards and the bed along the jump-free R2 crossing, "
            "and the stair treads along R3, for both teams\n";
    }
}
