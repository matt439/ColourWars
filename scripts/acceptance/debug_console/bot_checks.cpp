#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "game/debug/bot_driver.h"
#include "game/debug/replay.h"
#include "game/states/pause_menu.h"
#include "engine/input/gamepad_reader.h"
#include "engine/input/gamepads.h"
#include <cmath>
#include <cstdint>
#include <memory>

// Bots and the seeded planner.
namespace debug_console_acceptance
{
	namespace
	{
	// A view sequence that moves, lands, stalls and meets an opponent.
	BotSeatView scripted_view(int step)
	{
		BotSeatView view;
		view.step = step;
		view.self.center = { 1000.0f + 3.0f * static_cast<float>(step % 200), 800.0f };
		view.self.velocity = { step % 70 < 25 ? 0.0f : 300.0f, step % 40 < 20 ? 0.0f : -200.0f };
		view.self.team = PlayerTeam::a;
		view.self.on_ground = step % 40 < 20;
		view.self.ammo = static_cast<float>(100 - step % 100) / 100.0f;
		BotSeatView::Seat opponent;
		opponent.center = { 1200.0f - static_cast<float>(step % 300), 600.0f + static_cast<float>(step % 50) };
		opponent.team = PlayerTeam::b;
		opponent.alive = step % 150 < 120;
		view.others.push_back(opponent);
		return view;
	}

	std::vector<std::string> run_bot(const std::string& behaviour, std::uint64_t seed, int steps)
	{
		std::unique_ptr<Bot> bot = Bot::make(behaviour, seed, 2);
		require(bot != nullptr && bot->name() == behaviour, "Bot::make builds " + behaviour);
		std::vector<std::string> outputs;
		bool held = false;
		for (int step = 0; step < steps; ++step)
		{
			const PlayerInputData input = bot->think(scripted_view(step));
			require(input.connected && !input.toggle_debug,
				behaviour + ": a bot is connected and never opens a menu or the F1 overlay");
			require(!input.jump_pressed || (input.jump_held && !held),
				behaviour + ": jump_pressed only on the step a hold begins");
			held = input.jump_held;
			if (input.shoot_direction_requested)
			{
				require(std::fabs(input.shoot_direction.length() - 1.0f) < 1e-4f,
					behaviour + ": the aim is a unit vector");
			}
			outputs.push_back(replay_codec::encode(input));
		}
		return outputs;
	}
	}

