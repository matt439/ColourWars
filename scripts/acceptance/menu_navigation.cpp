#include "game/pch.h"
#include "game/states/menu_input.h"
#include "game/presentation.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace labrador;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    uint16_t button(GamepadButton value)
    {
        return static_cast<uint16_t>(1u << static_cast<unsigned>(value));
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Expected staged content directory");
        const Presentation look((std::filesystem::path(argv[1]) / "presentation.json").string().c_str());
        const float delay = look.menu().repeat_delay;
        const float interval = look.menu().repeat_interval;
        require(delay > interval && interval > 0, "Authored repeat must have a longer first delay");
        MenuDirectionRepeat navigation;
        GamepadState pad;
        pad.connected = true;
        const auto step = [&](float dt = 0.0f)
        {
            return navigation.update_pad(pad, dt, delay, interval);
        };
        pad.buttons = button(GamepadButton::dpad_down);
        require(step(10) == Direction::none, "Opening under a held dpad must stay neutral");
        pad.buttons = 0;
        require(step() == Direction::none, "Release must not navigate");
        pad.buttons = button(GamepadButton::dpad_down);
        require(step() == Direction::down, "Dpad must fire on press");
        require(step(delay * 0.75f) == Direction::none, "Repeat must wait for first delay");
        require(step(delay * 0.3f) == Direction::down, "Held dpad must repeat");
        require(step(interval * 0.5f) == Direction::none, "Repeat interval must be respected");
        require(step(interval * 0.6f) == Direction::down, "Later repeats use short interval");
        require(step(10) == Direction::down && step() == Direction::none,
            "A hitch must emit at most one move with no catch-up burst");
        navigation = MenuDirectionRepeat();
        require(step(10) == Direction::none, "New page must discard held repeat");
        pad.buttons = 0;
        step();
        pad.left_stick = { 0.0f, -0.9f };
        require(step() == Direction::up, "Stick must fire on engagement");
        pad.left_stick.y = -0.7f;
        require(step(delay + interval) == Direction::up, "Stick hysteresis must retain direction below engagement");
        pad.left_stick.y = -0.5f;
        require(step() == Direction::none, "Stick release must be neutral");
        pad.left_stick.y = -0.7f;
        require(step() == Direction::none, "Subthreshold stick must not engage after release");
        pad.left_stick = { 0.9f, -0.9f };
        require(step() == Direction::none, "Ambiguous diagonal must not choose a row");
        pad.left_stick.y = 0;
        require(step(10) == Direction::none, "Ambiguous input must rearm through neutral");
        pad.left_stick = {};
        step();
        pad.left_stick.x = 0.9f;
        require(step() == Direction::right, "Neutral must rearm stick navigation");

        GamepadReader reader;
        Gamepads pads(&reader);
        Keyboard keyboard;
        keyboard.set_focused(true);
        keyboard.poll();
        keyboard.poll();
        MenuInput input(&pads, &keyboard);
        const auto keys = [&](float dt = 0.0f)
        {
            keyboard.poll();
            input.update_directions(dt, delay, interval);
            const ProcessedMenuInput first = input.menu_inputs()[input_slots::keyboard_and_mouse];
            require(first.direction == input.menu_inputs()[input_slots::keyboard_and_mouse].direction,
                "Repeated reads must not advance repeat time");
            return first;
        };
        keys();
        keyboard.on_key_down(Key::s);
        require(keys().direction == Direction::down, "WASD must navigate on press");
        require(keys(delay + interval).direction == Direction::down, "Held keyboard must repeat");
        input.reset_directions();
        require(keys(10).direction == Direction::none, "Page change must block held keys");
        keyboard.on_key_up(Key::s);
        require(keys().direction == Direction::none, "Releasing across a page change must not move");
        keyboard.on_key_down(Key::down);
        require(keys().direction == Direction::down, "Arrow keys must rearm after neutral");
        keyboard.set_focused(false);
        require(keys().direction == Direction::none, "Focus loss must clear navigation");
        keyboard.set_focused(true);
        keyboard.on_key_down(Key::down);
        require(keys(10).direction == Direction::none, "Focus return must suppress held keys");
        require(keys(10).direction == Direction::none, "Focus-held keys must not repeat later");
        keyboard.on_key_up(Key::down);
        keys();
        keyboard.on_key_down(Key::up);
        require(keys().direction == Direction::up, "Focus return must rearm through neutral");
        keyboard.on_key_down(Key::enter);
        require(keys().action == MenuInputAction::none, "Confirm remains release-triggered");
        keyboard.on_key_up(Key::enter);
        require(keys().action == MenuInputAction::proceed, "Navigation must preserve confirm ownership");
        const Direction before_reset = input.menu_inputs()[input_slots::keyboard_and_mouse].direction;
        input.reset_pad_actions(0);
        require(input.menu_inputs()[input_slots::keyboard_and_mouse].direction == before_reset,
            "Resetting one pad must preserve other device results");
        std::cout << "PASS menu navigation: press, repeat, hysteresis, page arming, focus and stable reads\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
