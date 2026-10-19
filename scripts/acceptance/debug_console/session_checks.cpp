#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_binds.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_perf.h"
#include "game/debug/debug_text_field.h"
#include "game/debug/tune_document.h"
#include "game/objects/debug_text.h"
#include "game/session.h"
#include <cmath>
#include <limits>
#include <optional>

// Binds, history, live-console input, the perf ring and polish.
namespace debug_console_acceptance
{
	void check_binds(GameTuning& tuning, Presentation& presentation)
	{
		using labrador::Key;
		for (const auto& [key, name] : debug_keys::names())
		{
			const std::optional<KeyChord> chord = debug_keys::parse(name);
			require(chord && chord->key == key && debug_keys::chord_name(*chord) == name,
				"Key name " + name + " round-trips");
		}
		const std::optional<KeyChord> shifted = debug_keys::parse("SHIFT+F5");
		require(shifted && shifted->shift && !shifted->ctrl && shifted->key == Key::f5 &&
			debug_keys::chord_name(*shifted) == "shift+f5", "shift+f5 parses, case-insensitively");
		const std::optional<KeyChord> both = debug_keys::parse("shift+ctrl+f2");
		require(both && both->ctrl && both->shift && debug_keys::chord_name(*both) == "ctrl+shift+f2",
			"Modifiers in any order");
		require(!debug_keys::parse("alt+f2") && !debug_keys::parse("f13") && !debug_keys::parse("shift+"),
			"alt+, unknown keys and a bare modifier are not keys");

		const DebugBinds& defaults = DebugBinds::defaults();
		const std::vector<std::pair<Key, std::string>> expected = { { Key::f2, "overlay collision" },
			{ Key::f3, "overlay camera" }, { Key::f4, "overlay paint" }, { Key::f5, "restart" },
			{ Key::f6, "pause toggle" }, { Key::f7, "step 1" },
			{ Key::f9, "feedback toggle" } };
		require(defaults.entries().size() == expected.size(), "Seven default binds");
		for (const auto& [key, line] : expected)
		{
			const std::string* found = defaults.find(KeyChord{ key });
			require(found != nullptr && *found == line, "The active defaults retain their F2-F9 shortcuts");
		}
		require(defaults.find(KeyChord{ Key::f8 }) == nullptr, "F8 has no default bind");
		bool shift_held = false;
		const auto pressed_f5 = [](Key key) { return key == Key::f5; };
		const auto held = [&shift_held](Key key) { return key == Key::shift && shift_held; };
		auto fired = defaults.fired(pressed_f5, held);
		require(fired.size() == 1 && fired[0].second == "restart", "F5 fires restart");
		shift_held = true;
		require(defaults.fired(pressed_f5, held).empty(), "Shift+F5 is not F5");
		require(defaults.save_text() == "// ColourWars debug binds - written by the game (bind, unbind, "
			"binds.reset).\n// Only the differences from the default binds; this file is exec-able.\n",
			"The defaults save as the header alone");

		ScratchFolder scratch(L"binds");
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_user_directory(scratch.root);
		test.director.run_launch_arguments({});
		require(!impl.execute("bind grave echo x").ok && any_line(impl.log(), "bind: grave is reserved (the console)") &&
			!impl.execute("bind f10 echo x").ok && !impl.execute("bind shift+f11 echo x").ok,
			"grave, F10 and F11 are reserved with any modifier");
		require(impl.execute("bind a echo moved").ok &&
			log_contains(impl.log(), "bind: a also moves the keyboard player in a match", DebugSeverity::warning),
			"A player key binds, with a warning");
		require(impl.execute("unbind a").ok && !impl.execute("unbind a").ok, "unbind, and unbind of nothing");
		require(impl.execute("bind shift+f5 echo bound-shift-f5").ok, "bind shift+f5");
		fired = impl.binds().fired(pressed_f5, held);
		require(fired.size() == 1 && fired[0].second == "echo bound-shift-f5",
			"shift+f5 fires only with Shift held");
		require(impl.execute("unbind shift+f5").ok && impl.execute("unbind f4").ok &&
			impl.execute("bind f12 \"overlay perf\"").ok, "unbind f4; bind f12");
		const std::string saved = read_text(scratch.root / L"binds.cfg");
		require(saved.substr(saved.find("exec-able.\n") + 11) == "unbind f4\nbind f12 \"overlay perf\"\n",
			"binds.cfg holds exactly the two differences, got '" + saved + "'");
		require(impl.execute("bind f8 echo bound f8").ok && impl.binds().find(KeyChord{ Key::f8 }) != nullptr &&
			*impl.binds().find(KeyChord{ Key::f8 }) == "echo bound f8", "Several words join into one line");

		TestDirector reloaded(tuning, presentation);
		reloaded.impl.set_user_directory(scratch.root);
		reloaded.director.run_launch_arguments({});
		require(reloaded.impl.binds() == impl.binds(), "A new session reads the same table from binds.cfg");
		require(reloaded.impl.execute("binds").ok && any_line(reloaded.impl.log(), "F12       overlay perf  *") &&
			any_line(reloaded.impl.log(), "F4  (unbound; default overlay paint)"), "binds lists it");
		require(reloaded.impl.execute("binds.reset").ok && reloaded.impl.binds() == DebugBinds::defaults() &&
			!std::filesystem::exists(scratch.root / L"binds.cfg"), "binds.reset deletes binds.cfg");

		// binds.cfg is read as a script: '$' and '//' in a bound line survive.
		require(reloaded.impl.execute("bind f5 \"echo cost $1 $$ 5\"").ok &&
			reloaded.impl.execute("bind f6 echo//x").ok, "bind lines with $ and //");
		const std::string escaped = read_text(scratch.root / L"binds.cfg");
		require(escaped.find("bind f5 \"echo cost $$1 $$$$ 5\"\n") != std::string::npos &&
			escaped.find("bind f6 \"echo//x\"\n") != std::string::npos,
			"binds.cfg escapes $ and quotes //, got '" + escaped + "'");
		TestDirector third(tuning, presentation);
		third.impl.set_user_directory(scratch.root);
		third.director.run_launch_arguments({});
		require(third.impl.binds() == reloaded.impl.binds() &&
			*third.impl.binds().find(KeyChord{ Key::f5 }) == "echo cost $1 $$ 5" &&
			*third.impl.binds().find(KeyChord{ Key::f6 }) == "echo//x", "They read back unchanged");

		// A hand-written binds.cfg that starts with binds.reset keeps itself.
		const std::string by_hand = "binds.reset\nbind f12 echo by-hand\n";
		write_text(scratch.root / L"binds.cfg", by_hand);
		TestDirector fourth(tuning, presentation);
		fourth.impl.set_user_directory(scratch.root);
		fourth.director.run_launch_arguments({});
		require(fourth.impl.binds().find(KeyChord{ Key::f12 }) != nullptr &&
			read_text(scratch.root / L"binds.cfg") == by_hand,
			"binds.reset inside binds.cfg does not delete the file being read");

		// One that does not parse is moved aside, not overwritten by the next bind.
		write_text(scratch.root / L"binds.cfg", "bind f12 \"unterminated\n");
		TestDirector fifth(tuning, presentation);
		fifth.impl.set_user_directory(scratch.root);
		fifth.director.run_launch_arguments({});
		require(fifth.impl.binds() == DebugBinds::defaults() &&
			std::filesystem::exists(scratch.root / L"binds.cfg.bad") &&
			!std::filesystem::exists(scratch.root / L"binds.cfg"),
			"An unreadable binds.cfg keeps the defaults and is kept as binds.cfg.bad");
	}

