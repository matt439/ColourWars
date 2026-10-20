#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"

// Each authored rise, tapped and held, from rest and running, and the full staircases.
namespace movement_course_acceptance
{
    namespace
    {
    struct Rise
    {
        const Surface* lower;
        const Surface* upper;
        float direction;

        float height() const { return this->lower->rectangle.top() - this->upper->rectangle.top(); }
        float face() const
        {
            return this->direction > 0.0f ? this->upper->rectangle.left() : this->upper->rectangle.right();
        }
    };

    struct RiseResult
    {
        std::string first;
        std::string last;
    };

    // Press within 30 units of the face, holding direction until landing.
    RiseResult climb_rise(const Services& s, const Rise& rise, bool running, bool held)
    {
        const float d = rise.direction;
        const float speed = s.tuning.player().max_velocity.x;
        const float room = rise.lower->rectangle.width - s.tuning.player().size.x - 8.0f - speed * dt;
        const float distance = running ? std::min(300.0f, room) : 4.0f;
        Run run(s, front_at(s, *rise.lower, rise.face() - d * (distance + (running ? speed * dt : 0.0f)), d),
            { running ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, running ? d : 0.0f));
        require(run.support() == rise.lower->name, "a staircase trial did not start on its lower step");
        int press = -1;
        for (int tick = 0; tick < 400; ++tick)
        {
            if (press < 0 && d * (rise.face() - run.front(d)) <= 30.0f) { press = tick; }
            const bool holding = press >= 0 && (held ? run.landings.empty() : tick == press);
            run.tick(input(tick == press, holding, run.landings.empty() ? d : 0.0f));
            if (!run.landings.empty() && supported(*run.player) && run.player->velocity().x == 0.0f) { break; }
        }
        require(press >= 0 && !run.landings.empty(), "a staircase jump never landed");
        return { run.landings.front().surface, run.support() };
    }

    // Hold direction throughout and jump with a held press within 30 units of
    // each next face. Records the path; the per-rise trials make the claims.
    std::vector<std::string> run_staircase(const Services& s, const std::vector<const Surface*>& route,
        float d, bool running)
    {
        const float speed = s.tuning.player().max_velocity.x;
        const auto face = [d](const Surface& surface)
            { return d > 0.0f ? surface.rectangle.left() : surface.rectangle.right(); };
        const float distance = running ? 300.0f : 4.0f;
        Run run(s, front_at(s, *route[0], face(*route[1]) - d * (distance + (running ? speed * dt : 0.0f)), d),
            { running ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, running ? d : 0.0f));
        std::vector<std::string> path;
        size_t reached = 0;
        bool jumping = false;
        bool released = true;
        for (int tick = 0; tick < 900; ++tick)
        {
            const size_t next = reached + 1;
            bool pressing = false;
            if (!jumping && next < route.size() && supported(*run.player) && released &&
                d * (face(*route[next]) - run.front(d)) <= 30.0f)
            {
                pressing = true;
                jumping = true;
            }
            released = !jumping;
            const size_t landings = run.landings.size();
            run.tick(input(pressing, jumping, next < route.size() ? d : 0.0f));
            if (run.landings.size() > landings)
            {
                jumping = false;
                path.push_back(run.landings.back().surface);
                for (size_t index = next; index < route.size(); ++index)
                {
                    if (route[index]->name == path.back()) { reached = index; }
                }
            }
            if (reached + 1 == route.size() && supported(*run.player) &&
                run.player->velocity().x == 0.0f) { break; }
        }
        path.push_back("stopped on " + run.support());
        return path;
    }
    }

