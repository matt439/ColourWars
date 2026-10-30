#include "game/pch.h"
#include "scripts/acceptance/input_bindings/fixtures.h"
#include <algorithm>
#include <iostream>

namespace input_bindings_acceptance
{
    // ---- Keyboard actions slice (GAP#4.2): move, drop, shoot and pause keys.
    struct ExpectedMouse { MouseButton button; const char* name; const wchar_t* label; };
    constexpr std::array<ExpectedMouse, 5> expected_mouse{{
        { MouseButton::left, "mouse_left", L"Left click" }, { MouseButton::right, "mouse_right", L"Right click" },
        { MouseButton::middle, "mouse_middle", L"Middle click" }, { MouseButton::x1, "mouse_x1", L"Mouse 4" },
        { MouseButton::x2, "mouse_x2", L"Mouse 5" }
    }};
    constexpr std::array<KeyboardAction, 6> keyboard_actions{ KeyboardAction::move_left, KeyboardAction::move_right,
        KeyboardAction::jump, KeyboardAction::drop, KeyboardAction::shoot, KeyboardAction::pause };
    constexpr std::array<const char*, 6> keyboard_action_tokens{ "move_left", "move_right", "jump", "drop", "shoot", "pause" };

    DesktopControl key_control(Key key) { return { key, MouseButton::none }; }
    DesktopControl mouse_control(MouseButton button) { return { Key::none, button }; }

    MouseState mouse_with(std::initializer_list<MouseButton> held)
    {
        MouseState result; result.focused = true;
        for (MouseButton button : held) { result.buttons |= uint8_t(1u << static_cast<unsigned int>(button)); }
        return result;
    }

    // Every action moved off its default, spelt independently of the page.
    InputBindings custom_keyboard()
    {
        InputBindings result;
        result.keyboard_move_left = Key::j; result.keyboard_move_right = Key::l; result.keyboard_jump = Key::a;
        result.keyboard_drop = Key::k; result.keyboard_shoot = mouse_control(MouseButton::right); result.keyboard_pause = Key::p;
        return result;
    }

