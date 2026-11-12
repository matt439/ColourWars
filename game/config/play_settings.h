#pragma once

#include "game/input/input_slots.h"
#include <algorithm>
#include <array>
#include <optional>
#include <string>

// Percentages for one XInput slot, independent of the roster/view number.
// -1 keeps the existing authored movement or shared aiming preference.
struct StickSettings
{
	// The value that keeps the inherited deadzone, and the range a chosen one
	// keeps to: the Sticks page steps by deadzone_step and stops at
	// max_deadzone, and the save readers clamp what they read the same way.
	static constexpr int inherit = -1;
	static constexpr int max_deadzone = 90;
	static constexpr int deadzone_step = 5;

	int movement_deadzone = inherit;
	int aim_deadzone = inherit;

	static int normalized_deadzone(int value)
	{
		return value < 0 ? inherit : std::min(value, max_deadzone);
	}

	bool operator==(const StickSettings&) const = default;
};

// A per-slot comfort override: shared follows the Options switch of the same
// name, which still governs everything drawn on the shared screen.
enum class SeatSwitch
{
	off,
	shared,
	on,
};

// What only one pad's player sees: its own pane's camera, aim marker, HUD
// pulses and badge emphasis. Menus, the intro, the countdown, the reveal and
// world effects every pane draws (muzzle flashes, figure motion, drips) are
// shared, so they keep the Options switch.
//
// Toggle fire (REV#31) rides in the same record, saves and profiles, but is
// plain Off/On: no Options switch exists for it, so the keyboard seat holds
// to fire. On, a Shoot press starts firing and the next press stops it.
struct SeatComfort
{
	SeatSwitch view_shake = SeatSwitch::shared;
	SeatSwitch aim_marker = SeatSwitch::shared;
	SeatSwitch reduce_effects = SeatSwitch::shared;
	bool toggle_fire = false;

	static bool resolved(SeatSwitch value, bool shared)
	{
		return value == SeatSwitch::shared ? shared : value == SeatSwitch::on;
	}

	bool operator==(const SeatComfort&) const = default;
};

// The save's spelling of a SeatSwitch: "off", "shared" or "on".
inline const char* seat_switch_key(SeatSwitch value)
{
	switch (value)
	{
	case SeatSwitch::off: return "off";
	case SeatSwitch::on: return "on";
	case SeatSwitch::shared: break;
	}
	return "shared";
}

inline std::optional<SeatSwitch> seat_switch_from_key(const std::string& key)
{
	for (const SeatSwitch value : { SeatSwitch::off, SeatSwitch::shared, SeatSwitch::on })
	{
		if (key == seat_switch_key(value)) { return value; }
	}
	return std::nullopt;
}

// User preferences, borrowed from Save by a live match. Authored sound levels
// remain in tuning/presentation; the two volumes are the music and SFX sound
// groups' over that balance (game/config/sound_groups.h).
struct PlaySettings
{
	// The ranges Save accepts and the Controls page offers.
	static constexpr int min_aim_sensitivity = 1;
	static constexpr int max_aim_sensitivity = 5;
	static constexpr int max_volume = 100;
	static constexpr int volume_step = 10;

	int aim_sensitivity = 3;
	int music_volume = 100;
	int sfx_volume = 100;
	bool view_shake = true;
	bool aim_marker = true;
	bool reduce_effects = false;
	std::array<StickSettings, input_slots::pad_count> sticks{};
	std::array<bool, input_slots::pad_count> pad_rumble = input_slots::per_pad(true);
	std::array<SeatComfort, input_slots::pad_count> pad_comfort{};

	float music_gain() const { return static_cast<float>(music_volume) / static_cast<float>(max_volume); }
	float sfx_gain() const { return static_cast<float>(sfx_volume) / static_cast<float>(max_volume); }
	bool view_shake_enabled() const { return this->view_shake && !this->reduce_effects; }

	// The keyboard seat, bots and unknown slots follow the shared switches.
	const SeatComfort& seat_comfort(int slot) const
	{
		static const SeatComfort shared;
		return input_slots::is_pad(slot) ? this->pad_comfort[static_cast<std::size_t>(slot)] : shared;
	}
	bool reduce_effects_for(int slot) const
	{
		return SeatComfort::resolved(this->seat_comfort(slot).reduce_effects, this->reduce_effects);
	}
	bool view_shake_enabled(int slot) const
	{
		return SeatComfort::resolved(this->seat_comfort(slot).view_shake, this->view_shake) &&
			!this->reduce_effects_for(slot);
	}
	bool aim_marker_enabled(int slot) const
	{
		return SeatComfort::resolved(this->seat_comfort(slot).aim_marker, this->aim_marker);
	}
	bool toggle_fire_enabled(int slot) const
	{
		return this->seat_comfort(slot).toggle_fire;
	}
	bool rumble_enabled(int slot) const
	{
		return input_slots::is_pad(slot) && this->pad_rumble[static_cast<std::size_t>(slot)];
	}

	// Absolute stick aiming has no turn speed. Sensitivity controls the radial
	// deadzone; the default setting uses half the stick's travel.
	float aim_deadzone() const
	{
		constexpr std::array<float, max_aim_sensitivity - min_aim_sensitivity + 1> deadzones{
			0.70f, 0.60f, 0.50f, 0.35f, 0.20f };
		return deadzones[static_cast<std::size_t>(
			std::clamp(this->aim_sensitivity, min_aim_sensitivity, max_aim_sensitivity) - min_aim_sensitivity)];
	}

	const StickSettings& stick_settings(int slot) const
	{
		static const StickSettings inherited;
		return input_slots::is_pad(slot) ? this->sticks[static_cast<std::size_t>(slot)] : inherited;
	}

	float movement_deadzone(int slot, float authored) const
	{
		const int value = StickSettings::normalized_deadzone(this->stick_settings(slot).movement_deadzone);
		return value < 0 ? authored : static_cast<float>(value) / 100.0f;
	}

	float aim_deadzone(int slot) const
	{
		const int value = StickSettings::normalized_deadzone(this->stick_settings(slot).aim_deadzone);
		return value < 0 ? this->aim_deadzone() : static_cast<float>(value) / 100.0f;
	}
};
