#pragma once

#include "scripts/acceptance/bot_driver/ascent_checks.h"

namespace bot_ramp_acceptance
{
    inline MatchBotObservation fixture(const GameTuning& tuning, bool rises_right)
    {
        MatchBotObservation view = bot_ascent_acceptance::floor_fixture(tuning);
        const float feet = view.size.y * 0.5f;
        view.obstacles.clear(); view.supports.clear();
        const mattmath::RectangleF floor(rises_right ? -1000.0f : -100.0f, feet, 1100.0f, 100.0f);
        const mattmath::RectangleF ramp(rises_right ? 100.0f : -1500.0f, feet - 450.0f, 1400.0f, 450.0f);
        const mattmath::RectangleF summit(rises_right ? 1500.0f : -2500.0f, feet - 450.0f, 1000.0f, 450.0f);
        view.obstacles = { floor, ramp, summit };
        view.supports = { { floor, false }, { summit, false } };
        view.ramps.push_back({ ramp, rises_right });
        return view;
    }

    inline void check(const GameTuning& tuning)
    {
        using bot_ascent_acceptance::require;
        using bot_ascent_acceptance::first_decision;
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            for (const bool right : { false, true })
            {
                const float direction = right ? 1.0f : -1.0f;
                MatchBot bot(&tuning, difficulty, 7);
                MatchBotObservation view = fixture(tuning, right);
                PlayerInputData input = first_decision(bot, view, tuning, difficulty);
                require(input.x_movement == direction && !input.jump_held && !input.drop_through,
                    "A clear known upright ramp did not begin an ordinary uphill walk after its reaction delay");
                // Value poses isolate policy continuation; actual-driver hill
                // trials establish physical ramp contact and crest traversal.
                view.center.x = direction * 600.0f;
                const float corner = view.center.x + direction * view.size.x * 0.5f;
                const MatchBotRamp& ramp = view.ramps.front();
                const float along = (corner - ramp.bounds.left()) / ramp.bounds.width;
                const float feet = right ? ramp.bounds.bottom() - along * ramp.bounds.height :
                    ramp.bounds.top() + along * ramp.bounds.height;
                view.center.y = feet - view.size.y * 0.5f;
                input = bot.think(view);
                require(input.x_movement == direction && !input.jump_held && !input.drop_through,
                    "A ramp walk lost its uphill direction at a valid supporting-corner pose");
                view.ramps.clear();
                bot_ascent_acceptance::require_cancelled(bot.think(view),
                    "A vanished ramp retained its walk or jump/drop action");

                MatchBot interrupted(&tuning, difficulty, 7);
                view = fixture(tuning, right);
                (void)first_decision(interrupted, view, tuning, difficulty);
                view.alive = false;
                bot_ascent_acceptance::require_cancelled(interrupted.think(view),
                    "Death retained an active ramp walk");
                view.alive = true; view.ramps.clear();
                // A close enemy stops unrelated exploratory movement and
                // prevents fallback from masking an inherited ramp action.
                view.enemies.push_back({ { direction * 100.0f, 0.0f }, {} });
                input = first_decision(interrupted, view, tuning, difficulty);
                require(input.x_movement == 0.0f && !input.jump_held && !input.drop_through,
                    "Respawn retained a ramp direction after its ordinary reaction delay");

                MatchBot stalled(&tuning, difficulty, 7);
                view = fixture(tuning, right);
                (void)first_decision(stalled, view, tuning, difficulty);
                const float approach = 100.0f - view.size.x * 0.5f;
                const int budget = static_cast<int>(std::ceil((approach + 1400.0f + view.size.x) /
                    tuning.player().x_initial_velocity * 60.0f)) +
                    tuning.bots().progress_window_steps + tuning.bots().recovery_steps;
                bool cancelled = false;
                for (int step = 0; step < budget; ++step)
                {
                    input = stalled.think(view);
                    require(!input.jump_held && !input.drop_through,
                        "A stalled ramp walk emitted a jump/drop before its finite expiry");
                    if (input.x_movement == 0.0f) { cancelled = true; break; }
                }
                require(cancelled, "A stalled ramp walk exceeded its finite travel budget");

                MatchBot blocked(&tuning, difficulty, 7);
                view = fixture(tuning, right);
                (void)first_decision(blocked, view, tuning, difficulty);
                view.obstacles.emplace_back(right ? 500.0f : -800.0f, -240.0f, 300.0f, 40.0f);
                bot_ascent_acceptance::require_cancelled(blocked.think(view),
                    "A newly obstructed ramp path retained its walk");

                MatchBot opposed(&tuning, difficulty, 7);
                view = fixture(tuning, right);
                (void)first_decision(opposed, view, tuning, difficulty);
                view.enemies.push_back({ { direction * 100.0f, 0.0f }, {} });
                for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step)
                { input = opposed.think(view); }
                require(input.x_movement == 0.0f && input.primary_shoot && !input.jump_held && !input.drop_through,
                    "A ramp walk displaced a visible close enemy after the normal reaction interval");

                for (int invalid = 0; invalid < 3; ++invalid)
                {
                    MatchBot rejected(&tuning, difficulty, 7);
                    view = fixture(tuning, right);
                    if (invalid == 0)
                    {
                        // The current floor is known, but it ends before
                        // reaching the toe: walking there would cross a gap.
                        view.obstacles.front() = mattmath::RectangleF(right ? -1000.0f : -50.0f,
                            view.size.y * 0.5f, 1050.0f, 100.0f);
                        view.supports.front().bounds = view.obstacles.front();
                    }
                    if (invalid == 1) { view.supports.pop_back(); }
                    if (invalid == 2)
                    {
                        // A ceiling intersects the known rising body path,
                        // although the ramp's toe and crest remain clear.
                        view.obstacles.emplace_back(right ? 500.0f : -800.0f, -240.0f, 300.0f, 40.0f);
                    }
                    // An unrelated raised one-way surface directly overhead
                    // supplies a distinct eligible alternative. Rejection of
                    // the ramp must leave this ordinary jump available.
                    view.obstacles.emplace_back(-150.0f, view.size.y * 0.5f - 140.0f, 300.0f, 10.0f);
                    view.supports.push_back({ view.obstacles.back(), true });
                    input = first_decision(rejected, view, tuning, difficulty);
                    require(input.jump_pressed && input.jump_held && !input.drop_through,
                        "An unknown-floor, unknown-exit or obstructed ramp displaced a valid adjacent ascent");
                }
            }
        }
    }
}
