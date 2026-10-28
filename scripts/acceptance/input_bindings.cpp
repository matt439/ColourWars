#include "game/pch.h"
#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "game/save.h"
#include "game/objects/level_end_info.h"
#include "game/objects/player_input.h"
#include "game/states/menu_input.h"
#include "game/states/resume_countdown.h"
#include "engine/input/gamepad_reader.h"
#include <objbase.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

// The concrete reader link seam used by action_bindings.cpp: no XInput,
// window, graphics/audio device, physical keyboard or player's save.
namespace binding_test_pads { std::array<labrador::GamepadState, 4> states{}; }
namespace labrador
{
    struct GamepadReader::Impl { std::array<GamepadState, 4> states{}; bool suspended = false; };
    GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
    GamepadReader::~GamepadReader() = default;
    GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
    GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
    void GamepadReader::sample()
    { this->impl_->states = this->impl_->suspended ? std::array<GamepadState, 4>{} : binding_test_pads::states; }
    GamepadState GamepadReader::read(int slot) const
    { return slot >= 0 && slot < 4 ? this->impl_->states[static_cast<size_t>(slot)] : GamepadState{}; }
    void GamepadReader::suspend() { this->impl_->suspended = true; }
    void GamepadReader::resume() { this->impl_->suspended = false; }
}

using namespace labrador;

namespace
{
    void require(bool condition, const char* message)
    { if (!condition) { throw std::runtime_error(message); } }

