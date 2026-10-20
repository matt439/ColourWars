#include "engine/input/keyboard.h"
#include "engine/input/gamepad.h"
#include <array>
#include <cstdlib>
#include <iostream>

using namespace labrador;
struct Sample { bool held{}, pressed{}, released{}; };

void merge(Sample& a, Sample b)
{
    a.held |= b.held; a.pressed |= b.pressed; a.released |= b.released;
}

Sample keys(const std::array<Key, 1>& binding,
    const KeyboardState& now, const KeyboardState& before)
{
    Sample result;
    if (!now.focused) { return result; }
    for (Key key : binding)
        merge(result, { now.is_down(key), pressed(now, before, key), released(now, before, key) });
    return result;
}

Sample button_and_trigger(GamepadButton button, GamepadTrigger trigger, float threshold,
    const GamepadState& now, const GamepadState& before)
{
    Sample result;
    if (!now.connected) { return result; }
    merge(result, { now.is_down(button), pressed(now, before, button), released(now, before, button) });
    merge(result, { trigger_held(now, trigger, threshold),
        trigger_pressed(now, before, trigger, threshold), trigger_released(now, before, trigger, threshold) });
    return result;
}

int main()
{
    Keyboard keyboard;
    keyboard.set_focused(true); keyboard.poll(); keyboard.poll();
    std::array<Key, 1> saved_jump{ Key::space };
    keyboard.on_key_down(Key::j); keyboard.poll(); keyboard.poll();
    if (keys(saved_jump, keyboard.state(), keyboard.previous_state()).held) { return EXIT_FAILURE; }
    saved_jump[0] = Key::j; // A simulated applied preference, with J already held.
    Sample sample = keys(saved_jump, keyboard.state(), keyboard.previous_state());
    if (!sample.held || sample.pressed || sample.released) { return EXIT_FAILURE; }
    keyboard.on_key_up(Key::j); keyboard.poll();
    sample = keys(saved_jump, keyboard.state(), keyboard.previous_state());
    if (sample.held || sample.pressed || !sample.released) { return EXIT_FAILURE; }
    keyboard.on_key_down(Key::j); keyboard.poll();
    sample = keys(saved_jump, keyboard.state(), keyboard.previous_state());
    if (!sample.held || !sample.pressed || sample.released) { return EXIT_FAILURE; }

    GamepadState before{}, now{}; before.connected = now.connected = true;
    before.buttons = now.buttons = static_cast<uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a));
    now.left_trigger = 0.75f;
    sample = button_and_trigger(GamepadButton::a, GamepadTrigger::left, 0.5f, now, before);
    if (!sample.held || !sample.pressed || sample.released) { return EXIT_FAILURE; }
    before = now; now.buttons = 0; // A releases while LT remains held.
    sample = button_and_trigger(GamepadButton::a, GamepadTrigger::left, 0.5f, now, before);
    if (!sample.held || sample.pressed || !sample.released) { return EXIT_FAILURE; }
    std::cout << "PASS saved-key replacement and independent source-edge composition\n";
    return EXIT_SUCCESS;
}