    void check_keyboard_domain_and_mapping(const InputTuning& tuning)
    {
        require(keyboard_action_count == 6, "Keyboard action surface changed without its oracle");
        const InputBindings defaults;
        const std::array<DesktopControl, 6> default_controls{ key_control(Key::a), key_control(Key::d), key_control(Key::space),
            key_control(Key::s), mouse_control(MouseButton::left), DesktopControl{} };
        for (size_t index = 0; index < keyboard_actions.size(); ++index)
        {
            require(keyboard_action_name(keyboard_actions[index]) == keyboard_action_tokens[index] &&
                keyboard_control(defaults, keyboard_actions[index]) == default_controls[index],
                "A keyboard action's save key or default control changed");
        }
        // Untouched choices produce exactly the old literal desktop table.
        const ActionBindings effective_defaults = effective_action_bindings(defaults);
        for (size_t index = 0; index < game_action_count; ++index)
        {
            require(effective_defaults.game_desktop[index].keys == default_action_bindings().game_desktop[index].keys &&
                effective_defaults.game_desktop[index].mouse_buttons == default_action_bindings().game_desktop[index].mouse_buttons,
                "Default keyboard choices changed an existing desktop binding");
        }
        require_mapping(effective_defaults, Key::space);

        // Domains: the 27 keys everywhere, mouse buttons only for Shoot, None only for Pause.
        const auto mouse = shoot_mouse_buttons();
        for (size_t index = 0; index < expected_mouse.size(); ++index)
        {
            const DesktopControl control = mouse_control(expected_mouse[index].button);
            require(mouse[index] == expected_mouse[index].button && desktop_control_name(control) == expected_mouse[index].name &&
                desktop_control_label(control) == expected_mouse[index].label &&
                desktop_control_from_name(expected_mouse[index].name) == control,
                "A Shoot mouse button's token, label or order differs from its explicit contract");
            for (KeyboardAction action : keyboard_actions)
            {
                require(keyboard_control_supported(action, control) == (action == KeyboardAction::shoot),
                    "A mouse button became bindable outside Shoot");
            }
        }
        require(desktop_control_name({}) == "none" && desktop_control_label({}) == L"None" &&
            desktop_control_from_name("none") == DesktopControl{}, "The unbound control's token or label changed");
        for (KeyboardAction action : keyboard_actions)
        {
            require(keyboard_control_supported(action, {}) == (action == KeyboardAction::pause), "Only the extra Pause may be unbound");
            require(!keyboard_control_supported(action, { Key::j, MouseButton::left }), "A control with both a key and a button validated");
            for (const auto& expected : expected_keys)
            {
                require(keyboard_control_supported(action, key_control(expected.key)) &&
                    desktop_control_name(key_control(expected.key)) == expected.name &&
                    desktop_control_label(key_control(expected.key)) == expected.label &&
                    desktop_control_from_name(expected.name) == key_control(expected.key),
                    "A domain key was refused by an action or changed its token/label");
            }
            // Reserved, menu-only and developer keys stay outside every domain.
            for (Key reserved : { Key::escape, Key::enter, Key::backspace, Key::grave, Key::f1, Key::f2, Key::f5, Key::f9,
                Key::f10, Key::f11, Key::up, Key::down, Key::left, Key::right, Key::tab, Key::shift, Key::control, Key::alt,
                Key::digit_1, Key::numpad_0 })
            {
                require(!keyboard_control_supported(action, key_control(reserved)),
                    "A reserved, menu-only or developer key entered a keyboard action's domain");
            }
        }
        rejects([] { std::ignore = desktop_control_name({ Key::j, MouseButton::left }); }, "A two-source control acquired a token");
        rejects([] { std::ignore = desktop_control_name(mouse_control(static_cast<MouseButton>(42))); },
            "An unknown mouse button acquired a token");
        for (std::string_view name : { "", "Mouse_left", "MOUSE_LEFT", "mouse_x3", "mouse", "left_click", "escape",
            "down", "None", "f1" })
        { rejects([&] { std::ignore = desktop_control_from_name(name); }, "A noncanonical desktop control name was accepted"); }
        rejects([] { InputBindings value; set_keyboard_control(value, KeyboardAction::jump, mouse_control(MouseButton::right)); },
            "Jump accepted a mouse button");
        rejects([] { InputBindings value; set_keyboard_control(value, KeyboardAction::move_left, {}); }, "Move left accepted None");
        rejects([] { InputBindings value; set_keyboard_control(value, KeyboardAction::drop, key_control(Key::down)); },
            "Drop accepted its fixed Down alias");
        rejects([] { InputBindings value; set_keyboard_control(value, KeyboardAction::pause, key_control(Key::escape)); },
            "Extra Pause accepted its fixed Escape alias");

        // One control does one action; conflicts follow the pending choices.
        for (KeyboardAction action : keyboard_actions)
        {
            for (KeyboardAction other : keyboard_actions)
            {
                const DesktopControl used = keyboard_control(defaults, other);
                const auto conflict = keyboard_control_conflict(defaults, action, used);
                if (other == action || used == DesktopControl{}) { require(!conflict, "An action conflicted with itself or None"); }
                else { require(conflict && *conflict == other, "A default control in use was not reported against its owner"); }
            }
        }
        InputBindings moved; moved.keyboard_move_left = Key::j;
        require(!keyboard_control_conflict(moved, KeyboardAction::jump, key_control(Key::a)) &&
            keyboard_control_conflict(moved, KeyboardAction::jump, key_control(Key::j)) == KeyboardAction::move_left &&
            keyboard_jump_conflict(Key::a) == GameAction::move_left,
            "Jump conflicts did not follow the pending Move left key, or the default-set query changed");
        moved.keyboard_jump = Key::a; moved.validate();
        for (const auto& [action, key] : { std::pair{ KeyboardAction::pause, Key::space }, std::pair{ KeyboardAction::shoot, Key::d },
            std::pair{ KeyboardAction::drop, Key::a }, std::pair{ KeyboardAction::move_right, Key::s } })
        {
            InputBindings duplicate; set_keyboard_control(duplicate, action, key_control(key));
            rejects([&] { duplicate.validate(); }, "A keyboard key bound to two actions validated");
            rejects([&] { std::ignore = effective_action_bindings(duplicate); }, "A conflicting keyboard mapping was constructed");
        }
        InputBindings menu_aliases; menu_aliases.keyboard_drop = Key::r; menu_aliases.keyboard_pause = Key::w;
        menu_aliases.keyboard_shoot = key_control(Key::space); menu_aliases.keyboard_jump = Key::e;
        menu_aliases.validate(); // Menu-only R/W and the intro's Space are free for gameplay.

        // The custom mapping writes exactly the six sources beside the fixed aliases.
        const InputBindings custom = custom_keyboard(); custom.validate();
        const ActionBindings effective = effective_action_bindings(custom);
        const auto desktop = [&](GameAction action) -> const DesktopActionBinding&
        { return effective.game_desktop[static_cast<size_t>(action)]; };
        require(desktop(GameAction::move_left).keys == std::vector<Key>{ Key::j } &&
            desktop(GameAction::move_right).keys == std::vector<Key>{ Key::l } &&
            desktop(GameAction::jump).keys == std::vector<Key>{ Key::a } &&
            desktop(GameAction::drop).keys == std::vector<Key>{ Key::k, Key::down } &&
            desktop(GameAction::shoot).keys.empty() &&
            desktop(GameAction::shoot).mouse_buttons == std::vector<MouseButton>{ MouseButton::right } &&
            desktop(GameAction::pause).keys == std::vector<Key>{ Key::escape, Key::p } &&
            desktop(GameAction::debug).keys == std::vector<Key>{ Key::f1 } &&
            desktop(GameAction::skip_intro).keys == std::vector<Key>{ Key::space, Key::escape, Key::enter },
            "Custom keyboard mapping wrote the wrong sources or changed a fixed alias");
        for (size_t index = 0; index < game_action_count; ++index)
        {
            if (static_cast<GameAction>(index) != GameAction::shoot)
            { require(effective.game_desktop[index].mouse_buttons.empty(), "A mouse button reached a non-Shoot action"); }
            require(effective.game_pad[index].buttons == default_action_bindings().game_pad[index].buttons,
                "Keyboard choices changed a pad source");
        }
        require(effective.menu_keys == default_action_bindings().menu_keys, "Keyboard choices changed a fixed menu key");
        for (size_t index = 0; index < menu_direction_count; ++index)
        {
            require(effective.directions[index].keys == default_action_bindings().directions[index].keys,
                "Keyboard choices changed menu navigation");
        }
        // Samples through the pure evaluator: the moved keys act, the old ones do not.
        const auto held = [&](GameAction action, std::initializer_list<Key> down, std::initializer_list<MouseButton> buttons = {})
        { return game_action(effective, action, keys(down), keys({}), mouse_with(buttons), mouse_with({})).held; };
        require(held(GameAction::move_left, { Key::j }) && !held(GameAction::move_left, { Key::a }) &&
            held(GameAction::move_right, { Key::l }) && !held(GameAction::move_right, { Key::d }) &&
            held(GameAction::jump, { Key::a }) && !held(GameAction::jump, { Key::space }) &&
            held(GameAction::drop, { Key::k }) && held(GameAction::drop, { Key::down }) && !held(GameAction::drop, { Key::s }) &&
            held(GameAction::shoot, {}, { MouseButton::right }) && !held(GameAction::shoot, {}, { MouseButton::left }) &&
            held(GameAction::pause, { Key::p }) && held(GameAction::pause, { Key::escape }),
            "Effective custom keyboard samples did not follow the moved keys");
        InputBindings key_shoot; key_shoot.keyboard_shoot = key_control(Key::f);
        const ActionBindings keyed = effective_action_bindings(key_shoot);
        const auto& shoot = keyed.game_desktop[static_cast<size_t>(GameAction::shoot)];
        require(shoot.keys == std::vector<Key>{ Key::f } && shoot.mouse_buttons.empty() &&
            game_action(keyed, GameAction::shoot, keys({ Key::f }), keys({}), {}, {}).pressed &&
            !game_action(keyed, GameAction::shoot, keys({}), keys({}), mouse_with({ MouseButton::left }), mouse_with({})).held,
            "Shoot on a key kept the left click or did not press");
        GamepadState neutral; neutral.connected = true; GamepadState pad = neutral; pad.right_trigger = 1.0f;
        require(game_action(effective, GameAction::shoot, pad, neutral, tuning).held == (tuning.trigger_shoot_threshold < 1.0f),
            "Keyboard Shoot choices changed the pad's RT");
        std::cout << "PASS keyboard actions domain: six actions, 27 keys, five Shoot mouse buttons, optional extra Pause, "
            "20 reserved keys refused, per-profile conflicts and the custom/key-Shoot mappings\n";
    }

