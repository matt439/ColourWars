#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"

// The shared helpers fixtures.h declares.
namespace movement_course_acceptance
{
    [[noreturn]] void fail(const std::string& message)
    {
        throw std::runtime_error(message);
    }

    std::string fixed(float value, int precision)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    PlayerInputData input(bool pressed, bool held,
        float movement, bool drop)
    {
        PlayerInputData result;
        result.connected = true;
        result.jump_pressed = pressed;
        result.jump_held = held;
        result.x_movement = movement;
        result.drop_through = drop;
        return result;
    }

    PlayerInputData aimed(float angle, PlayerInputData actions)
    {
        actions.shoot_direction_requested = true;
        actions.shoot_angle = angle;
        actions.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return actions;
    }

    // COPIED from PlayerInput::pad_input (game/objects/player_input.cpp). It
    // reads the engine's concrete XInput Gamepads, which has no seam for raw
    // stick values, so these three mappings must be kept in step by hand.
    float pad_movement(float raw_x, const GameTuning& tuning)
    {
        return apply_deadzone(raw_x, tuning.input().movement_deadzone);
    }

    bool pad_drop(float raw_y, const GameTuning& tuning)
    {
        return raw_y > tuning.player().drop_down_analog_threshold;
    }

    PlayerInputData pad_aim(const Vector2F& raw, const PlaySettings& preferences,
        PlayerInputData actions)
    {
        const Vector2F aim = apply_deadzone(raw, preferences.aim_deadzone());
        actions.shoot_direction_requested = aim != Vector2F::ZERO;
        if (actions.shoot_direction_requested)
        {
            actions.shoot_direction = aim.normalized();
            actions.shoot_angle = actions.shoot_direction.angle();
        }
        return actions;
    }

    // Player::update_movement counts the jump buffer and ledge grace down once
    // per update. True when a window opened on one update is still positive
    // after `elapsed` further updates, using the player's own arithmetic.
    bool window_open(float window, int elapsed)
    {
        float remaining = window;
        for (int tick = 0; tick < elapsed; ++tick)
        {
            remaining = std::max(0.0f, remaining - dt);
        }
        return remaining > 0.0f;
    }

    bool supported(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground;
    }

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x)
    {
        const Vector2F size = s.tuning.player().size;
        return { centre_x - size.x / 2.0f, surface.rectangle.top() - size.y };
    }

    // The top-left that puts the leading edge, in `direction`, at `front`.
    Vector2F front_at(const Services& s, const Surface& surface, float front, float direction)
    {
        const Vector2F size = s.tuning.player().size;
        return { direction > 0.0f ? front - size.x : front, surface.rectangle.top() - size.y };
    }

    // Walk without jumping, braking with the player's own deceleration.
    void walk_to(Run& run, float target)
    {
        const PlayerTuning& tuning = run.services.tuning.player();
        for (int tick = 0; tick < 1200; ++tick)
        {
            const float error = target - run.player->center().x;
            const float speed = run.player->velocity().x;
            if (std::fabs(error) < 6.0f && speed == 0.0f) { return; }
            const float stopping = speed * speed / (2.0f * tuning.x_deacceleration) +
                std::fabs(speed) * dt + 4.0f;
            const bool toward = speed == 0.0f || (speed > 0.0f) == (error > 0.0f);
            run.tick(input(false, false, toward && std::fabs(error) > stopping ?
                (error > 0.0f ? 1.0f : -1.0f) : 0.0f));
        }
        fail("the player never settled at x=" + fixed(target));
    }

    void brake(Run& run)
    {
        for (int tick = 0; tick < 240; ++tick)
        {
            if (supported(*run.player) && run.player->velocity().x == 0.0f) { return; }
            run.tick();
        }
        fail("the player never came to rest");
    }

    // Release, press and hold one jump until the next landing. With a lip,
    // stop steering once the back edge is 20 units past it.
    std::string jump_and_land(Run& run, float movement,
        const std::optional<float>& lip)
    {
        run.tick(input());
        const size_t landings = run.landings.size();
        run.tick(input(true, true, movement));
        require(run.player->move_state() == PlayerMoveState::jumping,
            "a grounded press did not take off");
        for (int tick = 0; tick < 240 && run.landings.size() == landings; ++tick)
        {
            const float back = movement > 0.0f ? run.player->rectangle().left() :
                run.player->rectangle().right();
            if (lip.has_value() && movement * (back - *lip) > 20.0f) { movement = 0.0f; }
            run.tick(input(false, true, movement));
        }
        require(run.landings.size() > landings, "a held jump never landed");
        return run.landings.back().surface;
    }
}
