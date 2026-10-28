#include "game/pch.h"
#include "scripts/acceptance/input_bindings/fixtures.h"
#include "game/states/resume_countdown.h"
#include <iostream>

namespace input_bindings_acceptance
{
    constexpr std::array<PadControl, 4> faces{ PadControl::a, PadControl::b, PadControl::x, PadControl::y };
    bool same_input(const PlayerInputData& a, const PlayerInputData& b)
    {
        return a.connected == b.connected && a.x_movement == b.x_movement && a.jump_pressed == b.jump_pressed &&
            a.jump_held == b.jump_held && a.shoot_angle == b.shoot_angle && a.shoot_direction == b.shoot_direction &&
            a.shoot_direction_requested == b.shoot_direction_requested && a.primary_shoot == b.primary_shoot &&
            a.drop_through == b.drop_through && a.toggle_debug == b.toggle_debug &&
            a.toggle_pause_menu == b.toggle_pause_menu && a.pause_held == b.pause_held && a.skip_intro == b.skip_intro &&
            a.left_analog_stick == b.left_analog_stick && a.right_analog_stick == b.right_analog_stick;
    }

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
}