	void check_bots(GameTuning& tuning, Presentation& presentation)
	{
		for (const std::string& behaviour : { "walk", "jump", "chase", "paint" })
		{
			const std::vector<std::string> first = run_bot(behaviour, 7, 600);
			require(run_bot(behaviour, 7, 600) == first, behaviour + " is deterministic for a seed");
			if (behaviour != "chase")
			{
				require(run_bot(behaviour, 8, 600) != first, behaviour + " differs with another seed");
			}
		}
		require(Bot::make("idle", 1, 0) == nullptr && Bot::is_behaviour("idle") &&
			!Bot::is_behaviour("dance"), "idle is a word for neutral input, not a class");
		const std::vector<std::string> paint = run_bot("paint", 3, 600);
		require(std::any_of(paint.begin(), paint.end(), [](const std::string& text)
			{ PlayerInputData input; return replay_codec::decode(text, input) && input.primary_shoot; }) &&
			std::any_of(paint.begin(), paint.end(), [](const std::string& text)
			{ PlayerInputData input; return replay_codec::decode(text, input) && input.jump_pressed; }),
			"A paint bot shoots and jumps");

		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		DebugDirector& director = test.director;
		require(impl.execute("bot all paint").ok && impl.execute("bot 1 chase").ok, "bot assigns");
		FakeMatch match(director);
		match.settings = planned(4);
		director.attach_match(match);
		director.after_level_build(match);
		require(impl.status_strip().find("BOTS 4") != std::string::npos,
			"bot all drives the dummies and bot 1 the keyboard seat: " + impl.status_strip());
		require(director.drives_slot(input_slots::keyboard_and_mouse) && !director.drives_slot(0),
			"A bot on the keyboard seat drives its slot");
		std::vector<SlotInput> inputs(input_slots::count);
		update_match(director, match, inputs, 1.0f / 60.0f);
		require(match.last_filter != nullptr, "With bots, the update gets a seat filter");
		require(impl.execute("bot 1 off").ok && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"bot all paint drives only the three idle dummies: " + impl.status_strip());
		require(!director.drives_slot(input_slots::keyboard_and_mouse), "bot 1 off hands the seat back");
		require(impl.execute("bot off").ok && impl.status_strip().find("BOTS") == std::string::npos,
			"bot off clears every bot");
		update_match(director, match, inputs, 1.0f / 60.0f);
		require(match.last_filter == nullptr, "Without bots the update gets no seat filter");
		require(!impl.execute("bot 1 dance").ok && !impl.execute("bot 9 walk").ok && !impl.execute("bot 2").ok,
			"bot checks its seat and behaviour");

		// players N <bot>: the dummies of the rebuilt match get that bot.
		require(impl.execute("players 4 paint").ok && impl.request().players == 4,
			"players 4 paint queues four players");
		require(director.before_match_update(match, 1.0f / 60.0f) && match.rebuilds == 1 &&
			impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"and the rebuild puts paint bots on the three dummies");
		impl.execute("status");
		require(log_contains(impl.log(), " idle team b sprayer bot paint", DebugSeverity::info),
			"status names each seat's bot");
		director.detach_match(match);
		impl.execute("bot off");

		// A bot on a pad seat: that pad's absence never pauses the match.
		MatchRequest roster;
		roster.stage = "x";
		roster.players = 2;
		unsigned int seed = 1;
		const MatchPlan plan = MatchPlanner(&test_stages()).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{ true, false, false, false }, [&seed] { return seed++; });
		require(plan.ok && plan.settings.player_settings()[0].num == 0, "Seat 1 is pad 1");
		FakeMatch pad_match(director);
		pad_match.settings = plan.settings;
		director.attach_match(pad_match);
		labrador::GamepadReader reader;
		labrador::Gamepads pads(&reader);
		require(disconnected_player_slot(pad_match.settings, pads, director) == 0,
			"An unplugged pad seat pauses the match");
		impl.execute("bot 1 walk");
		require(director.drives_slot(0) && disconnected_player_slot(pad_match.settings, pads, director) == -1 &&
			!pad_seat_missing(pad_match.settings, 0, pads, director), "A bot on that seat keeps it from pausing");
		impl.execute("bot off");
		director.detach_match(pad_match);
	}

	void check_seeded_planner(GameTuning& tuning, Presentation& presentation)
	{
		const auto weapons_for = [&](const char* seed)
			{
				TestDirector test(tuning, presentation);
				test.impl.execute(std::string("seed ") + seed);
				test.impl.execute("stage x");
				test.impl.execute("players 4");
				test.impl.execute("weapon all random");
				FakePage page;
				page.start_result = true;
				require(test.director.before_page_update(page, 1.0f / 60.0f) && page.starts == 1,
					"A seeded stage starts from the menus");
				std::string weapons;
				for (const SeatSetup& seat : page.started.player_settings())
				{
					weapons += MatchPlanner::weapon_name(seat.weapon) + std::string(" ");
				}
				return weapons;
			};
		const std::string first = weapons_for("5");
		require(weapons_for("5") == first, "The same seed gives the same random weapons");
		bool differs = false;
		for (const char* other : { "6", "7", "8", "9" })
		{
			differs = differs || weapons_for(other) != first;
		}
		require(differs, "Other seeds give other weapons");

		// Not only the first plan after `seed`: every plan under it.
		TestDirector test(tuning, presentation);
		test.impl.execute("seed 5");
		const auto start = [&]
			{
				test.impl.execute("stage x");
				test.impl.execute("players 4");
				test.impl.execute("weapon all random");
				FakePage page;
				page.start_result = true;
				require(test.director.before_page_update(page, 1.0f / 60.0f) && page.starts == 1,
					"A seeded stage starts from the menus");
				std::string weapons;
				for (const SeatSetup& seat : page.started.player_settings())
				{
					weapons += MatchPlanner::weapon_name(seat.weapon) + std::string(" ");
				}
				return weapons;
			};
		require(start() == first && start() == first && start() == first,
			"Every plan under one seed draws the same random weapons");
	}
}
#endif