    template<class Action> void rejects(Action action, const char* message)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, message);
    }

    struct ExpectedKey { Key key; const char* name; const wchar_t* label; };
    // Explicit physical names and labels, independent of enum arithmetic and
    // the production conversion table. Supported does not imply conflict-free.
    constexpr std::array<ExpectedKey, 27> expected_keys{{
        { Key::a, "a", L"A" }, { Key::b, "b", L"B" }, { Key::c, "c", L"C" },
        { Key::d, "d", L"D" }, { Key::e, "e", L"E" }, { Key::f, "f", L"F" },
        { Key::g, "g", L"G" }, { Key::h, "h", L"H" }, { Key::i, "i", L"I" },
        { Key::j, "j", L"J" }, { Key::k, "k", L"K" }, { Key::l, "l", L"L" },
        { Key::m, "m", L"M" }, { Key::n, "n", L"N" }, { Key::o, "o", L"O" },
        { Key::p, "p", L"P" }, { Key::q, "q", L"Q" }, { Key::r, "r", L"R" },
        { Key::s, "s", L"S" }, { Key::t, "t", L"T" }, { Key::u, "u", L"U" },
        { Key::v, "v", L"V" }, { Key::w, "w", L"W" }, { Key::x, "x", L"X" },
        { Key::y, "y", L"Y" }, { Key::z, "z", L"Z" }, { Key::space, "space", L"Space" }
    }};

    struct ExpectedControl { PadControl control; const char* name; const wchar_t* label; };
    // Explicit tokens and labels in page order, with None first: independent of
    // the production table and of PadControl ordinals.
    constexpr std::array<ExpectedControl, 12> expected_controls{{
        { PadControl::none, "none", L"None" }, { PadControl::a, "a", L"A" }, { PadControl::b, "b", L"B" },
        { PadControl::x, "x", L"X" }, { PadControl::y, "y", L"Y" },
        { PadControl::left_shoulder, "lb", L"LB" }, { PadControl::right_shoulder, "rb", L"RB" },
        { PadControl::left_trigger, "lt", L"LT" }, { PadControl::right_trigger, "rt", L"RT" },
        { PadControl::left_stick, "ls", L"LS" }, { PadControl::right_stick, "rs", L"RS" },
        { PadControl::start, "start", L"Start" }
    }};
    constexpr std::array<PadControl, 4> faces{ PadControl::a, PadControl::b, PadControl::x, PadControl::y };
    constexpr std::array<PadAction, 3> pad_actions{ PadAction::jump, PadAction::shoot, PadAction::pause };

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
    GamepadState pressing(PadControl control, GamepadState state = {})
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

    const std::array<PadProfile, input_slots::pad_count> default_pads{};

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

    void require_mapping(const ActionBindings& actual, Key jump, const PadProfile& pad = PadProfile{})
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

    bool same_input(const PlayerInputData& a, const PlayerInputData& b)
    {
        return a.connected == b.connected && a.x_movement == b.x_movement && a.jump_pressed == b.jump_pressed &&
            a.jump_held == b.jump_held && a.shoot_angle == b.shoot_angle && a.shoot_direction == b.shoot_direction &&
            a.shoot_direction_requested == b.shoot_direction_requested && a.primary_shoot == b.primary_shoot &&
            a.drop_through == b.drop_through && a.toggle_debug == b.toggle_debug &&
            a.toggle_pause_menu == b.toggle_pause_menu && a.pause_held == b.pause_held && a.skip_intro == b.skip_intro &&
            a.left_analog_stick == b.left_analog_stick && a.right_analog_stick == b.right_analog_stick;
    }

    struct ReaderHarness
    {
        GamepadReader reader; Gamepads pads{ &reader }; Keyboard keyboard; Mouse mouse;
        GameTuning tuning; PlaySettings play; InputBindings bindings{ Key::j };
        PlayerInput player; MenuInput menu; std::vector<ProcessedMenuInput> menus;
        explicit ReaderHarness(const char* content) : tuning((std::filesystem::path(content) / "tuning.json").string().c_str()),
            player(&pads, &keyboard, &mouse, &tuning, &play, &bindings), menu(&pads, &keyboard, &tuning)
        {
            binding_test_pads::states = {}; keyboard.set_focused(true); mouse.set_focused(true);
            std::ignore = this->sample(); std::ignore = this->sample();
        }
        std::vector<PlayerInputData> sample()
        {
            pads.poll(); keyboard.poll(); mouse.poll(); menu.update_directions(0.0f, 0.4f, 0.1f, false);
            auto result = player.player_inputs(); const auto repeated = player.player_inputs();
            require(result.size() == input_slots::count && repeated.size() == result.size(), "Actual input reader changed slot count");
            for (size_t slot = 0; slot < result.size(); ++slot)
            { require(same_input(result[slot], repeated[slot]), "Repeated reader access consumed or changed a sampled action edge"); }
            menus = menu.menu_inputs(); return result;
        }
        PlayerInputData desktop() { return this->sample()[input_slots::keyboard_and_mouse]; }
    };

    void check_actual_reader(const char* content)
    {
        ReaderHarness h(content);
        rejects([&] { PlayerInput invalid(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, nullptr); },
            "PlayerInput accepted missing binding ownership");
        for (Key key : { Key::none, Key::a, Key::escape, static_cast<Key>(255) })
        {
            const InputBindings invalid{ key };
            rejects([&] { PlayerInput reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &invalid); },
                "PlayerInput constructed an invalid effective mapping");
        }
        h.keyboard.on_key_down(Key::space); auto input = h.desktop();
        require(!input.jump_pressed && !input.jump_held, "Actual custom Jump retained the old Space gameplay source");
        h.keyboard.on_key_up(Key::space); input = h.desktop();
        require(input.skip_intro && h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::proceed,
            "Space lost its independent menu/intro aliases after Jump changed");
        h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && input.jump_pressed && !input.skip_intro, "Actual custom Jump did not produce a fresh J press");
        h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Repeated key-down produced a second gameplay Jump");
        h.keyboard.on_key_up(Key::j); input = h.desktop();
        require(!input.jump_held && !input.jump_pressed && !input.skip_intro, "Releasing custom Jump skipped the intro or retained Jump");
        for (Key key : { Key::enter, Key::escape })
        {
            h.keyboard.on_key_down(key); std::ignore = h.desktop(); h.keyboard.on_key_up(key); input = h.desktop();
            require(input.skip_intro && input.toggle_pause_menu == (key == Key::escape),
                "Actual reader changed fixed Enter/Escape release behavior");
        }
        h.keyboard.on_key_down(Key::a); h.keyboard.on_key_down(Key::s); h.mouse.on_button_down(MouseButton::left);
        input = h.desktop(); require(input.x_movement == -1.0f && input.drop_through && input.primary_shoot,
            "Rebinding Jump changed movement, drop or mouse fire");
        h.keyboard.on_key_up(Key::a); h.keyboard.on_key_up(Key::s); h.mouse.on_button_up(MouseButton::left); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::j); std::ignore = h.desktop(); h.keyboard.set_focused(false); input = h.desktop();
        require(!input.jump_held && !input.jump_pressed && !input.skip_intro, "Focus loss leaked Jump or an intro release");
        h.keyboard.set_focused(true); h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Focus return invented a custom Jump press");
        input = h.desktop(); require(input.jump_held && !input.jump_pressed, "Focus-held custom Jump repeated its edge");
        h.keyboard.on_key_up(Key::j); std::ignore = h.desktop(); h.keyboard.on_key_down(Key::j); input = h.desktop();
        require(input.jump_pressed, "Release and fresh press did not rearm custom Jump after focus return");
        h.keyboard.on_key_up(Key::j); std::ignore = h.desktop();
        h.keyboard.on_key_down(Key::k); std::ignore = h.desktop(); std::ignore = h.desktop();
        h.bindings.keyboard_jump = Key::k; input = h.desktop();
        require(input.jump_held && !input.jump_pressed, "Changing to an already held key invented a Jump press");
        h.keyboard.on_key_up(Key::k); std::ignore = h.desktop(); h.keyboard.on_key_down(Key::k); input = h.desktop();
        require(input.jump_pressed, "Changed mapping did not accept a deliberate new key press");
        h.keyboard.on_key_up(Key::k); std::ignore = h.desktop();
        h.bindings.keyboard_jump = Key::r; std::ignore = h.desktop(); h.keyboard.on_key_down(Key::r); input = h.desktop();
        require(input.jump_pressed, "Menu shuffle key could not be used for gameplay Jump");
        h.keyboard.on_key_up(Key::r); std::ignore = h.desktop();
        require(h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::shuffle,
            "Gameplay R binding rewrote the actual menu shuffle alias");
        h.bindings.keyboard_jump = Key::w; std::ignore = h.desktop(); h.keyboard.on_key_down(Key::w); input = h.desktop();
        require(input.jump_pressed && h.menus[input_slots::keyboard_and_mouse].direction == Direction::up,
            "Gameplay W binding rewrote the actual menu direction alias");
        h.keyboard.on_key_up(Key::w); std::ignore = h.desktop();
        auto& pad = binding_test_pads::states[2]; pad.connected = true;
        pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
        auto all = h.sample(); require(all[2].jump_held && !all[2].jump_pressed && !all[0].connected,
            "Custom keyboard mapping changed reconnect guarding or sparse pad identity");
        pad.buttons = 0; std::ignore = h.sample(); pad.buttons = uint16_t(1u << static_cast<unsigned int>(GamepadButton::a));
        all = h.sample(); require(all[2].jump_pressed, "Custom keyboard mapping changed actual pad A");
        pad.left_trigger = 1.0f;
        all = h.sample(); require(all[2].jump_held && all[2].jump_pressed == (h.tuning.input().trigger_jump_threshold < 1.0f),
            "Held pad A suppressed the independent left-trigger Jump edge or changed its strict threshold");
        pad = {}; std::ignore = h.sample();
        h.bindings.keyboard_jump = Key::none;
        rejects([&] { h.player.player_inputs(); }, "Borrowed invalid preference silently replaced the cached mapping");
        h.bindings.keyboard_jump = Key::j; std::ignore = h.desktop();
        require_mapping(default_action_bindings(), Key::space);
    }

    void check_actual_pad_reader(const char* content)
    {
        ReaderHarness h(content);
        for (int slot = 0; slot < input_slots::pad_count; ++slot)
        {
            for (PadControl face : faces)
            {
                binding_test_pads::states = {}; std::ignore = h.sample();
                h.bindings.pads[slot] = jump_on(face);
                auto& pad = binding_test_pads::states[slot];
                pad = pressing(face);
                auto all = h.sample();
                require(all[slot].jump_held && !all[slot].jump_pressed,
                    "First/reconnect poll invented a selected-face Jump edge");
                pad.buttons = 0; std::ignore = h.sample();
                pad = pressing(face); all = h.sample();
                require(all[slot].jump_held && all[slot].jump_pressed,
                    "Actual reader did not use the selected physical slot's face button");
                all = h.sample(); require(all[slot].jump_held && !all[slot].jump_pressed,
                    "Held selected face repeated its Jump edge");
                pad.left_trigger = 1.0f; all = h.sample();
                require(all[slot].jump_pressed == (h.tuning.input().trigger_jump_threshold < 1.0f),
                    "Actual reader lost independent LT alias crossing while a selected face was held");
                h.reader.suspend(); all = h.sample();
                require(!all[slot].connected && !all[slot].jump_held && !all[slot].jump_pressed,
                    "Suspended pad leaked selected-face held/press state");
                h.reader.resume(); all = h.sample();
                require(all[slot].jump_held && !all[slot].jump_pressed,
                    "Resuming a held custom pad binding invented a Jump edge");
            }
        }
        binding_test_pads::states = {}; std::ignore = h.sample();
        h.bindings.pads = jump_faces({ PadControl::x, PadControl::b, PadControl::a, PadControl::y });
        auto& second = binding_test_pads::states[1]; auto& fourth = binding_test_pads::states[3];
        second.connected = fourth.connected = true; std::ignore = h.sample();
        second.buttons = button_bit(GamepadButton::b); fourth.buttons = button_bit(GamepadButton::y);
        auto all = h.sample();
        // A roster may request [slot 3, keyboard, slot 1]; reader output must
        // still address the physical slots, with no compaction over holes.
        const std::array<int, 3> roster{ 3, input_slots::keyboard_and_mouse, 1 };
        require(all[roster[0]].jump_pressed && all[roster[2]].jump_pressed &&
            !all[0].connected && !all[2].connected && all[roster[1]].connected,
            "Sparse/reordered roster selection changed physical binding identity");
        second.buttons |= button_bit(GamepadButton::x); std::ignore = h.sample(); std::ignore = h.sample();
        h.bindings.pads[1] = jump_on(PadControl::x); all = h.sample();
        require(all[1].jump_held && !all[1].jump_pressed,
            "Adopting an already held new face button invented a Jump edge");
        second.buttons = button_bit(GamepadButton::b); all = h.sample();
        require(!all[1].jump_held && !all[1].jump_pressed,
            "A former face button stayed active after slot remapping");
        second.buttons |= button_bit(GamepadButton::x); all = h.sample();
        require(all[1].jump_pressed && !all[3].jump_pressed && all[3].jump_held,
            "Fresh mapped face press was lost or another physical slot was remapped");
        ResumeCountdown countdown; countdown.begin(0.5f); countdown.filter_inputs(all);
        require(!all[1].jump_pressed && all[1].jump_held, "Resume filter lost mapped pad Jump hold/edge policy");
        require(countdown.advance(0.5f), "Mapped pad resume fixture did not complete");
        all = h.sample(); countdown.filter_inputs(all);
        require(!all[1].jump_pressed && all[1].jump_held, "Held mapped pad escaped resume release gate");
        second.buttons = 0; all = h.sample(); countdown.filter_inputs(all);
        second.buttons = button_bit(GamepadButton::x); all = h.sample(); countdown.filter_inputs(all);
        require(all[1].jump_pressed, "Fresh mapped pad press failed to rearm after resume release");

        // Shoot and Pause move too: slot 2 shoots on RB (and LT) and pauses on Y;
        // Jump takes RT. Menus, intro skip and the other slots keep theirs.
        binding_test_pads::states = {}; h.bindings.pads = default_pads; std::ignore = h.sample();
        h.bindings.pads[2] = profile(PadControl::right_trigger, PadControl::none, PadControl::right_shoulder,
            PadControl::left_trigger, PadControl::y, PadControl::none);
        auto& third = binding_test_pads::states[2]; auto& first = binding_test_pads::states[0];
        third.connected = first.connected = true; std::ignore = h.sample(); std::ignore = h.sample();
        third = pressing(PadControl::right_shoulder); first = pressing(PadControl::right_trigger); all = h.sample();
        require(all[2].primary_shoot && !all[2].jump_held && all[0].primary_shoot == (h.tuning.input().trigger_shoot_threshold < 1.0f),
            "Actual reader did not shoot on a rebound shoulder, or remapped another slot");
        third = pressing(PadControl::left_trigger); all = h.sample();
        require(all[2].primary_shoot == (h.tuning.input().trigger_shoot_threshold < 1.0f) && !all[2].jump_held,
            "Shoot's alternate LT lost the shoot threshold or still jumped");
        third = pressing(PadControl::right_trigger); all = h.sample();
        require(!all[2].primary_shoot && all[2].jump_pressed == (h.tuning.input().trigger_jump_threshold < 1.0f),
            "RT kept shooting or did not jump at the jump threshold after rebinding");
        third = pressing(PadControl::start); std::ignore = h.sample(); third = pressing(PadControl::none); all = h.sample();
        require(!all[2].toggle_pause_menu && all[2].skip_intro && h.menus[2].action == MenuInputAction::pause,
            "Start still paused gameplay, or lost its fixed intro/menu role, after Pause moved");
        third = pressing(PadControl::y); all = h.sample();
        require(all[2].pause_held && !all[2].toggle_pause_menu, "Rebound Pause did not hold on Y");
        third = pressing(PadControl::none); all = h.sample();
        require(all[2].toggle_pause_menu && !all[2].skip_intro, "Rebound Pause did not release on Y, or Y became an intro skip");
        h.bindings.pads[0][PadAction::jump].primary = PadControl::start;
        rejects([&] { h.player.player_inputs(); }, "Invalid connected-slot profile escaped actual reader validation");
        h.bindings.pads = default_pads;
        binding_test_pads::states = {}; std::ignore = h.sample();
    }

    void check_actual_resume(const char* content)
    {
        ReaderHarness h(content); ResumeCountdown countdown; countdown.begin(0.5f);
        h.keyboard.on_key_down(Key::j); auto inputs = h.sample();
        require(inputs[input_slots::keyboard_and_mouse].jump_pressed, "Resume fixture did not sample a real J press");
        countdown.filter_inputs(inputs);
        require(inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
            "Resume countdown failed to suppress mapped Jump while retaining held arc input");
        inputs = h.sample(); countdown.filter_inputs(inputs);
        require(countdown.advance(0.5f), "Actual mapped held-J final sample did not complete the countdown");
        for (int tick = 0; tick < 3; ++tick)
        {
            inputs = h.sample(); countdown.filter_inputs(inputs);
            require(inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
                "Held custom Jump leaked across countdown completion");
        }
        h.keyboard.on_key_up(Key::j); inputs = h.sample(); countdown.filter_inputs(inputs);
        require(!inputs[input_slots::keyboard_and_mouse].jump_held && !inputs[input_slots::keyboard_and_mouse].jump_pressed,
            "Custom Jump release was not neutral through ResumeCountdown");
        h.keyboard.on_key_down(Key::j); inputs = h.sample(); countdown.filter_inputs(inputs);
        require(inputs[input_slots::keyboard_and_mouse].jump_pressed && inputs[input_slots::keyboard_and_mouse].jump_held,
            "Fresh custom Jump remained blocked after a release beyond countdown completion");
    }

    class IsolatedSaveDirectory
    {
    public:
        explicit IsolatedSaveDirectory(const std::filesystem::path& retained_root = {}) : retained_(!retained_root.empty())
        {
            wchar_t* previous = nullptr; size_t length = 0;
            require(_wdupenv_s(&previous, &length, L"LOCALAPPDATA") == 0, "Cannot read LOCALAPPDATA");
            if (previous) { previous_ = previous; std::free(previous); }
            if (retained_)
            {
                root_ = std::filesystem::absolute(retained_root);
                std::filesystem::create_directories(root_ / L"ColourWars");
            }
            else
            {
                GUID guid{}; wchar_t name[40]{};
                require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0, "Cannot name input-binding acceptance profile");
                root_ = std::filesystem::temp_directory_path() / (std::wstring(L"ColourWars-input-bindings-") + name);
                require(std::filesystem::create_directory(root_) && std::filesystem::create_directory(root_ / L"ColourWars"),
                    "Cannot create input-binding acceptance profile");
            }
            require(_wputenv_s(L"LOCALAPPDATA", root_.c_str()) == 0, "Cannot isolate input-binding saves");
        }
        ~IsolatedSaveDirectory()
        {
            _wputenv_s(L"LOCALAPPDATA", previous_.c_str());
            if (retained_) { return; } // Explicit relaunch profiles belong to the caller across processes.
            std::error_code ignored;
            std::filesystem::remove(temporary_path(), ignored); std::filesystem::remove(save_path(), ignored);
            std::filesystem::remove(root_ / L"ColourWars", ignored); std::filesystem::remove(root_, ignored);
        }
        std::filesystem::path save_path() const { return root_ / L"ColourWars/save_data.json"; }
        std::filesystem::path temporary_path() const { return save_path().wstring() + L".tmp"; }
        void write(const std::string& json) const
        {
            std::ofstream stream(save_path(), std::ios::binary | std::ios::trunc); stream << json;
            require(stream.good(), "Cannot write private input-binding fixture");
        }
        std::string contents() const
        {
            std::ifstream stream(save_path(), std::ios::binary); require(stream.good(), "Cannot read private input-binding save");
            return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
        }
    private:
        std::filesystem::path root_; std::wstring previous_; bool retained_ = false;
    };

    const std::string independent_sections =
        "\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,\"team_palette\":\"pink_green\","
        "\"aim_sensitivity\":4,\"music_volume\":23,\"sfx_volume\":67,\"view_shake\":false,\"aim_marker\":false,\"reduce_effects\":true,"
        "\"pad_sticks\":[{\"movement_deadzone\":0,\"aim_deadzone\":90},{\"movement_deadzone\":10,\"aim_deadzone\":35},"
        "{\"movement_deadzone\":25,\"aim_deadzone\":55},{\"movement_deadzone\":-1,\"aim_deadzone\":-1}],"
        "\"records\":[{\"stage\":\"retired\",\"versus_played\":7,\"team_a_wins\":4,\"team_b_wins\":2,\"draws\":1,"
        "\"best_win_percentage\":75,\"practice_played\":3,\"best_coverage_percentage\":37.5}],"
        "\"last_match\":{\"players\":[{\"team\":\"b\",\"weapon\":\"sniper\"}],\"stage\":\"movement_course\","
        "\"layout\":\"one_player\",\"best_of\":1,\"duration\":45.25}";

    std::string fixture(const std::string& bindings = "", const std::string& version = "3")
    { return "{" + (version.empty() ? "" : "\"save_version\":" + version + ",") + independent_sections +
        (bindings.empty() ? "" : ",\"input_bindings\":" + bindings) + "}"; }

    Save load(const IsolatedSaveDirectory& directory, const std::string& bindings = "", const std::string& version = "3")
    { directory.write(fixture(bindings, version)); Save result; result.load_save_file(); return result; }

    void require_independent(const Save& save)
    {
        const auto& play = save.play_settings(); const StageRecord* record = save.records().find("retired");
        require(save.resolution() == ScreenResolution::s_1280_720 && save.fullscreen() && save.team_palette() == TeamColourPair::pink_green &&
            play.aim_sensitivity == 4 && play.music_volume == 23 && play.sfx_volume == 67 && !play.view_shake && !play.aim_marker && play.reduce_effects,
            "Input-binding save/recovery changed independent display or play preferences");
        const std::array<StickSettings, 4> profiles{ StickSettings{0, 90}, StickSettings{10, 35}, StickSettings{25, 55}, StickSettings{-1, -1} };
        for (size_t slot = 0; slot < profiles.size(); ++slot)
        { require(play.sticks[slot].movement_deadzone == profiles[slot].movement_deadzone && play.sticks[slot].aim_deadzone == profiles[slot].aim_deadzone,
            "Input-binding save/recovery changed an independent per-pad profile"); }
        require(save.records().stages().size() == 1 && record && record->versus_played == 7 && record->team_a_wins == 4 &&
            record->team_b_wins == 2 && record->draws == 1 && record->best_win_percentage == 75.0f &&
            record->practice_played == 3 && record->best_coverage_percentage == 37.5f,
            "Input-binding save/recovery lost an independent removed-stage record");
        require(save.last_match_setup() && save.last_match_setup()->players.size() == 1 &&
            save.last_match_setup()->players[0].team == PlayerTeam::b && save.last_match_setup()->players[0].weapon == WeaponType::sniper &&
            save.last_match_setup()->stage == "movement_course" && save.last_match_setup()->layout == ScreenLayout::one_player &&
            save.last_match_setup()->best_of == 1 && save.last_match_setup()->duration == 45.25f,
            "Input-binding save/recovery changed independent remembered match choices");
    }

    struct ExpectedKeyboard
    {
        const char* move_left = "a"; const char* move_right = "d"; const char* drop = "s";
        const char* shoot = "mouse_left"; const char* pause = "none";
    };

    void require_schema(const std::string& bytes, const char* name,
        const std::array<PadProfile, input_slots::pad_count>& expected_pads = default_pads,
        const ExpectedKeyboard& keyboard = {})
    {
        rapidjson::Document document; document.Parse(bytes.data(), bytes.size());
        require(!document.HasParseError() && document.IsObject() && document.HasMember("save_version") &&
            document["save_version"].IsInt() && document["save_version"].GetInt() == Save::version &&
            document.HasMember("input_bindings") && document["input_bindings"].IsObject(),
            "Save did not write the current version with the independent binding schema");
        const auto& section = document["input_bindings"];
        require(section.MemberCount() == 3 && section.HasMember("keyboard_jump") && section["keyboard_jump"].IsString() &&
            std::string_view(section["keyboard_jump"].GetString(), section["keyboard_jump"].GetStringLength()) == name,
            "Save binding section did not contain exactly its canonical physical Jump token, keyboard keys and pad profiles");
        require(section.HasMember("keyboard") && section["keyboard"].IsObject() && section["keyboard"].MemberCount() == 5,
            "Save did not write exactly the five other keyboard actions");
        for (const auto& [key, token] : { std::pair{ "move_left", keyboard.move_left }, std::pair{ "move_right", keyboard.move_right },
            std::pair{ "drop", keyboard.drop }, std::pair{ "shoot", keyboard.shoot }, std::pair{ "pause", keyboard.pause } })
        {
            const auto& value = section["keyboard"];
            require(value.HasMember(key) && value[key].IsString() &&
                std::string_view(value[key].GetString(), value[key].GetStringLength()) == token,
                "A saved keyboard action did not keep its canonical control token");
        }
        require(section.HasMember("pads") && section["pads"].IsArray() &&
            section["pads"].Size() == input_slots::pad_count,
            "Save did not write all four stable physical-slot pad profiles");
        const auto token = [](PadControl control)
        {
            const auto found = std::find_if(expected_controls.begin(), expected_controls.end(),
                [&](const ExpectedControl& expected) { return expected.control == control; });
            require(found != expected_controls.end(), "Schema oracle was given an unsupported pad control");
            return std::string_view(found->name);
        };
        for (size_t slot = 0; slot < expected_pads.size(); ++slot)
        {
            const auto& actual = section["pads"][static_cast<rapidjson::SizeType>(slot)];
            require(actual.IsObject() && actual.MemberCount() == 3, "A saved pad profile is not exactly jump/shoot/pause");
            for (size_t index = 0; index < pad_actions.size(); ++index)
            {
                const char* key = index == 0 ? "jump" : (index == 1 ? "shoot" : "pause");
                const PadActionChoice& choice = expected_pads[slot].actions[index];
                require(actual.HasMember(key) && actual[key].IsArray(), "A saved pad profile lost an action");
                const auto& controls = actual[key];
                require(controls.Size() == (choice.alternate == PadControl::none ? 1u : 2u),
                    "A saved pad action did not write its primary and only a real alternate");
                for (rapidjson::SizeType entry = 0; entry < controls.Size(); ++entry)
                {
                    require(controls[entry].IsString() && std::string_view(controls[entry].GetString(), controls[entry].GetStringLength()) ==
                        token(entry == 0 ? choice.primary : choice.alternate),
                        "Saved pad controls did not retain canonical tokens and physical slot order");
                }
            }
        }
    }

    void check_save_schema_and_recovery(const IsolatedSaveDirectory& directory)
    {
        require(Save::version >= 3, "Input binding schema needs save version 3 or later");
        for (const char* version : { "", "0", "1", "2" })
        {
            Save legacy = load(directory, "", version);
            require(legacy.input_bindings().keyboard_jump == Key::space, "Legacy save without bindings did not acquire default Space");
            require_independent(legacy); require(legacy.save_to_file(), "Legacy binding migration could not save");
            require_schema(directory.contents(), "space"); Save reread; reread.load_save_file(); require_independent(reread);
            require(reread.input_bindings().keyboard_jump == Key::space, "Migrated legacy binding did not round trip");
        }
        for (const char* version : { "0", "1", "2", "3", "999" })
        {
            Save known = load(directory, "{\"keyboard_jump\":\"j\"}", version);
            require(known.input_bindings().keyboard_jump == Key::j, "Versioned save discarded its known independent binding section");
            require_independent(known);
        }
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}");
        for (const auto& expected : expected_keys)
        {
            if (conflicting(expected.key)) { continue; }
            require(save.set_input_bindings_and_save(InputBindings{ expected.key }), "A valid physical Jump preference did not save");
            require(save.input_bindings().keyboard_jump == expected.key && save.save_data().input_bindings.keyboard_jump == expected.key,
                "Successful binding setter did not update both borrowed preference and SaveData copy");
            require_schema(directory.contents(), expected.name); Save reread; reread.load_save_file();
            require(reread.input_bindings().keyboard_jump == expected.key, "Canonical Jump preference did not round trip");
            require_independent(reread);
        }
        for (const char* broken : { "null", "false", "7", "\"j\"", "[]", "{}", "{\"jump\":\"j\"}",
            "{\"keyboard_jump\":null}", "{\"keyboard_jump\":false}", "{\"keyboard_jump\":7}", "{\"keyboard_jump\":[]}",
            "{\"keyboard_jump\":{}}", "{\"keyboard_jump\":\"unknown\"}", "{\"keyboard_jump\":\"J\"}",
            "{\"keyboard_jump\":\"Space\"}", "{\"keyboard_jump\":\"enter\"}", "{\"keyboard_jump\":\"grave\"}",
            "{\"keyboard_jump\":\"a\"}", "{\"keyboard_jump\":\"d\"}", "{\"keyboard_jump\":\"s\"}",
            "{\"keyboard_jump\":\"j\\u0000alias\"}" })
        {
            directory.write(fixture("{\"keyboard_jump\":\"j\"}")); save.load_save_file();
            directory.write(fixture(broken)); const std::string bytes = directory.contents(); save.load_save_file();
            require(save.input_bindings().keyboard_jump == Key::space && directory.contents() == bytes,
                "Malformed binding section retained old bindings or rewrote the player's bytes on load");
            require_independent(save);
        }
        directory.write("{\"save_version\":3,\"records\":false,\"resolution\":false,\"aim_sensitivity\":\"bad\","
            "\"last_match\":false,\"input_bindings\":{\"keyboard_jump\":\"j\"}}");
        save.load_save_file(); require(save.input_bindings().keyboard_jump == Key::j,
            "An unrelated malformed save section reset valid input bindings");
    }

    class ReplacementLock
    {
    public:
        explicit ReplacementLock(const std::filesystem::path& path)
        {
            handle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(handle_ != INVALID_HANDLE_VALUE, "Cannot lock private binding save replacement");
        }
        ~ReplacementLock() { CloseHandle(handle_); }
    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    void check_save_validation_failures_and_independence(const IsolatedSaveDirectory& directory, const char* content)
    {
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}");
        const InputBindings* borrowed = &save.input_bindings();
        const std::string original = directory.contents();
        for (Key key : { Key::a, Key::d, Key::s, Key::none, Key::enter, Key::grave, static_cast<Key>(255) })
        {
            rejects([&] { save.set_input_bindings_and_save(InputBindings{ key }); }, "Invalid binding setter did not validate before assignment/write");
            require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::j && directory.contents() == original &&
                !std::filesystem::exists(directory.temporary_path()), "Rejected binding setter changed active memory, disk bytes or staging state");
            require_independent(save);
        }
        for (bool replacement_failure : { false, true })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\"}"); const std::string bytes = directory.contents();
            ReaderHarness h(content);
            PlayerInput saved_reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, borrowed);
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save.set_input_bindings_and_save(InputBindings{ Key::k }), "Locked binding save replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own binding staging path");
                require(!save.set_input_bindings_and_save(InputBindings{ Key::k }), "Blocked binding staging write unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own binding staging blocker");
            }
            require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::k && directory.contents() == bytes &&
                !std::filesystem::exists(directory.temporary_path()), "Failed binding persistence rolled back active choice, damaged old bytes or left a temporary file");
            // This reader cached J before the failed write. It borrows the
            // Save member that GameLevel borrows, rather than a copied test preference.
            h.keyboard.on_key_down(Key::j); std::ignore = h.sample();
            auto actual = saved_reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(!actual.jump_held && !actual.jump_pressed, "Failed persistence left the actual saved reader on old J");
            h.keyboard.on_key_up(Key::j); std::ignore = h.sample();
            h.keyboard.on_key_down(Key::k); std::ignore = h.sample();
            actual = saved_reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(actual.jump_held && actual.jump_pressed &&
                same_input(actual, saved_reader.player_inputs()[input_slots::keyboard_and_mouse]),
                "Actual Save-borrowing reader did not follow active K after persistence failed");
            h.keyboard.on_key_up(Key::k); std::ignore = h.sample();
            require_independent(save); Save old_disk; old_disk.load_save_file();
            require(old_disk.input_bindings().keyboard_jump == Key::j, "Failed binding write damaged the reloadable previous preference");
            require_independent(old_disk);
            require(save.set_input_bindings_and_save(InputBindings{ Key::k }) && &save.input_bindings() == borrowed,
                "Targeted retry of the already active binding did not write after removing its real IO blocker");
            Save retried; retried.load_save_file(); require(retried.input_bindings().keyboard_jump == Key::k,
                "Retry did not persist the binding kept active after failure");
            require_schema(directory.contents(), "k"); require_independent(retried);
        }
        save = load(directory, "{\"keyboard_jump\":\"j\"}");
        auto settings_copy = save.play_settings(); auto controls_copy = save.play_settings();
        require(save.set_input_bindings_and_save(InputBindings{ Key::w }), "Cannot prepare independent saved binding");
        settings_copy.music_volume = 81; settings_copy.reduce_effects = false;
        require(save.set_play_settings_and_save(settings_copy) && save.input_bindings().keyboard_jump == Key::w,
            "An older Settings PlaySettings snapshot overwrote independent input bindings");
        controls_copy.aim_marker = true; controls_copy.sticks[2] = { 35, 75 };
        require(save.set_play_settings_and_save(controls_copy) && save.input_bindings().keyboard_jump == Key::w &&
            save.set_display_settings_and_save(ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "An older Controls snapshot or display setter overwrote independent input bindings");
        LevelEndInfo result; result.total_area = 200.0f; result.team_a_area = 150.0f; result.team_b_area = 50.0f;
        require(save.record_result_and_save("new_stage", result).recorded, "Cannot exercise independent record persistence");
        const LastMatchSetup remembered = *save.last_match_setup();
        require(save.remember_match_setup_and_save(remembered), "Cannot exercise independent remembered-match persistence");
        Save reread; reread.load_save_file();
        require(&save.input_bindings() == borrowed && borrowed->keyboard_jump == Key::w &&
            reread.input_bindings().keyboard_jump == Key::w && reread.resolution() == ScreenResolution::s_3840_2160 &&
            !reread.fullscreen() && reread.team_palette() == TeamColourPair::blue_orange && reread.play_settings().aim_marker &&
            reread.play_settings().sticks[2].movement_deadzone == 35 && reread.play_settings().sticks[2].aim_deadzone == 75 &&
            reread.records().find("retired") && reread.records().find("new_stage") &&
            reread.records().find("new_stage")->team_a_wins == 1 && reread.last_match_setup() &&
            reread.last_match_setup()->stage == remembered.stage && reread.last_match_setup()->players[0].weapon == WeaponType::sniper,
            "Settings/Controls/display/records/remembered saves overwrote the independent binding or their expected data");
        require_schema(directory.contents(), "w");
    }

    void check_pad_save(const IsolatedSaveDirectory& directory, const char* content)
    {
        // The earlier face-button array migrates per slot; Shoot, Pause and LT keep their defaults.
        const auto selected = jump_faces({ PadControl::b, PadControl::x, PadControl::y, PadControl::a });
        Save save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":[\"b\",\"x\",\"y\",\"a\"]}");
        require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == selected,
            "Legacy per-slot Jump preferences did not migrate in physical slot order");
        require_independent(save);
        require(save.save_to_file(), "Migrated pad profiles did not save");
        require_schema(directory.contents(), "j", selected);
        Save reread; reread.load_save_file(); require(reread.input_bindings() == save.input_bindings(),
            "Migrated pad profiles did not round trip");
        SaveData reset = save.save_data(); InputBindingsSection{}.reset(reset);
        require(reset.input_bindings == InputBindings{}, "Input section reset failed to restore default keyboard and every pad slot");

        for (const char* broken : { "null", "false", "7", "[]", "{}", "\"unknown\"", "\"B\"", "\"lt\"", "\"start\"", "\"b\\u0000alias\"" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":[\"b\"," + std::string(broken) + ",\"y\",\"x\"]}");
            const auto recovered = jump_faces({ PadControl::b, PadControl::a, PadControl::y, PadControl::x });
            require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == recovered,
                "Malformed legacy pad choice discarded valid keyboard or another slot's choice");
            require_independent(save);
        }
        for (const char* broken : { "null", "false", "7", "\"b\"", "{}" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pad_jump\":" + std::string(broken) + "}");
            require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == default_pads,
                "Malformed legacy pad array discarded valid keyboard or failed to use per-slot defaults");
        }
        for (const char* keyboard : { "", "\"keyboard_jump\":\"a\",", "\"keyboard_jump\":false," })
        {
            save = load(directory, "{" + std::string(keyboard) + "\"pad_jump\":[\"x\",\"y\",\"b\"]}");
            const auto recovered = jump_faces({ PadControl::x, PadControl::y, PadControl::b, PadControl::a });
            require(save.input_bindings().keyboard_jump == Key::space && save.input_bindings().pads == recovered,
                "Malformed/missing keyboard or short legacy array discarded independently valid pad choices");
        }

        // Full profiles: any control, alternates, and per-slot recovery.
        const std::array<PadProfile, input_slots::pad_count> custom{
            profile(PadControl::right_trigger, PadControl::a, PadControl::right_shoulder, PadControl::left_trigger,
                PadControl::y, PadControl::none),
            PadProfile{},
            profile(PadControl::left_stick, PadControl::none, PadControl::right_trigger, PadControl::x,
                PadControl::start, PadControl::right_stick),
            profile(PadControl::b, PadControl::left_shoulder, PadControl::right_trigger, PadControl::none,
                PadControl::start, PadControl::y) };
        const std::string slots[]{
            "{\"jump\":[\"rt\",\"a\"],\"shoot\":[\"rb\",\"lt\"],\"pause\":[\"y\"]}",
            "{\"jump\":[\"a\",\"lt\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"ls\"],\"shoot\":[\"rt\",\"x\"],\"pause\":[\"start\",\"rs\"]}",
            "{\"jump\":[\"b\",\"lb\"],\"shoot\":[\"rt\"],\"pause\":[\"start\",\"y\"]}" };
        const auto pads_json = [&](const std::string& second)
        { return "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "," + second + "," + slots[2] + "," + slots[3] + "]}"; };
        save = load(directory, pads_json(slots[1]));
        require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == custom,
            "Full per-slot pad profiles did not load");
        require(save.save_to_file(), "Full pad profiles did not save"); require_schema(directory.contents(), "j", custom);
        reread.load_save_file(); require(reread.input_bindings() == save.input_bindings(), "Full pad profiles did not round trip");
        for (const char* broken : { "null", "false", "7", "[]", "\"a\"", "{}",
            "{\"jump\":[\"a\"],\"shoot\":[\"rt\"]}", "{\"jump\":[],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"none\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"rt\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"a\",\"a\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"a\",\"b\",\"x\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"back\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[\"A\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":\"a\",\"shoot\":[\"rt\"],\"pause\":[\"start\"]}",
            "{\"jump\":[7],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}" })
        {
            save = load(directory, pads_json(broken));
            require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == custom,
                "A malformed pad profile discarded the keyboard key or another slot's profile");
            require_independent(save);
        }
        for (const char* broken : { "null", "false", "7", "\"a\"", "{}" })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":" + std::string(broken) + ",\"pad_jump\":[\"b\"]}");
            require(save.input_bindings().keyboard_jump == Key::j && save.input_bindings().pads == default_pads,
                "A malformed pads array discarded the keyboard key, or fell back to the older face array");
        }
        save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "],\"pad_jump\":[\"b\",\"x\"]}");
        require(save.input_bindings().pads[0] == custom[0] && save.input_bindings().pads[1] == PadProfile{},
            "A short pads array did not default its missing slots, or mixed in the older face array");
        save = load(directory, "{\"keyboard_jump\":\"j\",\"pads\":[" + slots[0] + "," + slots[1] + "," + slots[2] + "," + slots[3] + ",false]}");
        require(save.input_bindings().pads == custom, "Unknown extra slot discarded known physical-slot profiles");

        const InputBindings* borrowed = &save.input_bindings(); const std::string bytes = directory.contents();
        InputBindings invalid = *borrowed; invalid.pads[2][PadAction::jump].primary = PadControl::start;
        rejects([&] { save.set_input_bindings_and_save(invalid); }, "Invalid pad setter did not validate before mutation");
        require(&save.input_bindings() == borrowed && borrowed->pads == custom && directory.contents() == bytes &&
            !std::filesystem::exists(directory.temporary_path()), "Rejected pad setter mutated active choices, disk or staging state");
        for (bool replacement_failure : { false, true })
        {
            save = load(directory, "{\"keyboard_jump\":\"j\"}"); const std::string previous = directory.contents();
            InputBindings candidate = save.input_bindings(); candidate.pads = custom;
            if (replacement_failure)
            {
                const ReplacementLock lock(directory.save_path());
                require(!save.set_input_bindings_and_save(candidate), "Locked pad preference replacement unexpectedly succeeded");
            }
            else
            {
                require(std::filesystem::create_directory(directory.temporary_path()), "Cannot block own pad save staging path");
                require(!save.set_input_bindings_and_save(candidate), "Blocked pad preference staging unexpectedly succeeded");
                require(std::filesystem::remove(directory.temporary_path()), "Cannot remove own pad save staging blocker");
            }
            require(save.input_bindings() == candidate && directory.contents() == previous &&
                !std::filesystem::exists(directory.temporary_path()), "Failed pad write rolled back active preferences or damaged disk bytes");
            ReaderHarness h(content);
            PlayerInput actual(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &save.input_bindings());
            auto& pad = binding_test_pads::states[2]; pad.connected = true; std::ignore = h.sample();
            pad = pressing(PadControl::left_stick); std::ignore = h.sample();
            require(actual.player_inputs()[2].jump_pressed, "Save-borrowing reader ignored a pad profile kept active after write failure");
            pad = pressing(PadControl::none); std::ignore = h.sample();
            Save previous_disk; previous_disk.load_save_file();
            require(previous_disk.input_bindings().pads == default_pads,
                "Failed pad persistence damaged reloadable previous choices");
            require(save.set_input_bindings_and_save(candidate), "Pad preference retry failed after removing real IO blocker");
            require_schema(directory.contents(), "j", custom); require_independent(save);
        }
        auto play = save.play_settings(); play.music_volume = 81;
        require(save.set_play_settings_and_save(play) && save.input_bindings().pads == custom &&
            save.set_display_settings_and_save(ScreenResolution::s_3840_2160, false, TeamColourPair::blue_orange),
            "Unrelated settings save overwrote per-slot pad profiles");
        Save final; final.load_save_file();
        require(final.input_bindings().keyboard_jump == Key::j && final.input_bindings().pads == custom,
            "Independent display/play save did not preserve keyboard and per-slot pad profiles");
    }

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
        Save save = load(directory, "{\"keyboard_jump\":\"j\"}");
        require(same_keyboard(save.input_bindings(), InputBindings{ Key::j }), "A save without the keyboard object lost the defaults");
        require(save.save_to_file(), "Cannot write the keyboard object");
        require_schema(directory.contents(), "j"); require_independent(save);
        // Round trip of every action moved, with pads untouched by the page's merge.
        InputBindings custom = custom_keyboard();
        custom.pads[2][PadAction::jump].primary = PadControl::b;
        require(save.set_input_bindings_and_save(custom), "A valid custom keyboard set did not save");
        auto custom_pads = default_pads; custom_pads[2][PadAction::jump].primary = PadControl::b;
        require_schema(directory.contents(), "a", custom_pads, { "j", "l", "k", "mouse_right", "p" });
        { Save reread; reread.load_save_file(); require(reread.input_bindings() == custom, "Custom keyboard set did not round trip");
          require_independent(reread); }
        for (const auto& expected : expected_mouse)
        {
            InputBindings value; value.keyboard_shoot = mouse_control(expected.button);
            require(save.set_input_bindings_and_save(value), "A Shoot mouse button did not save");
            require_schema(directory.contents(), "space", default_pads, { "a", "d", "s", expected.name, "none" });
            Save reread; reread.load_save_file(); require(reread.input_bindings().keyboard_shoot == value.keyboard_shoot,
                "A Shoot mouse button did not round trip");
        }
        // Refused before mutation: the active choice and the disk survive.
        const InputBindings before = save.input_bindings(); const std::string bytes = directory.contents();
        InputBindings conflict; conflict.keyboard_drop = Key::d;
        rejects([&] { std::ignore = save.set_input_bindings_and_save(conflict); }, "A conflicting keyboard set was accepted");
        require(save.input_bindings() == before && directory.contents() == bytes,
            "A refused keyboard set mutated the active preference or the disk");

        const std::string valid_pads = ",\"pads\":[{\"jump\":[\"x\"],\"shoot\":[\"rt\"],\"pause\":[\"start\"]}]";
        const auto bindings = [&](const std::string& jump, const std::string& keyboard)
        { return "{\"keyboard_jump\":\"" + jump + "\"" + (keyboard.empty() ? "" : ",\"keyboard\":" + keyboard) + valid_pads + "}"; };
        const std::string good = "{\"move_left\":\"j\",\"move_right\":\"l\",\"drop\":\"k\",\"shoot\":\"f\",\"pause\":\"p\"}";
        save = load(directory, bindings("a", good));
        require(save.input_bindings().keyboard_move_left == Key::j && save.input_bindings().keyboard_jump == Key::a &&
            save.input_bindings().keyboard_shoot == key_control(Key::f) && save.input_bindings().keyboard_pause == Key::p &&
            save.input_bindings().pads[0][PadAction::jump].primary == PadControl::x,
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
                save.load_save_file();
                InputBindings expected; expected.keyboard_jump = std::string_view(jump) == "e" ? Key::e : Key::space;
                require(same_keyboard(save.input_bindings(), expected) && directory.contents() == disk,
                    "A damaged keyboard object did not recover the five, kept a now-clashing Jump, or rewrote the bytes");
                require(save.input_bindings().pads[0][PadAction::jump].primary == PadControl::x,
                    "A damaged keyboard object discarded a valid pad profile");
                require_independent(save);
            }
        }
        // A well-formed object that clashes with Jump recovers the five first.
        save = load(directory, bindings("j", good));
        require(same_keyboard(save.input_bindings(), InputBindings{ Key::j }),
            "A keyboard object clashing with Jump kept its keys or discarded Jump");
        // A damaged Jump keeps a valid object that does not need it.
        save = load(directory, bindings("enter", good));
        InputBindings recovered = custom_keyboard(); recovered.keyboard_jump = Key::space; recovered.keyboard_shoot = key_control(Key::f);
        require(same_keyboard(save.input_bindings(), recovered), "A damaged Jump discarded a valid keyboard object");
        std::cout << "PASS keyboard actions save: missing-object defaults, schema/round trip of all six and every Shoot "
            "mouse button, refusal before mutation, 13 damaged objects x 2 Jump cases, Jump-clash and damaged-Jump recovery, "
            "pads and independent sections intact\n";
    }

    void check_relaunch(const char* content, std::string_view mode, const std::filesystem::path& profile)
    {
        require(mode == "relaunch-write" || mode == "relaunch-read", "Unknown input-binding relaunch mode");
        const IsolatedSaveDirectory directory(profile);
        if (mode == "relaunch-write")
        {
            require(!std::filesystem::exists(directory.save_path()), "Relaunch writer requires a fresh private profile");
            directory.write(fixture()); Save save; save.load_save_file(); require_independent(save);
            require(save.input_bindings() == InputBindings{} && save.set_input_bindings_and_save(InputBindings{ Key::j }),
                "Relaunch writer did not persist J through the targeted Save setter");
            require_schema(directory.contents(), "j"); require_independent(save);
        }
        else
        {
            require(std::filesystem::is_regular_file(directory.save_path()), "Relaunch reader requires the writer's private save");
            const std::string before = directory.contents(); Save save; save.load_save_file(); require_independent(save);
            require(save.input_bindings() == InputBindings{ Key::j }, "Fresh-process Save lost the persisted J preference");
            ReaderHarness h(content);
            PlayerInput reader(&h.pads, &h.keyboard, &h.mouse, &h.tuning, &h.play, &save.input_bindings());
            h.keyboard.on_key_down(Key::j); std::ignore = h.sample();
            auto input = reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(input.jump_pressed && input.jump_held, "Fresh-process actual reader did not use persisted J");
            h.keyboard.on_key_up(Key::j); std::ignore = h.sample();
            require(!reader.player_inputs()[input_slots::keyboard_and_mouse].skip_intro, "Persisted J became an intro skip alias");
            h.keyboard.on_key_down(Key::space); std::ignore = h.sample();
            input = reader.player_inputs()[input_slots::keyboard_and_mouse];
            require(!input.jump_pressed && !input.jump_held, "Fresh-process custom reader retained default Space Jump");
            h.keyboard.on_key_up(Key::space); std::ignore = h.sample();
            require(reader.player_inputs()[input_slots::keyboard_and_mouse].skip_intro &&
                h.menus[input_slots::keyboard_and_mouse].action == MenuInputAction::proceed && directory.contents() == before,
                "Fresh-process Space aliases or unchanged relaunch save bytes were lost");
            require_independent(save);
        }
        std::cout << "PASS input bindings " << mode << ": explicit retained private profile, actual Save/reader; no game window or physical input\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 4) { check_relaunch(argv[1], argv[2], std::filesystem::path(argv[3])); return EXIT_SUCCESS; }
        require(argc == 2, "Usage: input_bindings.exe <content> [relaunch-write|relaunch-read <private profile root>]");
        const GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
        check_domain_and_mapping(tuning.input()); check_pad_domain_and_mapping(tuning.input());
        check_actual_reader(argv[1]); check_actual_pad_reader(argv[1]); check_actual_resume(argv[1]);
        const IsolatedSaveDirectory directory; check_save_schema_and_recovery(directory);
        check_save_validation_failures_and_independence(directory, argv[1]);
        check_pad_save(directory, argv[1]);
        check_keyboard_domain_and_mapping(tuning.input());
        check_keyboard_reader(argv[1]);
        check_keyboard_save(directory);
        std::cout << "PASS input bindings keyboard actions: explicit six-action domains (A-Z/Space, Shoot's five mouse "
            "buttons, optional extra Pause), reserved keys, defaults equal to the old literals, per-profile conflicts, "
            "effective mapping with fixed Escape/Down/F1/intro/menu aliases, actual reader remap/held adoption/focus, "
            "save schema/round trip, missing/damaged/conflicting keyboard-object recovery and pad independence\n";
        std::cout << "PASS input bindings: independent 27-key names/labels/domain, A/D/S conflicts and W/R aliases, "
            "isolated effective Jump with fixed menu/intro/pad controls; real readers, repeated snapshots/focus/remapping, "
            "held-J ResumeCountdown release gating, version-0/1/2 migration/version-3/future known sections, malformed-section isolation, "
            "validation before mutation, real staged-write/replacement failures with active-before-write preference and retry, "
            "independent Settings/Controls/display/records/remembered saves; per-slot Jump/Shoot/Pause over eleven controls with "
            "alternates, conflicts and trigger thresholds, legacy face migration, local recovery, "
            "sparse-slot/remap/reconnect/resume and failed-pad-save checks; GUID-isolated profile, no window/device\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL input bindings: " << error.what() << '\n'; return EXIT_FAILURE; }
}
