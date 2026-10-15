#include "game/pch.h"
#include "game/objects/stick_response.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using mattmath::Vector2F;

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	bool approximately(float actual, float expected)
	{
		return std::fabs(actual - expected) <= 0.00001f;
	}

	bool approximately(const Vector2F& actual, const Vector2F& expected)
	{
		return approximately(actual.x, expected.x) && approximately(actual.y, expected.y);
	}

	void check_thresholds()
	{
		PlaySettings preferences;
		preferences.sticks[0] = { 25, 40 };
		for (const float sign : { -1.0f, 1.0f })
		{
			for (const float travel : { 0.0f, 0.2499f, 0.25f })
			{
				const StickResponse response = stick_response({ travel * sign, 1.0f },
					{ 0.4f * sign, 0.0f }, 0.5f, preferences, 0);
				require(response.movement == 0.0f && response.aim == Vector2F::ZERO,
					"A stick at or below its selected threshold produced a response");
			}
			const StickResponse first = stick_response({ 0.250075f * sign, 0.0f },
				{ 0.40006f * sign, 0.0f }, 0.5f, preferences, 0);
			require(first.movement * sign > 0.0f && approximately(first.movement, sign * 0.0001f) &&
				first.aim.x * sign > 0.0f && approximately(first.aim, { sign * 0.0001f, 0.0f }),
				"The first live stick input jumped instead of starting continuously at zero");
			const StickResponse half = stick_response({ 0.625f * sign, -1.0f },
				{ 0.7f * sign, 0.0f }, 0.5f, preferences, 0);
			require(approximately(half.movement, 0.5f * sign) &&
				approximately(half.aim, { 0.5f * sign, 0.0f }),
				"The live movement or aiming range was not rescaled to full travel");
			const StickResponse full = stick_response({ sign, 0.0f },
				{ sign, 0.0f }, 0.5f, preferences, 0);
			require(approximately(full.movement, sign) && approximately(full.aim, { sign, 0.0f }),
				"A nonzero deadzone prevented full movement or aim travel");
		}

		for (const float x : { -1.0f, 1.0f })
		{
			for (const float y : { -1.0f, 1.0f })
			{
				const StickResponse diagonal = stick_response({ 0.0f, y },
					{ 0.42f * x, 0.56f * y }, 0.5f, preferences, 0);
				require(diagonal.movement == 0.0f &&
					approximately(diagonal.aim, { 0.3f * x, 0.4f * y }),
					"Radial aiming changed direction or vertical stick travel caused horizontal movement");
				const StickResponse full_diagonal = stick_response(Vector2F::ZERO,
					{ 0.6f * x, 0.8f * y }, 0.5f, preferences, 0);
				require(approximately(full_diagonal.aim, { 0.6f * x, 0.8f * y }) &&
					approximately(full_diagonal.aim.length(), 1.0f),
					"Full radial aim lost travel or changed its quadrant");
			}
		}
		// Equal radial distances have the same cutoff, including a diagonal
		// whose components are individually smaller than the radial threshold.
		const StickResponse diagonal_below = stick_response(Vector2F::ZERO,
			{ 0.234f, 0.312f }, 0.5f, preferences, 0);
		const StickResponse diagonal_above = stick_response(Vector2F::ZERO,
			{ 0.246f, 0.328f }, 0.5f, preferences, 0);
		require(diagonal_below.aim == Vector2F::ZERO && diagonal_above.aim.length() > 0.0f &&
			approximately(diagonal_above.aim.normalized(), { 0.6f, 0.8f }),
			"Aiming used separate axis cutoffs instead of a radial cutoff");
	}

	void check_slots_and_live_preferences()
	{
		PlaySettings preferences;
		preferences.sticks[0] = { 0, 90 };
		preferences.sticks[1] = { 25, 50 };
		preferences.sticks[2] = { 50, 25 };
		preferences.sticks[3] = { 90, 0 };
		const float movement[] = { 0.625f, 0.5f, 0.25f, 0.0f };
		const Vector2F aim[] = { Vector2F::ZERO, { 0.15f, 0.2f },
			{ 0.3f, 0.4f }, { 0.375f, 0.5f } };
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const StickResponse response = stick_response({ 0.625f, 0.0f },
				{ 0.375f, 0.5f }, 0.8f, preferences, slot);
			require(approximately(response.movement, movement[slot]) && approximately(response.aim, aim[slot]),
				"Stick response used another XInput slot's movement or aim preferences");
		}
		preferences.sticks[1] = { 75, 75 };
		const StickResponse changed = stick_response({ 0.625f, 0.0f },
			{ 0.375f, 0.5f }, 0.8f, preferences, 1);
		const StickResponse sibling = stick_response({ 0.625f, 0.0f },
			{ 0.375f, 0.5f }, 0.8f, preferences, 2);
		require(changed.movement == 0.0f && changed.aim == Vector2F::ZERO &&
			approximately(sibling.movement, 0.25f) && approximately(sibling.aim, { 0.3f, 0.4f }),
			"Changing a live preference failed to apply immediately or affected another slot");

		preferences.sticks[0] = { 90, 90 };
		const StickResponse upper = stick_response({ 0.95f, 0.0f },
			{ 0.0f, -0.95f }, 0.5f, preferences, 0);
		require(approximately(upper.movement, 0.5f) && approximately(upper.aim, { 0.0f, -0.5f }),
			"The maximum supported deadzone left no safe rescaled live range");
		preferences.sticks[0] = { 0, 0 };
		const StickResponse zero = stick_response({ -0.02f, 1.0f },
			{ 0.03f, -0.04f }, 0.5f, preferences, 0);
		require(approximately(zero.movement, -0.02f) && approximately(zero.aim, { 0.03f, -0.04f }),
			"A zero-percent override still removed or amplified live input");
	}

	void check_inheritance_and_invalid_slots()
	{
		PlaySettings preferences;
		preferences.aim_sensitivity = 4;
		for (const int slot : { 0, 1, 2, 3, -1, input_slots::keyboard_and_mouse,
			input_slots::count, (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)() })
		{
			const StickResponse inherited = stick_response({ -0.625f, 1.0f },
				{ 0.405f, 0.54f }, 0.25f, preferences, slot);
			require(approximately(inherited.movement, -0.5f) && approximately(inherited.aim, { 0.3f, 0.4f }),
				"An inherited or invalid slot changed authored movement or legacy global aiming");
		}
		// Preferences can be inspected before the Save setter normalizes them.
		// A corrupt in-memory value must still leave a finite usable response.
		preferences.sticks[0] = { 999, 999 };
		const StickResponse clamped = stick_response({ 0.95f, 0.0f },
			{ 0.95f, 0.0f }, 0.25f, preferences, 0);
		require(approximately(clamped.movement, 0.5f) && approximately(clamped.aim, { 0.5f, 0.0f }),
			"Oversized in-memory overrides did not retain a usable bounded response");
		preferences.sticks[0] = { -999, -999 };
		preferences.aim_sensitivity = 999;
		const StickResponse inherited = stick_response({ 0.625f, 0.0f },
			{ 0.6f, 0.0f }, 0.25f, preferences, 0);
		require(approximately(inherited.movement, 0.5f) && approximately(inherited.aim, { 0.5f, 0.0f }),
			"Invalid negative overrides or global sensitivity did not fall back safely");
	}
}

int main()
{
	try
	{
		check_thresholds();
		check_slots_and_live_preferences();
		check_inheritance_and_invalid_slots();
		std::cout << "PASS stick settings: shared gameplay/preview response, scalar movement, radial aiming, "
			"threshold continuity, full travel, four live pad profiles, inheritance and safe bounds\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL stick settings: " << error.what() << '\n';
		return 1;
	}
}
