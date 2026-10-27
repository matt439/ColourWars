#pragma once

#include "game/ai/match_bot.h"

namespace bot_ascent_acceptance
{
    inline void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    inline MatchBotObservation floor_fixture(const GameTuning& tuning)
    {
        MatchBotObservation view;
        view.alive = view.grounded = true;
        view.size = tuning.player().size;
        view.ammo = 1.0f;
        view.engage_distance = 800.0f;
        view.projectile_speed = 1000.0f;
        view.obstacles.emplace_back(-1000.0f, view.size.y * 0.5f, 2000.0f, 100.0f);
        view.supports.push_back({ view.obstacles.back(), false });
        return view;
    }

    inline void raised_support(MatchBotObservation& view, float rise, float width)
    {
        view.obstacles.emplace_back(-width * 0.5f, view.size.y * 0.5f - rise, width, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true });
    }

    inline PlayerInputData first_decision(MatchBot& bot, const MatchBotObservation& view,
        const GameTuning& tuning, BotDifficulty difficulty)
    {
        PlayerInputData input;
        const int reaction = tuning.bot(difficulty).reaction_steps;
        for (int step = 0; step < reaction; ++step)
        {
            input = bot.think(view);
            require(input.connected && !input.toggle_pause_menu && !input.pause_held &&
                !input.toggle_debug && !input.skip_intro,
                "Ascent policy emitted a device/menu/debug action");
            if (step + 1 < reaction)
            {
                require(input.x_movement == 0.0f && !input.jump_pressed && !input.jump_held &&
                    !input.drop_through, "Ascent bypassed the initial difficulty reaction delay");
            }
        }
        return input;
    }

    inline MatchBotObservation approach_fixture(const GameTuning& tuning)
    {
        MatchBotObservation view = floor_fixture(tuning);
        view.obstacles.emplace_back(100.0f, view.size.y * 0.5f - 140.0f, 300.0f, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true });
        return view;
    }

    inline void require_cancelled(const PlayerInputData& input, const char* message)
    {
        require(input.x_movement == 0.0f && input.left_analog_stick.x == 0.0f &&
            !input.jump_pressed && !input.jump_held && !input.drop_through, message);
    }

    inline void check_scoring_descent_landing(const GameTuning& tuning, BotDifficulty difficulty)
    {
        for (const bool paintable : { false, true })
        {
            MatchBot bot(&tuning, difficulty, 7);
            MatchBotObservation view = floor_fixture(tuning);
            view.on_platform = true;
            view.obstacles.front() = mattmath::RectangleF(-1000.0f, view.size.y * 0.5f, 2000.0f, 10.0f);
            view.supports.front() = { view.obstacles.front(), true, false };
            const float lower_top = view.size.y * 0.5f + 300.0f;
            view.obstacles.emplace_back(-300.0f, lower_top, 600.0f, 10.0f);
            view.supports.push_back({ view.obstacles.back(), true, paintable });
            view.obstacles.emplace_back(300.0f, lower_top, 1000.0f, 100.0f);
            view.supports.push_back({ view.obstacles.back(), false, false });
            const PlayerInputData drop = first_decision(bot, view, tuning, difficulty);
            require(drop.drop_through && !drop.jump_held,
                "Scoring-landing fixture did not begin its ordinary downward escape");

            // Value poses isolate the landing policy. Actual-driver arena
            // trials separately establish physics and named contacts. The
            // scoring role of a one-way top must complete the escape too.
            view.grounded = false; view.on_platform = false;
            view.center.y = 150.0f;
            (void)bot.think(view);
            view.grounded = true; view.on_platform = true;
            view.center.y = 300.0f;
            // This fixture advances value poses without integrating input.
            // Retain ordinary horizontal motion so the unrelated blocked-
            // movement jump does not fire during Easy's reaction interval.
            view.velocity.x = tuning.player().x_initial_velocity * 0.5f;
            (void)bot.think(view);

            // A subsequent step onto adjacent, unpaintable solid ground makes
            // a new raised support eligible. This distinguishes clearing the
            // descent on the one-way landing from clearing it on this ground.
            view.center.x = 500.0f; view.on_platform = false;
            view.obstacles.emplace_back(400.0f, lower_top - 140.0f, 300.0f, 10.0f);
            view.supports.push_back({ view.obstacles.back(), true, false });
            bool jumped = false;
            for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step)
            {
                const PlayerInputData input = bot.think(view);
                jumped = jumped || input.jump_pressed;
            }
            require(jumped == paintable, paintable ?
                "Landing on a scoring one-way top failed to release later ascent exploration" :
                "An unpaintable descent landing released exploration before reaching scoring ground");
        }
    }

    inline void check_jump_boundary_and_local_routes(const GameTuning& tuning, BotDifficulty difficulty)
    {
        // These adjacent authored step heights bracket the normal held-jump
        // apex. The actual-player arena trials own successful 200-unit contact.
        for (const float rise : { 200.0f, 201.0f })
        {
            MatchBot bot(&tuning, difficulty, 7);
            MatchBotObservation view = floor_fixture(tuning);
            raised_support(view, rise, 300.0f);
            const PlayerInputData input = first_decision(bot, view, tuning, difficulty);
            require(input.jump_pressed == (rise == 200.0f) && input.jump_held == input.jump_pressed,
                "Ascent did not distinguish a reachable 200-unit step from an unreachable 201-unit step");
        }

        MatchBot unknown_floor(&tuning, difficulty, 7);
        MatchBotObservation view = floor_fixture(tuning);
        view.supports.clear();
        raised_support(view, 140.0f, 300.0f);
        const PlayerInputData unknown = first_decision(unknown_floor, view, tuning, difficulty);
        require(!unknown.jump_pressed && !unknown.jump_held && !unknown.drop_through,
            "Ascent selected a launch without any known source support");

        MatchBot narrow_source(&tuning, difficulty, 7);
        view = floor_fixture(tuning);
        view.size = { 1.0f, 1.0f };
        view.obstacles.front() = mattmath::RectangleF(-0.5f, 0.5f, 1.0f, 100.0f);
        view.supports.front() = { view.obstacles.front(), false, false };
        raised_support(view, 140.0f, 300.0f);
        require(first_decision(narrow_source, view, tuning, difficulty).jump_pressed,
            "Takeoff inset invalidated a known source exactly as wide as a small body");

        MatchBot gap(&tuning, difficulty, 7);
        view = floor_fixture(tuning);
        view.obstacles.front() = mattmath::RectangleF(-200.0f, view.size.y * 0.5f, 250.0f, 100.0f);
        view.supports.front() = { view.obstacles.front(), false, false };
        view.obstacles.emplace_back(150.0f, view.size.y * 0.5f - 140.0f, 300.0f, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true, false });
        const PlayerInputData alignment = first_decision(gap, view, tuning, difficulty);
        require(alignment.x_movement > 0.0f && !alignment.jump_pressed,
            "A reachable gap route did not begin alignment on its source floor");
        // The whole body can overhang the lip, but the launch center stays
        // inside the known floor. It must jump here, not walk to the target.
        view.center.x = view.supports.front().bounds.right() - 2.0f;
        const PlayerInputData takeoff = gap.think(view);
        require(takeoff.x_movement == 0.0f && takeoff.jump_pressed && takeoff.jump_held,
            "A gap route walked beyond its known source instead of taking off inside the lip");
        view.grounded = false; view.center.y -= 10.0f;
        const PlayerInputData crossing = gap.think(view);
        require(crossing.x_movement > 0.0f && !crossing.drop_through,
            "A gap route did not steer toward its one-way landing after takeoff");

        MatchBot nearby(&tuning, difficulty, 7);
        view = floor_fixture(tuning);
        // A lower deck across a gap must not displace a higher support above
        // the current floor. Walking farther along one continuous floor is
        // different from committing to an extended airborne crossing.
        view.obstacles.front() = mattmath::RectangleF(-200.0f, view.size.y * 0.5f, 250.0f, 100.0f);
        view.supports.front() = { view.obstacles.front(), false, false };
        view.obstacles.emplace_back(300.0f, view.size.y * 0.5f - 80.0f, 200.0f, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true, false });
        view.obstacles.emplace_back(-250.0f, view.size.y * 0.5f - 140.0f, 200.0f, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true, false });
        const PlayerInputData local = first_decision(nearby, view, tuning, difficulty);
        require(local.x_movement < 0.0f && !local.jump_pressed,
            "A lower deck across a distant gap displaced a feasible higher ascent above the source floor");
    }

    inline void check_short_steps_and_headroom(const GameTuning& tuning, BotDifficulty difficulty)
    {
        MatchBot short_step(&tuning, difficulty, 7);
        MatchBotObservation view = floor_fixture(tuning);
        raised_support(view, 50.0f, 300.0f);
        const PlayerInputData short_jump = first_decision(short_step, view, tuning, difficulty);
        // A centered destination needs no walking. This also distinguishes the
        // selected ascent from an unrelated forward-obstacle recovery jump.
        require(short_jump.jump_pressed && short_jump.jump_held && !short_jump.drop_through &&
            short_jump.x_movement == 0.0f,
            "A reachable 50-unit step below half a body height did not receive a centered ascent jump");

        for (const bool enough_headroom : { true, false })
        {
            MatchBot ceiling(&tuning, difficulty, 7);
            view = floor_fixture(tuning);
            constexpr float rise = 140.0f;
            raised_support(view, rise, 300.0f);
            const float landing_head = view.center.y - view.size.y * 0.5f - rise;
            // The accepted ceiling leaves ten units above the landing body,
            // while blocking the normal full held-jump apex. Its rejecting
            // control intrudes ten units into the required landing headroom.
            // Normal collision owns the truncated jump in actual-player trials.
            const float ceiling_bottom = landing_head + (enough_headroom ? -10.0f : 10.0f);
            view.obstacles.emplace_back(-1000.0f, ceiling_bottom - 20.0f, 2000.0f, 20.0f);
            const PlayerInputData input = first_decision(ceiling, view, tuning, difficulty);
            require(input.jump_pressed == enough_headroom && input.jump_held == enough_headroom &&
                !input.drop_through && (!enough_headroom || input.x_movement == 0.0f),
                enough_headroom ?
                "A body-clear landing beneath a low ceiling was rejected because the full held-jump apex was blocked" :
                "A ceiling below the required landing headroom still produced an ascent jump");
        }
    }

    inline void check_lifetime(const GameTuning& tuning, BotDifficulty difficulty)
    {
        for (const bool airborne : { false, true })
        {
            MatchBot vanished(&tuning, difficulty, 7);
            MatchBotObservation view = airborne ? floor_fixture(tuning) : approach_fixture(tuning);
            if (airborne) { raised_support(view, 140.0f, 300.0f); }
            const PlayerInputData initial = first_decision(vanished, view, tuning, difficulty);
            require(airborne ? initial.jump_pressed : initial.x_movement > 0.0f && !initial.jump_pressed,
                "Target-disappearance fixture did not select its takeoff/alignment action");
            if (airborne)
            {
                view.grounded = false;
                view.center.y = -10.0f;
                view.velocity.y = tuning.player().jump_launch_velocity;
            }
            view.supports.pop_back(); view.obstacles.pop_back();
            require_cancelled(vanished.think(view),
                "A vanished ascent support retained its alignment, held jump or drop");
        }

        MatchBot occupied(&tuning, difficulty, 7);
        MatchBotObservation view = approach_fixture(tuning);
        const PlayerInputData alignment = first_decision(occupied, view, tuning, difficulty);
        require(alignment.x_movement > 0.0f && !alignment.jump_pressed,
            "Landing-obstruction fixture did not first select an alignment route");
        view.obstacles.emplace_back(100.0f, -190.0f, 300.0f, 40.0f);
        require_cancelled(occupied.think(view),
            "An ascent landing that lost body clearance did not immediately cancel its route");

        MatchBot killed(&tuning, difficulty, 7);
        view = floor_fixture(tuning); raised_support(view, 140.0f, 300.0f);
        require(first_decision(killed, view, tuning, difficulty).jump_pressed,
            "Ascent-death fixture did not begin its held takeoff");
        view.alive = false;
        require_cancelled(killed.think(view), "Death retained an active ascent action");
        view = floor_fixture(tuning);
        const PlayerInputData living = first_decision(killed, view, tuning, difficulty);
        if (living.jump_pressed || living.jump_held || living.drop_through)
        {
            throw std::runtime_error("Respawn retained an ascent target or held jump after the ordinary reaction delay: "
                "difficulty=" + std::to_string(static_cast<int>(difficulty)) +
                " move=" + std::to_string(living.x_movement) +
                " pressed=" + std::to_string(living.jump_pressed) +
                " held=" + std::to_string(living.jump_held) +
                " drop=" + std::to_string(living.drop_through) +
                " size=" + std::to_string(view.size.x) + "," + std::to_string(view.size.y) +
                " center=" + std::to_string(view.center.x) + "," + std::to_string(view.center.y) +
                " floor=" + std::to_string(view.obstacles.front().top()) +
                " reaction=" + std::to_string(tuning.bot(difficulty).reaction_steps));
        }

        MatchBot stalled(&tuning, difficulty, 7);
        view = approach_fixture(tuning);
        require(first_decision(stalled, view, tuning, difficulty).x_movement > 0.0f,
            "Ascent-timeout fixture did not begin its alignment attempt");
        const int budget = tuning.bots().progress_window_steps + tuning.bots().recovery_steps;
        bool cancelled = false;
        // This is a value-policy fixture: a frozen pose models an alignment
        // that never makes progress. The real-player trial owns actual jumps.
        for (int step = 0; step < budget; ++step)
        {
            const PlayerInputData input = stalled.think(view);
            require(!input.jump_pressed && !input.jump_held && !input.drop_through,
                "A stalled ascent skipped its still-distant takeoff alignment");
            if (input.x_movement == 0.0f)
            {
                require_cancelled(input, "Ascent expiry left part of its route action active");
                cancelled = true; break;
            }
        }
        require(cancelled, "A stalled ascent retained its route beyond the finite policy budget");
        // Put the value pose directly under the same failed support. An
        // immediate retry would now issue a centered vertical takeoff; normal
        // fallback remains free to move and later use ordinary stuck recovery.
        view.center.x = view.supports.back().bounds.left() + view.size.x * 0.5f;
        const PlayerInputData after_expiry = stalled.think(view);
        require(!after_expiry.jump_pressed && !after_expiry.jump_held && !after_expiry.drop_through,
            "An expired ascent immediately retried the same failed support");
        view.alive = false;
        require_cancelled(stalled.think(view), "Death retained an expired ascent action");
        view.alive = true;
        const PlayerInputData respawned = first_decision(stalled, view, tuning, difficulty);
        require(respawned.jump_pressed && respawned.jump_held && !respawned.drop_through,
            "Respawn retained the failed-support exclusion from the previous life");
    }

    inline void check_continuing_chain_and_runup(const GameTuning& tuning, BotDifficulty difficulty)
    {
        for (const bool continuation : { false, true })
        {
            MatchBot bot(&tuning, difficulty, 7);
            MatchBotObservation view = floor_fixture(tuning);
            const float feet = view.size.y * 0.5f;
            view.obstacles.emplace_back(-400.0f, feet - 105.0f, 150.0f, 10.0f);
            view.supports.push_back({ view.obstacles.back(), true });
            view.obstacles.emplace_back(100.0f, feet - 160.0f, 300.0f, 10.0f);
            view.supports.push_back({ view.obstacles.back(), true });
            if (continuation)
            {
                view.obstacles.emplace_back(100.0f, feet - 335.0f, 300.0f, 10.0f);
                view.supports.push_back({ view.obstacles.back(), true });
            }
            const PlayerInputData input = first_decision(bot, view, tuning, difficulty);
            require(continuation ? input.x_movement > 0.0f : input.x_movement < 0.0f,
                "Ascent failed to prefer a reachable continuing chain over a lower dead-end shelf, or lost local ranking without it");
        }

        MatchBot crossing(&tuning, difficulty, 7);
        MatchBotObservation view = floor_fixture(tuning);
        const float feet = view.size.y * 0.5f;
        view.obstacles.front() = mattmath::RectangleF(-100.0f, feet, 150.0f, 100.0f);
        view.supports.front().bounds = view.obstacles.front();
        view.obstacles.emplace_back(175.0f, feet - 160.0f, 250.0f, 100.0f);
        view.supports.push_back({ view.obstacles.back(), false });
        require(first_decision(crossing, view, tuning, difficulty).x_movement > 0.0f,
            "Solid-gap fixture did not begin its known-floor approach");
        view.center.x = 44.0f; view.velocity.x = 200.0f;
        PlayerInputData input = crossing.think(view);
        require(input.x_movement > 0.0f && input.jump_pressed && input.jump_held,
            "A solid-gap takeoff braked away ordinary run-up speed instead of jumping inside the source lip");
        view.grounded = false; view.center.y -= 10.0f;
        input = crossing.think(view);
        require(input.x_movement > 0.0f && !input.drop_through,
            "A solid-gap jump stopped steering across before its feet cleared the target lip");
    }

    inline void check(const GameTuning& tuning)
    {
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            check_continuing_chain_and_runup(tuning, difficulty);
            check_lifetime(tuning, difficulty);
            check_scoring_descent_landing(tuning, difficulty);
            check_jump_boundary_and_local_routes(tuning, difficulty);
            check_short_steps_and_headroom(tuning, difficulty);
            // A centered one-way ascent needs a vertical takeoff, even though
            // the desired horizontal movement is zero. It must not become a
            // deliberate descent or inherit a jump before the reaction ends.
            MatchBot centered(&tuning, difficulty, 7);
            MatchBotObservation view = floor_fixture(tuning);
            raised_support(view, 140.0f, 300.0f);
            PlayerInputData input = first_decision(centered, view, tuning, difficulty);
            require(input.jump_pressed && input.jump_held && !input.drop_through,
                "A clear reachable centered one-way support did not receive an ordinary ascent jump");
            require(input.x_movement == 0.0f,
                "A centered vertical ascent invented horizontal movement away from its landing");

            for (int invalid = 0; invalid < 4; ++invalid)
            {
                MatchBot rejected(&tuning, difficulty, 7);
                view = floor_fixture(tuning);
                if (invalid == 0) { raised_support(view, 1000.0f, 300.0f); }
                if (invalid == 1) { raised_support(view, 140.0f, view.size.x * 0.5f); }
                if (invalid == 2)
                {
                    raised_support(view, 140.0f, 300.0f);
                    // The landing body is clear; a solid band between its
                    // feet and the initial head blocks the upward body path.
                    view.obstacles.emplace_back(-1000.0f, -70.0f, 2000.0f, 5.0f);
                }
                // invalid==3 intentionally has no raised support at all.
                input = first_decision(rejected, view, tuning, difficulty);
                require(!input.jump_pressed && !input.jump_held && !input.drop_through,
                    "An over-height, too-narrow, occluded or absent ascent support produced a jump/drop");
            }

            MatchBot starting_platform(&tuning, difficulty, 7);
            view = floor_fixture(tuning);
            view.on_platform = true;
            view.supports.front().one_way = true;
            raised_support(view, 140.0f, 300.0f);
            view.obstacles.emplace_back(-1000.0f, view.size.y * 3.5f, 2000.0f, 100.0f);
            view.supports.push_back({ view.obstacles.back(), false });
            input = first_decision(starting_platform, view, tuning, difficulty);
            require(input.drop_through && !input.jump_pressed && !input.jump_held,
                "A fresh one-way spawn abandoned its known lower escape for exploratory ascent");

            MatchBot enemy(&tuning, difficulty, 7);
            view = floor_fixture(tuning);
            raised_support(view, 140.0f, 300.0f);
            view.enemies.push_back({ { 100.0f, 0.0f }, {} });
            input = first_decision(enemy, view, tuning, difficulty);
            require(!input.jump_pressed && !input.jump_held && !input.drop_through &&
                input.x_movement == 0.0f && input.primary_shoot,
                "Exploratory ascent displaced an ordinary visible close-range engagement");
        }
    }
}