	void check_history(GameTuning& tuning, Presentation& presentation)
	{
		ScratchFolder scratch(L"history");
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		impl.set_user_directory(scratch.root);
		test.director.run_launch_arguments({});
		const auto submit = [&impl](const std::string& line)
			{
				impl.field().set_text(line);
				impl.submit_console_line(impl.field().submit());
			};
		for (int i = 0; i < 70; ++i)
		{
			submit("echo h" + std::to_string(i));
		}
		submit("echo same");
		submit("echo same");
		const std::string text = read_text(scratch.root / L"history.txt");
		require(count_of(text, "\n") == DebugTextField::history_capacity &&
			text.rfind("echo h7\n", 0) == 0 && text.find("echo same\necho same") == std::string::npos,
			"history.txt keeps the last 64 lines and collapses repeats");

		TestDirector next(tuning, presentation);
		next.impl.set_user_directory(scratch.root);
		next.director.run_launch_arguments({});
		require(next.impl.field().history() == impl.field().history(), "The next session reads it back");
		DebugKeyEvents up;
		up.up = true;
		next.impl.field().apply(up);
		require(next.impl.field().text() == "echo same", "Up recalls the last line of the last session");
	}

	void check_live_console_input(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		const auto busy = []
			{
				std::vector<PlayerInputData> inputs(input_slots::count);
				for (PlayerInputData& input : inputs)
				{
					input.connected = true;
					input.x_movement = 1.0f;
					input.jump_pressed = input.jump_held = true;
					input.primary_shoot = true;
					input.drop_through = true;
					input.toggle_debug = true;
					input.toggle_pause_menu = true;
				}
				return inputs;
			};
		std::vector<PlayerInputData> inputs = busy();
		test.director.filter_match_inputs(inputs);
		require(inputs[kbm].x_movement == 1.0f && inputs[kbm].toggle_pause_menu,
			"With no live console the keyboard seat is untouched");

		impl.set_live_console_open(true);
		inputs = busy();
		test.director.filter_match_inputs(inputs);
		const PlayerInputData& keys = inputs[kbm];
		require(keys.connected && keys.x_movement == 0.0f && !keys.jump_pressed && !keys.jump_held &&
			!keys.primary_shoot && !keys.drop_through && !keys.toggle_debug && !keys.toggle_pause_menu,
			"While it is open the keyboard seat is connected and idle");
		require(inputs[0].x_movement == 1.0f && inputs[0].toggle_pause_menu,
			"Pad seats keep their input");

		impl.set_live_console_open(false);
		inputs = busy();
		test.director.filter_match_inputs(inputs);
		require(!inputs[kbm].toggle_pause_menu && inputs[kbm].x_movement == 0.0f,
			"The frame it closes on is blank too (its Escape release is the pause key)");
		inputs = busy();
		test.director.filter_match_inputs(inputs);
		require(inputs[kbm].toggle_pause_menu && inputs[kbm].x_movement == 1.0f, "The next frame is live");

		impl.set_live_console_open(true);
		FakePage page;
		test.director.before_page_update(page, 1.0f / 60.0f);
		require(!impl.live_console_open(), "A page over the match closes the live console");
		require(impl.execute("console.live 1").ok && impl.execute("console.live").ok &&
			any_line(impl.log(), "console.live = on"), "console.live is a variable");
		impl.execute("console.live 0");
	}

