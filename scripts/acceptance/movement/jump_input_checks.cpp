#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"

#include <cmath>
#include <iostream>

// The landing buffer, ledge grace, count-in release, and pause and
// platform-drop input isolation.
namespace movement_acceptance
{
    void check_jump_input(const Suite& suite)
    {
        const GameTuning& tuning = suite.tuning;
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return suite.course(bottom, velocity);
        };

        for (const auto type : { CollisionObjectType::structure,
            CollisionObjectType::structure_jump_through })
        {
            auto buffered = course(-4, { 0, 100 });
            buffered->floor(type);
            buffered->tick(input(true, false)); // A tap released before landing.
            require(buffered->player->velocity().y > 0,
                "airborne press launched without support");
            require(buffered->player->animation_state() != PlayerAnimationState::taking_off,
                "queued airborne jump displayed takeoff before launch");
            buffered->tick();
            require(buffered->player->move_state() == PlayerMoveState::jumping &&
                buffered->player->velocity().y == tuning.player().jump_launch_velocity,
                "landing did not consume buffered tap on its contact tick");
            require(buffered->player->animation_state() == PlayerAnimationState::taking_off &&
                buffered->player->action_age() == 0.0f && buffered->player->action_frame() == 0,
                "buffered launch did not display its first takeoff frame on the contact tick");

            auto expired = course(-400);
            expired->floor(type);
            expired->tick(input(true, true));
            for (int tick = 0; tick < 120; ++tick)
            {
                expired->tick(input(false, true));
            }
            require(expired->player->velocity().y == 0 &&
                std::fabs(expired->player->rectangle().bottom()) < 0.01f,
                "expired buffer or held button repeated a jump on landing");
        }
        std::cout << "PASS landing buffer: solid/platform, released taps, expiry, held input\n";

        auto grace = course(0, { tuning.player().max_velocity.x, 0 });
        grace->leave_ledge();
        grace->tick(input(true, true, 1));
        require(grace->player->move_state() == PlayerMoveState::jumping,
            "fresh ledge grace jump failed");
        require(grace->feedback_count(MovementFeedbackKind::takeoff) == 1,
            "successful grace jump did not produce exactly one takeoff cue");
        require(grace->player->animation_state() == PlayerAnimationState::taking_off &&
            grace->player->action_age() == 0.0f,
            "successful grace jump did not display takeoff immediately");
        grace->tick(input(false, false, 1));
        grace->tick(input(true, false, 1));
        require(grace->player->velocity().y > tuning.player().jump_launch_velocity,
            "one takeoff left a second grace jump available");

        auto late = course(0, { tuning.player().max_velocity.x, 0 });
        late->leave_ledge();
        const int grace_ticks = static_cast<int>(std::ceil(tuning.player().ledge_grace_time / dt)) + 1;
        for (int tick = 0; tick < grace_ticks; ++tick) { late->tick(input(false, false, 1)); }
        late->tick(input(true, true, 1));
        require(late->player->velocity().y > 0, "expired ledge grace allowed a jump");
        require(late->feedback_count(MovementFeedbackKind::takeoff) == 0,
            "failed grace jump produced a takeoff cue");
        require(late->player->animation_state() != PlayerAnimationState::taking_off,
            "failed grace jump displayed takeoff");
        std::cout << "PASS ledge grace: first jump, expiry, no midair second jump\n";

        auto paused = course(-4, { 0, 100 });
        paused->floor();
        paused->tick(input(true, true));
        paused->player->reset_jump_input();
        paused->tick(input(true, true));
        require(paused->player->velocity().y == 0,
            "pause left a queued/menu-held landing jump");
        paused->tick(input(false, true));
        require(paused->player->velocity().y == 0, "pause held jump was rearmed");
        paused->tick();
        paused->tick(input(true, true));
        require(paused->player->velocity().y < 0, "release did not rearm jump after pause");

        auto counted_release = course();
        counted_release->floor();
        counted_release->tick();
        counted_release->player->reset_jump_input();
        const RectangleF counted_position = counted_release->player->rectangle();
        const Vector2F counted_velocity = counted_release->player->velocity();
        const float counted_health = counted_release->player->health();
        const float counted_ammo = counted_release->player->weapon_ammo();
        counted_release->player->observe_jump_release(false);
        require(counted_release->player->rectangle() == counted_position &&
            counted_release->player->velocity() == counted_velocity &&
            counted_release->player->health() == counted_health &&
            counted_release->player->weapon_ammo() == counted_ammo,
            "Observing a count-in release advanced the frozen player");
        counted_release->tick(input(true, true));
        require(counted_release->player->velocity().y < 0 &&
            counted_release->feedback_count(MovementFeedbackKind::takeoff) == 1,
            "A neutral count-in sample failed to permit the first live jump press");

