#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_chord.h"
#include "game/debug/debug_menu_model.h"
#include "game/debug/debug_registry.h"

// The debug menu: the pad chord and its input blanking, the registry's menu
// traits and the menu model.
namespace debug_console_acceptance
{
	void check_chord()
	{
		// 0.125 s frames are exact in binary: the fourth fires.
		DebugChord chord;
		DebugChord::Pads pads{};
		pads[0].connected = true;
		pads[1].connected = true;
		const float step = 0.125f;

		pads[0].back = true;
		require(!chord.update(pads, step) && !chord.suppressing(0), "Back alone starts no chord");
		pads[0].back = false;
		pads[0].start = true;
		require(!chord.update(pads, step) && !chord.suppressing(0), "Start alone starts no chord");

		pads[0].back = true;
		std::vector<bool> fired;
		for (int frame = 0; frame < 6; ++frame)
		{
			fired.push_back(chord.update(pads, step));
			require(chord.suppressing(0), "A held chord suppresses its pad");
		}
		require(fired == std::vector<bool>{ false, false, false, true, false, false },
			"The chord fires once, on the frame the hold reaches 0.5 s");
		require(!chord.suppressing(1) && chord.any_suppressing(), "Pads are independent");

		pads[0].back = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"Releasing Back first: Start's release is still the chord's");
		pads[0].start = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"The frame both are up is still suppressed");
		require(!chord.update(pads, step) && !chord.suppressing(0) && !chord.any_suppressing(),
			"The frame after, the pad is the game's again");

		// Aborted: released early, never fires, both edges still eaten.
		pads[0].back = pads[0].start = true;
		require(!chord.update(pads, step) && !chord.update(pads, step), "Two frames do not fire");
		pads[0].back = pads[0].start = false;
		require(!chord.update(pads, step) && chord.suppressing(0),
			"An aborted chord still suppresses its release frame");
		require(!chord.update(pads, step) && !chord.suppressing(0), "Then it is over");

		// A disconnect ends the session.
		pads[0].back = pads[0].start = true;
		chord.update(pads, step);
		pads[0].connected = false;
		chord.update(pads, step);
		pads[0].connected = true;
		pads[0].back = pads[0].start = false;
		require(!chord.update(pads, step) && !chord.suppressing(0), "A disconnect ends the chord");

		// At 60 Hz the thirtieth frame fires, whatever single-precision sums do.
		chord.reset();
		pads[0].back = pads[0].start = true;
		int fired_on = 0;
		for (int frame = 1; frame <= 40; ++frame)
		{
			if (chord.update(pads, 1.0f / 60.0f)) { fired_on = frame; }
		}
		require(fired_on == 30, "At 60 Hz the chord fires on frame 30, got " +
			std::to_string(fired_on));
	}

	void check_input_filter(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugChord::Pads pads{};
		pads[0].connected = true;
		pads[0].back = pads[0].start = true;
		test.impl.chord().update(pads, 1.0f / 60.0f);

		std::vector<SlotInput> inputs(input_slots::count);
		for (SlotInput& input : inputs)
		{
			input.toggle_debug = true;
			input.toggle_pause_menu = true;
		}
		test.director.filter_match_inputs(inputs);
		require(!inputs[0].toggle_debug && !inputs[0].toggle_pause_menu,
			"The chord's pad loses its Back and Start edges");
		require(inputs[1].toggle_debug && inputs[1].toggle_pause_menu,
			"Another pad keeps its edges");
		const std::size_t kbm = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
		require(inputs[kbm].toggle_debug && inputs[kbm].toggle_pause_menu,
			"The keyboard keeps F1 and Escape");
	}

	void check_registry_menu_traits(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		const DebugRegistry& registry = test.impl.registry();
		require(registry.categories() == std::vector<std::string>{
			"match", "player", "view", "time", "tuning", "replay", "content", "console" },
			"Categories come in the fixed order");
		for (const char* name : { "stage", "players", "weapon" })
		{
			require(registry.find(name)->menu().staged, std::string(name) + " is staged");
		}
		for (const char* name : { "restart", "goto", "god", "timescale", "screenshot" })
		{
			require(!registry.find(name)->menu().staged, std::string(name) + " is not staged");
		}
		for (const char* name : { "set", "get", "list", "help", "clear", "logfile" })
		{
			require(registry.find(name)->menu().form == DebugMenuTraits::Form::hidden,
				std::string(name) + " is console-only");
		}
		using Form = DebugMenuTraits::Form;
		const DebugMenuTraits& weapon = registry.find("weapon")->menu();
		require(weapon.form == Form::pick && weapon.pick_argument == 1 && weapon.prefix == "all",
			"weapon picks its second argument, for every seat");
		require(registry.find("stage")->menu().form == Form::pick &&
			registry.find("goto")->menu().form == Form::pick &&
			registry.find("overlay")->menu().form == Form::pick,
			"stage, goto and overlay are pickers");
		require(registry.find("restart")->menu().form == Form::run &&
			registry.find("respawn")->menu().form == Form::run,
			"restart and respawn run");
		require(registry.find("timescale")->menu().form == Form::variable &&
			registry.find("overlay.collision")->menu().category == "view",
			"Variables are variable rows in their category");

		DebugDirectorImpl& impl = test.impl;
		require(impl.execute("timescale next").ok && impl.clock().timescale() == 2.0f,
			"timescale next from 1 is 2");
		impl.execute("timescale 0.25");
		require(impl.execute("timescale prev").ok && impl.clock().timescale() == 0.25f,
			"timescale prev stops at 0.25");
		require(impl.execute("timescale 0.3").ok && impl.clock().timescale() == 0.3f,
			"Any timescale in range is accepted");
		require(!impl.execute("timescale 9").ok && impl.clock().timescale() == 0.3f &&
			log_contains(impl.log(), "timescale: expected 0.25-4, got '9'", DebugSeverity::error),
			"An out-of-range timescale fails with the range");
		require(impl.execute("timescale").ok && log_contains(impl.log(), "timescale = 0.3",
			DebugSeverity::info), "timescale alone prints it");

		require(impl.registry().variable("god")->step_word(1) == "toggle",
			"A bool steps by toggling");
		require(impl.execute("ammo next").ok && impl.registry().variable("ammo")->value_text() == "inf" &&
			impl.execute("ammo next").ok && impl.registry().variable("ammo")->value_text() == "normal",
			"A choice steps and wraps");
	}

	void check_menu_model(GameTuning& tuning, Presentation& presentation)
	{
		TestDirector test(tuning, presentation);
		DebugDirectorImpl& impl = test.impl;
		FakeMatch match(test.director);
		match.settings = planned(1);
		test.director.attach_match(match);
		const float dt = 1.0f / 60.0f;

		DebugMenuModel model(impl);
		require(labels(model.rows()) == std::vector<std::string>{
			"match", "player", "view", "time", "tuning", "replay", "content", "console" },
			"With nothing queued the root lists the categories");

		model.confirm(row_index(model.rows(), "match"));
		require(model.title() == "DEBUG / match", "The title names the category");
		require(labels(model.rows()) == std::vector<std::string>{
			"intro", "menu", "players", "restart", "stage", "weapon" },
			"A category lists its entries by name");
		model.confirm(row_index(model.rows(), "stage"));
		require(labels(model.rows()) == std::vector<std::string>{
			"movement_course", "x", "next", "prev", "random" }, "stage opens a picker");
		require(model.confirm(row_index(model.rows(), "x")) == DebugMenuModel::Result::stay &&
			impl.request().stage == "x" && match.rebuilds == 0,
			"A staged choice queues without rebuilding and stays open");
		require(log_contains(impl.log(), "[menu] stage x", DebugSeverity::echo),
			"The menu's statements are echoed");

		model.confirm(row_index(model.rows(), "weapon"));
		require(labels(model.rows()).size() == MatchPlanner::weapon_names().size(),
			"weapon picks among the weapon names");
		model.confirm(row_index(model.rows(), "roller"));
		require(impl.request().weapons.size() == 1 && impl.request().weapons[0].seat == -1 &&
			impl.request().weapons[0].weapon == "roller", "The picker builds weapon all roller");

		require(model.back() == DebugMenuModel::Result::stay, "Back from a category stays open");
		const std::vector<DebugMenuRow> root = model.rows();
		require(root[0].kind == DebugMenuRow::Kind::apply && root[0].enabled &&
			root[0].label == "Apply: stage x, weapon all roller",
			"Apply names what is queued, got '" + root[0].label + "'");
		require(root[1].kind == DebugMenuRow::Kind::discard, "Discard follows Apply");
		require(model.focus_hint() == 0, "Back after staging lands on Apply");
		require(model.confirm(0) == DebugMenuModel::Result::close, "Apply closes the menu");
		require(test.director.before_match_update(match, dt) && match.rebuilds == 1 &&
			match.settings.stage() == "x" && match.settings.player_weapon(0) == WeaponType::roller,
			"What was staged applies once, through the normal path");

		impl.execute("stage x");
		require(model.confirm(row_index(model.rows(), "Discard queued")) ==
			DebugMenuModel::Result::stay && impl.request().empty() &&
			log_contains(impl.log(), "discarded in the debug menu", DebugSeverity::warning),
			"Discard drops the request and says so");

		model.confirm(row_index(model.rows(), "time"));
		const std::size_t timescale = row_index(model.rows(), "timescale");
		require(model.rows()[timescale].value == "1", "A variable row shows its value");
		require(model.step(timescale, 1) == DebugMenuModel::Result::stay &&
			impl.clock().timescale() == 2.0f, "Right steps a number up");
		require(model.step(timescale, -1) == DebugMenuModel::Result::stay &&
			impl.clock().timescale() == 1.0f, "Left steps it down");
		require(model.confirm(row_index(model.rows(), "step")) == DebugMenuModel::Result::close,
			"A command that asks the console to close closes the menu");
		impl.execute("pause off");

		model.back();
		model.confirm(row_index(model.rows(), "view"));
		model.step(row_index(model.rows(), "overlay.collision"), 1);
		require(match.overlays.collision && !match.overlays.camera,
			"Stepping an overlay variable applies to the match at once");
		model.confirm(row_index(model.rows(), "overlay.collision"));
		require(!match.overlays.collision, "Confirm on a bool steps it too");
		require(model.back() == DebugMenuModel::Result::stay &&
			model.back() == DebugMenuModel::Result::close, "Back at the top closes");
		test.director.detach_match(match);
	}
}
#endif
