#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"

#include <cmath>
#include <iostream>

// Movement feedback: pulses, falls, seams and slopes, pause and lethal
// cleanup, presentation rejection, on/off equivalence and bounded storage.
namespace movement_acceptance
{
    void check_movement_feedback(const Suite& suite)
    {
        const GameTuning& tuning = suite.tuning;
        const MovementFeedbackPresentation& feedback = suite.feedback;
        const Presentation& presentation = suite.presentation;
        RenderResources& render = suite.render;
        AudioResources& audio = suite.audio;
        const PlaySettings& preferences = suite.preferences;
        const std::filesystem::path& content = suite.content;
        const ProjectileSpray& shot = suite.shot;
        const int fatal_hits = suite.fatal_hits;
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return suite.course(bottom, velocity);
        };

        auto pulses = course();
        pulses->floor();
        for (int tick = 0; tick < 30; ++tick) { pulses->tick(); }
        require(pulses->feedback_events.empty(),
            "initial settling or sustained support emitted a movement cue");
        require(pulses->player->animation_state() == PlayerAnimationState::idle,
            "initial settling or sustained support displayed a landing clip");
        const Vector2F launch_anchor(pulses->player->center().x,
            pulses->player->rectangle().bottom());
        pulses->tick(input(true, false));
        require(pulses->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            pulses->feedback_events.front().effect.position == launch_anchor,
            "grounded takeoff was not emitted once at its launch contact");
        require(pulses->player->animation_state() == PlayerAnimationState::taking_off &&
            pulses->player->action_age() == 0.0f && pulses->player->action_frame() == 0,
            "grounded takeoff did not display its first frame on the launch tick");
        bool pulse_landed = false;
        bool takeoff_second_frame = false;
        for (int tick = 0; tick < 150; ++tick)
        {
            pulses->tick();
            takeoff_second_frame = takeoff_second_frame ||
                (pulses->player->animation_state() == PlayerAnimationState::taking_off &&
                    pulses->player->action_frame() == 1);
            if (pulses->player->move_state() == PlayerMoveState::on_ground)
            {
                pulse_landed = true;
                break;
            }
        }
        require(pulse_landed && pulses->feedback_count(MovementFeedbackKind::landing) == 1,
            "small hop did not emit exactly one landing");
        require(takeoff_second_frame && pulses->player->animation_state() == PlayerAnimationState::landing &&
            pulses->player->action_age() == 0.0f && pulses->player->action_frame() == 0,
            "hop skipped the second takeoff frame or actual landing's immediate first frame");
        require(std::fabs(pulses->feedback_events.back().effect.position.y) < 0.01f,
            "landing cue was not attached to the resolved floor");
        const float longest_duration = std::max(feedback.takeoff.duration,
            feedback.landing.duration);
        const int expiry_ticks = static_cast<int>(std::ceil(longest_duration / dt)) + 1;
        bool landing_second_frame = false;
        for (int tick = 0; tick < expiry_ticks; ++tick)
        {
            pulses->tick();
            landing_second_frame = landing_second_frame ||
                (pulses->player->animation_state() == PlayerAnimationState::landing &&
                    pulses->player->action_frame() == 1);
        }
        require(pulses->player->movement_feedback().effects().empty() &&
            pulses->feedback_events.size() == 2,
            "movement cues leaked past expiry or repeated while standing");
        require(landing_second_frame && pulses->player->animation_state() == PlayerAnimationState::idle,
            "landing skipped its second frame or failed to return to idle");

        auto long_fall = course(0, { tuning.player().max_velocity.x, 0 });
        long_fall->leave_ledge();
        long_fall->surface({ -10000, 1000, 20000, 100 });
        for (int tick = 0; tick < 180; ++tick) { long_fall->tick(); }
        require(long_fall->feedback_count(MovementFeedbackKind::takeoff) == 0 &&
            long_fall->feedback_count(MovementFeedbackKind::landing) == 1 &&
            std::fabs(long_fall->feedback_events.back().effect.position.y - 1000) < 0.01f &&
            long_fall->feedback_events.back().effect.strength >
                pulses->feedback_events.back().effect.strength,
            "long non-jump fall did not produce one stronger cue at its actual landing");

        for (const auto type : { CollisionObjectType::structure,
            CollisionObjectType::structure_jump_through })
        {
            auto rebound = course();
            rebound->floor(type);
            rebound->tick();
            rebound->tick(input(true, false));
            bool queued = false;
            for (int tick = 0; tick < 150; ++tick)
            {
                const bool press = !queued && rebound->player->velocity().y > 0 &&
                    rebound->player->rectangle().bottom() > -25.0f;
                rebound->tick(input(press, false));
                queued = queued || press;
                if (rebound->feedback_count(MovementFeedbackKind::takeoff) == 2) { break; }
            }
            require(queued && rebound->feedback_events.size() == 3,
                "buffered rebound missed or duplicated a movement event");
            const auto& landing = rebound->feedback_events[1];
            const auto& takeoff = rebound->feedback_events[2];
            require(landing.effect.kind == MovementFeedbackKind::landing &&
                takeoff.effect.kind == MovementFeedbackKind::takeoff &&
                landing.tick == takeoff.tick &&
                landing.effect.position == takeoff.effect.position,
                "buffered rebound did not emit landing then takeoff at the same contact tick");
        }
        std::cout << "PASS movement feedback: takeoffs, small hops, long falls, buffered ordering, placement, expiry\n";

