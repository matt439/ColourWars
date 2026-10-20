#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"

// Jump buffer, ledge grace and carried input: the ceiling, the drop-through
// platform, the lip across pause and death, and the authored spawns.
namespace movement_course_acceptance
{
    void ceiling(const Services& s)
    {
        const Surface& floor = s.course.named("running_landing");
        const Surface& block = s.course.named("low_ceiling");
        const float clearance = floor.rectangle.top() - block.rectangle.bottom() -
            s.tuning.player().size.y;
        const float speed = s.tuning.player().max_velocity.x;

        // Middle bonk, and the reference timing for buffered presses.
        int bonk = -1;
        int landing = -1;
        {
            Run run(s, standing_on(s, floor, block.rectangle.center().x));
            run.settle();
            const float feet = run.player->rectangle().bottom();
            float rise = 0.0f;
            for (int tick = 0; tick < 120 && landing < 0; ++tick)
            {
                run.tick(input(tick == 0, true));
                rise = std::max(rise, feet - run.player->rectangle().bottom());
                if (bonk < 0 && run.player->move_state() == PlayerMoveState::on_ceiling) { bonk = tick; }
                if (!run.landings.empty()) { landing = tick; }
            }
            require(bonk >= 0 && landing > bonk && run.landings.front().surface == floor.name &&
                run.player->velocity().y == 0.0f, "a middle bonk did not return to the floor");
            require(rise <= clearance + surface_tolerance, "a jump rose through the low ceiling");
            require(run.count(MovementFeedbackKind::takeoff) == 1 &&
                run.count(MovementFeedbackKind::landing) == 1,
                "a ceiling bonk emitted a landing or duplicated a cue");
            std::cout << "MEASURE ceiling middle bonk: clearance=" << fixed(clearance, 0) << " rise="
                << fixed(rise, 2) << " bonk_tick=" << bonk << " landing_tick=" << landing << '\n';
        }

        // A fresh press after the bonk launches only on the floor landing, and
        // only while the player's own buffer arithmetic says it is still live.
        const float buffer = s.tuning.player().jump_buffer_time;
        std::string fired;
        for (int press = bonk + 1; press <= landing; ++press)
        {
            Run run(s, standing_on(s, floor, block.rectangle.center().x));
            run.settle();
            int second = -1;
            for (int tick = 0; tick < landing + 30; ++tick)
            {
                run.tick(tick <= bonk ? input(tick == 0, true) : input(tick == press, false));
                if (second < 0 && run.count(MovementFeedbackKind::takeoff) == 2) { second = tick; }
            }
            const bool expected = window_open(buffer, landing - press);
            require(second == (expected ? landing : -1),
                "a press during a bonk jumped off the ceiling or missed its buffered floor launch");
            fired += " " + std::to_string(landing - press) + (second >= 0 ? ":launch" : ":none");
        }
        std::cout << "MEASURE ceiling buffered press (ticks before landing:result):" << fired << '\n';

        // Running passes in both directions, pressing at positions from outside
        // the near end to the far end. Brake once 20 past the far end.
        for (const float d : { 1.0f, -1.0f })
        {
            const float near_end = d > 0.0f ? block.rectangle.left() : block.rectangle.right();
            const float far_end = d > 0.0f ? block.rectangle.right() : block.rectangle.left();
            std::string row;
            for (const float press_at : { near_end - d * 60.0f, near_end + d * 20.0f,
                block.rectangle.center().x, far_end - d * 60.0f, far_end - d * 20.0f, far_end - d * 4.0f })
            {
                Run run(s, standing_on(s, floor, press_at - d * (80.0f + speed * dt)), { d * speed, 0.0f });
                run.settle(1, input(false, false, d));
                const float feet = run.player->rectangle().bottom();
                float rise = 0.0f;
                bool bonked = false;
                bool pressed = false;
                bool steering = true;
                for (int tick = 0; tick < 300; ++tick)
                {
                    const bool press = !pressed && d * (run.player->center().x - press_at) >= 0.0f;
                    pressed = pressed || press;
                    const RectangleF& body = run.player->rectangle();
                    if (!run.landings.empty() || (d > 0.0f ? body.left() > far_end + 20.0f :
                        body.right() < far_end - 20.0f)) { steering = false; }
                    run.tick(input(press, pressed && run.landings.empty(), steering ? d : 0.0f));
                    rise = std::max(rise, feet - run.player->rectangle().bottom());
                    bonked = bonked || run.player->move_state() == PlayerMoveState::on_ceiling;
                    if (!run.landings.empty() && supported(*run.player) &&
                        run.player->velocity().x == 0.0f) { break; }
                }
                require(pressed && run.count(MovementFeedbackKind::takeoff) == 1 &&
                    run.landings.size() == 1 && run.count(MovementFeedbackKind::landing) == 1,
                    "a ceiling pass took off twice or landed more than once");
                require(!bonked || rise <= clearance + surface_tolerance,
                    "a bonked jump continued through the ceiling");
                row += " press@" + fixed(d * (press_at - far_end), 0) + ":" + (bonked ? "bonk" : "clear") +
                    "/rise=" + fixed(rise, 0) + "/" + run.landings.front().surface;
            }
            std::cout << "MEASURE ceiling pass dir=" << (d > 0.0f ? "right" : "left")
                << " (press position relative to the far end):" << row << '\n';
        }
        std::cout << "PASS low ceiling: bonk returns once, buffered presses launch only from the floor, "
            "passes both ways take off once\n";
    }