    void staircases(const Services& s)
    {
        const CourseData& c = s.course;
        const Surface& running = c.named("running_landing");
        const Surface& short_step = c.named("short_step");
        const Surface& held_step = c.named("held_step");
        const Surface& high_step = c.named("high_step");
        const Surface& high_return = c.named("high_step_return");
        const Surface& short_return = c.named("short_step_return");
        const Surface& runway_b = c.named("spawn_b_runway");
        const std::vector<Rise> rises{ { &running, &short_step, 1.0f }, { &short_step, &held_step, 1.0f },
            { &held_step, &high_step, 1.0f }, { &runway_b, &short_return, -1.0f },
            { &short_return, &high_return, -1.0f }, { &high_return, &high_step, -1.0f } };
        for (const Rise& rise : rises)
        {
            // The authored 60-unit rises are the short-jump stations.
            const bool tap_station = rise.upper == &short_step || rise.upper == &short_return;
            for (const bool moving : { false, true })
            {
                const RiseResult held = climb_rise(s, rise, moving, true);
                const RiseResult tap = climb_rise(s, rise, moving, false);
                const std::string detail = " [rise=" + fixed(rise.height(), 0) + " lower=" +
                    rise.lower->name + " upper=" + rise.upper->name + " dir=" +
                    (rise.direction > 0.0f ? "right" : "left") + " start=" + (moving ? "run800" : "rest") +
                    " held:first=" + held.first + ",last=" + held.last +
                    " tap:first=" + tap.first + ",last=" + tap.last + "]";
                std::cout << "MEASURE stair rise=" << fixed(rise.height(), 0) << ' ' << rise.lower->name
                    << " -> " << rise.upper->name << " dir=" << (rise.direction > 0.0f ? "right" : "left")
                    << " start=" << (moving ? "run800" : "rest") << ": held=" << held.first
                    << " stopped=" << held.last << " tap=" << tap.first << " stopped=" << tap.last << '\n';
                // Clearing means landing at or above the upper step; a held jump
                // may carry on to a higher one.
                const auto clears = [&](const std::string& name)
                    { return c.named(name).rectangle.top() <= rise.upper->rectangle.top(); };
                require(clears(held.first), ("a held jump did not clear a staircase rise" + detail).c_str());
                require(clears(tap.first) == tap_station,
                    ("a tap cleared a held-jump rise or failed the short step" + detail).c_str());
                if (tap_station)
                {
                    // The shorter airtime lands on this tread before reaching
                    // the next one, despite retaining the 200-unit rise reach
                    // needed by existing arenas. No automatic shortening is used.
                    require(tap.first == rise.upper->name && tap.last == rise.upper->name &&
                        held.first == rise.upper->name && held.last == rise.upper->name,
                        ("tap and held jumps did not land and stop on the short rise" + detail).c_str());
                }
            }
        }

        for (const bool moving : { false, true })
        {
            for (const auto& [d, route] : { std::pair{ 1.0f, std::vector<const Surface*>{
                &running, &short_step, &held_step, &high_step } }, std::pair{ -1.0f,
                std::vector<const Surface*>{ &runway_b, &short_return, &high_return, &high_step } } })
            {
                std::string text;
                for (const std::string& step : run_staircase(s, route, d, moving)) { text += " > " + step; }
                std::cout << "MEASURE staircase-ascent dir=" << (d > 0.0f ? "right" : "left") << " start="
                    << (moving ? "run800" : "rest") << ":" << text << '\n';
            }
        }

        // Descending: walk off each step from rest, then run the whole way down.
        for (const auto& [from, d] : { std::pair{ &high_step, 1.0f }, std::pair{ &high_return, 1.0f },
            std::pair{ &short_return, 1.0f }, std::pair{ &high_step, -1.0f },
            std::pair{ &held_step, -1.0f }, std::pair{ &short_step, -1.0f } })
        {
            const float edge = d > 0.0f ? from->rectangle.right() : from->rectangle.left();
            Run run(s, front_at(s, *from, edge - d * 2.0f, d));
            run.settle();
            for (int tick = 0; tick < 240 && run.landings.empty(); ++tick) { run.tick(input(false, false, d)); }
            require(!run.landings.empty() && run.count(MovementFeedbackKind::takeoff) == 0,
                "walking off a step did not land without a jump");
            const Surface& below = c.named(run.landings.front().surface);
            require(below.rectangle.top() > from->rectangle.top(), "walking off a step landed higher");
            if (from == &high_return || from == &held_step)
            {
                require(below.name == (d > 0.0f ? runway_b.name : running.name),
                    ("walking off the middle step did not carry over the narrow lower step to the runway [from=" +
                        from->name + " dir=" + (d > 0.0f ? "right" : "left") +
                        " first=" + below.name + " last=" + run.support() + "]").c_str());
            }
            brake(run);
            std::cout << "MEASURE stair-descent-from-rest " << from->name << " dir="
                << (d > 0.0f ? "right" : "left") << ": landed=" << below.name
                << " drop=" << fixed(below.rectangle.top() - from->rectangle.top(), 0) << '\n';
        }
        for (const float d : { 1.0f, -1.0f })
        {
            const Surface& bottom = d > 0.0f ? runway_b : running;
            Run run(s, standing_on(s, high_step, high_step.rectangle.center().x),
                { d * s.tuning.player().max_velocity.x, 0.0f });
            run.settle(1, input(false, false, d));
            std::string text;
            for (int tick = 0; tick < 600 && (run.landings.empty() ||
                run.landings.back().surface != bottom.name); ++tick)
            {
                const size_t landings = run.landings.size();
                run.tick(input(false, false, d));
                if (run.landings.size() > landings) { text += " > " + run.landings.back().surface; }
            }
            require(!run.landings.empty() && run.landings.back().surface == bottom.name,
                "a full-speed descent did not reach the runway level");
            brake(run);
            std::cout << "MEASURE stair-descent-run800 from high_step dir=" << (d > 0.0f ? "right" : "left")
                << ":" << text << " > stopped on " << run.support() << '\n';
        }
        std::cout << "PASS staircases: held jumps clear every rise and taps only the 60-unit steps, "
            "both directions from rest and at speed; descents land without intersecting a step\n";
    }
}
