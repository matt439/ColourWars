#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"

#include <cmath>
#include <iostream>

// The ceiling bonk, disabled buffer and grace windows, and tuning validation.
namespace movement_acceptance
{
    void check_ceiling_and_windows(const Suite& suite)
    {
        const GameTuning& tuning = suite.tuning;
        const MovementFeedbackPresentation& feedback = suite.feedback;
        const Presentation& presentation = suite.presentation;
        RenderResources& render = suite.render;
        AudioResources& audio = suite.audio;
        const PlaySettings& preferences = suite.preferences;
        const std::filesystem::path& content = suite.content;
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return suite.course(bottom, velocity);
        };

        auto ceiling = course();
        ceiling->floor();
        ceiling->surface({ -1000, -180, 2000, 20 });
        ceiling->tick();
        ceiling->tick(input(true, true));
        bool bonked = false;
        for (int tick = 0; tick < 30; ++tick)
        {
            ceiling->tick(input(false, true));
            bonked = bonked || ceiling->player->move_state() == PlayerMoveState::on_ceiling;
            require(ceiling->player->rectangle().top() >= -160.01f,
                "jump escaped the ceiling");
        }
        require(bonked && ceiling->player->velocity().y == 0,
            "ceiling contact did not return the player to the floor");
        require(ceiling->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            ceiling->feedback_count(MovementFeedbackKind::landing) == 1,
            "ceiling contact emitted a landing or duplicated the return-to-floor cue");

        auto short_return = course();
        short_return->floor();
        short_return->surface({ -1000, -145, 2000, 20 });
        short_return->tick();
        short_return->tick(input(true, false));
        bool rapid_landing = false;
        for (int tick = 0; tick < 15; ++tick)
        {
            short_return->tick();
            if (short_return->feedback_count(MovementFeedbackKind::landing) == 1)
            {
                require(short_return->player->animation_state() == PlayerAnimationState::landing &&
                    short_return->player->action_age() == 0.0f,
                    "rapid low-ceiling return retained takeoff instead of its genuine landing");
                rapid_landing = true;
                break;
            }
        }
        require(rapid_landing, "low-ceiling fixture did not return to its floor");

        ContentVariant variant(content / "tuning.json");
        const GameTuning disabled = variant.tuning("0.0", "0.0");
        Course unbuffered(disabled, feedback, presentation.combat_feedback(), presentation.respawn_redraw(), render, audio,
            preferences, -4, { 0, 100 });
        unbuffered.floor();
        unbuffered.tick(input(true, true));
        unbuffered.tick(input(false, true));
        require(unbuffered.player->velocity().y == 0, "zero did not disable jump buffering");
        unbuffered.tick();
        unbuffered.tick(input(true, true));
        require(unbuffered.player->velocity().y < 0,
            "zero buffer disabled ordinary grounded jumps");
        Course ungraced(disabled, feedback, presentation.combat_feedback(), presentation.respawn_redraw(), render, audio,
            preferences, 0,
            { tuning.player().max_velocity.x, 0 });
        ungraced.leave_ledge();
        ungraced.tick(input(true, true, 1));
        require(ungraced.player->velocity().y > 0, "zero did not disable ledge grace");
        for (const char* invalid : { "-0.01", "1.01", "1e40" })
        {
            for (const bool buffer : { false, true })
            {
                bool rejected = false;
                try { variant.tuning(buffer ? invalid : "0", buffer ? "0" : invalid); }
                catch (const std::exception&) { rejected = true; }
                require(rejected, "invalid movement forgiveness duration was accepted");
            }
        }
        std::cout << "PASS ceiling, disabled windows and tuning validation\n";
    }
}
