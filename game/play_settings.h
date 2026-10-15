#pragma once

// User preferences, borrowed from Save by a live match. Authored sound levels
// remain in tuning/presentation; these are multipliers over that balance.
struct PlaySettings
{
	int aim_sensitivity = 3;
	int music_volume = 100;
	int sfx_volume = 100;
	bool view_shake = true;
	bool aim_marker = true;

	float music_gain() const { return static_cast<float>(music_volume) / 100.0f; }
	float sfx_gain() const { return static_cast<float>(sfx_volume) / 100.0f; }

	// Absolute stick aiming has no turn speed. Sensitivity controls the radial
	// deadzone; the default setting uses half the stick's travel.
	float aim_deadzone() const
	{
		constexpr float deadzones[] = { 0.70f, 0.60f, 0.50f, 0.35f, 0.20f };
		return deadzones[aim_sensitivity - 1];
	}
};