    void drop_through(const Services& s)
    {
        const Surface& platform = s.course.named("drop_through_platform");
        const Surface& runway = s.course.named("spawn_a_runway");
        const float buffer = s.tuning.player().jump_buffer_time;
        const auto climb = [&](Run& run)
        {
            run.settle();
            require(jump_and_land(run, 0.0f) == platform.name &&
                run.player->move_state() == PlayerMoveState::on_drop_down_ground &&
                std::fabs(run.player->rectangle().bottom() - platform.rectangle.top()) < surface_tolerance &&
                run.count(MovementFeedbackKind::takeoff) == 1 && run.count(MovementFeedbackKind::landing) == 1,
                "a held jump did not pass up through and land once on the drop platform");
        };
        // Released runs first: its runway landing tick times the late press.
        enum class Variant { released, held, fresh_press, late_press };
        int drop_landing = -1;
        for (const Variant variant : { Variant::released, Variant::held, Variant::fresh_press,
            Variant::late_press })
        {
            Run run(s, standing_on(s, runway, platform.rectangle.center().x));
            climb(run);
            run.tick(input(false, variant == Variant::held, 0.0f, true));
            require(run.player->move_state() == PlayerMoveState::dropping_down, "drop did not leave the platform");
            const int dropped = run.tick_count;
            int press = -1;
            int landed = -1;
            for (int tick = 0; tick < 120 && landed < 0; ++tick)
            {
                PlayerInputData actions = input(false, variant == Variant::held);
                if (variant == Variant::fresh_press && tick == 0) { actions = input(true, true); }
                if (variant == Variant::late_press && run.tick_count + 1 == drop_landing - 3)
                {
                    actions = input(true, false);
                    press = run.tick_count + 1;
                }
                const size_t takeoffs = run.count(MovementFeedbackKind::takeoff);
                run.tick(actions);
                for (const Run::Event& event : run.events)
                {
                    if (event.kind == MovementFeedbackKind::landing && event.tick == run.tick_count &&
                        std::fabs(event.position.y - runway.rectangle.top()) < surface_tolerance)
                    {
                        landed = run.tick_count;
                    }
                }
                require(run.count(MovementFeedbackKind::takeoff) == takeoffs || landed == run.tick_count,
                    "a platform drop produced an airborne jump");
            }
            require(landed > 0, "a drop did not land on the runway");
            if (variant == Variant::released) { drop_landing = landed; }
            for (int tick = 0; tick < 40; ++tick) { run.tick(input(false, variant == Variant::held)); }
            const size_t takeoffs = run.count(MovementFeedbackKind::takeoff);
            if (variant == Variant::late_press)
            {
                require(press > 0 && takeoffs == (window_open(buffer, drop_landing - press) ? 2u : 1u),
                    "a deliberate press after a drop did not follow the landing buffer");
            }
            else
            {
                require(takeoffs == 1 && run.player->velocity().y == 0.0f && run.support() == runway.name,
                    "drop input became a held, grace or buffered jump");
            }
            const char* name = variant == Variant::held ? "jump held through the drop" :
                variant == Variant::fresh_press ? "fresh press the tick after the drop" :
                variant == Variant::released ? "released" : "press 3 ticks before landing";
            std::cout << "MEASURE drop-through " << name << ": fall_ticks=" << landed - dropped
                << " extra_takeoffs=" << takeoffs - 1 << '\n';
            for (int tick = 0; tick < 120 && !supported(*run.player); ++tick) { run.tick(); }
            run.tick();
            run.tick(input(true, true));
            require(run.player->move_state() == PlayerMoveState::jumping &&
                run.player->velocity().y < 0.0f, "a normal jump did not follow the drop");
        }
        std::cout << "PASS authored drop-through: climb, single landing, no held/grace/buffered drop jump, normal jump after\n";
    }

