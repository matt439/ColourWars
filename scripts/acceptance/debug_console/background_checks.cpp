#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_background.h"
#include "scripts/acceptance/debug_console/cpu_level.h"
#include <array>
#include <iostream>
#include <tuple>

namespace debug_console_acceptance
{
	namespace
	{
		DebugBackgroundState facts(unsigned int bits)
		{
			return { (bits & 1u) != 0, (bits & 2u) != 0, (bits & 4u) != 0 };
		}

		// Background policy can notify the target, but must not run gameplay,
		// consume pending clock steps or change unrelated queued developer work.
		auto unrelated_state(const TestDirector& test, const FakeMatch& match)
		{
			return std::tuple{
				std::array<int, 16>{ match.updates, match.rebuilds, match.adopts, match.leaves,
					match.audio_syncs, match.overlay_sets, match.player_text_sets, match.cheat_sets,
					match.filtered_updates, match.seat_label_sets, static_cast<int>(match.placed.size()),
					static_cast<int>(match.respawned.size()), static_cast<int>(match.killed.size()),
					test.impl.level_updates(), test.impl.clock().pending_steps(), match.settings.player_count() },
				match.damage_immune, match.infinite_ammo, match.last_filter,
				match.settings.stage(), test.impl.request().describe(),
				test.impl.clock().timescale(), test.impl.clock().paused(),
				test.impl.recording(), test.impl.playing() };
		}

		struct DetachMatch
		{
			DebugDirector& director;
			const FakeMatch& match;
			~DetachMatch() { this->director.detach_match(this->match); }
		};

		auto input_state(const PlayerInputData& input)
		{
			return std::tuple{ input.x_movement, input.jump_pressed, input.jump_held, input.shoot_angle,
				input.shoot_direction.x, input.shoot_direction.y, input.shoot_direction_requested,
				input.primary_shoot, input.drop_through, input.toggle_debug, input.toggle_pause_menu,
				input.pause_held, input.skip_intro, input.connected, input.left_analog_stick.x,
				input.left_analog_stick.y, input.right_analog_stick.x, input.right_analog_stick.y };
		}

		void require_neutral_seats(const std::vector<PlayerInputData>& seats)
		{
			require(seats.size() == 2, "The background input fixture must retain two by-view seats");
			for (std::size_t seat = 0; seat < seats.size(); ++seat)
			{
				PlayerInputData expected;
				expected.connected = seat == 0;
				require(input_state(seats[seat]) == input_state(expected),
					"Background input retained a physical field or changed truthful device presence");
			}
		}

		class BackgroundDelegateSpy final : public SeatInputFilter
		{
		public:
			explicit BackgroundDelegateSpy(const Level& expected) : expected_(expected) {}
			int calls = 0;
			void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
			{
				require(&level == &this->expected_, "The background wrapper changed the real Level argument");
				require_neutral_seats(seats);
				++this->calls;
				seats[0].x_movement = -0.75f;
				seats[0].primary_shoot = true;
				seats[0].shoot_direction = { -0.6f, 0.8f };
				seats[0].shoot_direction_requested = true;
				seats[1].jump_pressed = true;
			}
		private:
			const Level& expected_;
		};

		void check_post_mouse_input(const std::filesystem::path& content, GameTuning& tuning,
			Presentation& presentation)
		{
			// A real CPU Level supplies the filter's reference; no device, frame,
			// fake Level cast or production-only test accessor is needed.
			CpuWorld world(content, tuning, presentation);
			MatchRequest request;
			request.stage = "movement_course";
			request.players = 2;
			const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, request,
				MatchPlanner::ConnectedPads{}, [] { return 7u; });
			require(plan.ok, "The background input CPU roster must resolve");
			world.viewports.set_layout(plan.settings.screen_layout());
			const auto level = world.builder->build_level(plan.settings, TeamColourPair::blue_orange, OpeningMode::off);
			PlayerInputData physical;
			physical.x_movement = 0.75f;
			physical.jump_pressed = physical.jump_held = physical.shoot_direction_requested = true;
			physical.shoot_angle = 1.25f;
			physical.shoot_direction = { 0.6f, -0.8f };
			physical.primary_shoot = physical.drop_through = physical.toggle_debug = true;
			physical.toggle_pause_menu = physical.pause_held = physical.skip_intro = true;
			physical.left_analog_stick = { 0.25f, -0.5f };
			physical.right_analog_stick = { -0.5f, 0.75f };
			std::vector<PlayerInputData> adversarial(2, physical);
			adversarial[0].connected = true;
			adversarial[1].connected = false;

