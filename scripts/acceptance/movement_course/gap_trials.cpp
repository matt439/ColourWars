#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"

// The three authored gaps and their recovery climbs, over the routes main builds.
namespace movement_course_acceptance
{
    namespace
    {
    enum class Takeoff { rest, running, grace };

    struct Start
    {
        Takeoff kind;
        float value; // run-up distance before the lip, or ticks after leaving it
        const char* label;
    };

    struct GapResult
    {
        std::string first;
        std::string last;
        size_t takeoffs = 0;
        float rise = 0.0f;
        int airtime = 0;
        float landed_past_lip = 0.0f;
        float stopped_past_lip = 0.0f;
    };

    // Hold direction until the back edge is 20 past the far lip, then brake in
    // the air; braking on landing otherwise. hold==0 holds jump to landing.
    GapResult jump_gap(const Services& s, const Gap& gap, float d, const Start& start, int hold)
    {
        const float speed = s.tuning.player().max_velocity.x;
        const Surface& from = d > 0.0f ? *gap.left : *gap.right;
        const Surface& to = d > 0.0f ? *gap.right : *gap.left;
        const float lip = d > 0.0f ? from.rectangle.right() : from.rectangle.left();
        const float far_lip = d > 0.0f ? to.rectangle.left() : to.rectangle.right();
        const bool moving = start.kind != Takeoff::rest;
        const float run_up = start.kind == Takeoff::running ? start.value :
            (start.kind == Takeoff::grace ? 100.0f : 0.0f);
        // The settling tick advances a running player by one step.
        Run run(s, front_at(s, from, lip - d * (run_up + (moving ? speed * dt : 0.0f)), d),
            { moving ? d * speed : 0.0f, 0.0f });
        run.settle(1, input(false, false, moving ? d : 0.0f));
        require(run.support() == from.name, "a gap trial did not start on its takeoff platform");
        const float start_feet = run.player->rectangle().bottom();
        float highest = start_feet;
        int press = start.kind == Takeoff::grace ? -1 : 0;
        int landed = -1;
        bool steering = true;
        GapResult result;
        for (int tick = 0; tick < 600; ++tick)
        {
            if (press < 0 && run.player->move_state() == PlayerMoveState::in_air)
            {
                press = tick + static_cast<int>(start.value) - 1;
            }
            const RectangleF& body = run.player->rectangle();
            if (landed >= 0 || (d > 0.0f ? body.left() > far_lip + 20.0f : body.right() < far_lip - 20.0f))
            {
                steering = false;
            }
            const bool started = press >= 0 && tick >= press;
            const bool holding = started && (hold == 0 ? landed < 0 : tick < press + hold);
            run.tick(input(tick == press, holding, steering ? d : 0.0f));
            highest = std::min(highest, run.player->rectangle().bottom());
            if (landed < 0 && started && !run.landings.empty())
            {
                landed = tick;
                result.first = run.landings.front().surface;
                result.airtime = tick - press;
                result.landed_past_lip = d * (run.player->center().x - far_lip);
            }
            if (landed >= 0 && supported(*run.player) && run.player->velocity().x == 0.0f) { break; }
        }
        require(landed >= 0, "a gap trial never landed");
        result.last = run.support();
        result.takeoffs = run.count(MovementFeedbackKind::takeoff);
        result.rise = start_feet - highest;
        result.stopped_past_lip = d * (run.player->center().x - far_lip);
        return result;
    }
    }

