#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/input/debug_binds.h"
#include "game/debug/console/debug_console_view.h"
#include "game/debug/input/debug_keys.h"
#include "game/debug/console/debug_menu_model.h"
#include "game/debug/capture/debug_perf.h"
#include "game/debug/console/debug_text_field.h"
#include "game/objects/debug_text.h"
#include <cmath>
#include <limits>
#include <optional>

// Binds, history, live-console input and polish.
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
		// help names the binds in force, not the defaults.
		require(impl.execute("help").ok, "help runs");
		const std::string keys = impl.log().lines().back().text;
		require(keys.starts_with("keys: ") && keys.find("F8 echo bound f8") != std::string::npos &&
			keys.find("F12 overlay perf") != std::string::npos &&
			keys.find("F2 overlay collision") != std::string::npos &&
			keys.find("F4 ") == std::string::npos,
			"help lists the binds as they are now, got '" + keys + "'");

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

		// Any other line is skipped, and the warning names all three it reads.
		write_text(scratch.root / L"binds.cfg", "echo stray\nbind f12 echo by-hand\n");
		TestDirector stray(tuning, presentation);
		stray.impl.set_user_directory(scratch.root);
		stray.director.run_launch_arguments({});
		require(log_contains(stray.impl.log(), "only bind, unbind and binds.reset lines are read",
			DebugSeverity::warning) && stray.impl.binds().find(KeyChord{ Key::f12 }) != nullptr,
			"A stray line in binds.cfg is skipped with a warning naming binds.reset");

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
				std::vector<SlotInput> inputs(input_slots::count);
				for (SlotInput& input : inputs)
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
		std::vector<SlotInput> inputs = busy();
		test.director.filter_match_inputs(inputs);
		require(inputs[kbm].x_movement == 1.0f && inputs[kbm].toggle_pause_menu,
			"With no live console the keyboard seat is untouched");

		impl.set_live_console_open(true);
		inputs = busy();
		test.director.filter_match_inputs(inputs);
		const SlotInput& keys = inputs[kbm];
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

	void check_polish(GameTuning& tuning, Presentation& presentation)
	{
		using labrador::RectangleF;
		using labrador::Vector2F;
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
		// The one debug stem rule (debug_file_name::valid_stem): a device name
		// is refused, as exec and the replay and screenshot names treat it.
		for (const std::string& command : { "perf.capture 1 CON", "perf.capture 1 nul",
			"perf.capture 1 Com1", "perf.capture 1 lpt9" })
		{
			const DebugStatus result = perf_impl.execute(command);
			require(!result.ok && result.message.find("reserved Windows name") != std::string::npos &&
				!perf_impl.perf().capture_active() && !perf_impl.perf().capture_ready(),
				"A reserved Windows name is refused as a capture label: " + command);
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