	void check_perf_frame_rate()
	{
		DebugPerf perf;
		require(perf.current_frame_ms() == 0.0, "No render interval exists before the first draw");
		perf.begin_draw(1000.0);
		require(perf.current_frame_ms() == 0.0, "The first draw cannot invent a render interval");
		perf.end_draw(1002.0);
		require(perf.current_frame_ms() == 0.0, "A completed priming draw has no current snapshot");
		double begin = 1000.0;
		for (const auto& [fps, updates] : { std::pair{ 30, 0 }, std::pair{ 60, 1 }, std::pair{ 120, 4 } })
		{
			for (int update = 0; update < updates; ++update) { perf.add_update(0.25); }
			const double interval = 1000.0 / fps;
			begin += interval;
			perf.begin_draw(begin);
			const double snapshot = perf.current_frame_ms();
			require(std::abs(snapshot - interval) < 1.0e-9 && perf.current_frame_ms() == snapshot &&
				DebugText::frame_rate_text(snapshot) == L"render fps: " + std::to_wstring(fps) + L".0",
				"Render FPS follows measured draw cadence independently of zero, one or four fixed simulation updates");
			perf.end_draw(begin + 0.5);
			require(perf.current_frame_ms() == 0.0 && perf.at(0).updates == updates,
				"Ending a draw clears its current snapshot without changing the simulation update count");
		}
		const std::size_t samples = perf.size();
		perf.break_interval();
		require(samples == 3 && perf.size() == samples && perf.current_frame_ms() == 0.0,
			"A timing break clears the current FPS without reading stale samples from the retained ring");
		perf.begin_draw(9000.0);
		require(perf.current_frame_ms() == 0.0,
			"The first draw after a timing gap shows unavailable instead of a pre-gap average");
		perf.end_draw(9002.0);
		perf.begin_draw(9016.0);
		require(perf.current_frame_ms() == 16.0, "A second post-gap draw restores its fresh interval");
		perf.begin_draw(8999.0);
		require(perf.current_frame_ms() == 0.0, "A backwards timestamp invalidates the current FPS snapshot");
		perf.begin_draw(10000.0);
		perf.end_draw(10001.0);
		perf.begin_draw(10016.0);
		perf.end_draw((std::numeric_limits<double>::quiet_NaN)());
		require(perf.current_frame_ms() == 0.0, "An invalid draw end clears its FPS snapshot");
		perf.reset();
		require(perf.current_frame_ms() == 0.0, "Reset leaves no current FPS snapshot");

		for (const double invalid : { 0.0, -1.0, (std::numeric_limits<double>::quiet_NaN)(),
			(std::numeric_limits<double>::infinity)(), -(std::numeric_limits<double>::infinity)(),
			(std::numeric_limits<double>::denorm_min)() })
		{
			require(DebugText::frame_rate_text(invalid) == L"render fps: n/a",
				"Missing, invalid or reciprocally overflowing timing never formats a false FPS");
		}
		require(DebugText::frame_rate_text(1000.0 / 30.0) == L"render fps: 30.0" &&
			DebugText::frame_rate_text(1000.0 / 60.0) == L"render fps: 60.0" &&
			DebugText::frame_rate_text(1000.0 / 120.0) == L"render fps: 120.0" &&
			DebugText::frame_rate_text((std::numeric_limits<double>::max)()) == L"render fps: 0.0",
			"The readout formats measured FPS, including an extreme finite interval, to one decimal place");
	}