			TestDirector test(tuning, presentation);
			FakeMatch match(test.director);
			match.settings = planned(2);
			test.director.attach_match(match);
			const DetachMatch detach{ test.director, match };
			const float dt = 1.0f / 60.0f;
			const std::vector<PlayerInputData> slots(input_slots::count);
			test.director.update_match(match, slots, dt);
			require(match.last_filter == nullptr, "Default-off active input must retain its original filter path");
			test.impl.set_background_probe([] { return DebugBackgroundState{ true, false, false }; });
			require(test.impl.execute("script.background 1").ok &&
				!test.director.before_match_update(match, dt, false), "The physical-input fixture opens its inactive gate");
			test.director.update_match(match, slots, dt);
			auto* delivered = dynamic_cast<DebugBackgroundInput*>(match.last_filter);
			require(delivered != nullptr, "The actual inactive director update did not deliver the background wrapper");
			auto seats = adversarial;
			delivered->filter_seats(seats, *level);
			require_neutral_seats(seats);
			require(!test.director.before_match_update(match, dt, true), "The foreground input hook must resume normally");
			test.director.update_match(match, slots, dt);
			require(match.last_filter == nullptr, "Opted-in foreground input must retain the original unwrapped path");

			DebugBackgroundInput wrapper;
			BackgroundDelegateSpy spy(*level);
			wrapper.set_next(&spy);
			seats = adversarial;
			wrapper.filter_seats(seats, *level);
			PlayerInputData expected_a;
			expected_a.connected = expected_a.primary_shoot = expected_a.shoot_direction_requested = true;
			expected_a.x_movement = -0.75f;
			expected_a.shoot_direction = { -0.6f, 0.8f };
			PlayerInputData expected_b;
			expected_b.jump_pressed = true;
			require(spy.calls == 1 && input_state(seats[0]) == input_state(expected_a) &&
				input_state(seats[1]) == input_state(expected_b),
				"A delegate must see neutral physical input and retain the effective input it supplies afterward");
			wrapper.set_next(nullptr);
			seats = adversarial;
			wrapper.filter_seats(seats, *level);
			require_neutral_seats(seats);
			require(spy.calls == 1, "Clearing the background delegate retained a stale next filter");
		}

		void check_inactive_input_edges(GameTuning& tuning, Presentation& presentation)
		{
			TestDirector test(tuning, presentation);
			auto& director = test.director;
			auto& clock = test.impl.clock();
			FakeMatch match(director);
			match.settings = planned(2);
			director.attach_match(match);
			const DetachMatch detach{ director, match };
			test.impl.set_background_probe([] { return DebugBackgroundState{ true, false, false }; });
			require(test.impl.execute("script.background 1").ok, "The edge fixture opts into visible background work");
			const float dt = 1.0f / 60.0f;
			const std::vector<PlayerInputData> neutral(input_slots::count);
			auto tap = neutral;
			auto& keyboard = tap[static_cast<std::size_t>(input_slots::keyboard_and_mouse)];
			keyboard.jump_pressed = keyboard.toggle_debug = keyboard.skip_intro = true;
			const auto no_edges = [&]
			{
				require(match.jumps_delivered() == 0 && match.debug_edges == 0 && match.intro_skip_edges == 0,
					"A zero-run foreground jump/debug/intro edge survived the inactive match hook");
			};

			clock.set_timescale(0.25f);
			director.update_match(match, tap, dt);
			require(match.updates == 0, "The foreground tap must be latched on an actual zero-run step");
			for (int step = 0; step < 3; ++step)
			{
				require(!director.before_match_update(match, dt, false), "An open inactive hook should permit neutral work");
				director.update_match(match, neutral, dt);
				require(match.updates == (step == 2 ? 1 : 0),
					"Discarding foreground edges reset or consumed the quarter-speed accumulator");
			}
			no_edges();

			// Also preserve a half-step accumulator while paused updates are queued.
			match.reset_counts();
			clock.reset_accumulator();
			clock.set_timescale(0.5f);
			director.update_match(match, tap, dt);
			clock.request_steps(2);
			require(!director.before_match_update(match, dt, false) && clock.paused() &&
				clock.pending_steps() == 2 && clock.timescale() == 0.5f && match.updates == 0,
				"Discarding input edges changed pause, pending steps or timescale");
			for (int step = 0; step < 3; ++step)
			{
				director.update_match(match, neutral, dt);
				require(match.updates == std::min(step + 1, 2), "Inactive input reset lost or invented a queued step");
				require(!director.before_match_update(match, dt, false), "A paused open gate still permits the hook");
			}
			require(clock.paused() && clock.pending_steps() == 0, "Stepping must leave the clock paused");
			clock.set_paused(false);
			director.update_match(match, neutral, dt);
			require(match.updates == 3, "Input-edge discard lost the half-step accumulated before pausing");
			no_edges();
		}

