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

    inline void check(const GameTuning& tuning)
    {
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            check_lifetime(tuning, difficulty);
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
