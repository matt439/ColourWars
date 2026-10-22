#pragma once

#include "game/input_slots.h"
#include <algorithm>
#include <array>

// Percentages for one XInput slot, independent of the roster/view number.
// -1 keeps the existing authored movement or shared aiming preference.
struct StickSettings
{
	int movement_deadzone = -1;
	int aim_deadzone = -1;

	static int normalized_deadzone(int value)
	{
		return value < 0 ? -1 : std::min(value, 90);
	}
};

// User preferences, borrowed from Save by a live match. Authored sound levels
// remain in tuning/presentation; the two volumes are the music and SFX sound
// groups' over that balance (game/sound_groups.h).
struct PlaySettings
{
	int aim_sensitivity = 3;
	int music_volume = 100;
	int sfx_volume = 100;
	bool view_shake = true;
	bool aim_marker = true;
	bool reduce_effects = false;
	std::array<StickSettings, input_slots::pad_count> sticks{};

	float music_gain() const { return static_cast<float>(music_volume) / 100.0f; }
	float sfx_gain() const { return static_cast<float>(sfx_volume) / 100.0f; }
	bool view_shake_enabled() const { return this->view_shake && !this->reduce_effects; }

	// Absolute stick aiming has no turn speed. Sensitivity controls the radial
	// deadzone; the default setting uses half the stick's travel.
	float aim_deadzone() const
	{
		constexpr float deadzones[] = { 0.70f, 0.60f, 0.50f, 0.35f, 0.20f };
		return deadzones[std::clamp(this->aim_sensitivity, 1, 5) - 1];
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
