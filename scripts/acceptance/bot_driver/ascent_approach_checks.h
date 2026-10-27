#pragma once

#include "scripts/acceptance/bot_driver/ascent_checks.h"

namespace bot_ascent_approach_acceptance
{
    using bot_ascent_acceptance::first_decision;
    using bot_ascent_acceptance::floor_fixture;
    using bot_ascent_acceptance::require;

    inline MatchBotObservation fixture(const GameTuning& tuning, float direction,
        float distance = 800.0f)
    {
        MatchBotObservation view = floor_fixture(tuning);
        view.obstacles.front() = mattmath::RectangleF(-2500.0f, view.size.y * 0.5f, 5000.0f, 100.0f);
        view.supports.front().bounds = view.obstacles.front();
        const float left = direction > 0.0f ? distance : -distance - 300.0f;
        view.obstacles.emplace_back(left, view.size.y * 0.5f - 140.0f, 300.0f, 10.0f);
        view.supports.push_back({ view.obstacles.back(), true });
        // Fallback seeks paint in the opposite direction. Walking toward the
        // raised destination therefore distinguishes ascent from wandering.
        view.territory.push_back({ -direction * 200.0f, 0.0f });
        return view;
    }

    inline void check(const GameTuning& tuning)
    {
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            for (const float direction : { -1.0f, 1.0f })
            {
                MatchBot nearby(&tuning, difficulty, 7);
                MatchBotObservation alternatives = floor_fixture(tuning);
                alternatives.obstacles.front() = mattmath::RectangleF(-2500.0f,
                    alternatives.size.y * 0.5f, 5000.0f, 100.0f);
                alternatives.supports.front().bounds = alternatives.obstacles.front();
                const float feet = alternatives.size.y * 0.5f;
                const float local_left = direction > 0.0f ? 100.0f : -400.0f;
                const float remote_left = direction > 0.0f ? -1100.0f : 800.0f;
                for (const float rise : { 140.0f, 315.0f })
                {
                    alternatives.obstacles.emplace_back(local_left, feet - rise, 300.0f, 10.0f);
                    alternatives.supports.push_back({ alternatives.obstacles.back(), true });
                }
                for (const float rise : { 80.0f, 255.0f })
                {
                    alternatives.obstacles.emplace_back(remote_left, feet - rise, 300.0f, 10.0f);
                    alternatives.supports.push_back({ alternatives.obstacles.back(), true });
                }
                // Both shelves continue upward. The lower remote shelf is
                // cheaper only if the supported walk beyond local jump reach
                // is omitted; the nearby chain stays within that locality.
                const PlayerInputData local = first_decision(nearby, alternatives, tuning, difficulty);
                require(local.x_movement == direction && !local.jump_pressed && !local.drop_through,
                    "A lower remote shelf displaced a useful nearby continuing ascent");

                MatchBot bot(&tuning, difficulty, 7);
                MatchBotObservation view = fixture(tuning, direction);
                PlayerInputData input = first_decision(bot, view, tuning, difficulty);
                require(input.x_movement == direction && !input.jump_pressed && !input.drop_through,
                    "A long supported approach was rejected using the airborne jump reach");
                const mattmath::RectangleF& destination = view.supports.back().bounds;
                const float takeoff = direction > 0.0f ? destination.left() + view.size.x * 0.5f :
                    destination.right() - view.size.x * 0.5f;
                const float stride = tuning.player().x_initial_velocity / 60.0f;
                int walked = 0;
                // Value poses model supported progress at ordinary minimum
                // moving speed. Arena trials separately integrate real physics.
                while (std::fabs(takeoff - view.center.x) > stride + 3.0f)
                {
                    view.center.x += direction * stride;
                    view.velocity.x = direction * tuning.player().x_initial_velocity;
                    input = bot.think(view);
                    require(input.x_movement * direction >= 0.0f && !input.jump_pressed && !input.drop_through,
                        "A supported ascent approach expired, reversed or jumped before its takeoff");
                    require(++walked < 1000, "Long-approach fixture exceeded its finite walk budget");
                }
                require(walked > tuning.bots().progress_window_steps + tuning.bots().recovery_steps,
                    "Long-approach fixture failed to exceed the old fixed attempt budget");
                view.center.x = takeoff;
                view.velocity.x = 0.0f;
                input = bot.think(view);
                require(input.jump_pressed && input.jump_held && !input.drop_through,
                    "A completed long supported approach did not launch its selected ascent");

                MatchBot stalled(&tuning, difficulty, 7);
                view = fixture(tuning, direction);
                require(first_decision(stalled, view, tuning, difficulty).x_movement == direction,
                    "Long-ascent expiry fixture did not select its supported approach");
                bool cancelled = false;
                const int maximum_steps = static_cast<int>(std::ceil(tuning.bots().sight_distance /
                    tuning.player().x_initial_velocity * 60.0f)) +
                    tuning.bots().progress_window_steps + tuning.bots().recovery_steps;
                for (int step = 0; step < maximum_steps; ++step)
                {
                    input = stalled.think(view);
                    require(!input.jump_pressed && !input.jump_held && !input.drop_through,
                        "A stalled long approach skipped its still-distant takeoff");
                    if (input.x_movement == 0.0f)
                    {
                        bot_ascent_acceptance::require_cancelled(input,
                            "A stalled long approach retained part of its action on expiry");
                        cancelled = true;
                        break;
                    }
                }
                require(cancelled, "A stalled long approach retained its route beyond a finite sight-derived budget");

                for (const bool too_far : { false, true })
                {
                    MatchBot rejected(&tuning, difficulty, 7);
                    view = fixture(tuning, direction,
                        too_far ? tuning.bots().sight_distance + 300.0f : 800.0f);
                    if (!too_far)
                    {
                        // The walk is short, but the airborne gap from this
                        // source lip exceeds normal jump reach.
                        view.obstacles.front() = direction > 0.0f ?
                            mattmath::RectangleF(-500.0f, view.size.y * 0.5f, 800.0f, 100.0f) :
                            mattmath::RectangleF(-300.0f, view.size.y * 0.5f, 800.0f, 100.0f);
                        view.supports.front().bounds = view.obstacles.front();
                    }
                    input = first_decision(rejected, view, tuning, difficulty);
                    require(input.x_movement == -direction && !input.jump_pressed && !input.drop_through,
                        too_far ? "An ascent walk exceeded the sight bound instead of retaining its paint goal" :
                        "A long unsupported gap was accepted as a supported ascent approach");
                }
            }
        }
    }
}