    void gaps(const Services& s, const std::vector<Gap>& routes)
    {
        const std::vector<Start> starts{ { Takeoff::rest, 0.0f, "rest-at-lip" },
            { Takeoff::running, 120.0f, "run800-lip-120" }, { Takeoff::running, 70.0f, "run800-lip-70" },
            { Takeoff::running, 18.0f, "run800-lip-18" }, { Takeoff::grace, 1.0f, "run800-grace+1" } };
        const std::vector<int> holds{ 1, 6, 12, 21, 30, 0 };
        std::cout << "TABLE gap outcomes (a landing back on the takeoff platform fails): F far platform, H/L recovery step "
            "high/low, G lower floor (Y=1400), D drop-through platform. Columns are jump-held ticks; 'held' holds to landing.\n";
        for (const Gap& gap : routes)
        {
            for (const float d : { 1.0f, -1.0f })
            {
                const Surface& from = d > 0.0f ? *gap.left : *gap.right;
                const Surface& to = d > 0.0f ? *gap.right : *gap.left;
                for (const Start& start : starts)
                {
                    std::string row;
                    for (const int hold : holds)
                    {
                        const GapResult result = jump_gap(s, gap, d, start, hold);
                        std::string code;
                        if (result.first == to.name) { code = "F"; }
                        else if (result.first == from.name)
                        {
                            fail(std::string("a ") + gap.label + " jump fell back onto its takeoff platform");
                        }
                        else if (result.first == gap.high->name) { code = "H"; }
                        else if (result.first == gap.low->name) { code = "L"; }
                        else if (result.first == "recovery_floor") { code = "G"; }
                        else if (result.first == "drop_through_platform") { code = "D"; }
                        else { fail(std::string("a missed ") + gap.label + " jump landed on '" + result.first + "'"); }
                        require(result.takeoffs == 1, "a gap trial did not take off exactly once");
                        if (gap.left->name == "spawn_a_runway")
                        {
                            // Holding a leftward jump reaches the platform above
                            // the runway; shorter presses choose the runway itself.
                            const bool upper_landing = d < 0.0f && (hold == 0 || hold >= 21);
                            require(result.first == (upper_landing ? "drop_through_platform" : to.name),
                                "a short-gap jump did not reach its chosen runway or drop-through landing");
                        }
                        else
                        {
                            require(code != "D", "a different gap reached the short-gap drop-through platform");
                        }
                        row += " h" + (hold == 0 ? std::string("eld") : std::to_string(hold)) + "=" + code;
                        if (hold == 0 && start.kind == Takeoff::running)
                        {
                            std::cout << "MEASURE gap=" << gap.label << " width=" << fixed(gap.width(), 0)
                                << " dir=" << (d > 0.0f ? "right" : "left") << " start=" << start.label
                                << " held: landed=" << result.first << " rise=" << fixed(result.rise)
                                << " airtime_ticks=" << result.airtime
                                << " landed_past_far_lip=" << fixed(result.landed_past_lip)
                                << " stopped_past_far_lip=" << fixed(result.stopped_past_lip)
                                << " stopped_on=" << result.last << '\n';
                        }
                        if (hold == 0 && start.kind == Takeoff::running)
                        {
                            // The scratch probe's case, from authored geometry without
                            // teleports, from every running lip: across the gap, at or
                            // above the far platform. Leftward over the short gap that
                            // can be the drop platform.
                            const float far_lip = d > 0.0f ? to.rectangle.left() : to.rectangle.right();
                            for (const std::string& name : { result.first, result.last })
                            {
                                const RectangleF& landing = s.course.named(name).rectangle;
                                require((d > 0.0f ? landing.left() >= far_lip : landing.right() <= far_lip) &&
                                    landing.top() <= to.rectangle.top(),
                                    "a held full-speed jump did not land and brake beyond the far lip");
                            }
                        }
                    }
                    std::cout << "MEASURE gap=" << gap.label << " width=" << fixed(gap.width(), 0)
                        << " dir=" << (d > 0.0f ? "right" : "left") << " start=" << start.label
                        << " |" << row << '\n';
                }
            }
        }
        std::cout << "PASS gaps: every trial takes off exactly once; held full-speed jumps from every "
            "running lip land and brake beyond the far lip both ways; none falls back onto its takeoff "
            "platform, and every other landing is a recovery step, the lower floor or the drop-through platform\n";
    }

