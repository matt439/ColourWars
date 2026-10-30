#include "game/pch.h"
#include "game/action_bindings.h"
#include "game/input_slots.h"
#include "game/objects/player_input.h"
#include "game/states/menu_input.h"
#include "game/match_setup.h"
#include "engine/input/gamepad_reader.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

// The existing concrete reader link seam: no XInput, window or player save.
namespace action_test_pads { std::array<labrador::GamepadState, 4> states{}; }
namespace labrador
{
	struct GamepadReader::Impl { std::array<GamepadState, 4> states{}; bool suspended = false; };
	GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
	GamepadReader::~GamepadReader() = default;
	GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
	GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
	void GamepadReader::sample()
	{
		this->impl_->states = this->impl_->suspended ? std::array<GamepadState, 4>{} : action_test_pads::states;
	}
	GamepadState GamepadReader::read(int slot) const
	{
		return slot >= 0 && slot < 4 ? this->impl_->states[static_cast<size_t>(slot)] : GamepadState{};
	}
	void GamepadReader::suspend() { this->impl_->suspended = true; }
	void GamepadReader::resume() { this->impl_->suspended = false; }
}

using namespace labrador;
using mattmath::Vector2F;

namespace
{
	using acceptance::require;
	uint16_t button(GamepadButton value) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(value)); }
	bool pad_down(const GamepadState& state, GamepadButton value) { return (state.buttons & button(value)) != 0; }
	bool key_down(const KeyboardState& state, Key value)
	{
		const auto index = static_cast<unsigned int>(value);
		return (state.keys[index / 64] & (uint64_t(1) << (index % 64))) != 0;
	}
	void set_key(KeyboardState& state, Key value)
	{
		const auto index = static_cast<unsigned int>(value);
		state.keys[index / 64] |= uint64_t(1) << (index % 64);
	}
	bool held(const KeyboardState& state, Key value) { return state.focused && key_down(state, value); }
	bool near_value(float a, float b) { return std::fabs(a - b) <= 0.00001f; }
	bool near_value(const Vector2F& a, const Vector2F& b) { return near_value(a.x, b.x) && near_value(a.y, b.y); }
	void merge(ActionSample& target, const ActionSample& source)
	{
		target.held = target.held || source.held; target.pressed = target.pressed || source.pressed;
		target.released = target.released || source.released; target.was_held = target.was_held || source.was_held;
	}
	ActionSample legacy_key(const KeyboardState& now, const KeyboardState& before, Key key, bool menu = false)
	{
		if (!now.focused) { return {}; }
		const bool down = key_down(now, key), was = key_down(before, key);
		ActionSample result{ down, before.focused && down && !was, before.focused && !down && was, before.focused && was };
		if (menu) { result.was_held = (result.held && !result.pressed) || result.released; }
		return result;
	}
	ActionSample legacy_button(const GamepadState& now, const GamepadState& before, GamepadButton key, bool menu = false)
	{
		if (!now.connected) { return {}; }
		const bool down = pad_down(now, key), was = pad_down(before, key);
		return { down, before.connected && down && !was, before.connected && !down && was, menu ? was : before.connected && was };
	}
	ActionSample legacy_trigger(const GamepadState& now, const GamepadState& before, float value, float previous, float threshold)
	{
		if (!now.connected) { return {}; }
		return { value > threshold, before.connected && value > threshold && previous <= threshold,
			before.connected && value <= threshold && previous > threshold, before.connected && previous > threshold };
	}

	// Independent expected mappings from frozen commit 10e4942 PlayerInput and
	// MenuInput. Literal physical controls below never come from ActionBindings.
	std::vector<Key> legacy_game_keys(GameAction action)
	{
		switch (action)
		{
		case GameAction::move_left: return { Key::a };
		case GameAction::move_right: return { Key::d };
		case GameAction::jump: return { Key::space };
		case GameAction::drop: return { Key::s, Key::down };
		case GameAction::pause: return { Key::escape };
		case GameAction::debug: return { Key::f1 };
		case GameAction::skip_intro: return { Key::space, Key::escape, Key::enter };
		default: return {};
		}
	}
	ActionSample legacy_desktop(GameAction action, const KeyboardState& now, const KeyboardState& before,
		const MouseState& mouse, const MouseState& previous_mouse)
	{
		ActionSample result;
		for (Key key : legacy_game_keys(action)) { merge(result, legacy_key(now, before, key)); }
		if (action == GameAction::shoot && mouse.focused)
		{
			const uint8_t mask = static_cast<uint8_t>(1u << static_cast<unsigned int>(MouseButton::left));
			const bool down = (mouse.buttons & mask) != 0, was = (previous_mouse.buttons & mask) != 0;
			const bool valid = previous_mouse.focused && mouse.button_epoch == previous_mouse.button_epoch;
			merge(result, { down, valid && down && !was, valid && !down && was, valid && was });
		}
		return result;
	}
	ActionSample legacy_pad_action(GameAction action, const GamepadState& now, const GamepadState& before, const InputTuning& tuning)
	{
		ActionSample result;
		if (action == GameAction::jump)
		{
			merge(result, legacy_button(now, before, GamepadButton::a));
			merge(result, legacy_trigger(now, before, now.left_trigger, before.left_trigger, tuning.trigger_jump_threshold));
		}
		if (action == GameAction::shoot)
			{ merge(result, legacy_trigger(now, before, now.right_trigger, before.right_trigger, tuning.trigger_shoot_threshold)); }
		if (action == GameAction::pause) { merge(result, legacy_button(now, before, GamepadButton::start)); }
		if (action == GameAction::debug) { merge(result, legacy_button(now, before, GamepadButton::back)); }
		if (action == GameAction::skip_intro)
		{
			for (const auto key : { GamepadButton::a, GamepadButton::b, GamepadButton::start })
				{ merge(result, legacy_button(now, before, key)); }
		}
		return result;
	}
	std::vector<Key> legacy_menu_keys(MenuInputAction action)
	{
		if (action == MenuInputAction::proceed) { return { Key::enter, Key::space }; }
		if (action == MenuInputAction::back) { return { Key::escape, Key::backspace }; }
		if (action == MenuInputAction::shuffle) { return { Key::r }; }
		return {};
	}
	ActionSample legacy_menu_key(MenuInputAction action, const KeyboardState& now, const KeyboardState& before)
	{
		ActionSample result;
		for (Key key : legacy_menu_keys(action)) { merge(result, legacy_key(now, before, key, true)); }
		return result;
	}
	ActionSample legacy_menu_pad(MenuInputAction action, const GamepadState& now, const GamepadState& before)
	{
		switch (action)
		{
		case MenuInputAction::proceed: return legacy_button(now, before, GamepadButton::a, true);
		case MenuInputAction::back: return legacy_button(now, before, GamepadButton::b, true);
		case MenuInputAction::pause: return legacy_button(now, before, GamepadButton::start, true);
		case MenuInputAction::shuffle: return legacy_button(now, before, GamepadButton::x, true);
		default: return {};
		}
	}
	void same_sample(const ActionSample& actual, const ActionSample& expected, const std::string& where)
	{
		require(actual.held == expected.held && actual.pressed == expected.pressed &&
			actual.released == expected.released && actual.was_held == expected.was_held, where + ": action sample differs from legacy");
	}
	void check_pure(const InputTuning& tuning)
	{
		require(game_action_count == 8 && menu_action_count == 4 && menu_direction_count == 4,
			"The bounded default action surface changed without its legacy comparison");
		const ActionBindings& bindings = default_action_bindings();
		const auto desktop = [&](const KeyboardState& now, const KeyboardState& before,
			const MouseState& mouse = {}, const MouseState& previous_mouse = {})
		{
			for (int index = 0; index < 8; ++index)
			{
				const auto action = static_cast<GameAction>(index);
				same_sample(game_action(bindings, action, now, before, mouse, previous_mouse),
					legacy_desktop(action, now, before, mouse, previous_mouse), "desktop/game" + std::to_string(index));
			}
			for (const auto action : { MenuInputAction::proceed, MenuInputAction::back, MenuInputAction::pause, MenuInputAction::shuffle })
				{ same_sample(menu_action(bindings, action, now, before), legacy_menu_key(action, now, before), "desktop/menu" + std::to_string(static_cast<int>(action))); }
			require(keyboard_direction_held(bindings, now) == std::array<bool, 4>{ held(now, Key::up) || held(now, Key::w),
				held(now, Key::down) || held(now, Key::s), held(now, Key::left) || held(now, Key::a),
				held(now, Key::right) || held(now, Key::d) }, "Keyboard direction aliases changed");
		};
		for (const Key key : { Key::a, Key::d, Key::space, Key::s, Key::down, Key::escape, Key::f1, Key::enter,
			Key::backspace, Key::r, Key::up, Key::w, Key::left, Key::right, Key::grave, Key::f10, Key::f11, Key::tab })
		{
			KeyboardState empty; empty.focused = true;
			KeyboardState pushed = empty; set_key(pushed, key);
			desktop(pushed, empty); desktop(pushed, pushed); desktop(empty, pushed);
			KeyboardState unfocused = pushed; unfocused.focused = false;
			desktop(unfocused, pushed); desktop(pushed, unfocused);
		}
		KeyboardState aliases; aliases.focused = true; set_key(aliases, Key::enter);
		KeyboardState combined = aliases; set_key(combined, Key::space); desktop(combined, aliases); desktop(aliases, combined);
		for (const bool current_focus : { false, true })
		{
			for (const bool previous_focus : { false, true })
			{
				for (const bool same_epoch : { false, true })
				{
					MouseState before; before.focused = previous_focus; before.button_epoch = 2; before.buttons = 2;
					MouseState now = before; now.focused = current_focus; now.button_epoch = same_epoch ? 2 : 3; now.buttons = 0;
					desktop({}, {}, now, before); desktop({}, {}, before, now);
				}
			}
		}
		const auto pad = [&](const GamepadState& now, const GamepadState& before, const InputTuning& look)
		{
			for (int index = 0; index < 8; ++index)
			{
				const auto action = static_cast<GameAction>(index);
				same_sample(game_action(bindings, action, now, before, look), legacy_pad_action(action, now, before, look), "pad/game" + std::to_string(index));
			}
			for (const auto action : { MenuInputAction::proceed, MenuInputAction::back, MenuInputAction::pause, MenuInputAction::shuffle })
				{ same_sample(menu_action(bindings, action, now, before), legacy_menu_pad(action, now, before), "pad/menu" + std::to_string(static_cast<int>(action))); }
			require(dpad_direction_held(bindings, now) == std::array<bool, 4>{ now.connected && pad_down(now, GamepadButton::dpad_up),
				now.connected && pad_down(now, GamepadButton::dpad_down), now.connected && pad_down(now, GamepadButton::dpad_left),
				now.connected && pad_down(now, GamepadButton::dpad_right) }, "D-pad direction mappings changed");
		};
		for (const auto key : { GamepadButton::a, GamepadButton::b, GamepadButton::x, GamepadButton::y, GamepadButton::start,
			GamepadButton::back, GamepadButton::dpad_up, GamepadButton::dpad_down, GamepadButton::dpad_left, GamepadButton::dpad_right })
		{
			GamepadState empty; empty.connected = true;
			GamepadState pushed = empty; pushed.buttons = button(key);
			pad(pushed, empty, tuning); pad(pushed, pushed, tuning); pad(empty, pushed, tuning);
			GamepadState absent = pushed; absent.connected = false;
			pad(absent, pushed, tuning); pad(pushed, absent, tuning);
		}
		for (const float threshold : { 0.0f, tuning.trigger_jump_threshold, 0.5f, 1.0f })
		{
			InputTuning look = tuning; look.trigger_jump_threshold = look.trigger_shoot_threshold = threshold;
			GamepadState before; before.connected = true; before.buttons = button(GamepadButton::a);
			before.left_trigger = before.right_trigger = threshold;
			for (const float travel : { std::nextafter(threshold, -std::numeric_limits<float>::infinity()), threshold,
				std::nextafter(threshold, std::numeric_limits<float>::infinity()) })
			{
				GamepadState now = before; now.left_trigger = now.right_trigger = travel;
				pad(now, before, look); pad(before, now, look);
			}
			GamepadState fixed = before; fixed.left_trigger = fixed.right_trigger = 0.6f;
			look.trigger_jump_threshold = look.trigger_shoot_threshold = 0.7f; pad(fixed, fixed, look);
			look.trigger_jump_threshold = look.trigger_shoot_threshold = 0.3f; pad(fixed, fixed, look);
		}
		InputTuning asymmetric = tuning; asymmetric.trigger_jump_threshold = 0.3f; asymmetric.trigger_shoot_threshold = 0.7f;
		GamepadState asymmetric_pad; asymmetric_pad.connected = true; asymmetric_pad.left_trigger = asymmetric_pad.right_trigger = 0.5f;
		pad(asymmetric_pad, asymmetric_pad, asymmetric);
	}

	SlotInput legacy_player(const GamepadState& now, const GamepadState& before,
		const GameTuning& tuning, const PlaySettings& preferences, int slot)
	{
		SlotInput result;
		if (!now.connected) { return result; }
		result.connected = true;
		const auto jump = legacy_pad_action(GameAction::jump, now, before, tuning.input());
		result.jump_held = jump.held; result.jump_pressed = jump.pressed;
		result.primary_shoot = legacy_pad_action(GameAction::shoot, now, before, tuning.input()).held;
		const auto pause = legacy_pad_action(GameAction::pause, now, before, tuning.input());
		result.toggle_pause_menu = pause.released; result.pause_held = pause.held;
		result.toggle_debug = legacy_pad_action(GameAction::debug, now, before, tuning.input()).released;
		result.skip_intro = legacy_pad_action(GameAction::skip_intro, now, before, tuning.input()).released;
		result.drop_through = now.left_stick.y > tuning.player().drop_down_analog_threshold;
		const auto& profile = preferences.sticks[static_cast<size_t>(slot)];
		const float movement = profile.movement_deadzone < 0 ? tuning.input().movement_deadzone :
			static_cast<float>(std::min(profile.movement_deadzone, 90)) / 100.0f;
		const float magnitude = std::fabs(now.left_stick.x);
		result.x_movement = magnitude <= movement ? 0.0f : std::copysign((magnitude - movement) / (1.0f - movement), now.left_stick.x);
		constexpr float aims[] = { 0.70f, 0.60f, 0.50f, 0.35f, 0.20f };
		const float cutoff = profile.aim_deadzone < 0 ? aims[std::clamp(preferences.aim_sensitivity, 1, 5) - 1] :
			static_cast<float>(std::min(profile.aim_deadzone, 90)) / 100.0f;
		const float distance = std::sqrt(now.right_stick.x * now.right_stick.x + now.right_stick.y * now.right_stick.y);
		result.shoot_direction_requested = distance > cutoff;
		if (result.shoot_direction_requested)
			{ result.shoot_direction = now.right_stick / distance; result.shoot_angle = std::atan2(now.right_stick.y, now.right_stick.x); }
		result.left_analog_stick = now.left_stick; result.right_analog_stick = now.right_stick;
		return result;
	}
	SlotInput legacy_player(const KeyboardState& now, const KeyboardState& before, const MouseState& mouse)
	{
		SlotInput result; result.connected = true;
		result.x_movement = (held(now, Key::d) ? 1.0f : 0.0f) - (held(now, Key::a) ? 1.0f : 0.0f);
		const auto jump = legacy_desktop(GameAction::jump, now, before, {}, {});
		result.jump_held = jump.held; result.jump_pressed = jump.pressed;
		result.primary_shoot = legacy_desktop(GameAction::shoot, now, before, mouse, {}).held;
		result.drop_through = held(now, Key::s) || held(now, Key::down);
		const auto pause = legacy_desktop(GameAction::pause, now, before, {}, {});
		result.toggle_pause_menu = pause.released; result.pause_held = pause.held;
		result.toggle_debug = legacy_desktop(GameAction::debug, now, before, {}, {}).released;
		result.skip_intro = legacy_desktop(GameAction::skip_intro, now, before, {}, {}).released;
		return result;
	}
	void same_player(const SlotInput& a, const SlotInput& b, const std::string& where)
	{
		require(a.connected == b.connected && near_value(a.x_movement, b.x_movement) && a.jump_held == b.jump_held &&
			a.jump_pressed == b.jump_pressed && a.primary_shoot == b.primary_shoot && a.drop_through == b.drop_through &&
			a.toggle_pause_menu == b.toggle_pause_menu && a.pause_held == b.pause_held && a.toggle_debug == b.toggle_debug &&
			a.skip_intro == b.skip_intro && a.shoot_direction_requested == b.shoot_direction_requested &&
			near_value(a.shoot_direction, b.shoot_direction) && near_value(a.shoot_angle, b.shoot_angle) &&
			a.left_analog_stick == b.left_analog_stick && a.right_analog_stick == b.right_analog_stick,
			where + ": PlayerInput differs from frozen legacy mapping");
	}
	struct LegacyMenu
	{
		std::array<std::array<bool, 4>, 5> armed{};
		std::vector<ProcessedMenuInput> read(const Gamepads& pads, const Keyboard& keyboard)
		{
			std::vector<ProcessedMenuInput> result(5);
			for (size_t slot = 0; slot < 5; ++slot)
			{
				if (slot < 4 && !pads.state(static_cast<int>(slot)).connected) { continue; }
				std::array<ActionSample, 4> samples{};
				int down = 0, previous = 0;
				for (int index = 0; index < 4; ++index)
				{
					const auto action = static_cast<MenuInputAction>(index);
					samples[static_cast<size_t>(index)] = slot == 4 ? legacy_menu_key(action, keyboard.state(), keyboard.previous_state()) :
						legacy_menu_pad(action, pads.state(static_cast<int>(slot)), pads.previous_state(static_cast<int>(slot)));
					down += samples[static_cast<size_t>(index)].held ? 1 : 0; previous += samples[static_cast<size_t>(index)].was_held ? 1 : 0;
				}
				result[slot].connected = true;
				const bool boundary = slot == 4 ? keyboard.state().focused && !keyboard.previous_state().focused :
					!pads.previous_state(static_cast<int>(slot)).connected;
				for (size_t index = 0; index < 4; ++index)
				{
					const auto& sample = samples[index];
					armed[slot][index] = !boundary && (sample.pressed || (armed[slot][index] && sample.was_held));
					if (armed[slot][index] && down == 0 && previous == 1 && sample.was_held)
						{ result[slot].action = static_cast<MenuInputAction>(index); }
				}
			}
			return result;
		}
	};
	struct Harness
	{
		GamepadReader reader; Gamepads pads{ &reader }; Keyboard keyboard; Mouse mouse;
		GameTuning tuning; PlaySettings preferences; InputBindings input_bindings;
		PlayerInput players; MenuInput menu; LegacyMenu legacy;
		std::vector<SlotInput> gameplay; std::vector<ProcessedMenuInput> menus;
		explicit Harness(const char* directory) : tuning((std::filesystem::path(directory) / "tuning.json").string().c_str()),
			players(&pads, &keyboard, &mouse, &tuning, &preferences, &input_bindings), menu(&pads, &keyboard, &tuning)
		{
			action_test_pads::states = {}; keyboard.set_focused(true); mouse.set_focused(true);
		}
		void step(const std::string& name, float dt = 0.0f, bool use_stick = true)
		{
			pads.poll(); keyboard.poll(); mouse.poll(); menu.update_directions(dt, 0.4f, 0.1f, use_stick);
			gameplay = players.player_inputs();
			require(gameplay.size() == 5, name + ": gameplay slot count changed");
			for (int slot = 0; slot < 4; ++slot)
				{ same_player(gameplay[static_cast<size_t>(slot)], legacy_player(pads.state(slot), pads.previous_state(slot), tuning, preferences, slot), name); }
			same_player(gameplay[4], legacy_player(keyboard.state(), keyboard.previous_state(), mouse.state()), name);
			const auto repeated = players.player_inputs();
			for (size_t slot = 0; slot < 5; ++slot) { same_player(repeated[slot], gameplay[slot], name + "/repeated"); }
			for (int repeat = 0; repeat < 2; ++repeat)
			{
				const auto expected = legacy.read(pads, keyboard);
				const auto actual = menu.menu_inputs();
				require(actual.size() == 5, name + ": menu slot count changed");
				for (size_t slot = 0; slot < 5; ++slot)
				{
					require(actual[slot].action == expected[slot].action && actual[slot].connected == expected[slot].connected &&
						(repeat == 0 || actual[slot].direction == menus[slot].direction), name + ": MenuInput differs from legacy/repeated read");
				}
				menus = actual;
			}
		}
		void reset() { menu.reset_actions(); legacy = {}; }
		void reset_pad(int slot)
		{
			menu.reset_pad_actions(slot);
			if (slot >= 0 && slot < 4) { legacy.armed[static_cast<size_t>(slot)] = {}; }
		}
	};

	void check_wrappers(const char* directory)
	{
		Harness h(directory);
		h.step("first neutral focus"); h.step("neutral");
		auto& pad = action_test_pads::states[3];
		pad.connected = true; pad.buttons = static_cast<uint16_t>(button(GamepadButton::a) | button(GamepadButton::start) | button(GamepadButton::back));
		pad.left_trigger = pad.right_trigger = 1.0f;
		h.step("connect with held actions");
		require(h.gameplay[3].jump_held && !h.gameplay[3].jump_pressed && h.menus[3].action == MenuInputAction::none,
			"Reconnect must retain held gameplay without acquiring a menu release");
		h.step("held entry remains unarmed"); pad.buttons = 0; pad.left_trigger = pad.right_trigger = 0;
		h.step("release entry actions"); require(h.menus[3].action == MenuInputAction::none, "A page must not own an entry-held release");
		pad.buttons = button(GamepadButton::a); h.step("fresh pad A");
		pad.left_trigger = std::nextafter(h.tuning.input().trigger_jump_threshold, std::numeric_limits<float>::infinity());
		h.step("A held and LT press"); require(h.gameplay[3].jump_pressed, "A held must not suppress the LT source's new jump edge");
		pad.buttons = 0; h.step("A release with LT held");
		require(h.gameplay[3].jump_held && h.gameplay[3].skip_intro && h.menus[3].action == MenuInputAction::proceed,
			"Releasing one alias must retain another's held state and keep its own release edge");
		pad.left_trigger = 0; h.step("neutral after LT");
		for (const bool staggered : { false, true })
		{
			pad.buttons = static_cast<uint16_t>(button(GamepadButton::a) | button(GamepadButton::b)); h.step("pad action chord");
			if (staggered) { pad.buttons = button(GamepadButton::b); h.step("pad staggered first release"); }
			pad.buttons = 0; h.step("pad chord final release");
			require(h.menus[3].action == (staggered ? MenuInputAction::back : MenuInputAction::none),
				"Only a final single-action release may act; simultaneous releases must not choose by table order");
			h.step("pad chord settled");
		}
		h.keyboard.on_key_down(Key::enter); h.step("Enter down");
		h.keyboard.on_key_down(Key::space); h.step("Space pressed while Enter held");
		require(h.gameplay[4].jump_pressed, "A fresh Space must remain a gameplay press while another intro alias is held");
		h.keyboard.on_key_up(Key::enter); h.step("Enter released while Space held");
		require(h.gameplay[4].skip_intro && h.menus[4].action == MenuInputAction::none, "Menu aliases count as one held logical action");
		h.keyboard.on_key_up(Key::space); h.step("final Proceed alias release");
		require(h.menus[4].action == MenuInputAction::proceed, "Releasing the final Proceed alias must confirm once");
		for (const bool staggered : { false, true })
		{
			h.keyboard.on_key_down(Key::enter); h.keyboard.on_key_down(Key::escape); h.step("keyboard action chord");
			if (staggered) { h.keyboard.on_key_up(Key::enter); h.step("keyboard staggered first release"); }
			h.keyboard.on_key_up(Key::enter); h.keyboard.on_key_up(Key::escape); h.step("keyboard chord final release");
			require(h.menus[4].action == (staggered ? MenuInputAction::back : MenuInputAction::none), "Keyboard chord release policy changed");
			h.step("keyboard chord settled");
		}
		h.keyboard.on_key_down(Key::enter); h.step("press before reset"); h.reset(); h.step("reset while held");
		h.keyboard.on_key_up(Key::enter); h.step("unowned reset release");
		require(h.menus[4].action == MenuInputAction::none, "Context reset must discard the old release owner");
		h.keyboard.on_key_down(Key::enter); h.step("press before focus loss");
		h.keyboard.set_focused(false); h.step("focus lost");
		h.keyboard.set_focused(true); h.keyboard.on_key_down(Key::enter); h.step("focus return held"); h.step("focus held remains unowned");
		h.keyboard.on_key_up(Key::enter); h.step("unowned focus release");
		require(h.menus[4].action == MenuInputAction::none, "Focus return must require a newly owned press");
		h.keyboard.on_key_down(Key::a); h.keyboard.on_key_down(Key::d); h.keyboard.on_key_down(Key::s); h.keyboard.on_key_down(Key::down);
		h.mouse.on_button_down(MouseButton::left); h.step("opposing movement and drop aliases");
		require(h.gameplay[4].x_movement == 0.0f && h.gameplay[4].drop_through && h.gameplay[4].primary_shoot,
			"Opposing movement cancels while drop aliases and mouse shooting remain independent");
		h.keyboard.on_key_up(Key::a); h.keyboard.on_key_up(Key::d); h.keyboard.on_key_up(Key::s); h.step("one drop alias remains");
		h.keyboard.on_key_up(Key::down); h.mouse.cancel_buttons(); h.step("capture cancelled");
		require(!h.gameplay[4].primary_shoot && !h.gameplay[4].drop_through, "Capture cancellation or final drop release remained held");
		h.reset(); h.step("navigation neutral");
		pad.left_stick.x = h.tuning.input().menu_engage_threshold; h.step("stick exactly engages");
		require(h.menus[3].direction == Direction::right, "Menu stick engagement must remain inclusive");
		pad.left_stick.x = h.tuning.input().menu_release_threshold; h.step("stick exactly retains", 0.5f);
		require(h.menus[3].direction == Direction::right, "Menu stick release threshold must remain inclusive during repeat");
		pad.left_stick.x = std::nextafter(h.tuning.input().menu_release_threshold, 0.0f); h.step("below release");
		require(h.menus[3].direction == Direction::none, "Below release must rearm through neutral");
		pad.left_stick.x = std::nextafter(h.tuning.input().menu_engage_threshold, 0.0f); h.step("below engagement");
		require(h.menus[3].direction == Direction::none, "A subthreshold stick must not engage");
		pad.left_stick = {}; h.step("neutral before dpad only");
		pad.left_stick = { -1, 1 }; pad.right_stick = { 1, -1 }; pad.buttons = button(GamepadButton::dpad_up);
		h.step("Dpad only with both sticks held", 0.0f, false);
		require(h.menus[3].direction == Direction::up, "Calibration navigation must ignore opposing sticks and keep the D-pad");
		h.step("Dpad only repeat", 0.5f, false); require(h.menus[3].direction == Direction::up, "D-pad-only repeat was lost");
		pad.buttons = 0; pad.left_stick = {}; pad.right_stick = {}; h.step("all neutral");
		h.keyboard.on_key_down(Key::up); h.keyboard.on_key_down(Key::w); h.step("same-direction aliases");
		require(h.menus[4].direction == Direction::up, "Up+W must count as one direction");
		h.keyboard.on_key_down(Key::down); h.step("opposed navigation");
		require(h.menus[4].direction == Direction::none, "Opposed navigation must not select by table order");
		h.keyboard.on_key_up(Key::down); h.step("conflict requires neutral", 1.0f);
		require(h.menus[4].direction == Direction::none, "Removing one conflicting direction must not bypass neutral rearming");
		h.keyboard.on_key_up(Key::up); h.keyboard.on_key_up(Key::w); h.step("keyboard neutral");
		for (int slot = 0; slot < 4; ++slot)
		{
			auto& state = action_test_pads::states[static_cast<size_t>(slot)];
			state.connected = true; state.left_stick = { 0.625f, 0.0f }; state.right_stick = { 0.375f, 0.5f };
			h.preferences.sticks[static_cast<size_t>(slot)] = { slot * 25, slot * 20 };
		}
		h.step("four distinct live stick profiles");
		const float sibling = h.gameplay[2].x_movement;
		h.preferences.sticks[3] = { 75, 75 }; h.step("one live profile changed");
		require(h.gameplay[3].x_movement == 0.0f && !h.gameplay[3].shoot_direction_requested && h.gameplay[2].x_movement == sibling,
			"A live per-slot preference changed another slot or was captured by the action layer");
		pad.left_stick.y = h.tuning.player().drop_down_analog_threshold; h.step("drop exactly at threshold");
		require(!h.gameplay[3].drop_through, "Drop threshold must remain strict");
		pad.left_stick.y = std::nextafter(h.tuning.player().drop_down_analog_threshold, std::numeric_limits<float>::infinity());
		h.step("drop next representable above"); require(h.gameplay[3].drop_through, "First value above drop threshold was lost");
		pad.left_trigger = pad.right_trigger = 0.5f;
		auto& live = const_cast<InputTuning&>(h.tuning.input()); live.trigger_jump_threshold = 0.3f; live.trigger_shoot_threshold = 0.7f;
		h.step("unequal live trigger thresholds");
		require(h.gameplay[3].jump_held && !h.gameplay[3].primary_shoot, "Jump/shoot threshold selectors were swapped");
		live.trigger_jump_threshold = 0.7f; live.trigger_shoot_threshold = 0.3f; h.step("same raw triggers new thresholds");
		require(!h.gameplay[3].jump_pressed && !h.gameplay[3].jump_held && h.gameplay[3].primary_shoot,
			"Threshold changes must recompute both snapshots without inventing an edge");
		pad.buttons = button(GamepadButton::a); action_test_pads::states[1].buttons = button(GamepadButton::x);
		h.keyboard.on_key_down(Key::enter); h.step("three independent menu owners"); h.reset_pad(3);
		pad.buttons = 0; action_test_pads::states[1].buttons = 0; h.keyboard.on_key_up(Key::enter); h.step("reset one owner only");
		require(h.menus[3].action == MenuInputAction::none && h.menus[1].action == MenuInputAction::shuffle &&
			h.menus[4].action == MenuInputAction::proceed, "Resetting one pad must preserve other pad and keyboard owners");
		MatchSetup roster; roster.set_player_count(2); roster.begin_input_claims(3); require(roster.claim_input_slot(4), "Roster setup failed");
		const auto ordered = h.menu.player_inputs(roster);
		require(ordered.size() == 2 && ordered[0].action == h.menus[3].action && ordered[1].action == h.menus[4].action &&
			ordered[0].connected == h.menus[3].connected && ordered[1].connected == h.menus[4].connected &&
			ordered[0].direction == h.menus[3].direction && ordered[1].direction == h.menus[4].direction,
			"Input extraction must preserve sparse device-to-roster mapping");
		pad.buttons = button(GamepadButton::a); h.step("before suspension"); h.reader.suspend(); h.step("suspended");
		require(!h.gameplay[3].connected && !h.menus[3].connected && h.gameplay[4].connected, "Absent slots must be neutral without losing keyboard");
		h.reader.resume(); h.step("resume with held A"); require(!h.gameplay[3].jump_pressed, "A reconnect held press leaked through");
		pad.buttons = 0; h.step("unowned reconnect release"); require(h.menus[3].action == MenuInputAction::none, "Reconnect acquired an old menu release");
	}
}

int main(int argc, char* argv[])
{
	return acceptance::run_probe("default actions", [&]
	{
		require(argc == 2, "Expected the staged game directory");
		GameTuning tuning((std::filesystem::path(argv[1]) / "tuning.json").string().c_str());
		check_pure(tuning.input()); check_wrappers(argv[1]);
		std::cout << "PASS default actions: independent frozen legacy mappings, aliases/source edges, focus/connection/epoch boundaries, "
			"strict triggers/drop, menu arming/chords/reset/repeated reads, direction aliases, raw sticks and live per-slot preferences; no hardware/window\n";
		return 0;
	});
}
