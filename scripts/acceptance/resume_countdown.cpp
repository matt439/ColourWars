#include "game/pch.h"
#include "game/states/resume_countdown.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	std::vector<PlayerInputData> sampled()
	{
		return std::vector<PlayerInputData>(input_slots::count);
	}

	void check_clock()
	{
		ResumeCountdown count;
		require(!count.active() && count.remaining() == 0.0f && !count.advance(1.0f),
			"An idle count must not produce a completion");
		count.begin(1.0f);
		require(count.active() && count.remaining() == 1.0f, "Begin did not arm the count");
		for (const float dt : { -2.0f, 0.0f, std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::quiet_NaN() })
		{
			require(!count.advance(dt) && count.remaining() == 1.0f,
				"Invalid or non-positive time changed the count");
		}
		for (int tick = 0; tick < 7; ++tick)
		{
			require(!count.advance(0.125f), "The count completed before its final tick");
		}
		require(count.remaining() == 0.125f && count.advance(0.125f) && !count.active(),
			"The final tick did not produce exactly one completion");
		require(count.remaining() == 0.0f && !count.advance(10.0f),
			"An expired count repeated its completion");
		count.begin(0.5f);
		require(count.advance(100.0f) && count.remaining() == 0.0f && !count.advance(1.0f),
			"A hitch retained excess time or produced a completion burst");
		count.begin(1.0f);
		for (int tick = 1; tick <= 60; ++tick)
		{
			require(count.advance(1.0f / 60.0f) == (tick == 60),
				"The 60 Hz count completed on the wrong tick");
		}
		count.begin(3.0f);
		count.advance(2.0f);
		count.begin(3.0f);
		require(count.remaining() == 3.0f && !count.advance(2.0f),
			"Restart carried elapsed time into a new count");
		count.cancel();
		require(!count.active() && count.remaining() == 0.0f && !count.advance(1.0f),
			"Cancel left an active clock or completion");
		for (const float duration : { -1.0f, 0.0f, std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::quiet_NaN() })
		{
			count.begin(2.0f);
			count.begin(duration);
			require(!count.active() && count.remaining() == 0.0f,
				"Invalid duration left an old count active");
		}
	}

	void check_action_rearming()
	{
		ResumeCountdown count;
		count.begin(1.0f);
		const size_t slot = input_slots::keyboard_and_mouse;
		std::vector<PlayerInputData> inputs = sampled();
		PlayerInputData& input = inputs[slot];
		input.connected = true;
		input.jump_pressed = input.jump_held = input.primary_shoot = input.drop_through = true;
		input.x_movement = -0.75f;
		input.shoot_direction_requested = true;
		input.shoot_direction = { 0.6f, 0.8f };
		input.shoot_angle = 0.9f;
		count.filter_inputs(inputs);
		require(!input.jump_pressed && input.jump_held && !input.primary_shoot && !input.drop_through,
			"Counting leaked an action or changed the existing held jump");
		require(input.connected && input.x_movement == -0.75f && input.shoot_direction_requested &&
			input.shoot_direction == mattmath::Vector2F(0.6f, 0.8f) && input.shoot_angle == 0.9f,
			"Counting changed movement, continuous aim or device identity");
		require(count.advance(1.0f), "The final sampled tick did not complete");
		input.jump_pressed = input.primary_shoot = input.drop_through = true;
		count.filter_inputs(inputs);
		require(!input.jump_pressed && input.jump_held && !input.primary_shoot && !input.drop_through,
			"Final-sample held actions leaked into live play");
		input.jump_held = false;
		count.filter_inputs(inputs);
		input.jump_pressed = input.jump_held = input.primary_shoot = input.drop_through = true;
		count.filter_inputs(inputs);
		require(input.jump_pressed && input.jump_held && input.primary_shoot && input.drop_through,
			"A neutral sample did not rearm fresh actions");

		// A neutral final sample permits deliberate new actions on the first
		// live tick; actions pressed during the count still need their release.
		count.begin(1.0f);
		inputs = sampled();
		inputs[1].primary_shoot = true;
		count.filter_inputs(inputs);
		inputs[1].primary_shoot = true;
		count.filter_inputs(inputs);
		count.advance(1.0f);
		inputs[1].primary_shoot = inputs[3].primary_shoot = true;
		inputs[3].jump_pressed = inputs[3].jump_held = inputs[3].drop_through = true;
		count.filter_inputs(inputs);
		require(!inputs[1].primary_shoot && inputs[3].primary_shoot && inputs[3].jump_pressed &&
			inputs[3].drop_through, "Sparse slots shared latches or neutral final input stayed blocked");
		inputs[1].primary_shoot = false;
		count.filter_inputs(inputs);
		inputs[1].primary_shoot = true;
		count.filter_inputs(inputs);
		require(inputs[1].primary_shoot, "Fire did not rearm independently after release");
	}

	void check_pause_and_cancel()
	{
		ResumeCountdown count;
		count.begin(1.0f);
		std::vector<PlayerInputData> inputs = sampled();
		inputs[2].pause_held = true;
		count.filter_inputs(inputs);
		count.advance(1.0f);
		inputs[2].pause_held = false;
		inputs[2].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(!inputs[2].toggle_pause_menu, "Pause held across GO reopened the menu on release");
		inputs[2].pause_held = true;
		count.filter_inputs(inputs);
		inputs[2].pause_held = false;
		inputs[2].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(inputs[2].toggle_pause_menu, "A deliberate new pause was suppressed after GO");

		count.begin(1.0f);
		inputs = sampled();
		inputs[0].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(!inputs[0].toggle_pause_menu, "The resume menu's release leaked into pause");
		inputs[0].pause_held = true;
		count.filter_inputs(inputs);
		inputs[0].pause_held = false;
		inputs[0].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(inputs[0].toggle_pause_menu && count.active(),
			"A fresh pause could not interrupt an active count");

		// Restart arms every device again, including slots not sampled yet.
		count.begin(1.0f);
		std::vector<PlayerInputData> absent;
		count.filter_inputs(absent);
		inputs = sampled();
		inputs[input_slots::keyboard_and_mouse].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(!inputs[input_slots::keyboard_and_mouse].toggle_pause_menu,
			"An absent sample lost a later slot's initial pause latch");
		count.cancel();
		inputs[0].jump_pressed = inputs[0].primary_shoot = inputs[0].drop_through = true;
		inputs[0].toggle_pause_menu = true;
		count.filter_inputs(inputs);
		require(inputs[0].jump_pressed && inputs[0].primary_shoot && inputs[0].drop_through &&
			inputs[0].toggle_pause_menu, "Cancel retained input latches for a later match");
	}
}

int main()
{
	try
	{
		check_clock();
		check_action_rearming();
		check_pause_and_cancel();
		std::cout << "PASS resume count-in timing, held-action rearming and deliberate pause\n";
		return EXIT_SUCCESS;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL resume count-in: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
