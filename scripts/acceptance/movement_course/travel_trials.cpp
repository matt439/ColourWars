#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"

// Ground travel from rest and the analog stick mapped onto it.
namespace movement_course_acceptance
{
    void from_rest_travel(const Services& s)
    {
        const PlayerTuning& tuning = s.tuning.player();
        const CourseData& course = s.course;
        struct Lane
        {
            const char* label;
            Vector2F spawn;
            PlayerTeam team;
            const Surface* runway;
            float direction;
        };
        for (const Lane& lane : {
            Lane{ "team_a spawn 1 toward the short gap", course.info.team_a_spawns().front(),
                PlayerTeam::a, &course.named("spawn_a_runway"), 1.0f },
            Lane{ "team_b spawn 4 toward the return step", course.info.team_b_spawns().back(),
                PlayerTeam::b, &course.named("spawn_b_runway"), -1.0f } })
        {
            const float d = lane.direction;
            Run run(s, lane.spawn, Vector2F::ZERO, WeaponType::sprayer, lane.team, lane.spawn);
            run.settle(30);
            require(run.support() == lane.runway->name, "an authored spawn did not settle on its runway");
            const float end = d > 0.0f ? lane.runway->rectangle.right() : lane.runway->rectangle.left();
            const float start = run.player->center().x;
            const float start_margin = d * (end - run.front(d));
            int ticks_to_max = -1;
            float distance_to_max = 0.0f;
            float margin_at_max = 0.0f;
            float previous = 0.0f;
            for (int tick = 1; tick <= 240 && ticks_to_max < 0 && d * (end - run.front(d)) > 0.0f; ++tick)
            {
                run.tick(input(false, false, d));
                const float speed = d * run.player->velocity().x;
                require(speed >= previous && speed <= tuning.max_velocity.x,
                    "held movement from rest decelerated or exceeded its cap");
                previous = speed;
                if (speed == tuning.max_velocity.x)
                {
                    ticks_to_max = tick;
                    distance_to_max = d * (run.player->center().x - start);
                    margin_at_max = d * (end - run.front(d));
                }
            }
            const float released = run.player->center().x;
            int stop_ticks = 0;
            for (; run.player->velocity().x != 0.0f && stop_ticks < 120; ++stop_ticks) { run.tick(); }
            require(run.player->velocity().x == 0.0f, "releasing movement did not stop the player");
            for (int tick = 0; tick < 10; ++tick)
            {
                run.tick();
                require(run.player->velocity().x == 0.0f, "a released stop crept afterwards");
            }
            std::cout << "MEASURE from-rest " << lane.label << ": runway_ahead=" << fixed(start_margin)
                << " ticks_to_max=" << ticks_to_max << " (" << fixed(static_cast<float>(ticks_to_max) * dt, 3)
                << " s) distance_to_max=" << fixed(distance_to_max)
                << " runway_left_at_max=" << fixed(margin_at_max)
                << " | release: ticks_to_stop=" << stop_ticks
                << " stop_distance=" << fixed(d * (run.player->center().x - released))
                << " runway_left_at_stop=" << fixed(d * (end - run.front(d)))
                << " support=" << run.support() << '\n';
        }
    }