        auto seam = course();
        seam->surface({ -1000, 0, 1026, 100 });
        seam->surface({ 26, 0, 1000, 100 });
        seam->tick();
        seam->tick(input(true, false));
        for (int tick = 0; tick < 100; ++tick) { seam->tick(); }
        require(seam->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            seam->feedback_count(MovementFeedbackKind::landing) == 1,
            "two simultaneous support contacts duplicated a landing cue");
        for (int tick = 0; tick < 20; ++tick) { seam->tick(input(false, false, 1)); }
        require(seam->feedback_count(MovementFeedbackKind::landing) == 1,
            "walking across a platform seam retriggered landing feedback");
        require(seam->player->animation_state() != PlayerAnimationState::landing,
            "walking across a platform seam retriggered the landing clip");

        auto edge = course();
        edge->surface({ -1000, 0, 1020, 100 });
        edge->tick();
        edge->tick(input(true, false));
        require(edge->feedback_events.front().effect.position == Vector2F(10, 0),
            "edge takeoff was placed outside the actual support overlap");
        for (int tick = 0; tick < 100; ++tick) { edge->tick(); }
        require(edge->feedback_count(MovementFeedbackKind::landing) == 1 &&
            edge->feedback_events.back().effect.position == Vector2F(10, 0),
            "edge landing was placed outside the actual support overlap");

        for (const bool uphill_right : { false, true })
        {
            // Match the existing King of the Hill ramps' rise/run while leaving
            // enough width to sustain travel in both directions.
            const float gradient = 450.0f / 1400.0f;
            const float ramp_height = 1000.0f * gradient;
            auto slope = course(uphill_right ? -tuning.player().size.x * gradient : 0.0f);
            const TriangleRightAxisAligned triangle = uphill_right
                ? TriangleRightAxisAligned(1000, -ramp_height, -1000, ramp_height, 1000, ramp_height)
                : TriangleRightAxisAligned(-1000, -ramp_height, -1000, ramp_height, 1000, ramp_height);
            slope->ramp(triangle, uphill_right
                ? CollisionObjectType::structure_ramp_right
                : CollisionObjectType::structure_ramp_left);
            slope->tick();
            require(slope->player->move_state() == (uphill_right
                ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left),
                "slope fixture did not start supported");
            slope->tick(input(true, false));
            for (int tick = 0; tick < 120; ++tick) { slope->tick(); }
            require(slope->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
                slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "slope landing was missing or repeated under sustained ramp gravity");
            for (const auto& event : slope->feedback_events)
            {
                const Vector2F position = event.effect.position;
                const float expected_x = uphill_right ? tuning.player().size.x : 0.0f;
                const float expected_y = position.x * (uphill_right ? -gradient : gradient);
                require(std::fabs(position.x - expected_x) < 0.01f &&
                    std::fabs(position.y - expected_y) < 0.01f,
                    "ramp cue was not attached to the actual supporting corner");
            }
            for (int tick = 0; tick < 15; ++tick)
            {
                slope->tick(input(false, false, uphill_right ? 1.0f : -1.0f));
            }
            require(slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "walking uphill repeatedly emitted landing cues");
            for (int tick = 0; tick < 60; ++tick)
            {
                slope->tick(input(false, false, uphill_right ? -1.0f : 1.0f));
            }
            require(slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "running downhill repeatedly emitted landing cues");
        }
        std::cout << "PASS movement feedback: drop, ceiling, seams and both slope directions\n";

        auto frozen = course();
        frozen->floor();
        frozen->tick();
        frozen->tick(input(true, true));
        const auto before_pause = frozen->player->movement_feedback().effects().front();
        frozen->player->reset_jump_input();
        const auto& paused_effect = frozen->player->movement_feedback().effects().front();
        require(paused_effect.age == before_pause.age &&
            paused_effect.position == before_pause.position,
            "pause input reset cleared or advanced a live movement cue");
        frozen->tick(input(true, true));
        require(frozen->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            frozen->player->movement_feedback().effects().front().age > before_pause.age &&
            frozen->player->movement_feedback().effects().front().position == before_pause.position,
            "resume retriggered a takeoff or moved its world-space anchor");