    void check_keyboard_reader(const char* content)
    {
        ReaderHarness h(content);
        h.bindings = custom_keyboard(); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::j); auto input = h.desktop();
        require(input.x_movement == -1.0f, "Actual reader ignored the moved Move left key");
        h.keyboard.on_key_down(Key::l); input = h.desktop();
        require(input.x_movement == 0.0f, "Opposing moved movement keys did not cancel");
        h.keyboard.on_key_up(Key::j); input = h.desktop();
        require(input.x_movement == 1.0f, "Actual reader ignored the moved Move right key");
        h.keyboard.on_key_up(Key::l); h.keyboard.on_key_down(Key::a); h.keyboard.on_key_down(Key::d); input = h.desktop();
        require(input.x_movement == 0.0f && input.jump_pressed && input.jump_held,
            "Old A/D still moved, or A did not jump once Jump moved onto it");
        h.keyboard.on_key_up(Key::a); h.keyboard.on_key_up(Key::d); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::a); input = h.desktop();
        require(h.menus[input_slots::keyboard_and_mouse].direction == Direction::left,
            "Moving gameplay keys changed fixed menu navigation on A");
        h.keyboard.on_key_up(Key::a); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::s); input = h.desktop(); require(!input.drop_through, "Old S still dropped");
        h.keyboard.on_key_up(Key::s); h.keyboard.on_key_down(Key::k); input = h.desktop(); require(input.drop_through, "K did not drop");
        h.keyboard.on_key_up(Key::k); h.keyboard.on_key_down(Key::down); input = h.desktop();
        require(input.drop_through, "The fixed Down drop alias stopped working");
        h.keyboard.on_key_up(Key::down); std::ignore = h.desktop();
        h.mouse.on_button_down(MouseButton::left); input = h.desktop(); require(!input.primary_shoot, "Left click still shot");
        h.mouse.on_button_up(MouseButton::left); h.mouse.on_button_down(MouseButton::right); input = h.desktop();
        require(input.primary_shoot, "Right click did not shoot once Shoot moved onto it");
        h.mouse.on_button_up(MouseButton::right); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::p); input = h.desktop(); require(input.pause_held && !input.toggle_pause_menu, "P did not hold Pause");
        h.keyboard.on_key_up(Key::p); input = h.desktop(); require(input.toggle_pause_menu && !input.skip_intro,
            "Extra Pause P did not toggle on release, or became an intro skip");
        h.keyboard.on_key_down(Key::escape); std::ignore = h.desktop(); h.keyboard.on_key_up(Key::escape); input = h.desktop();
        require(input.toggle_pause_menu && input.skip_intro, "Escape lost its fixed Pause or intro role");
        h.keyboard.on_key_down(Key::space); std::ignore = h.desktop(); h.keyboard.on_key_up(Key::space); input = h.desktop();
        require(!input.jump_held && input.skip_intro, "Space kept Jump or lost its fixed intro skip");
        // Adopting an already held key as a new binding never manufactures a press.
        h.keyboard.on_key_down(Key::m); std::ignore = h.desktop(); std::ignore = h.desktop();
        h.bindings.keyboard_move_left = Key::m; h.bindings.keyboard_shoot = key_control(Key::f); input = h.desktop();
        require(input.x_movement == -1.0f, "A newly adopted held Move left key was not read");
        h.keyboard.on_key_up(Key::m); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::f); input = h.desktop(); require(input.primary_shoot, "Shoot on F did not shoot through the reader");
        h.keyboard.set_focused(false); input = h.desktop(); require(!input.primary_shoot && input.x_movement == 0.0f,
            "Focus loss leaked a moved keyboard action");
        h.keyboard.set_focused(true); h.keyboard.on_key_up(Key::f); std::ignore = h.desktop();
        h.bindings.keyboard_pause = Key::f; rejects([&] { h.player.player_inputs(); },
            "Borrowed conflicting keyboard preference silently kept the cached mapping");
        h.bindings = InputBindings{ Key::j }; std::ignore = h.desktop();
        std::cout << "PASS keyboard actions reader: moved movement/jump/drop/shoot/pause, cancelling opposites, fixed Down, "
            "Escape, Space intro and menu A navigation, held-key adoption, focus loss and conflict refusal\n";
    }

    void check_keyboard_save(const IsolatedSaveDirectory& directory)
    {
        // Missing object: older saves keep the defaults and gain the object on write.
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}"); SettingsSession save_session(save);
        require(same_keyboard(save_session.input_bindings(), InputBindings{ Key::j }), "A save without the keyboard object lost the defaults");
        require(save.save_to_file(), "Cannot write the keyboard object");
        require_schema(directory.contents(), "j"); require_independent(save);
        // Round trip of every action moved, with pads untouched by the page's merge.
        InputBindings custom = custom_keyboard();
        custom.pads[2][PadAction::jump].primary = PadControl::b;
        require(save_session.set_input_bindings_and_save(custom), "A valid custom keyboard set did not save");
        auto custom_pads = default_pads; custom_pads[2][PadAction::jump].primary = PadControl::b;
        require_schema(directory.contents(), "a", custom_pads, { "j", "l", "k", "mouse_right", "p" });
        { Save reread; SettingsSession reread_session(reread); reread_session.load_save_file(); require(reread_session.input_bindings() == custom, "Custom keyboard set did not round trip");
          require_independent(reread); }
        for (const auto& expected : expected_mouse)
        {
            InputBindings value; value.keyboard_shoot = mouse_control(expected.button);
            require(save_session.set_input_bindings_and_save(value), "A Shoot mouse button did not save");
            require_schema(directory.contents(), "space", default_pads, { "a", "d", "s", expected.name, "none" });
            Save reread; SettingsSession reread_session(reread); reread_session.load_save_file(); require(reread_session.input_bindings().keyboard_shoot == value.keyboard_shoot,
                "A Shoot mouse button did not round trip");
        }
        // Refused before mutation: the active choice and the disk survive.
        const InputBindings before = save_session.input_bindings(); const std::string bytes = directory.contents();
        InputBindings conflict; conflict.keyboard_drop = Key::d;
        rejects([&] { std::ignore = save_session.set_input_bindings_and_save(conflict); }, "A conflicting keyboard set was accepted");
        require(save_session.input_bindings() == before && directory.contents() == bytes,
            "A refused keyboard set mutated the active preference or the disk");

        const std::string valid_pads = ",\"pads\":[{\"jump\":[\"x\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}]";
        const auto bindings = [&](const std::string& jump, const std::string& keyboard)
        { return "{\"keyboard_jump\":\"" + jump + "\"" + (keyboard.empty() ? "" : ",\"keyboard\":" + keyboard) + valid_pads + "}"; };
        const std::string good = "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}";
        save = load(directory, bindings("a", good)); save_session.load_save_file();
        require(save_session.input_bindings().keyboard_move_left == Key::j && save_session.input_bindings().keyboard_jump == Key::a &&
            save_session.input_bindings().keyboard_shoot == key_control(Key::f) && save_session.input_bindings().keyboard_pause == Key::p &&
            save_session.input_bindings().pads[0][PadAction::jump].primary == PadControl::x,
            "A valid keyboard object with Jump on the freed A did not load");
        // Damage to the object recovers the five together; Jump survives unless it then clashes.
        for (const char* broken : { "null", "7", "\"j\"", "[]", "{}",
            "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\"}",
            "{\"move_left\":\"J\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}",
            "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"down\",\"shoot\":\"f\",\"pause\":\"p\"}",
            "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"mouse_x3\",\"pause\":\"p\"}",
            "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"escape\"}",
            "{\"move_left\":\"none\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}",
            "{\"move_left\":\"mouse_left\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}",
            "{\"move_left\":\"j\",\"move_right\":\"j\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}",
            "{\"move_left\":7,\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}" })
        {
            for (const char* jump : { "e", "a" })
            {
                directory.write(fixture(bindings(jump, broken))); const std::string disk = directory.contents();
                save_session.load_save_file();
                InputBindings expected; expected.keyboard_jump = std::string_view(jump) == "e" ? Key::e : Key::space;
                require(same_keyboard(save_session.input_bindings(), expected) && directory.contents() == disk,
                    "A damaged keyboard object did not recover the five, kept a now-clashing Jump, or rewrote the bytes");
                require(save_session.input_bindings().pads[0][PadAction::jump].primary == PadControl::x,
                    "A damaged keyboard object discarded a valid pad profile");
                require_independent(save);
            }
        }
        // A well-formed object that clashes with Jump recovers the five first.
        save = load(directory, bindings("j", good)); save_session.load_save_file();
        require(same_keyboard(save_session.input_bindings(), InputBindings{ Key::j }),
            "A keyboard object clashing with Jump kept its keys or discarded Jump");
        // A damaged Jump keeps a valid object that does not need it.
        save = load(directory, bindings("enter", good)); save_session.load_save_file();
        InputBindings recovered = custom_keyboard(); recovered.keyboard_jump = Key::space; recovered.keyboard_shoot = key_control(Key::f);
        require(same_keyboard(save_session.input_bindings(), recovered), "A damaged Jump discarded a valid keyboard object");
        std::cout << "PASS keyboard actions save: missing-object defaults, schema/round trip of all six and every Shoot "
            "mouse button, refusal before mutation, 13 damaged objects x 2 Jump cases, Jump-clash and damaged-Jump recovery, "
            "pads and independent sections intact\n";
    }
}
