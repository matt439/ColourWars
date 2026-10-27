#include "game/pch.h"
#include "game/ai/match_bot.h"
#include <filesystem>
#include <iostream>

namespace
{
    using namespace mattmath;

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    MatchBotObservation fixture(const GameTuning& tuning)
    {
        MatchBotObservation view;
        view.alive = view.grounded = true;
        view.size = tuning.player().size;
        view.ammo = 1.0f;
        view.engage_distance = 800.0f;
        view.projectile_speed = 1000.0f;
        view.territory.push_back({ 500.0f, 50.0f });
        view.obstacles.emplace_back(-4000.0f, view.size.y * 0.5f, 8000.0f, 100.0f);
        view.supports.push_back({ view.obstacles.back(), false });
        return view;
    }

    PlayerInputData first_recovery(MatchBot& bot, MatchBotObservation& view,
        const GameTuning& tuning, BotDifficulty difficulty, bool oscillating)
    {
        bool moved_right = false;
        const int limit = tuning.bots().stuck_turn_steps + tuning.bots().progress_window_steps +
            tuning.bot(difficulty).reaction_steps * 3;
        for (int step = 0; step < limit; ++step)
        {
            // The velocity-only detector cannot see a collision loop whose
            // integrated velocity is nonzero but resolved positions repeat.
            if (oscillating)
            {
                view.center.x = step % 2 == 0 ? 0.0f : tuning.bots().progress_distance * 0.1f;
                view.velocity.x = step % 2 == 0 ? 120.0f : -120.0f;
            }
            const PlayerInputData input = bot.think(view);
            require(input.connected && !input.toggle_pause_menu && !input.pause_held,
                "Recovery emitted an unavailable device/menu action");
            moved_right = moved_right || input.x_movement > 0.0f;
            if (input.x_movement < 0.0f)
            {
                require(moved_right, "A failed goal never received its original movement decision");
                return input;
            }
        }
        throw std::runtime_error(oscillating ?
            "Nonzero-velocity positional oscillation never triggered recovery" :
            "A blocked bot never reversed into recovery");
    }

    void check_commitment(const GameTuning& tuning, BotDifficulty difficulty, bool oscillating)
    {
        MatchBot bot(&tuning, difficulty, 19);
        MatchBotObservation view = fixture(tuning);
        (void)first_recovery(bot, view, tuning, difficulty, oscillating);
        require(!bot.needs_territory(), "Recovery immediately requested the same failed territory");
        // Keep the original target visible and begin moving away. A successful
        // escape must retain its direction beyond a single policy reaction.
        for (int step = 1; step < tuning.bots().recovery_steps; ++step)
        {
            require(!bot.needs_territory(), "A bot resampled territory before escape commitment ended");
            view.center.x -= 2.0f;
            view.velocity.x = -120.0f;
            const PlayerInputData input = bot.think(view);
            require(input.x_movement < 0.0f && input.left_analog_stick.x < 0.0f,
                "A bot abandoned its committed escape for the visible failed goal");
        }
        require(bot.needs_territory(), "Escape did not release territory sampling after its final output");
        bool replanned = false;
        const int limit = tuning.bots().recovery_steps + tuning.bot(difficulty).replan_steps +
            tuning.bot(difficulty).reaction_steps;
        for (int step = 0; step < limit; ++step)
        {
            replanned = replanned || bot.needs_territory();
            view.center.x -= 2.0f;
            view.velocity.x = -120.0f;
            (void)bot.think(view);
        }
        require(replanned, "A completed escape permanently disabled territory replanning");
    }

    void check_death_reset(const GameTuning& tuning, BotDifficulty difficulty)
    {
        MatchBot bot(&tuning, difficulty, 19);
        MatchBotObservation view = fixture(tuning);
        (void)first_recovery(bot, view, tuning, difficulty, false);
        view.alive = false;
        const PlayerInputData dead = bot.think(view);
        require(dead.x_movement == 0.0f && !dead.jump_pressed && !dead.jump_held &&
            !dead.drop_through && !dead.primary_shoot, "Death retained a recovery action");
        require(bot.needs_territory(), "Death retained the escape territory holdoff");
        view.alive = true;
        view.velocity.x = 120.0f;
        PlayerInputData input;
        for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step)
        {
            input = bot.think(view);
            if (step + 1 < tuning.bot(difficulty).reaction_steps)
            {
                require(input.x_movement == 0.0f, "Respawn bypassed its ordinary reaction delay");
            }
        }
        require(input.x_movement > 0.0f, "Respawn retained its pre-death escape direction");
    }

    void check_progress_and_idle(const GameTuning& tuning, BotDifficulty difficulty)
    {
        MatchBot moving(&tuning, difficulty, 19);
        MatchBotObservation view = fixture(tuning);
        const int limit = (tuning.bots().progress_window_steps + tuning.bots().recovery_steps) * 3;
        const float stride = tuning.bots().progress_distance * 2.0f /
            static_cast<float>(tuning.bots().progress_window_steps);
        for (int step = 0; step < limit; ++step)
        {
            view.center.x += stride;
            view.velocity.x = stride * 60.0f;
            view.territory = { view.center + Vector2F(500.0f, 50.0f) };
            require(moving.think(view).x_movement >= 0.0f,
                "A bot making real positional progress falsely entered escape");
        }

        MatchBot idle(&tuning, difficulty, 19);
        view = fixture(tuning);
        view.enemies.push_back({ { 100.0f, 0.0f }, {} });
        for (int step = 0; step < limit; ++step)
        {
            require(idle.think(view).x_movement == 0.0f,
                "An intentional close-range combat stop falsely entered escape");
        }
        view.enemies.clear();
        view.velocity.x = 120.0f;
        bool advanced = false;
        for (int step = 0; step < tuning.bot(difficulty).reaction_steps * 2; ++step)
        {
            const PlayerInputData input = idle.think(view);
            require(input.x_movement >= 0.0f, "Intentional idle accumulated a stale recovery window");
            advanced = advanced || input.x_movement > 0.0f;
        }
        require(advanced, "A bot never resumed ordinary territory movement after combat idle");
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 2, "Usage: bot_recovery <staged-content-directory>");
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
        {
            check_commitment(tuning, difficulty, false);
            check_commitment(tuning, difficulty, true);
            check_death_reset(tuning, difficulty);
            check_progress_and_idle(tuning, difficulty);
        }
        std::cout << "PASS bot recovery: all difficulties retain finite escape, resume replanning, reset on death, "
            "detect positional oscillation and preserve real progress/intentional idle\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL bot recovery: " << error.what() << '\n';
        return 1;
    }
}