		void check_inactive_script_stop(GameTuning& tuning, Presentation& presentation)
		{
			ScratchFolder scratch(L"background_gate");
			struct ScriptCase { const char* name; const char* text; const char* sentinel; bool queued; };
			const std::array cases{
				ScriptCase{ "off_before_restart", "script.background 0; echo off-before-restart; restart\n",
					"off-before-restart", false },
				ScriptCase{ "restart_before_off", "restart; script.background 0; echo restart-before-off\n",
					"restart-before-off", true },
			};
			const float dt = 1.0f / 60.0f;
			for (const auto& script : cases)
			{
				write_text(scratch.root / (std::string(script.name) + ".cfg"), script.text);
				TestDirector test(tuning, presentation);
				auto& director = test.director;
				auto& impl = test.impl;
				FakeMatch match(director);
				match.settings = planned(2);
				director.attach_match(match);
				const DetachMatch detach{ director, match };
				impl.set_script_directories({ scratch.root });
				impl.set_background_probe([] { return DebugBackgroundState{ true, false, false }; });
				require(impl.execute("script.background 1").ok && impl.execute(std::string("exec ") + script.name).ok,
					"The background OFF-batch fixture must queue its actual script");
				impl.log().clear();
				for (int hook = 0; hook < 3; ++hook)
				{
					require(director.before_match_update(match, dt, false) && !director.background_scripts() &&
						impl.scripts().running() && !any_line(impl.log(), script.sentinel) &&
						impl.request().restart == script.queued && match.rebuilds == 0 && match.updates == 0,
						"An inactive OFF statement must hold the next statement and any already queued rebuild");
				}
				require(director.before_match_update(match, dt, true) && any_line(impl.log(), script.sentinel) &&
					!impl.scripts().running() && impl.request().empty() && match.rebuilds == 1 && match.updates == 0,
					"A foreground hook must resume the held script and consume its single rebuild");
			}

			// Default-off inactive menu ticking is an intentional legacy boundary.
			write_text(scratch.root / L"legacy_menu.cfg", "echo legacy-inactive-menu\n");
			TestDirector menu(tuning, presentation);
			menu.impl.set_script_directories({ scratch.root });
			FakePage page;
			require(menu.impl.execute("exec legacy_menu").ok, "The legacy menu fixture queues a script");
			menu.director.before_page_update(page, dt, false);
			require(!menu.director.background_scripts() && any_line(menu.impl.log(), "legacy-inactive-menu") &&
				!menu.impl.scripts().running() && page.starts == 0 && page.dismissals == 0,
				"Default-off inactive menu hooks must retain their existing script-only behavior");
		}
	}

	void check_background_policy(const std::filesystem::path& content, GameTuning& tuning,
		Presentation& presentation)
	{
		// Only visible, restored, non-power-suspended facts (bit pattern 001)
		// permit this opt-in. Every other truth-table entry is a hard gate.
		for (unsigned int bits = 0; bits < 8; ++bits)
		{
			require(background_window_allows(facts(bits)) == (bits == 1u),
				"Background hard-gate truth table differs for facts " + std::to_string(bits));
		}
		DebugBackgroundWindow unavailable;
		const auto missing = unavailable.state();
		require(!missing.visible && !missing.minimized && !missing.power_suspended &&
			!background_window_allows(missing), "An unattached background observer must fail closed");
		int native_notifications = 0;
		require(!unavailable.attach(nullptr, [&native_notifications] { ++native_notifications; }) &&
			native_notifications == 0 && !background_window_allows(unavailable.state()),
			"An absent native window must neither open the background gate nor notify a fake change");

		TestDirector test(tuning, presentation);
		auto& director = test.director;
		auto& impl = test.impl;
		FakeMatch match(director);
		match.settings = planned(2);
		director.attach_match(match);
		const DetachMatch detach{ director, match };
		require(!director.background_scripts() && !director.background_updates_allowed(),
			"A fresh attached director must default background scripts off");
		auto* variable = impl.registry().variable("script.background");
		require(variable != nullptr && variable->default_text() == "off" && variable->value_text() == "off",
			"script.background must be a registered session-only off-by-default Boolean");

		// Seed unrelated pending work so a policy toggle cannot silently clear it.
		require(impl.execute("players 3").ok && !impl.request().empty(),
			"The background fixture needs a real queued match request");
		impl.clock().set_timescale(0.25f);
		impl.clock().request_steps(2);
		const auto original = unrelated_state(test, match);
		const auto unchanged = [&]
		{
			require(unrelated_state(test, match) == original,
				"Background policy unexpectedly changed match, clock, filter, replay or queued request state");
		};
		const auto query = [&](bool enabled)
		{
			const int notifications = match.background_changes;
			impl.log().clear();
			const auto status = impl.execute("script.background");
			require(status.ok && !status.close_console && director.background_scripts() == enabled &&
				log_contains(impl.log(), std::string("script.background = ") + (enabled ? "on" : "off"),
					DebugSeverity::info) && match.background_changes == notifications,
				"Reading script.background changed policy or requested target reconciliation");
			unchanged();
		};
		const auto set = [&](bool enabled)
		{
			const int notifications = match.background_changes;
			const auto status = impl.execute(enabled ? "script.background 1" : "script.background 0");
			require(status.ok && !status.close_console && director.background_scripts() == enabled &&
				match.background_changes == notifications + 1,
				"A valid background setting must reconcile the attached fake target immediately");
			require(variable->value_text() == (enabled ? "on" : "off"),
				"The background variable and director policy disagree");
			unchanged();
		};
		const auto reject = [&]
		{
			const bool enabled = director.background_scripts();
			for (const char* line : { "script.background 2", "script.background -1",
				"script.background potato", "script.background 1 0" })
			{
				const int notifications = match.background_changes;
				const auto status = impl.execute(line);
				require(!status.ok && !status.close_console && director.background_scripts() == enabled &&
					match.background_changes == notifications,
					std::string("Rejected background input changed policy or notified the target: ") + line);
				unchanged();
			}
		};
		query(false);
		reject();
		set(true);
		require(!director.background_updates_allowed(),
			"Opting in cannot invent facts for an unattached native window");
		query(true);
		reject();

		DebugBackgroundState observed{};
		int fact_reads = 0;
		const auto probe = [&]() { ++fact_reads; return observed; };
		for (unsigned int bits = 0; bits < 8; ++bits)
		{
			observed = facts(bits);
			const int notifications = match.background_changes;
			impl.set_background_probe(probe);
			require(match.background_changes == notifications + 1 &&
				director.background_updates_allowed() == (bits == 1u),
				"An opted-in director did not apply injected native hard gates");
			unchanged();
		}
		observed = facts(1);
		impl.set_background_probe(probe);
		require(director.background_updates_allowed(), "Visible restored facts should permit the opt-in");
		const int notifications = match.background_changes;
		impl.set_background_probe({});
		require(match.background_changes == notifications + 1 && !director.background_updates_allowed(),
			"Removing a probe must restore the unavailable native observer and close the gate");
		impl.set_background_probe([] { return DebugBackgroundState{}; });
		require(!director.background_updates_allowed(), "Explicit missing/closed facts must fail closed");
		set(false);
		for (unsigned int bits = 0; bits < 8; ++bits)
		{
			observed = facts(bits);
			impl.set_background_probe(probe);
			const int before = fact_reads;
			require(!director.background_updates_allowed() && fact_reads == before,
				"Off background policy must stay closed without consulting an injected window");
			unchanged();
		}
		impl.set_background_probe({});
		query(false);
		set(true);
		TestDirector fresh(tuning, presentation);
		require(!fresh.director.background_scripts() && !fresh.director.background_updates_allowed() &&
			director.background_scripts(), "Background opt-in leaked into another director/session");
		require(!DebugDirector::inert().background_scripts() &&
			!DebugDirector::inert().background_updates_allowed(), "The inert director must never permit background work");
		// No Application/audio service exists in this CPU fixture. These calls
		// only exercise its safe missing-service path, not gain or voice behavior.
		director.sync_background_audio(false);
		director.sync_background_audio(true);
		DebugDirector unattached(&tuning, &presentation);
		unattached.sync_background_audio(false);
		unattached.sync_background_audio(true);
		unchanged();
		check_inactive_input_edges(tuning, presentation);
		check_inactive_script_stop(tuning, presentation);
		check_post_mouse_input(content, tuning, presentation);
		std::cout << "PASS background CPU policy: 8 native-fact truth states, default-off/session isolation, "
			"query/0/1/refusal side effects, 16 injected on/off gate cases, removed/missing observer, "
			"immediate FakeMatch notifications, absent-service audio no-op, inactive queued-edge discard with "
			"clock pacing/paused steps retained, 2 held OFF batches, legacy inactive menu scripts, "
			"post-mouse neutralization/presence/delegate ordering and actual active/inactive filter selection; "
			"no native focus/audio acceptance\n";
	}
}
#endif
