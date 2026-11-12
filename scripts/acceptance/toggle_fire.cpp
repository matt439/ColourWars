#include "game/pch.h"
#include "game/objects/player_input.h"
#include "game/input/input_slots.h"
#include "engine/input/gamepad_reader.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <string>

// REV#31: a pad's Toggle fire turns Shoot into a latch. Scripted pad
// snapshots through the real PlayerInput; no XInput, window or player save.
namespace toggle_fire_pads { std::array<labrador::GamepadState, 4> states{}; }
namespace labrador
{
	struct GamepadReader::Impl { std::array<GamepadState, 4> states{}; bool suspended = false; };
	GamepadReader::GamepadReader() : impl_(std::make_unique<Impl>()) {}
	GamepadReader::~GamepadReader() = default;
	GamepadReader::GamepadReader(GamepadReader&&) noexcept = default;
	GamepadReader& GamepadReader::operator=(GamepadReader&&) noexcept = default;
	void GamepadReader::sample()
	{
		this->impl_->states = this->impl_->suspended ? std::array<GamepadState, 4>{} : toggle_fire_pads::states;
	}
	GamepadState GamepadReader::read(int slot) const
	{
		return slot >= 0 && slot < 4 ? this->impl_->states[static_cast<size_t>(slot)] : GamepadState{};
	}
	void GamepadReader::suspend() { this->impl_->suspended = true; }
	void GamepadReader::resume() { this->impl_->suspended = false; }
	bool GamepadReader::vibrate(int, const Vibration&) { return false; }
	void GamepadReader::stop_vibration(int) {}
}

using namespace labrador;

namespace
{
	using acceptance::require;

	uint16_t button(GamepadButton value) { return static_cast<uint16_t>(1u << static_cast<unsigned int>(value)); }

	struct Harness
	{
		GamepadReader reader;
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
			toggle_fire_pads::states = {};
			for (GamepadState& pad : toggle_fire_pads::states) { pad.connected = true; }
			keyboard.set_focused(true);
			mouse.set_focused(true);
			this->step();
			this->step();
		}
		GamepadState& pad(int slot) { return toggle_fire_pads::states[static_cast<size_t>(slot)]; }
		void step()
		{
			pads.poll();
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
		std::cout << "PASS toggle fire: per-pad latch, hold default, alternates, repeated reads, release_toggles, "
			"disconnect/reconnect, Off mid-latch and the keyboard seat; no hardware/window\n";
		return EXIT_SUCCESS;
	});
}