        for (int hit = 0; hit < fatal_hits && frozen->player->health() > 0; ++hit)
        {
            frozen->player->on_contact(shot, Vector2F::ZERO, 0);
        }
        frozen->player->end_contacts();
        require(frozen->player->movement_feedback().effects().empty(),
            "fatal contact retained a live movement cue until the next update");
        frozen->tick(input(true, true));
        for (int tick = 0; tick < 360; ++tick) { frozen->tick(input(false, true)); }
        require(frozen->player->state() == PlayerState::alive &&
            frozen->player->move_state() == PlayerMoveState::on_ground &&
            frozen->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            frozen->feedback_count(MovementFeedbackKind::landing) == 0 &&
            frozen->player->movement_feedback().effects().empty(),
            "death/respawn settling emitted or retained a movement cue");
        frozen->tick();
        frozen->tick(input(true, false));
        for (int tick = 0; tick < 100; ++tick) { frozen->tick(); }
        require(frozen->feedback_count(MovementFeedbackKind::takeoff) == 2 &&
            frozen->feedback_count(MovementFeedbackKind::landing) == 1,
            "respawn failed to rearm ordinary movement feedback");
        std::cout << "PASS movement feedback: pause freeze/resume, lethal contact cleanup, respawn suppression and rearming\n";

        ContentVariant presentation_variant(content / "presentation.json");
        for (const auto& invalid : {
            std::pair{ "duration", "0" },
            std::pair{ "pixel_size", "-1" },
            std::pair{ "spread", "1e40" },
            std::pair{ "landing_min_speed", "1001" },
            std::pair{ "minimum_strength", "1.01" },
            std::pair{ "particle_size", "1" } })
        {
            bool rejected = false;
            try { presentation_variant.presentation(invalid.first, invalid.second); }
            catch (const std::exception&) { rejected = true; }
            require(rejected, "invalid movement presentation was accepted");
        }
        const Presentation without_feedback = presentation_variant.presentation("enabled", "false");
        Course decorated(tuning, feedback, presentation.combat_feedback(), presentation.respawn_redraw(), render, audio,
            preferences);
        Course undecorated(tuning, without_feedback.movement_feedback(),
            without_feedback.combat_feedback(), without_feedback.respawn_redraw(), render, audio, preferences);
        decorated.floor();
        undecorated.floor();
        for (int tick = 0; tick < 240; ++tick)
        {
            if (tick == 15)
            {
                decorated.player->reset_jump_input();
                undecorated.player->reset_jump_input();
            }
            const PlayerInputData actions = input(tick == 1 || tick == 120,
                tick >= 1 && tick < 45, tick < 80 ? 1.0f : -1.0f);
            decorated.tick(actions);
            undecorated.tick(actions);
            require(decorated.player->rectangle() == undecorated.player->rectangle() &&
                decorated.player->velocity() == undecorated.player->velocity() &&
                decorated.player->move_state() == undecorated.player->move_state() &&
                decorated.player->health() == undecorated.player->health() &&
                decorated.player->weapon_ammo() == undecorated.player->weapon_ammo(),
                "cosmetic movement feedback changed simulation or resources");
        }
        require(decorated.feedback_count(MovementFeedbackKind::takeoff) == 2 &&
            decorated.feedback_count(MovementFeedbackKind::landing) == 2 &&
            undecorated.feedback_events.empty(),
            "disabled feedback still emitted cues or equivalence route omitted jumps");

        MovementFeedback bounded(&feedback, &render);
        for (int burst = 0; burst < 40; ++burst)
        {
            bounded.takeoff({ static_cast<float>(burst), 0 });
        }
        require(bounded.effects().size() == 8 &&
            bounded.effects().front().position.x == 32 &&
            bounded.effects().back().position.x == 39,
            "repeated movement cues exceeded capacity or discarded the newest cue");
        bounded.clear();
        require(bounded.effects().empty(), "movement cue cleanup retained live effects");
        bounded.land({ 1000, 1000 }, feedback.landing_min_speed / 2.0f);
        require(bounded.effects().empty(), "micro-detach impact emitted a landing cue");
        bounded.takeoff({ 1000, 1000 });
        bounded.land({ -1000, -1000 }, feedback.full_impact_speed);
        const RectangleF actor(-26, -120, 52, 120);
        const RectangleF expanded = bounded.bounds(actor);
        require(expanded.contains(actor) &&
            expanded.contains(RectangleF(999, 999, 2, 2)) &&
            expanded.contains(RectangleF(-1001, -1001, 2, 2)),
            "culling bounds omitted a live effect left behind by its player");
        bounded.update(longest_duration);
        require(bounded.effects().empty() && bounded.bounds(actor) == actor,
            "expired cues remained alive or kept stale culling bounds");
        std::cout << "PASS movement feedback: presentation rejection, on/off simulation equivalence, bounded storage and culling lifetime\n";
    }
}