	void check_perf()
	{
		check_perf_frame_rate();
		DebugPerf perf;
		require(perf.size() == 0 && perf.frame().max == 0.0, "Empty");
		for (int i = 0; i < 300; ++i)
		{
			const double begin = 16.0 * i;
			perf.add_update(0.5);
			perf.add_update(i % 2 == 0 ? 0.25 : 0.75);
			perf.begin_draw(begin);
			perf.end_draw(begin + 2.0 + (i % 3));
		}
		require(perf.size() == DebugPerf::capacity, "The ring keeps 240 frames");
		require(perf.at(0).draw_ms == 2.0 + (299 % 3) && perf.at(1).draw_ms == 2.0 + (298 % 3),
			"at(0) is the newest");
		require(perf.frame().min == 16.0 && perf.frame().max == 16.0 && perf.frame().avg == 16.0,
			"frame_ms is the interval between draws");
		require(perf.draw().min == 2.0 && perf.draw().max == 4.0 && perf.draw().avg == 3.0,
			"draw min / avg / max are exact");
		require(perf.update().min == 0.75 && perf.update().max == 1.25 && perf.update().avg == 1.0 &&
			perf.at(0).updates == 2, "update sums each frame's updates");
		perf.set_counts(182, 12);
		const std::vector<std::string> lines = perf.lines();
		require(lines.size() == 5 && lines[0] == "CPU ms    min    avg    max  (240 frames)" &&
			lines[1] == "frame   16.00  16.00  16.00" &&
			lines[2] == "update   0.75   1.00   1.25  x2.00 a frame" &&
			lines[3] == "draw     2.00   3.00   4.00" && lines[4] == "objects 182  projectiles 12",
			"perf lines, got '" + lines[2] + "'");
		perf.end_draw(9999.0);
		require(perf.size() == DebugPerf::capacity && perf.at(0).draw_ms == 2.0 + (299 % 3),
			"An end without a begin (a screenshot's redraw) adds nothing");
		perf.reset();
		require(perf.size() == 0, "reset");

		// The first draw after a reset has no interval: it commits nothing, so
		// min is never a fake 0 straight after perf.reset.
		perf.add_update(9.0);
		perf.begin_draw(1000.0);
		perf.end_draw(1001.0);
		require(perf.size() == 0, "The first draw after reset only starts an interval");
		perf.add_update(0.5);
		perf.begin_draw(1016.0);
		perf.end_draw(1018.0);
		require(perf.size() == 1 && perf.frame().min == 16.0 && perf.frame().max == 16.0 &&
			perf.update().max == 0.5 && perf.at(0).updates == 1,
			"The second draw is the first frame, without the first's updates");

		// A gap (a build, the menus, a page over the match, a capture): the
		// next draw starts again instead of recording the gap as one frame.
		perf.add_update(7.0);
		perf.break_interval();
		perf.begin_draw(9000.0);
		perf.end_draw(9001.0);
		perf.begin_draw(9016.0);
		perf.end_draw(9018.0);
		require(perf.size() == 2 && perf.frame().max == 16.0 && perf.update().max == 0.5,
			"break_interval: the gap and the updates before it are not a frame");

		// A capture keeps its whole chronological recording independently of
		// the overlay ring. Starting one must not include the previous gap.
		require(!perf.start_capture(0) && !perf.start_capture(36001) &&
			!perf.capture_active() && !perf.capture_ready() && perf.size() == 2,
			"Capture bounds reject zero and more than 36000 without disturbing the ring");
		perf.add_update(99.0);
		require(perf.start_capture(300) && perf.capture_active() && !perf.capture_ready() &&
			!perf.capture_complete() && perf.capture_target() == 300 &&
			perf.captured_frames().empty() && perf.capture_reason().empty(),
			"A bounded capture starts empty and active");
		require(!perf.start_capture(1) && perf.capture_target() == 300,
			"An active recording cannot be replaced");
		perf.end_draw(99999.0);
		require(perf.capture_active() && perf.captured_frames().empty(),
			"An unmatched draw end does not consume or interrupt the capture");
		perf.add_update(88.0);
		perf.begin_draw(10000.0);
		perf.end_draw(10002.0);
		require(perf.size() == 2 && perf.captured_frames().empty() && perf.capture_active(),
			"The first capture draw only primes its interval and discards earlier updates");
		for (int i = 1; i <= 300; ++i)
		{
			const int updates = i % 3;
			if (updates > 0) { perf.add_update(0.25); }
			if (updates > 1) { perf.add_update(0.5); }
			perf.set_counts(100 + i, i % 7);
			const double begin = 10000.0 + 17.0 * i;
			perf.begin_draw(begin);
			perf.end_draw(begin + 1.0 + (i % 3));
			require(perf.captured_frames().size() == static_cast<std::size_t>(i) &&
				perf.capture_active() == (i < 300),
				"Capture counts actual rendered frames, including zero-update and catch-up frames");
		}
		require(!perf.capture_active() && perf.capture_ready() && perf.capture_complete() &&
			perf.capture_reason() == "complete" && perf.size() == DebugPerf::capacity,
			"The exact target completes while the overlay ring remains bounded");
		for (int i = 1; i <= 300; ++i)
		{
			const DebugPerf::Frame& frame = perf.captured_frames()[static_cast<std::size_t>(i - 1)];
			const int updates = i % 3;
			const double update_ms = updates == 0 ? 0.0 : updates == 1 ? 0.25 : 0.75;
			require(frame.frame_ms == 17.0 && frame.draw_ms == 1.0 + (i % 3) &&
				frame.updates == updates && frame.update_ms == update_ms &&
				frame.objects == 100 + i && frame.projectiles == i % 7,
				"Capture retains each interval, CPU sum and population snapshot in chronological order");
		}
		require(perf.at(0).objects == 400 && perf.at(DebugPerf::capacity - 1).objects == 161 &&
			!perf.start_capture(1), "The ring retains its newest rows and a completed capture awaits disposal");
		perf.set_counts(999, 999);
		perf.begin_draw(15117.0);
		perf.end_draw(15119.0);
		perf.break_interval();
		require(perf.captured_frames().size() == 300 && perf.captured_frames().back().objects == 400 &&
			perf.capture_complete() && perf.capture_reason() == "complete" && perf.at(0).objects == 999,
			"Later frames and gaps cannot overrun or change a completed recording");
		perf.discard_capture();
		require(!perf.capture_active() && !perf.capture_ready() && !perf.capture_complete() &&
			perf.capture_target() == 0 && perf.capture_reason().empty() &&
			perf.captured_frames().empty() && perf.size() == DebugPerf::capacity,
			"Discard clears only capture state and preserves the overlay history");

		perf.reset();
		require(perf.start_capture(4), "Start an interruptible capture");
		perf.begin_draw(100.0);
		perf.end_draw(102.0);
		perf.add_update(0.5);
		perf.begin_draw(116.0);
		perf.end_draw(118.0);
		perf.add_update(7.0);
		perf.break_interval();
		require(!perf.capture_active() && perf.capture_ready() && !perf.capture_complete() &&
			perf.capture_target() == 4 && perf.capture_reason() == "interrupted" &&
			perf.captured_frames().size() == 1 && perf.size() == 1 && !perf.start_capture(1),
			"A gap retains an explicitly incomplete recording and refuses replacement");
		perf.begin_draw(9000.0);
		perf.end_draw(9001.0);
		perf.add_update(0.25);
		perf.begin_draw(9016.0);
		perf.end_draw(9018.0);
		require(perf.size() == 2 && perf.frame().max == 16.0 && perf.at(0).update_ms == 0.25 &&
			perf.captured_frames().size() == 1 && perf.capture_reason() == "interrupted",
			"The ring re-primes after a gap but an interrupted capture never silently resumes");
		perf.discard_capture();
		require(perf.start_capture(1), "A discarded recording permits a new one-frame capture");
		perf.begin_draw(10000.0);
		perf.end_draw(10001.0);
		require(perf.capture_active() && perf.captured_frames().empty(),
			"A fresh capture primes again even when the ring already has a previous frame");
		const double nan = (std::numeric_limits<double>::quiet_NaN)();
		const double infinity = (std::numeric_limits<double>::infinity)();
		for (const double invalid : { -1.0, nan, infinity, -infinity }) { perf.add_update(invalid); }
		perf.add_update(0.0);
		perf.begin_draw(10016.0);
		perf.end_draw(10016.0);
		require(perf.capture_complete() && perf.captured_frames().size() == 1 &&
			perf.captured_frames()[0].updates == 1 && perf.captured_frames()[0].update_ms == 0.0 &&
			perf.captured_frames()[0].draw_ms == 0.0,
			"Invalid update durations are ignored; genuine zero-duration updates and draws remain valid");

		// Invalid timestamps must produce an interrupted partial capture,
		// rather than a negative/non-finite interval or a poisoned ring.
		for (const bool invalid_end : { false, true })
		{
			for (const double invalid : { -1.0, nan, infinity, -infinity, 115.0, 116.0 })
			{
				perf.reset();
				require(perf.start_capture(3), "Start a capture for invalid-clock coverage");
				perf.begin_draw(100.0);
				perf.end_draw(102.0);
				perf.add_update(0.5);
				perf.begin_draw(116.0);
				perf.end_draw(118.0);
				perf.add_update(9.0);
				if (invalid_end)
				{
					perf.begin_draw(132.0);
					perf.end_draw(invalid);
				}
				else
				{
					perf.begin_draw(invalid);
					perf.end_draw(134.0);
				}
				require(perf.capture_ready() && !perf.capture_complete() &&
					perf.capture_reason() == "interrupted" && perf.captured_frames().size() == 1 &&
					perf.size() == 1 && perf.at(0).frame_ms == 16.0 && perf.at(0).update_ms == 0.5,
					"Negative, non-finite and backwards timestamps retain only valid rows");
				perf.begin_draw(9000.0);
				perf.end_draw(9002.0);
				perf.begin_draw(9016.0);
				perf.end_draw(9018.0);
				require(perf.size() == 2 && perf.at(0).frame_ms == 16.0 &&
					perf.at(0).updates == 0 && perf.captured_frames().size() == 1,
					"Invalid-clock recovery re-primes and drops pending updates without reviving the capture");
			}
		}
		perf.reset();
		require(!perf.capture_active() && !perf.capture_ready() && perf.capture_target() == 0 &&
			perf.captured_frames().empty() && perf.capture_reason().empty() && perf.size() == 0,
			"Reset clears interrupted captures and all overlay samples");
		require(perf.start_capture(36000), "The documented upper capture bound is accepted");
		perf.reset();
		require(!perf.capture_active() && !perf.capture_ready() && perf.capture_target() == 0 &&
			perf.captured_frames().empty() && perf.size() == 0,
			"Reset also cancels an active capture");
	}

