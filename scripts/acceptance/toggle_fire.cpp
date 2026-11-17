#include "game/pch.h"
#include "game/objects/player_input.h"
#include "game/states/resume_countdown.h"
#include "game/input/input_slots.h"
#include "engine/input/gamepad_reader.h"
#include "engine/input/scripted_gamepad_source.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <string>

// REV#31: a pad's Toggle fire turns Shoot into a latch. Scripted pad
// snapshots through the real PlayerInput; no XInput, window or player save.
using namespace labrador;

namespace
{
	using acceptance::require;

	uint16_t button(GamepadButton value) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(value)); }

	struct Harness
	{
		ScriptedGamepadSource source;
		std::array<GamepadState, 4> states{};
		GamepadReader reader{ source };
		Gamepads pads{ &reader };
		Keyboard keyboard;
		Mouse mouse;
		GameTuning tuning;
		PlaySettings preferences;
		InputBindings bindings;
		PlayerInput players;
		std::vector<SlotInput> inputs;

		explicit Harness(const std::filesystem::path& content) :
			tuning((content / "tuning.json").string().c_str()),
			players(&pads, &keyboard, &mouse, &tuning, &preferences, &bindings)
		{
			for (int slot = 0; slot < 4; ++slot)
			{
				states[static_cast<size_t>(slot)].connected = true;
				source.set_vibration_available(slot, false);
			}
			keyboard.set_focused(true);
			mouse.set_focused(true);
			this->step();
			this->step();
		}
		GamepadState& pad(int slot) { return states[static_cast<size_t>(slot)]; }
		void step()
		{
			source.clear_records();
			for (int slot = 0; slot < 4; ++slot) { source.set_state(slot, states[static_cast<size_t>(slot)]); }
			pads.poll();
			require(!source.records_overflowed(), "Scripted pad records overflowed");
			keyboard.poll();
			mouse.poll();
			inputs = players.player_inputs();
			// The match reads once a frame; a second read of the frame must agree.
			const std::vector<SlotInput> repeated = players.player_inputs();
			for (size_t slot = 0; slot < inputs.size(); ++slot)
			{
				require(repeated[slot].primary_shoot == inputs[slot].primary_shoot,
					"A repeated read of one frame changed Shoot");
			}
		}
		bool shooting(int slot) const { return inputs[static_cast<size_t>(slot)].primary_shoot; }
	};

	void check_latch(const std::filesystem::path& content)
	{
		Harness h(content);
		require(!PlaySettings{}.toggle_fire_enabled(0) && !SeatComfort{}.toggle_fire,
			"Toggle fire must default to Off (hold to fire)");
		h.preferences.pad_comfort[1].toggle_fire = true;
		require(h.preferences.toggle_fire_enabled(1) && !h.preferences.toggle_fire_enabled(0) &&
			!h.preferences.toggle_fire_enabled(input_slots::keyboard_and_mouse),
			"Toggle fire must belong to its pad slot alone; the keyboard seat holds");

		// Pad 0 (Off) holds; pad 1 (On) latches on the press.
		h.pad(0).right_trigger = 1.0f;
		h.pad(1).right_trigger = 1.0f;
		h.step();
		require(h.shooting(0) && h.shooting(1), "A Shoot press must fire on both pads");
		h.pad(0).right_trigger = 0.0f;
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(!h.shooting(0), "A hold-to-fire pad must stop on release");
		require(h.shooting(1), "A toggle-fire pad must keep firing after release");
		h.step();
		require(h.shooting(1), "A toggle-fire pad must keep firing with nothing held");
		h.pad(1).right_trigger = 1.0f;
		h.step();
		require(!h.shooting(1), "The next Shoot press must stop a toggle-fire pad");
		h.step();
		require(!h.shooting(1), "Holding the stopping press must not restart firing");
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(!h.shooting(1), "Releasing the stopping press must stay stopped");

		// The pad's chosen Shoot controls, an alternate included, all toggle.
		h.bindings.pads[1][PadAction::shoot].alternate = PadControl::left_shoulder;
		h.pad(1).buttons = button(GamepadButton::left_shoulder);
		h.step();
		h.pad(1).buttons = 0;
		h.step();
		require(h.shooting(1), "An alternate Shoot control must toggle firing on");
		h.pad(1).right_trigger = 1.0f;
		h.step();
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(!h.shooting(1), "Either Shoot control must toggle firing off");

		// Leaving play stops firing; only a fresh press starts it again.
		h.pad(1).right_trigger = 1.0f;
		h.step();
		require(h.shooting(1), "Toggle fire did not start");
		h.players.release_toggles();
		h.step();
		require(!h.shooting(1), "release_toggles must stop a held-press toggle");
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(!h.shooting(1), "A released toggle must not resume");
		h.pad(1).right_trigger = 1.0f;
		h.step();
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(h.shooting(1), "A fresh press after release_toggles must start firing");

		// The resume count-in, as GameLevel runs it: focus loss releases the
		// latch, and every counting sample is read, released, then filtered.
		// A Shoot press during the count is dropped, so the first press after
		// it starts firing at once rather than undoing a hidden latch.
		h.players.release_toggles();
		h.step();
		h.pad(1).right_trigger = 1.0f;
		h.step();
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(h.shooting(1), "Toggle fire did not start before the count-in");
		h.players.release_toggles();
		ResumeCountdown count;
		count.begin(0.25f);
		const auto count_sample = [&](float trigger)
		{
			h.pad(1).right_trigger = trigger;
			h.step();
			if (count.active()) { h.players.release_toggles(); }
			count.filter_inputs(h.inputs);
			const bool shooting = h.shooting(1);
			count.advance(0.1f);
			return shooting;
		};
		require(!count_sample(0.0f), "The count-in must not fire a released latch");
		require(!count_sample(1.0f), "A Shoot press during the count-in must not fire");
		require(!count_sample(0.0f), "The count-in must stay stopped after a press");
		require(!count.active(), "The probe count-in did not finish");
		require(!count_sample(0.0f), "A press during the count-in must not latch past it");
		require(count_sample(1.0f), "The first press after the count-in must start firing");
		require(count_sample(0.0f), "Toggle fire must latch after the count-in");
		require(!count_sample(1.0f), "The next press after the count-in must stop firing");
		h.pad(1).right_trigger = 0.0f;
		h.step();

		// A disconnect stops firing, and a trigger held across the reconnect
		// does not start it.
		h.pad(1).connected = false;
		h.step();
		require(!h.shooting(1), "A disconnected pad must not fire");
		h.pad(1).connected = true;
		h.pad(1).right_trigger = 1.0f;
		h.step();
		require(!h.shooting(1), "A reconnect under a held trigger must not start firing");
		h.step();
		require(!h.shooting(1), "A trigger held across reconnect must stay stopped");
		h.pad(1).right_trigger = 0.0f;
		h.step();
		h.pad(1).right_trigger = 1.0f;
		h.step();
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(h.shooting(1), "A fresh press after reconnect must start firing");

		// Turning the option off stops a running latch and restores holding.
		h.preferences.pad_comfort[1].toggle_fire = false;
		h.step();
		require(!h.shooting(1), "Turning Toggle fire off must stop the latch");
		h.pad(1).right_trigger = 1.0f;
		h.step();
		require(h.shooting(1), "Off must fire while held");
		h.pad(1).right_trigger = 0.0f;
		h.step();
		require(!h.shooting(1), "Off must stop on release");

		// The keyboard seat holds whatever the pads choose.
		h.preferences.pad_comfort.fill(SeatComfort{ SeatSwitch::shared, SeatSwitch::shared, SeatSwitch::shared, true });
		h.mouse.on_button_down(MouseButton::left);
		h.step();
		require(h.shooting(input_slots::keyboard_and_mouse), "The keyboard seat's click must fire");
		h.mouse.on_button_up(MouseButton::left);
		h.step();
		require(!h.shooting(input_slots::keyboard_and_mouse), "The keyboard seat must hold to fire");
	}
}

int main(int argc, char** argv)
{
	return acceptance::run_probe("toggle fire", [&]
	{
		require(argc == 2, "Expected staged content directory");
		check_latch(argv[1]);
		std::cout << "PASS toggle fire: per-pad latch, hold default, alternates, repeated reads, release_toggles, count-in, "
			"disconnect/reconnect, Off mid-latch and the keyboard seat; no hardware/window\n";
		return EXIT_SUCCESS;
	});
}