    void analog_movement(const Services& s)
    {
        const Surface& lane = s.course.named("recovery_floor");
        const float max_speed = s.tuning.player().max_velocity.x;
        std::cout << "NOTE analog magnitudes use a COPY of PlayerInput::pad_input's mapping: "
            "apply_deadzone(raw, stick_deadzone=" << s.tuning.input().movement_deadzone
            << "), drop when raw y > drop_down_analog_threshold.\n";
        std::vector<std::vector<float>> traces;
        const std::vector<float> raws{ 0.45f, 0.55f, 0.65f, 0.75f, 0.9f, 1.0f };
        for (const float raw : raws)
        {
            const float movement = pad_movement(raw, s.tuning);
            Run run(s, standing_on(s, lane, lane.rectangle.left() + 100.0f));
            run.settle();
            const float start = run.player->center().x;
            std::vector<float> trace;
            int ticks_to_max = -1;
            float distance_to_max = 0.0f;
            for (int tick = 1; tick <= 900; ++tick)
            {
                run.tick(input(false, false, movement));
                trace.push_back(run.player->velocity().x);
                if (ticks_to_max < 0 && run.player->velocity().x == max_speed)
                {
                    ticks_to_max = tick;
                    distance_to_max = run.player->center().x - start;
                }
                if ((ticks_to_max >= 0 && tick >= 60) ||
                    run.front(1.0f) > lane.rectangle.right() - 100.0f) { break; }
            }
            traces.push_back(trace);
            if (movement == 0.0f)
            {
                require(std::all_of(trace.begin(), trace.end(), [](float v) { return v == 0.0f; }),
                    "stick travel inside the deadzone moved the player");
            }
            const auto at = [&trace](size_t tick) { return tick <= trace.size() ? trace[tick - 1] : 0.0f; };
            std::cout << "MEASURE analog raw=" << fixed(raw, 2) << " x_movement=" << fixed(movement, 2)
                << " speed@10=" << fixed(at(10)) << " speed@30=" << fixed(at(30))
                << " speed@60=" << fixed(at(60)) << " ticks_to_max=" << ticks_to_max
                << " distance_to_max=" << fixed(distance_to_max) << '\n';
        }
        for (size_t index = 0; index + 1 < traces.size(); ++index)
        {
            const size_t length = std::min(traces[index].size(), traces[index + 1].size());
            for (size_t tick = 0; tick < length; ++tick)
            {
                require(traces[index][tick] <= traces[index + 1][tick] + 0.001f,
                    "a larger stick magnitude was slower than a smaller one");
            }
        }

        for (const float d : { 1.0f, -1.0f })
        {
            int release_ticks = 0;
            float release_distance = 0.0f;
            int previous_ticks = 300;
            float previous_distance = std::numeric_limits<float>::max();
            for (const float raw : { 0.0f, 0.55f, 0.65f, 0.75f, 0.9f, 1.0f })
            {
                Run run(s, standing_on(s, lane, lane.rectangle.center().x), { d * max_speed, 0.0f });
                run.settle(1, input(false, false, d));
                require(d * run.player->velocity().x == max_speed,
                    "braking fixture did not start at full speed");
                const float movement = -d * pad_movement(raw, s.tuning);
                const float start = run.player->center().x;
                float previous_speed = max_speed;
                int ticks = 0;
                for (; d * run.player->velocity().x > 0.0f && ticks < 300; ++ticks)
                {
                    run.tick(input(false, false, movement));
                    const float speed = d * run.player->velocity().x;
                    require(speed <= previous_speed && std::fabs(speed) <= max_speed,
                        "braking accelerated forward or exceeded the speed cap");
                    previous_speed = speed;
                }
                require(d * run.player->velocity().x <= 0.0f, "braking never reached zero speed");
                const float after = run.player->velocity().x;
                const float distance = d * (run.player->center().x - start);
                require(ticks <= previous_ticks && distance <= previous_distance + 0.01f,
                    "a stronger opposite stick took longer or farther to stop");
                previous_ticks = ticks;
                previous_distance = distance;
                if (movement == 0.0f)
                {
                    release_ticks = ticks;
                    release_distance = distance;
                    for (int tick = 0; tick < 10; ++tick)
                    {
                        run.tick();
                        require(run.player->velocity().x == 0.0f, "a released stick did not hold a stop");
                    }
                }
                else
                {
                    require(ticks <= release_ticks && distance <= release_distance + 0.01f,
                        "opposite stick braking was weaker than release braking");
                    require(std::fabs(after) <= s.tuning.player().x_acceleration *
                        std::fabs(movement) * dt + 0.001f,
                        "braking propelled the player past the available reverse acceleration");
                    float reverse_speed = -d * after;
                    const float crossing_speed = reverse_speed;
                    for (int tick = 0; tick < 90; ++tick)
                    {
                        run.tick(input(false, false, movement));
                        const float speed = -d * run.player->velocity().x;
                        require(speed >= reverse_speed && speed <= max_speed,
                            "held reversal oscillated, lost reverse speed or exceeded its cap");
                        reverse_speed = speed;
                    }
                    require(reverse_speed > crossing_speed,
                        "held opposite input did not build speed in the new direction");
                }
                std::cout << "MEASURE brake-from-800 dir=" << (d > 0.0f ? "right" : "left")
                    << ' ' << (movement == 0.0f ? "release" : "reverse")
                    << " raw=" << fixed(-d * raw, 2) << " x_movement=" << fixed(movement, 2)
                    << " ticks_to_zero=" << ticks << " distance=" << fixed(distance)
                    << " velocity_on_crossing=" << fixed(after) << '\n';
            }
        }

        const Surface& platform = s.course.named("drop_through_platform");
        const float threshold = s.tuning.player().drop_down_analog_threshold;
        for (const float raw : { threshold - 0.01f, threshold + 0.01f })
        {
            Run run(s, standing_on(s, platform, platform.rectangle.center().x));
            run.settle(2);
            require(run.player->move_state() == PlayerMoveState::on_drop_down_ground,
                "drop fixture did not stand on the authored platform");
            const bool drop = pad_drop(raw, s.tuning);
            run.tick(input(false, false, 0.0f, drop));
            const bool dropped = run.player->move_state() == PlayerMoveState::dropping_down;
            for (int tick = 0; tick < 90 && run.landings.empty(); ++tick)
            {
                run.tick(input(false, false, 0.0f, drop));
            }
            require(dropped == (raw > threshold), "the drop stick threshold was not respected");
            if (dropped)
            {
                require(!run.landings.empty() && run.landings.front().surface == "spawn_a_runway" &&
                    run.count(MovementFeedbackKind::takeoff) == 0,
                    "an analog drop did not land on the runway without a jump");
            }
            else
            {
                require(run.support() == platform.name, "a stick below the threshold dropped");
            }
            std::cout << "MEASURE analog-drop raw_y=" << fixed(raw, 2) << " threshold=" << fixed(threshold, 2)
                << " dropped=" << dropped << " support=" << run.support() << '\n';
        }
        std::cout << "PASS analog magnitudes: deadzone, monotonic speed, release stop, stronger opposite braking "
            "and bounded reversal both ways, drop threshold\n";
    }
}
