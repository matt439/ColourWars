#include "game/pch.h"
#include "scripts/acceptance/input_bindings/fixtures.h"
#include <algorithm>
#include <iostream>

namespace input_bindings_acceptance
{
    void require(bool condition, const char* message)
    { if (!condition) { throw std::runtime_error(message); } }

    uint16_t button_bit(GamepadButton button)
    { return uint16_t(1u << static_cast<unsigned int>(button)); }

    bool is_trigger(PadControl control)
    { return control == PadControl::left_trigger || control == PadControl::right_trigger; }

    // The oracle's own physical mapping for a non-trigger control.
    GamepadButton oracle_button(PadControl control)
    {
        switch (control)
        {
        case PadControl::a: return GamepadButton::a;
        case PadControl::b: return GamepadButton::b;
        case PadControl::x: return GamepadButton::x;
        case PadControl::y: return GamepadButton::y;
        case PadControl::left_shoulder: return GamepadButton::left_shoulder;
        case PadControl::right_shoulder: return GamepadButton::right_shoulder;
        case PadControl::left_stick: return GamepadButton::left_stick;
        case PadControl::right_stick: return GamepadButton::right_stick;
        case PadControl::start: return GamepadButton::start;
        default: throw std::logic_error("Oracle asked for a trigger or None as a button");
        }
    }

    // Presses one control fully: a button bit or a trigger at 1.0.
    GamepadState pressing(PadControl control, GamepadState state)
    {
        state.connected = true;
        if (control == PadControl::left_trigger) { state.left_trigger = 1.0f; }
        else if (control == PadControl::right_trigger) { state.right_trigger = 1.0f; }
        else if (control != PadControl::none) { state.buttons |= button_bit(oracle_button(control)); }
        return state;
    }

    PadProfile profile(PadControl jump, PadControl jump_also, PadControl shoot, PadControl shoot_also,
        PadControl pause, PadControl pause_also)
    {
        PadProfile result;
        result[PadAction::jump] = { jump, jump_also };
        result[PadAction::shoot] = { shoot, shoot_also };
        result[PadAction::pause] = { pause, pause_also };
        return result;
    }

    PadProfile jump_on(PadControl primary)
    { PadProfile result; result[PadAction::jump].primary = primary; return result; }

    std::array<PadProfile, input_slots::pad_count> jump_faces(const std::array<PadControl, input_slots::pad_count>& primaries)
    {
        std::array<PadProfile, input_slots::pad_count> result{};
        for (size_t slot = 0; slot < result.size(); ++slot) { result[slot] = jump_on(primaries[slot]); }
        return result;
    }

    bool conflicting(Key key) { return key == Key::a || key == Key::d || key == Key::s; }

    std::vector<Key> expected_game_keys(GameAction action, Key jump)
    {
        switch (action)
        {
        case GameAction::move_left: return { Key::a };
        case GameAction::move_right: return { Key::d };
        case GameAction::jump: return { jump };
        case GameAction::drop: return { Key::s, Key::down };
        case GameAction::pause: return { Key::escape };
        case GameAction::debug: return { Key::f1 };
        case GameAction::skip_intro: return { Key::space, Key::escape, Key::enter };
        default: return {};
        }
    }