    namespace
    {
    enum class Transition { none, pause, death };

    const char* transition_name(Transition transition)
    {
        return transition == Transition::none ? "none" :
            transition == Transition::pause ? "pause" : "death+respawn";
    }

    // A player on long_landing_running_takeoff after the transition, at rest.
    // The respawn point is a test position on that platform, at the authored
    // spawns' 5-unit settle height; respawn itself is the real path.
    std::unique_ptr<Run> lip_run(const Services& s, Transition transition)
    {
        const Surface& platform = s.course.named("long_landing_running_takeoff");
        const Vector2F respawn(platform.rectangle.left() + 220.0f,
            platform.rectangle.top() - s.tuning.player().size.y - 5.0f);
        auto run = std::make_unique<Run>(s, respawn, Vector2F::ZERO, WeaponType::sprayer,
            PlayerTeam::a, respawn);
        run->settle(30);
        if (transition == Transition::pause)
        {
            run->tick(input(false, false, 1.0f));
            // Level::suspend -> pause_player_sounds calls this for every player.
            run->player->reset_jump_input();
            run->tick(input(true, true, 0.0f)); // the menu's activating press
            require(run->player->velocity().y == 0.0f, "a pause press jumped on resume");
            run->tick();
        }
        else if (transition == Transition::death)
        {
            run->tick(input(false, false, 1.0f));
            run->kill();
            run->tick(input(true, true, 1.0f, true));
            require(run->player->state() == PlayerState::dead, "fatal damage did not retire the player");
            for (int tick = 0; tick < 400 && run->player->state() == PlayerState::dead; ++tick)
            {
                run->tick(input(true, true, 1.0f, true));
            }
            require(run->player->state() == PlayerState::alive, "eliminated player did not respawn");
            for (int tick = 0; tick < 30; ++tick) { run->tick(input(false, true)); }
            require(run->support() == platform.name && run->player->velocity() == Vector2F::ZERO &&
                run->player->rectangle().x == respawn.x &&
                run->count(MovementFeedbackKind::takeoff) == 0,
                "respawn did not settle at its point without carried input");
            run->tick();
        }
        run->landings.clear();
        run->events.clear();
        return run;
    }

    int walk_off(Run& run)
    {
        for (int tick = 0; tick < 240; ++tick)
        {
            run.tick(input(false, false, 1.0f));
            if (run.player->move_state() == PlayerMoveState::in_air) { return run.tick_count; }
        }
        fail("the player never walked off the running takeoff lip at " +
            fixed(run.services.course.named("long_landing_running_takeoff").rectangle.right(), 0));
    }
    }