    namespace
    {
    // Floor to low step, low to high step, high step sideways to a main platform.
    std::vector<std::string> climb_pair(const Services& s, const Gap& gap, float start_centre,
        float exit, bool steer_throughout = false)
    {
        const Surface& floor = s.course.named("recovery_floor");
        Run run(s, standing_on(s, floor, start_centre));
        run.settle();
        require(run.support() == floor.name, "recovery trial did not start on the lower floor");
        walk_to(run, gap.low->rectangle.center().x);
        require(run.count(MovementFeedbackKind::takeoff) == 0 && run.landings.empty(),
            "the lower lane needed a jump to reach a recovery pair");
        std::vector<std::string> path;
        path.push_back(jump_and_land(run, 0.0f));
        path.push_back(jump_and_land(run, 0.0f));
        const Surface& platform = exit > 0.0f ? *gap.right : *gap.left;
        path.push_back(jump_and_land(run, exit, steer_throughout ? std::nullopt :
            std::optional<float>(exit > 0.0f ? platform.rectangle.left() : platform.rectangle.right())));
        brake(run);
        path.push_back(run.support());
        return path;
    }
    }

    void recovery(const Services& s, const std::vector<Gap>& routes)
    {
        for (const Gap& gap : routes)
        {
            const float centre = gap.low->rectangle.center().x;
            const float offset = gap.low->rectangle.width / 2.0f + 150.0f;
            for (const float approach : { 1.0f, -1.0f })
            {
                for (const float exit : { 1.0f, -1.0f })
                {
                    const auto path = climb_pair(s, gap, centre - approach * offset, exit);
                    const std::string& platform = exit > 0.0f ? gap.right->name : gap.left->name;
                    if (path[0] != gap.low->name || path[1] != gap.high->name ||
                        path[2] != platform || path[3] != platform)
                    {
                        fail(std::string("the ") + gap.label + " recovery pair climbed " + path[0] + " -> " +
                            path[1] + " -> " + path[2] + " -> " + path[3] + ", not low, high, " + platform);
                    }
                    // Continuous steering can carry through the short right
                    // landing. Releasing early above still makes every exit safe.
                    const auto steered = climb_pair(s, gap, centre - approach * offset, exit, true);
                    const bool short_slide = gap.left->name == "spawn_a_runway" && exit > 0.0f;
                    require(steered[0] == gap.low->name && steered[1] == gap.high->name &&
                        steered[2] == platform && steered[3] == (short_slide ? "recovery_floor" : platform),
                        "a continuously steered recovery exit did not reach its accepted landing and support");
                    std::cout << "MEASURE recovery " << gap.label << " approach="
                        << (approach > 0.0f ? "from-left" : "from-right") << " exit="
                        << (exit > 0.0f ? "right" : "left") << ": " << path[0] << " -> " << path[1]
                        << " -> " << path[2] << " (stopped on " << path[3]
                        << ") | steering throughout the exit jump stops on " << steered[3] << '\n';
                }
            }
        }
        // The rescue lane is recovery_floor plus the recovery pairs. From
        // beneath the staircase and the far right, the floor leads back to the
        // running-gap pair without any jump.
        const Gap& running = routes.back();
        for (const float start : { s.course.named("high_step").rectangle.center().x,
            s.course.named("spawn_b_runway").rectangle.right() - 60.0f })
        {
            for (const float exit : { 1.0f, -1.0f })
            {
                const auto path = climb_pair(s, running, start, exit);
                const std::string& platform = exit > 0.0f ? running.right->name : running.left->name;
                require(path[2] == platform && path[3] == platform,
                    "the rescue lane did not return to the course");
                std::cout << "MEASURE rescue-lane from x=" << fixed(start, 0) << " -> " << path[0]
                    << " -> " << path[1] << " -> " << path[2] << '\n';
            }
        }
        std::cout << "PASS recovery: every pair from rest, both approaches and exits; rescue lane from the staircase underside\n";
    }
}
