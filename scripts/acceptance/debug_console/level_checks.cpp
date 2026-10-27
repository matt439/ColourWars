#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"
#include "scripts/acceptance/debug_console/cpu_level.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/objects/crude_drawing.h"
#include "game/states/pause_menu.h"
#include "engine/render/text_encoding.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

// The opening and overlay.stations on a real Level built on the CPU without
// a device.
namespace debug_console_acceptance
{
	// The opening on a real Level: it runs before the countdown, nobody moves
	// until GO, a skip lands on the split countdown, a quick rematch is
	// shorter by the settle, and `intro` and recordings override the game.
	void check_opening(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"opening");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		GameData data;
		data.set_stages(&world.stages);
		data.set_level_infos(&world.levels);
		DebugDirector director(&tuning, &presentation);
		director.attach(nullptr, &data);
		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);
		impl.set_content_directories(scratch.source, scratch.staged);
		const float dt = 1.0f / 60.0f;

		MatchRequest roster;
		roster.stage = "close_quarters";
		roster.players = 2;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The opening's roster resolves");
		CpuMatch match(world, director);
		match.settings = plan.settings;
		match.settings.set_duration(1.25f);
		director.attach_match(match);

		const std::vector<PlayerInputData> idle(input_slots::count);
		const auto ticks_while = [&](auto still)
			{
				int ticks = 0;
				for (; ticks < 20000 && still(); ++ticks)
				{
					match.level->update(idle, dt);
				}
				return ticks;
			};
		const OpeningTuning& timing = tuning.match().opening;
		const float whole = timing.settle + timing.preview * 2.0f + timing.undo + timing.leave;
		const auto about = [&](int ticks, float seconds)
			{
				return std::abs(static_cast<float>(ticks) * dt - seconds) <=
					std::max(dt * 2.0f, seconds * 0.001f);
			};

		match.opening = OpeningMode::full;
		match.build();
		Level& level = *match.level;
		require(level.state() == LevelState::opening && level.intro_playing() &&
			!level.match_started(), "A full opening starts before the countdown");
		const mattmath::Vector2F start = level.player(0).rectangle().position();
		const int opening_ticks = ticks_while([&] { return match.level->state() == LevelState::opening; });
		require(about(opening_ticks, whole) && level.state() == LevelState::start_countdown &&
			level.intro_playing(), "The opening runs its length, then counts down in the picture");
		require(level.player(0).rectangle().position() == start,
			"Nobody moves in the opening, however long it is");
		const int countdown_ticks = ticks_while([&] { return !match.level->match_started(); });
		require(about(countdown_ticks, tuning.match().countdown) && !level.intro_playing() &&
			level.state() == LevelState::active, "The countdown ends at GO and the intro with it");

		// The end: back into the same monitor, and the reveal in place of the overview.
		const int played = ticks_while([&] { return match.level->state() == LevelState::active; });
		require(about(played, *match.settings.duration()) && level.state() == LevelState::zoom_out &&
			level.revealed(), "The clock ends into the reveal");
		const RevealTuning& reveal = tuning.match().reveal;
		// Nobody painted here, so the person closes the program and nobody
		// is left to celebrate.
		require(level.level_end_info().winning_team() == PlayerTeam::none,
			"A match nobody painted has no winner to click");
		const int revealing = ticks_while([&] { return match.level->state() != LevelState::finished; });
		require(about(revealing, tuning.match().zoom_out + reveal.walk_in + reveal.hover * 2.0f +
			reveal.decide + reveal.show + reveal.leave), "The pull back and the reveal run their length");

		// A, B or Start from anyone in the match: straight to the split countdown.
		match.build();
		std::vector<PlayerInputData> skip(input_slots::count);
		skip[static_cast<size_t>(match.level->player(0).player_num())].skip_intro = true;
		match.level->update(skip, dt);
		require(match.level->state() == LevelState::start_countdown &&
			!match.level->intro_playing(), "A skip leaves the opening for the split countdown");
		require(about(ticks_while([&] { return !match.level->match_started(); }),
			tuning.match().countdown), "A skipped opening still counts down in full");
		match.build();
		skip.assign(input_slots::count, PlayerInputData());
		for (int slot = 0; slot < input_slots::count; ++slot)
		{
			bool held = false;
			for (int view = 0; view < match.level->player_count(); ++view)
			{
				held = held || match.level->player(view).player_num() == slot;
			}
			skip[static_cast<size_t>(slot)].skip_intro = !held;
		}
		match.level->update(skip, dt);
		require(match.level->state() == LevelState::opening,
			"A device nobody in the match holds does not skip");

		// Start over the intro skips it; the same press released after GO
		// must not then open pause (GameLevel's gate, here without a window).
		{
			std::array<bool, input_slots::count> latched{};
			std::vector<PlayerInputData> pad(input_slots::count);
			const auto frame = [&](bool held, bool released)
				{
					const size_t seat = static_cast<size_t>(match.level->player(0).player_num());
					pad.assign(input_slots::count, PlayerInputData());
					pad[seat].pause_held = held;
					pad[seat].toggle_pause_menu = released;
					pad[seat].skip_intro = released;
					latch_intro_pause(match.level->intro_playing(), pad, latched);
					match.level->update(pad, dt);
					return pad[seat].toggle_pause_menu;
				};
			require(match.level->intro_playing() && !frame(true, false),
				"Start goes down over the intro");
			for (int ticks = 0; ticks < 20000 && !match.level->match_started(); ++ticks)
			{
				frame(true, false);
			}
			require(!match.level->intro_playing() && !frame(false, true),
				"A Start pressed over the intro and released after GO does not pause");
			require(!frame(true, false) && frame(false, true),
				"A fresh Start after GO pauses");
			match.build();
			require(!frame(false, true) && match.level->state() == LevelState::start_countdown,
				"A Start released over the intro skips it without pausing");
		}

		// A rematch starts with the colours being tried.
		match.opening = OpeningMode::quick;
		match.build();
		require(about(ticks_while([&] { return match.level->state() == LevelState::opening; }),
			whole - timing.settle), "A quick opening leaves out the settle");

		// The developer tools choose over the game.
		require(impl.execute("intro off").ok, "intro off");
		match.opening = OpeningMode::full;
		match.build();
		require(match.level->state() == LevelState::start_countdown && !match.level->intro_playing() &&
			!match.level->revealed(), "intro off starts at the split countdown and ends without a reveal");
		require(impl.execute("intro auto").ok, "intro auto");
		match.build();
		require(match.level->state() == LevelState::opening, "intro auto is the game's choice again");
		director.detach_match(match);
		std::cout << "PASS opening and reveal: full, quick, skipped by a seat in the match only, frozen "
			"until GO, the reveal's length, intro overrides\n";
	}

	// overlay.stations on a real Level: one labrador::UiText per station in
	// the Level's Scene, at its authored place, made on first use and only
	// hidden after that; and a match that shows and hides them plays out
	// bit for bit as one that never does.
	void check_station_labels(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"stations");
		GameTuning tuning((scratch.staged / L"tuning.json").string().c_str());
		Presentation presentation((scratch.staged / L"presentation.json").string().c_str());
		CpuWorld world(game_directory, tuning, presentation);
		GameData data;
		data.set_stages(&world.stages);
		data.set_level_infos(&world.levels);
		DebugDirector director(&tuning, &presentation);
		director.attach(nullptr, &data);
		DebugDirectorImpl& impl = *director.impl();
		impl.log().set_mirror_to_debugger(false);
		impl.set_content_directories(scratch.source, scratch.staged);
		const float dt = 1.0f / 60.0f;
		const int length = 400;

		MatchRequest roster;
		roster.stage = "movement_course";
		roster.players = 2;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The stations roster resolves");
		CpuMatch match(world, director);
		match.settings = plan.settings;
		director.attach_match(match);
		const StationLabelPresentation& look = presentation.debug_overlay().station_label;
		const std::vector<LevelStation> stations = world.levels.get("movement_course")->stations();
		// Shown and hidden alike carry the presentation colour premultiplied;
		// hidden is UiText::set_hidden, which skips the draw.
		const labrador::Colour shown_tint = CrudeDrawing::premultiplied(look.colour);
		const auto hidden = [&](int index) { return match.level->station_label(index).hidden(); };
		require(stations.size() == 12, "The course has ten stations and two spawns");

		// The reference: the same seed and keys with the overlay never on.
		std::srand(7);
		match.build();
		require(match.level->station_label_count() == 0, "A level makes no labels until overlay.stations");
		for (int step = 0; step < length; ++step)
		{
			match.level->update(keyboard_step(step), dt);
		}
		require(match.level->station_label_count() == 0, "Nor does a match without the overlay");
		const MatchFingerprint reference = MatchFingerprint::of(*match.level, length);

		std::srand(7);
		match.build();
		Level& level = *match.level;
		require(impl.execute("overlay stations").ok && level.station_label_count() == 12,
			"overlay stations makes one label per station before any update, as a paused match needs");
		require(impl.status_strip().find("stations") != std::string::npos, "The strip names the overlay");
		for (int index = 0; index < level.station_label_count(); ++index)
		{
			const labrador::UiText& label = level.station_label(index);
			const LevelStation& station = stations[static_cast<std::size_t>(index)];
			const mattmath::RectangleF bounds = label.bounds();
			const mattmath::Vector2F expected = station.position + look.offset;
			require(!label.hidden() && label.text() == labrador::widen(station.name) && label.colour() == shown_tint,
				station.name + " is shown, labelled with its name in the station colour, premultiplied");
			require(std::fabs(bounds.x - expected.x) < 0.01f && std::fabs(bounds.y - expected.y) < 0.01f &&
				bounds.width > 0.0f && bounds.height > 0.0f,
				station.name + "'s label is a real box at the station plus the offset, which the scene culls by");
		}
		const auto visible = [&] { return !hidden(0) && level.station_label(0).colour() == shown_tint; };
		for (int step = 0; step < length; ++step)
		{
			if (step == 100)
			{
				const mattmath::RectangleF shown_box = level.station_label(0).bounds();
				require(impl.execute("overlay stations").ok && level.station_label_count() == 12 && hidden(0) &&
					hidden(9) && level.station_label(0).colour() == shown_tint &&
					level.station_label(0).bounds() == shown_box,
					"Turned off, the labels stay, hidden, keeping their colour and their box");
				require(impl.execute("tune presentation.debug_overlay.station_label.colour 10203000").ok,
					"An invisible overlay may still receive an audited transparent colour");
				level.refresh_layout();
				require(hidden(0) && hidden(9) &&
					level.station_label(0).colour() == CrudeDrawing::premultiplied(labrador::Colour(16, 32, 48, 0)) &&
					impl.execute("tune.reset presentation.debug_overlay.station_label.colour").ok,
					"A live colour edit is adopted by hidden station labels without revealing them, and resets independently");
			}
			if (step == 101)
			{
				require(impl.execute("overlay.stations on").ok && level.station_label_count() == 12 && visible(),
					"Turned on again, the same labels show and none are added");
			}
			match.level->update(keyboard_step(step), dt);
		}
		const MatchFingerprint shown = MatchFingerprint::of(level, length);
		require(shown.hash == reference.hash,
			"Labels in the scene change nothing in the match: " + MatchFingerprint::difference(reference, shown));

		// Do the explicit rand sentinel after the seeded simulation comparison,
		// so the sentinel's own seeding cannot perturb that comparison.
		const MatchFingerprint before_colour = MatchFingerprint::of(level, length);
		std::vector<const labrador::UiText*> original_labels;
		for (int index = 0; index < level.station_label_count(); ++index)
			{ original_labels.push_back(&level.station_label(index)); }
		std::srand(8417); const int expected_random = std::rand(); std::srand(8417);
		require(impl.execute("tune presentation.debug_overlay.station_label.colour 12345678").ok,
			"A paused CPU match accepts an audited station-label RGBA colour");
		level.refresh_layout();
		const labrador::Colour edited_tint = CrudeDrawing::premultiplied(labrador::Colour(18, 52, 86, 120));
		for (int index = 0; index < level.station_label_count(); ++index)
		{
			require(&level.station_label(index) == original_labels[static_cast<std::size_t>(index)] &&
				level.station_label(index).colour() == edited_tint,
				"Existing station labels adopt all RGBA channels, premultiplied, without replacing their scene objects");
		}
		require(std::rand() == expected_random && MatchFingerprint::difference(before_colour,
			MatchFingerprint::of(level, length)).empty(),
			"Live station-label colour adoption changes neither gameplay RNG nor the frozen match fingerprint");
		require(impl.execute("tune.reset presentation.debug_overlay.station_label.colour").ok,
			"Station-label colour reset restores the original file token before rebuilding");
		level.refresh_layout();
		require(visible(), "The same labels immediately adopt the restored station colour");

		// Every build re-applies the overlay, and overlay none hides the labels.
		match.build();
		require(match.level->station_label_count() == 12 && !hidden(0) &&
			match.level->station_label(0).colour() == shown_tint,
			"A rebuild shows the labels again");
		require(impl.execute("overlay none").ok && hidden(0) && match.level->station_label(0).colour() == shown_tint,
			"overlay none hides them and keeps their colour");
		director.detach_match(match);
		std::cout << "PASS overlay.stations: a UiText per station in the Scene, at the station, made once, "
			"hidden by set_hidden with its colour and box kept, and the match unchanged by it\n";
	}
}
#endif