    void lip_timing(const Services& s)
    {
        const float grace = s.tuning.player().ledge_grace_time;
        const float buffer = s.tuning.player().jump_buffer_time;
        const float lip = s.course.named("long_landing_running_takeoff").rectangle.right();
        for (const Transition transition : { Transition::none, Transition::pause, Transition::death })
        {
            std::string table;
            int last_grace = 0;
            for (int after = 1; after <= 8; ++after)
            {
                auto run = lip_run(s, transition);
                walk_off(*run);
                for (int tick = 1; tick < after; ++tick) { run->tick(input(false, false, 1.0f)); }
                run->tick(input(true, true, 1.0f));
                const bool jumped = run->count(MovementFeedbackKind::takeoff) == 1;
                // Grace was set on the last supported update; the leaving
                // update and each one to the press count it down.
                const bool expected = window_open(grace, after + 1);
                require(jumped == expected, "a ledge press did not follow the grace window");
                last_grace = jumped ? after : last_grace;
                table += " +" + std::to_string(after) + (jumped ? ":jump" : ":none");
            }
            require(last_grace >= 1, "no ledge grace press succeeded");

            auto reference = lip_run(s, transition);
            const int leave = walk_off(*reference);
            for (int tick = 0; tick < 240 && reference->landings.empty(); ++tick)
            {
                reference->tick(input(false, false, 1.0f));
            }
            require(!reference->landings.empty(), "walking off the lip never landed");
            const int landing = reference->landings.front().tick;
            const std::string surface = reference->landings.front().surface;
            std::string buffered;
            for (int press = std::max(leave + last_grace + 1, landing - 8); press <= landing; ++press)
            {
                auto run = lip_run(s, transition);
                walk_off(*run);
                int launched = -1;
                for (int tick = 0; tick < 240 && run->tick_count < landing + 20; ++tick)
                {
                    run->tick(input(run->tick_count + 1 == press, false, 1.0f));
                    if (launched < 0 && run->count(MovementFeedbackKind::takeoff) == 1) { launched = run->tick_count; }
                }
                const bool expected = window_open(buffer, landing - press);
                require(launched == (expected ? landing : -1),
                    "a falling press did not launch exactly on the landing contact tick");
                buffered += " -" + std::to_string(landing - press) + (launched >= 0 ? ":launch" : ":none");
            }
            std::cout << "MEASURE lip-" << fixed(lip, 0) << " after " << transition_name(transition)
                << ": press ticks after leaving" << table << " | fall lands on " << surface
                << ", press ticks before landing" << buffered << '\n';
        }
        std::cout << "PASS ledge grace, late press and buffered landing on the authored " << fixed(lip, 0)
            << " lip after none/pause/death\n";
    }

    void spawns(const Services& s)
    {
        const CourseData& course = s.course;
        constexpr int deaths = 3;
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            const std::vector<Vector2F> points = team == PlayerTeam::a ?
                course.info.team_a_spawns() : course.info.team_b_spawns();
            const Surface& runway = course.named(team == PlayerTeam::a ? "spawn_a_runway" : "spawn_b_runway");
            const float d = team == PlayerTeam::a ? 1.0f : -1.0f;
            for (size_t index = 0; index < points.size(); ++index)
            {
                const Vector2F spawn = points[index];
                Run run(s, spawn, Vector2F::ZERO, WeaponType::sprayer, team, spawn);
                int respawn_ticks = 0;
                for (int life = 0; life <= deaths; ++life)
                {
                    for (int tick = 0; tick < 60; ++tick) { run.tick(input(false, life > 0)); }
                    require(run.support() == runway.name && run.player->rectangle().x == spawn.x &&
                        std::fabs(run.player->rectangle().bottom() - runway.rectangle.top()) < surface_tolerance &&
                        run.player->velocity() == Vector2F::ZERO &&
                        run.player->move_state() == PlayerMoveState::on_ground,
                        "a spawn did not settle on its runway at the authored point");
                    require(run.count(MovementFeedbackKind::takeoff) == 0,
                        "a held jump carried across death into the new life");
                    if (life == deaths) { break; }
                    for (int tick = 0; tick < 20; ++tick) { run.tick(input(tick == 0, true, d)); }
                    run.events.clear();
                    run.kill();
                    const int died = run.tick_count;
                    while (run.tick_count - died < 600 &&
                        (run.player->state() == PlayerState::dead || run.tick_count == died))
                    {
                        run.tick(input(true, true, d, true));
                    }
                    require(run.player->state() == PlayerState::alive, "a spawn never respawned");
                    respawn_ticks = run.tick_count - died;
                    run.events.clear();
                }
                run.tick();
                run.tick(input(true, true));
                require(run.player->move_state() == PlayerMoveState::jumping,
                    "repeated respawns did not rearm the jump");
                std::cout << "MEASURE spawn team_" << (team == PlayerTeam::a ? "a" : "b") << '[' << index
                    << "] x=" << fixed(spawn.x, 0) << ": settled feet=" << fixed(runway.rectangle.top(), 0)
                    << " after " << deaths << " deaths, respawn_ticks=" << respawn_ticks << '\n';
            }
        }
        std::cout << "PASS repeated spawns: every authored spawn settles at feet Y=1000 after repeated deaths with no carried input\n";
    }
}