    void require_mapping(const ActionBindings& actual, Key jump, const PadProfile& pad)
    {
        require(game_action_count == 8 && menu_action_count == 4 && menu_direction_count == 4,
            "Input binding test surface changed without its explicit physical-control oracle");
        for (size_t index = 0; index < game_action_count; ++index)
        {
            const GameAction action = static_cast<GameAction>(index);
            require(actual.game_desktop[index].keys == expected_game_keys(action, jump) &&
                actual.game_desktop[index].mouse_buttons == (action == GameAction::shoot ?
                    std::vector<MouseButton>{ MouseButton::left } : std::vector<MouseButton>{}),
                "Effective bindings changed another gameplay keyboard/mouse action");
            std::vector<GamepadButton> buttons;
            std::vector<TriggerActionBinding> triggers;
            const auto rebindable = [&](PadAction pad_action)
            {
                for (PadControl control : { pad[pad_action].primary, pad[pad_action].alternate })
                {
                    if (control == PadControl::none) { continue; }
                    if (is_trigger(control))
                    {
                        triggers.push_back({ control == PadControl::left_trigger ? GamepadTrigger::left : GamepadTrigger::right,
                            pad_action == PadAction::shoot ? ActionTriggerThreshold::shoot : ActionTriggerThreshold::jump });
                    }
                    else { buttons.push_back(oracle_button(control)); }
                }
            };
            if (action == GameAction::jump) { rebindable(PadAction::jump); }
            if (action == GameAction::shoot) { rebindable(PadAction::shoot); }
            if (action == GameAction::pause) { rebindable(PadAction::pause); }
            if (action == GameAction::debug) { buttons = { GamepadButton::back }; }
            if (action == GameAction::skip_intro) { buttons = { GamepadButton::a, GamepadButton::b, GamepadButton::start }; }
            require(actual.game_pad[index].buttons == buttons, "Effective pad buttons differ from the profile or a fixed alias changed");
            const auto& actual_triggers = actual.game_pad[index].triggers;
            require(actual_triggers.size() == triggers.size(), "Effective pad triggers differ from the profile");
            for (size_t trigger = 0; trigger < triggers.size(); ++trigger)
            {
                require(actual_triggers[trigger].trigger == triggers[trigger].trigger &&
                    actual_triggers[trigger].threshold == triggers[trigger].threshold,
                    "Effective pad trigger or its tuning selector differs from the profile");
            }
        }
        require(actual.menu_keys[static_cast<size_t>(MenuInputAction::proceed)] == std::vector<Key>{ Key::enter, Key::space } &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::back)] == std::vector<Key>{ Key::escape, Key::backspace } &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::pause)].empty() &&
            actual.menu_keys[static_cast<size_t>(MenuInputAction::shuffle)] == std::vector<Key>{ Key::r },
            "Keyboard Jump changed fixed menu keyboard aliases");
        const std::array<GamepadButton, 4> menu_buttons{ GamepadButton::a, GamepadButton::b,
            GamepadButton::start, GamepadButton::x };
        for (size_t index = 0; index < menu_buttons.size(); ++index)
        { require(actual.menu_buttons[index] == std::vector<GamepadButton>{ menu_buttons[index] },
            "Keyboard Jump changed a fixed menu pad alias"); }
        const std::array<Direction, 4> directions{ Direction::up, Direction::down, Direction::left, Direction::right };
        const std::array<std::vector<Key>, 4> keys{ std::vector<Key>{ Key::up, Key::w }, { Key::down, Key::s },
            { Key::left, Key::a }, { Key::right, Key::d } };
        const std::array<GamepadButton, 4> dpad{ GamepadButton::dpad_up, GamepadButton::dpad_down,
            GamepadButton::dpad_left, GamepadButton::dpad_right };
        for (size_t index = 0; index < directions.size(); ++index)
        { require(actual.directions[index].direction == directions[index] && actual.directions[index].keys == keys[index] &&
            actual.directions[index].button == dpad[index], "Keyboard Jump changed a fixed menu navigation alias"); }
    }

    KeyboardState keys(std::initializer_list<Key> pressed)
    {
        KeyboardState result; result.focused = true;
        for (Key key : pressed)
        {
            const auto index = static_cast<unsigned int>(key);
            result.keys[index / 64] |= uint64_t(1) << (index % 64);
        }
        return result;
    }

    void check_domain_and_mapping(const InputTuning& tuning)
    {
        require(InputBindings{}.keyboard_jump == Key::space, "New input preferences do not default to Space");
        InputBindings{}.validate(); require_mapping(default_action_bindings(), Key::space);
        for (const auto& expected : expected_keys)
        {
            require(keyboard_jump_key_supported(expected.key) && input_key_name(expected.key) == expected.name &&
                input_key_label(expected.key) == expected.label && input_key_from_name(expected.name) == expected.key,
                "Physical Jump key name/label/domain differs from its explicit 27-entry contract");
            require(input_key_name(expected.key) == labrador::key_name(expected.key) &&
                labrador::key_from_name(expected.name) == expected.key,
                "A saved Jump token no longer matches the engine's key_name spelling");
            const auto conflict = keyboard_jump_conflict(expected.key);
            require(conflict.has_value() == conflicting(expected.key), "Jump conflict set is not exactly A/D/S within the supported domain");
            if (conflict)
            {
                require(*conflict == (expected.key == Key::a ? GameAction::move_left :
                    (expected.key == Key::d ? GameAction::move_right : GameAction::drop)), "Jump conflict names the wrong gameplay action");
                rejects([&] { InputBindings{ expected.key }.validate(); }, "Conflicting Jump preference validated");
                rejects([&] { effective_action_bindings(InputBindings{ expected.key }); }, "Conflicting Jump mapping was constructed");
                continue;
            }
            const InputBindings preferences{ expected.key }; preferences.validate();
            const ActionBindings effective = effective_action_bindings(preferences);
            require_mapping(effective, expected.key); require_mapping(default_action_bindings(), Key::space);
            const auto down = game_action(effective, GameAction::jump, keys({ expected.key }), keys({}), {}, {});
            const auto held = game_action(effective, GameAction::jump, keys({ expected.key }), keys({ expected.key }), {}, {});
            const auto up = game_action(effective, GameAction::jump, keys({}), keys({ expected.key }), {}, {});
            require(down.held && down.pressed && !down.released && !down.was_held && held.held && !held.pressed &&
                held.was_held && !up.held && !up.pressed && up.released && up.was_held,
                "Effective Jump did not use ordinary physical-key held/press/release samples");
            require(game_action(effective, GameAction::jump, keys({ Key::space }), keys({}), {}, {}).held ==
                (expected.key == Key::space), "Effective Jump retained Space as an extra gameplay alias");
            for (Key intro : { Key::space, Key::escape, Key::enter })
            { require(game_action(effective, GameAction::skip_intro, keys({}), keys({ intro }), {}, {}).released,
                "Rebinding Jump changed a fixed introductory skip release"); }
            if (expected.key != Key::space)
            { require(!game_action(effective, GameAction::skip_intro, keys({}), keys({ expected.key }), {}, {}).released,
                "A new Jump key became an introductory skip alias"); }
            GamepadState neutral; neutral.connected = true;
            GamepadState pad = neutral; pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
            require(game_action(effective, GameAction::jump, pad, neutral, tuning).pressed,
                "Rebinding keyboard Jump changed pad A");
            pad = neutral; pad.left_trigger = 1.0f;
            const auto trigger = game_action(effective, GameAction::jump, pad, neutral, tuning);
            require(trigger.held == (tuning.trigger_jump_threshold < 1.0f) &&
                trigger.pressed == (tuning.trigger_jump_threshold < 1.0f),
                "Rebinding keyboard Jump changed its pad left trigger or strict threshold");
        }
        for (unsigned int value = 0; value <= static_cast<unsigned int>(Key::count); ++value)
        {
            const Key key = static_cast<Key>(value);
            const bool supported = std::any_of(expected_keys.begin(), expected_keys.end(),
                [key](const ExpectedKey& expected) { return expected.key == key; });
            require(keyboard_jump_key_supported(key) == supported, "An unlisted engine key entered the bounded Jump domain");
            if (!supported)
            {
                rejects([&] { input_key_name(key); }, "Out-of-domain key acquired a save name");
                rejects([&] { input_key_label(key); }, "Out-of-domain key acquired a help label");
                rejects([&] { InputBindings{ key }.validate(); }, "Out-of-domain Jump preference validated");
            }
        }
        rejects([] { InputBindings{ static_cast<Key>(255) }.validate(); }, "Invalid key enumerator validated");
        for (std::string_view name : { "", "J", "Space", "SPACE", "space ", " space", "enter", "escape", "grave", "f1", "1", "jump", "left" })
        { rejects([&] { input_key_from_name(name); }, "Noncanonical or unknown key name was accepted"); }
        rejects([] { input_key_from_name(std::string_view("j\0alias", 7)); }, "Embedded-null key name was accepted");
        require(!keyboard_jump_conflict(Key::w) && !keyboard_jump_conflict(Key::r) &&
            menu_action(effective_action_bindings(InputBindings{ Key::r }), MenuInputAction::shuffle, keys({}), keys({ Key::r })).released &&
            keyboard_direction_held(effective_action_bindings(InputBindings{ Key::w }), keys({ Key::w }))[0],
            "Menu-only W/R aliases incorrectly prevented Jump binding or stopped working");
    }

    void check_pad_domain_and_mapping(const InputTuning& tuning)
    {
        InputBindings preferences{ Key::j };
        const PadProfile defaults = profile(PadControl::a, PadControl::left_trigger, PadControl::right_trigger,
            PadControl::none, PadControl::start, PadControl::none);
        for (const PadProfile& pad : preferences.pads)
        { require(pad == defaults, "Missing pad profiles must default to A / LT Jump, RT Shoot and Start Pause"); }
        const auto listed = pad_controls();
        require(listed.size() + 1 == expected_controls.size(), "Pad control domain changed size");
        for (size_t index = 0; index < listed.size(); ++index)
        { require(listed[index] == expected_controls[index + 1].control, "Pad control page order changed"); }
        for (const auto& expected : expected_controls)
        {
            require(pad_control_supported(expected.control) == (expected.control != PadControl::none) &&
                pad_control_name(expected.control) == expected.name && pad_control_label(expected.control) == expected.label &&
                pad_control_from_name(expected.name) == expected.control,
                "Pad control domain, canonical token or label differs from its explicit contract");
        }
        require(pad_action_name(PadAction::jump) == "jump" && pad_action_name(PadAction::shoot) == "shoot" &&
            pad_action_name(PadAction::pause) == "pause", "Pad action save keys changed");
        require(pad_control_conflict(defaults, PadAction::jump, false, PadControl::right_trigger) == PadAction::shoot &&
            pad_control_conflict(defaults, PadAction::jump, true, PadControl::a) == PadAction::jump &&
            pad_control_conflict(defaults, PadAction::pause, true, PadControl::left_trigger) == PadAction::jump &&
            !pad_control_conflict(defaults, PadAction::jump, false, PadControl::a) &&
            !pad_control_conflict(defaults, PadAction::shoot, true, PadControl::none) &&
            !pad_control_conflict(defaults, PadAction::shoot, false, PadControl::right_shoulder),
            "Pad conflict query named the wrong owner or flagged an entry's own control");
        rejects([] { pad_control_name(static_cast<PadControl>(200)); }, "Unknown pad control acquired a token");
        rejects([] { pad_control_label(static_cast<PadControl>(200)); }, "Unknown pad control acquired a label");

        // Every control on every action and entry, in every physical slot.
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            for (PadAction action : pad_actions)
            {
                for (bool alternate : { false, true })
                {
                    for (PadControl control : listed)
                    {
                        PadProfile pad = defaults;
                        // Move any other entry off the control so the profile stays valid.
                        for (PadActionChoice& choice : pad.actions)
                        {
                            for (PadControl* entry : { &choice.primary, &choice.alternate })
                            {
                                if (*entry != control) { continue; }
                                *entry = entry == &choice.alternate ? PadControl::none :
                                    (control == PadControl::left_shoulder ? PadControl::right_shoulder : PadControl::left_shoulder);
                            }
                        }
                        PadActionChoice& target = pad[action];
                        (alternate ? target.alternate : target.primary) = control;
                        if (!alternate && target.alternate == control) { target.alternate = PadControl::none; }
                        pad.validate();
                        preferences.pads = default_pads; preferences.pads[slot] = pad;
                        const ActionBindings effective = effective_action_bindings(preferences, slot);
                        require_mapping(effective, Key::j, pad);
                        const GameAction game = action == PadAction::jump ? GameAction::jump :
                            (action == PadAction::shoot ? GameAction::shoot : GameAction::pause);
                        const float threshold = action == PadAction::shoot ? tuning.trigger_shoot_threshold : tuning.trigger_jump_threshold;
                        const bool reaches = !is_trigger(control) || threshold < 1.0f;
                        GamepadState before; before.connected = true;
                        const GamepadState now = pressing(control);
                        const auto press = game_action(effective, game, now, before, tuning);
                        const auto hold = game_action(effective, game, now, now, tuning);
                        const auto release = game_action(effective, game, before, now, tuning);
                        require(press.pressed == reaches && press.held == reaches && !press.released &&
                            hold.held == reaches && !hold.pressed && release.released == reaches && !release.held,
                            "A chosen pad control lost physical press/hold/release semantics or its action's threshold");
                        for (int other = 0; other < input_slots::pad_count; ++other)
                        {
                            if (other != slot) { require_mapping(effective_action_bindings(preferences, other), Key::j); }
                        }
                    }
                }
            }
        }
        // Moved defaults stop working.
        preferences.pads = default_pads;
        preferences.pads[1] = profile(PadControl::right_trigger, PadControl::none, PadControl::right_shoulder,
            PadControl::left_trigger, PadControl::y, PadControl::none);
        const ActionBindings moved = effective_action_bindings(preferences, 1);
        GamepadState before; before.connected = true;
        require(!game_action(moved, GameAction::jump, pressing(PadControl::a), before, tuning).held &&
            !game_action(moved, GameAction::pause, before, pressing(PadControl::start), tuning).released &&
            game_action(moved, GameAction::skip_intro, before, pressing(PadControl::start), tuning).released &&
            game_action(moved, GameAction::jump, pressing(PadControl::right_trigger), before, tuning).pressed ==
                (tuning.trigger_jump_threshold < 1.0f) &&
            game_action(moved, GameAction::shoot, pressing(PadControl::left_trigger), before, tuning).held ==
                (tuning.trigger_shoot_threshold < 1.0f) &&
            game_action(moved, GameAction::pause, before, pressing(PadControl::y), tuning).released,
            "Moved pad defaults kept their old controls, intro skip changed, or a trigger used the wrong threshold");

        for (const PadProfile& invalid : {
            profile(PadControl::none, PadControl::none, PadControl::right_trigger, PadControl::none, PadControl::start, PadControl::none),
            profile(PadControl::a, PadControl::none, PadControl::none, PadControl::none, PadControl::start, PadControl::none),
            profile(PadControl::a, PadControl::none, PadControl::right_trigger, PadControl::none, PadControl::none, PadControl::none),
            profile(PadControl::a, PadControl::a, PadControl::right_trigger, PadControl::none, PadControl::start, PadControl::none),
            profile(PadControl::right_trigger, PadControl::none, PadControl::right_trigger, PadControl::none, PadControl::start, PadControl::none),
            profile(PadControl::a, PadControl::none, PadControl::right_trigger, PadControl::start, PadControl::start, PadControl::none),
            profile(PadControl::a, PadControl::left_trigger, PadControl::right_trigger, PadControl::none, PadControl::y, PadControl::left_trigger),
            profile(static_cast<PadControl>(200), PadControl::none, PadControl::right_trigger, PadControl::none, PadControl::start, PadControl::none),
            profile(PadControl::a, static_cast<PadControl>(200), PadControl::right_trigger, PadControl::none, PadControl::start, PadControl::none) })
        {
            rejects([&] { invalid.validate(); }, "An unbound, duplicate or unknown pad control validated");
            InputBindings bindings = preferences; bindings.pads[2] = invalid;
            rejects([&] { bindings.validate(); }, "An invalid pad profile escaped preference validation");
            rejects([&] { effective_action_bindings(bindings, 0); }, "Invalid other-slot profile escaped binding validation");
        }
        for (std::string_view token : { "", "A", "LT", "Start", "None", " a", "a ", "back", "dpad_up", "left_trigger",
            "left_shoulder", "l3", "jump" })
        { rejects([&] { pad_control_from_name(token); }, "Noncanonical pad control token was accepted"); }
        rejects([] { pad_control_from_name(std::string_view("b\0alias", 7)); }, "Embedded-null pad token was accepted");
        for (int slot : { -1, input_slots::pad_count, 99 })
        {
            bool rejected = false;
            try { effective_action_bindings(preferences, slot); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected, "Out-of-range physical pad slot was accepted");
        }
        require_mapping(default_action_bindings(), Key::space);
    }
}