        auto counted_hold = course();
        counted_hold->floor();
        counted_hold->tick();
        counted_hold->player->reset_jump_input();
        counted_hold->player->observe_jump_release(true);
        counted_hold->tick(input(true, true));
        require(counted_hold->player->velocity().y == 0 &&
            counted_hold->feedback_count(MovementFeedbackKind::takeoff) == 0,
            "A jump held through count-in became a fresh live jump");
        counted_hold->player->observe_jump_release(false);
        counted_hold->tick(input(true, true));
        require(counted_hold->player->velocity().y < 0,
            "Releasing a held count-in jump did not rearm the next press");

        auto paused_grace = course(0, { tuning.player().max_velocity.x, 0 });
        paused_grace->leave_ledge();
        paused_grace->player->reset_jump_input();
        paused_grace->tick(input(false, false, 1));
        paused_grace->tick(input(true, true, 1));
        require(paused_grace->player->velocity().y > 0,
            "pause retained a ledge grace jump");

        auto uninterrupted = course();
        auto suspended = course();
        uninterrupted->floor();
        suspended->floor();
        uninterrupted->tick();
        suspended->tick();
        bool returned_to_floor = false;
        for (int tick = 0; tick < 150; ++tick)
        {
            if (tick == 10)
            {
                suspended->player->reset_jump_input();
                suspended->player->observe_jump_release(true);
            }
            uninterrupted->tick(input(tick == 0, true));
            // Model the held jump plus an activation edge from the menu on
            // resume. It must not become a new buffer or cut the current jump.
            suspended->tick(input(tick == 0 || tick == 10, true));
            require(suspended->player->rectangle().position() ==
                    uninterrupted->player->rectangle().position() &&
                suspended->player->velocity() == uninterrupted->player->velocity() &&
                suspended->player->move_state() == uninterrupted->player->move_state(),
                "pause/resume changed an existing held jump trajectory");
            returned_to_floor = returned_to_floor ||
                suspended->player->move_state() == PlayerMoveState::on_ground;
        }
        require(returned_to_floor && suspended->player->velocity().y == 0,
            "pause/resume queued an extra jump while the button remained held");
        std::cout << "PASS count-in jump release: first live press, held suppression and preserved airborne arc\n";

        auto dropped = course();
        dropped->floor(CollisionObjectType::structure_jump_through);
        dropped->surface({ -10000, 80, 20000, 100 });
        dropped->tick();
        dropped->tick(input(false, false, 0, true));
        require(dropped->player->move_state() == PlayerMoveState::dropping_down,
            "drop did not leave the platform");
        dropped->tick(input(true, true));
        require(dropped->player->velocity().y > 0, "drop supplied a grace jump");
        for (int tick = 0; tick < 60; ++tick) { dropped->tick(input(false, true)); }
        require(dropped->player->velocity().y == 0 &&
            std::fabs(dropped->player->rectangle().bottom() - 80) < 0.01f,
            "drop carried a held/queued jump to the lower floor");
        require(dropped->feedback_count(MovementFeedbackKind::takeoff) == 0 &&
            dropped->feedback_count(MovementFeedbackKind::landing) == 1,
            "platform drop emitted a takeoff or missed/duplicated its lower landing");
        dropped->tick();
        dropped->tick(input(true, true));
        require(dropped->player->velocity().y < 0, "drop failed to rearm after release");

        auto simultaneous = course();
        simultaneous->floor(CollisionObjectType::structure_jump_through);
        simultaneous->tick();
        simultaneous->tick(input(true, true, 0, true));
        require(simultaneous->player->move_state() == PlayerMoveState::jumping,
            "fresh jump lost its precedence over simultaneous drop");

        auto buffered_drop = course(-4, { 0, 100 });
        buffered_drop->floor(CollisionObjectType::structure_jump_through);
        buffered_drop->tick(input(true, false));
        buffered_drop->tick(input(false, false, 0, true));
        require(buffered_drop->player->move_state() == PlayerMoveState::on_drop_down_ground,
            "old airborne buffer overrode a held drop on platform landing");
        buffered_drop->tick(input(false, false, 0, true));
        require(buffered_drop->player->move_state() == PlayerMoveState::dropping_down,
            "held drop did not proceed after buffered platform landing");
        std::cout << "PASS pause/release and platform-drop input isolation\n";
    }
}