	void check_polish(GameTuning& tuning, Presentation& presentation)
	{
		using mattmath::RectangleF;
		using mattmath::Vector2F;
		// The end page offers the next playable action for each kind of finish.
		LevelEndInfo win;
		win.team_a_area = 1.0f;
		MenuEndText custom = presentation.menu_text().end;
		custom.rematch = L"Single {again}";
		custom.play_series_again = L"Series {again}";
		custom.replay_draw = L"Draw {again}";
		custom.next_round = L"Round {next}";
		const auto check_continuation = [&](const Session& session, const wchar_t* expected)
		{
			const bool series = session.is_series(), complete = session.complete();
			const int round = session.round(), a_wins = session.team_a_wins(), b_wins = session.team_b_wins();
			const std::wstring score = session.score_string(), result = session.result_string();
			std::wstring selected = session.continue_string(custom);
			require(selected == expected, "Session selects the supplied continuation without formatting literal braces");
			selected.clear();
			require(session.continue_string(custom) == expected && session.is_series() == series &&
				session.complete() == complete && session.round() == round && session.team_a_wins() == a_wins &&
				session.team_b_wins() == b_wins && session.score_string() == score && session.result_string() == result,
				"Continuation selection returns an owned copy and leaves the Session ledger unchanged");
		};
		for (const bool solo : { false, true })
		{
			Session single;
			LevelEndInfo finish = win;
			finish.team_b_played = !solo;
			single.finish_round(finish);
			require(single.complete() && single.continue_string(presentation.menu_text().end) == L"Rematch",
				"A finished single round or solo practice offers Rematch");
			check_continuation(single, L"Single {again}");
		}
		for (const int length : { 3, 5 })
		{
			Session series(length);
			series.finish_round(LevelEndInfo{});
			require(!series.complete() && series.continue_string(presentation.menu_text().end) == L"Replay Draw",
				"An unresolved series draw offers Replay Draw");
			check_continuation(series, L"Draw {again}");
			series.next_round();
			for (int wins = 1; wins <= length / 2 + 1; ++wins)
			{
				series.finish_round(win);
				if (series.complete())
				{
					require(series.continue_string(presentation.menu_text().end) == L"Play Series Again",
						"A completed best-of series offers Play Series Again");
					check_continuation(series, L"Series {again}");
				}
				else
				{
					require(series.continue_string(presentation.menu_text().end) == L"Next Round",
						"A decisive unfinished series offers Next Round");
					check_continuation(series, L"Round {next}");
					series.next_round();
				}
			}
		}
		require(custom.rematch == L"Single {again}" && custom.play_series_again == L"Series {again}" &&
			custom.replay_draw == L"Draw {again}" && custom.next_round == L"Round {next}",
			"Session selection leaves supplied continuation text unchanged");
		// The strip at 1280x720 with a KEYBOARD + MOUSE card at the bottom
		// right and the paint panel bottom left: lifted clear of both.
		const Vector2F screen(1280.0f, 720.0f);
		const RectangleF card(760.0f, 560.0f, 420.0f, 120.0f);
		const RectangleF paint = DebugConsoleView::place_panel(Vector2F(700.0f, 40.0f),
			DebugConsoleView::Anchor::bottom_left, screen, {});
		const RectangleF strip = DebugConsoleView::place_panel(Vector2F(600.0f, 20.0f),
			DebugConsoleView::Anchor::bottom_right, screen, { card, paint });
		require(!strip.intersects(card) && !strip.intersects(paint) && strip.top() >= 0.0f,
			"The strip is lifted above the card and the paint panel");
		const RectangleF free = DebugConsoleView::place_panel(Vector2F(200.0f, 20.0f),
			DebugConsoleView::Anchor::bottom_right, screen, { card });
		require(free.bottom() >= 700.0f || !free.intersects(card), "A panel that fits stays down");

		// A staged rebuild says it waits for the menu.
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		FakeMatch match(test.director);
		match.settings = planned(4);
		test.director.attach_match(match);
		DebugMenuModel model(impl);
		model.confirm(row_index(model.rows(), "match"));
		model.confirm(row_index(model.rows(), "players"));
		model.confirm(row_index(model.rows(), "3"));
		require(log_contains(impl.log(), "players 3: queued; rebuilds when the debug menu closes",
			DebugSeverity::info) && match.rebuilds == 0, "A staged rebuild says when it happens");
		require(impl.execute("players 2").ok &&
			log_contains(impl.log(), "players 2: rebuilding the match", DebugSeverity::info),
			"Typed with no menu open, it rebuilds");
		impl.drop_request("test");

		// Bot seats are labelled in the HUD; the keyboard seat keeps its device.
		require(impl.execute("players 4 paint").ok && test.director.before_match_update(match, 1.0f / 60.0f),
			"players 4 paint rebuilds");
		require(match.seat_labels.size() == 4 && match.seat_labels[0].empty() &&
			match.seat_labels[1] == L"Bot: paint" && match.seat_labels[3] == L"Bot: paint",
			"Dummies driven by bots say Bot: paint");
		require(impl.execute("bot 1 jump").ok && match.seat_labels[0] == L"Bot: jump",
			"bot pushes the labels at once");
		require(impl.execute("bot off").ok && match.seat_labels.empty(), "bot off clears them");
		test.director.detach_match(match);

		// Old empty tune folders are found; anything else is left alone.
		ScratchFolder temp(L"tune_temp");
		std::filesystem::create_directories(temp.root / L"ColourWars-tune-999999");
		std::filesystem::create_directories(temp.root / L"ColourWars-tune-999998");
		write_text(temp.root / L"ColourWars-tune-999998" / L"keep.txt", "keep");
		std::filesystem::create_directories(temp.root / L"ColourWars-tune-abc");
		write_text(temp.root / L"ColourWars-tune-1-tuning.json", "{}");
		const std::vector<std::filesystem::path> stale = stale_tune_directories(temp.root);
		require(stale.size() == 1 && stale[0].filename() == L"ColourWars-tune-999999",
			"Only the empty ColourWars-tune-<digits> folder is stale");

		// Grammar and availability fail before any device or output is needed.
		// A real capture/barrier uses the hardware probe's Application loop.
		TestDirector perf_test(tuning, presentation);
		DebugDirectorImpl& perf_impl = perf_test.impl;
		for (const std::string& command : { "perf.capture", "perf.capture 1",
			"perf.capture 1 label extra", "perf.capture 0 label", "perf.capture -1 label",
			"perf.capture 36001 label", "perf.capture 2147483648 label", "perf.capture abc label",
			"perf.capture 1.5 label", "perf.capture 1e2 label", "perf.capture nan label",
			"perf.capture 1 \"\"", "perf.capture 1 ../escape", "perf.capture 1 sub/dir",
			"perf.capture 1 sub\\dir", "perf.capture 1 C:escape", "perf.capture 1 \"two words\"" })
		{
			const DebugStatus result = perf_impl.execute(command);
			require(!result.ok && result.message.find("needs a focused live match") == std::string::npos &&
				!perf_impl.perf().capture_active() &&
				!perf_impl.perf().capture_ready(), "Invalid capture grammar cannot arm a recording: " + command);
		}
		const DebugStatus long_label = perf_impl.execute("perf.capture 1 " + std::string(65, 'a'));
		require(!long_label.ok && long_label.message.find("label must use 1-64") != std::string::npos,
			"Capture labels have a bounded safe filename");
		for (const std::string& command : { std::string("perf.capture 1 valid-A_9"),
			"perf.capture 36000 " + std::string(64, 'a') })
		{
			const DebugStatus result = perf_impl.execute(command);
			require(!result.ok && result.message.find("needs a focused live match") != std::string::npos &&
				!perf_impl.perf().capture_active() && !perf_impl.perf().capture_ready(),
				"Valid capture arguments still require a live focused Application match");
		}
	}
}
#endif
