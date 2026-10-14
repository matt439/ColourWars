#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/debug/tune_scope.h"
#include "game/directory_consts.h"
#include "game/objects/level.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/states/pause_menu.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/collision/partitioner.h"
#include "engine/core/registry.h"
#include "engine/input/mouse.h"
#include "engine/render/render_resources.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/sprite_font_file.h"
#include "engine/render/text_encoding.h"
#include "engine/render/viewport_manager.h"
#include <bit>
#include <cstdint>
#include <memory>
#include <optional>
#include <process.h>

// Replay: the codec and the file, and record and play on a real Level built
// on the CPU without a device.
namespace debug_console_acceptance
{
	namespace
	{
	int count_of_lines(const DebugLog& log, const std::string& text)
	{
		int count = 0;
		for (const DebugLogLine& line : log.lines())
		{
			count += line.text.find(text) != std::string::npos ? 1 : 0;
		}
		return count;
	}

	bool same_inputs(const std::vector<std::vector<PlayerInputData>>& a,
		const std::vector<std::vector<PlayerInputData>>& b)
	{
		if (a.size() != b.size()) { return false; }
		for (std::size_t seat = 0; seat < a.size(); ++seat)
		{
			if (a[seat].size() != b[seat].size()) { return false; }
			for (std::size_t i = 0; i < a[seat].size(); ++i)
			{
				if (!replay_codec::same(a[seat][i], b[seat][i])) { return false; }
			}
		}
		return true;
	}
	}

	void check_replay_codec()
	{
		PlayerInputData input;
		input.x_movement = -0.0f;
		input.shoot_direction = { 1e-40f, -1.0f };
		input.shoot_angle = 3.14159265f;
		input.left_analog_stick = { 0.25f, -0.75f };
		input.right_analog_stick = { std::nextafter(0.5f, 1.0f), 0.0f };
		input.jump_pressed = input.jump_held = input.primary_shoot = input.connected = true;
		input.drop_through = true;
		PlayerInputData decoded;
		require(replay_codec::decode(replay_codec::encode(input), decoded) &&
			std::bit_cast<std::uint32_t>(decoded.x_movement) == std::bit_cast<std::uint32_t>(-0.0f) &&
			decoded.shoot_direction.x == 1e-40f && decoded.right_analog_stick.x == input.right_analog_stick.x &&
			decoded.jump_pressed && decoded.drop_through && !decoded.toggle_debug &&
			replay_codec::same(decoded, input), "An input encodes and decodes bit-exactly (-0.0, denormals)");
		require(!replay_codec::decode("zz|0|0", decoded) && !replay_codec::decode("80|0|0|0|0|0|0|0|q", decoded),
			"Malformed inputs do not decode");

		std::vector<PlayerInputData> stream(5, input);
		stream.push_back(PlayerInputData());
		stream.insert(stream.end(), 3, input);
		const std::vector<std::string> entries = replay_codec::run_length(stream);
		require(entries.size() == 3 && entries[0].rfind("5*", 0) == 0 && entries[2].rfind("3*", 0) == 0,
			"Runs compress");
		const std::vector<PlayerInputData> expanded = replay_codec::expand(entries);
		require(same_inputs({ expanded }, { stream }), "Runs expand back");
		require(replay_name("my run!") == "my_run_" && replay_name("").rfind("replay-", 0) == 0,
			"Replay names are safe file stems");

		const std::filesystem::path directory = std::filesystem::temp_directory_path() /
			(L"colourwars_replay_codec_" + std::to_wstring(_getpid()));
		const std::filesystem::path path = directory / L"round.json";
		ReplayFile file;
		file.build = "test \"build\"";
		file.dt_bits = std::bit_cast<std::uint32_t>(1.0f / 60.0f);
		file.seed = 4000000000u;
		file.tuning_hash = 0x0123456789abcdefull;
		file.presentation_hash = 42;
		file.tune_diff = { "tuning.player.gravity 2000.0 -> 2100.0" };
		file.god = true;
		file.settings = planned(3);
		file.settings.set_workshop_visual_mode(PlayerVisualMode::authored);
		file.settings.set_player_weapon(1, WeaponType::roller);
		file.updates = 120;
		file.suspends = { 0, 61 };
		file.inputs = { stream, std::vector<PlayerInputData>(9), expanded };
		file.checkpoints = { { 60, 0xfeedfacecafebeefull }, { 120, 7 } };
		file.final_print.updates = 120;
		file.final_print.hash = 99;
		file.final_print.team_a = std::bit_cast<std::uint32_t>(12.5f);
		file.final_print.seats.push_back({ 1, 2, 3, 4, 5, 6, 7, 2, 1 });
		file.write(path);
		const ReplayFile read = ReplayFile::read(path);
		require(read.build == file.build && read.dt_bits == file.dt_bits && read.seed == file.seed &&
			read.tuning_hash == file.tuning_hash && read.presentation_hash == 42 &&
			read.tune_diff == file.tune_diff && read.god && !read.infinite_ammo &&
			read.updates == 120 && read.suspends == file.suspends &&
			same_inputs(read.inputs, file.inputs) && read.checkpoints.size() == 2 &&
			read.checkpoints[0].hash == 0xfeedfacecafebeefull &&
			MatchFingerprint::difference(file.final_print, read.final_print).empty() &&
			read.final_print.hash == 99, "A replay file round-trips");
		const std::vector<MenuPlayerSettings> seats = read.settings.player_settings();
		require(seats.size() == 3 && seats[0].num == input_slots::keyboard_and_mouse && seats[1].num == -1 &&
			seats[1].weapon == WeaponType::roller && seats[1].team == PlayerTeam::b &&
			read.settings.stage() == "x" && read.settings.workshop_visual_mode() == PlayerVisualMode::authored &&
			read.settings.screen_layout() == labrador::ScreenLayout::three_player,
			"The recorded settings rebuild verbatim");

		std::string text = read_text(path);
		text.replace(text.find("\"version\": 1"), 12, "\"version\": 2");
		write_text(path, text);
		std::string message;
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("format version 2; this build reads 1") != std::string::npos,
			"Another format version is refused: " + message);
		write_text(path, "{ \"version\": 1, ");
		message.clear();
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("not valid JSON") != std::string::npos, "A broken file is refused: " + message);
		for (const char* layout : { "\"layout\": 7,", "\"layout\": 0," })
		{
			file.write(path);
			text = read_text(path);
			text.replace(text.find("\"layout\": 2,"), 12, layout);
			write_text(path, text);
			message.clear();
			try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("is not the 3-player layout") != std::string::npos,
				std::string("A replay with ") + layout + " for three seats is refused: " + message);
		}
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
	}

	namespace
	{
	// ------------------------------------------- a real Level on the CPU

	// What LevelBuilder needs, with no window or device: sprite sheets and
	// fonts whose atlases are null handles, a silent bank, the levels, and
	// the shell services by value.
	struct CpuWorld
	{
		labrador::RenderResources render;
		labrador::AudioResources audio;
		PlaySettings preferences;
		labrador::Registry<LevelLoadedInfo> levels{ "Level" };
		StageList stages;
		labrador::ResolutionManager resolution;
		labrador::ViewportManager viewports{ &resolution };
		labrador::Mouse mouse;
		labrador::Partitioner partitioner;
		std::unique_ptr<LevelBuilder> builder;

		CpuWorld(const std::filesystem::path& content, const GameTuning& tuning,
			const Presentation& presentation)
		{
			for (const char* sheet : { "sprite_sheet_1", "menu_paint", "weapon_paint", "workshop_kit" })
			{
				this->render.add_sprite_sheet(sheet, labrador::read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
					labrador::TextureHandle{}));
			}
			for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
				"gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
				"gill_sans_mt_bold_144", "courier_new_bold_16" })
			{
				const labrador::SpriteFontFile file = labrador::read_sprite_font_file(
					(content / "fonts" / (std::string(font) + ".spritefont")).string());
				auto loaded = std::make_unique<labrador::Font>(labrador::TextureHandle{}, file.glyphs,
					file.line_spacing);
				if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
				this->render.add_font(font, std::move(loaded));
			}
			this->audio.add_sound_bank("sound_bank_1", labrador::SoundBank::silent());
			for (const char* level : { "movement_course", "close_quarters" })
			{
				auto info = std::make_unique<LevelLoadedInfo>(
					(content / "levels" / (std::string(level) + ".json")).string().c_str());
				info->validate();
				this->levels.add(level, std::move(info));
			}
			this->stages = { { "movement_course", "Movement Course", "icon", false },
				{ "close_quarters", "Close Quarters", "icon", true } };
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &tuning, &presentation, &this->resolution, nullptr,
				&this->partitioner, &this->mouse);
		}
	};

	// GameLevel's debug face over a real Level: one build path through
	// before_level_build and after_level_build, as GameLevel has.
	class CpuMatch final : public DebugMatchTarget
	{
	public:
		CpuMatch(CpuWorld& world, DebugDirector& director) : world_(world), director_(director) {}

		MenuLevelSettings settings;
		std::unique_ptr<Level> level;
		int builds = 0;
		// What GameLevel would choose; the director may override it.
		OpeningMode opening = OpeningMode::off;

		void build()
		{
			this->level.reset();
			this->director_.before_level_build(*this);
			this->world_.viewports.set_layout(this->settings.screen_layout());
			this->level = this->world_.builder->build_level(this->settings, TeamColourPair::blue_orange,
				this->director_.opening_mode(this->opening));
			++this->builds;
			this->director_.after_level_build(*this);
		}

		// What GameLevel::on_suspend does when a state covers the match.
		void suspend()
		{
			this->level->suspend();
			this->director_.match_suspended(*this);
		}

		const MenuLevelSettings& debug_settings() const override { return this->settings; }
		void debug_rebuild(const MenuLevelSettings& plan, bool) override
		{
			this->settings = plan;
			this->build();
		}
		void debug_adopt(const MenuLevelSettings& plan, bool) override { this->settings = plan; }
		void debug_leave() override {}
		void debug_update_level(const std::vector<PlayerInputData>& inputs, float dt,
			SeatInputFilter* seats) override
		{
			this->level->update(inputs, dt, seats);
		}
		bool debug_level_finished() const override { return this->level->state() == LevelState::finished; }
		const Level* debug_level() const override { return this->level.get(); }
		// GameLevel's, uncovered and in focus: only the debug clock pauses it,
		// which pauses the voices and leaves the players alone.
		void debug_sync_audio() override
		{
			if (this->director_.match_audio_paused())
			{
				this->level->pause_audio();
			}
			else
			{
				this->level->resume();
			}
		}
		std::vector<std::string> debug_status() const override { return {}; }
		void debug_set_overlays(const LevelOverlays& overlays) override { this->level->set_overlays(overlays); }
		void debug_set_player_text(bool showing) override { this->level->set_players_showing_debug(showing); }
		void debug_set_cheats(bool immune, bool infinite) override
		{
			this->god = immune;
			this->level->set_player_cheats(immune, infinite);
		}
		bool god = false;
		std::vector<LevelStation> debug_stations() const override
		{
			return this->world_.levels.get(this->settings.stage())->stations();
		}
		bool debug_place(int seat, const mattmath::Vector2F& top_left) override
		{
			return this->level->place_player(seat, top_left);
		}
		bool debug_respawn(int seat) override { return this->level->respawn_player(seat); }
		bool debug_kill(int seat) override { return this->level->kill_player(seat); }
		MatchPaint debug_paint() const override
		{
			const LevelEndInfo info = this->level->level_end_info();
			MatchPaint paint;
			paint.team_a = info.team_a_area;
			paint.team_b = info.team_b_area;
			paint.total = info.total_area;
			return paint;
		}
		bool debug_countdown_over() const override
		{
			return this->level->match_started();
		}
		void debug_set_seat_labels(const std::vector<std::wstring>& labels) override
		{
			this->seat_labels = labels;
			this->level->set_seat_labels(labels);
		}
		std::vector<mattmath::RectangleF> debug_screen_obstacles() const override
		{
			return this->level->opening_cards();
		}
		std::vector<std::wstring> seat_labels;

	private:
		CpuWorld& world_;
		DebugDirector& director_;
	};

	// The keyboard seat's own scripted input (the bots drive the rest).
	std::vector<PlayerInputData> keyboard_step(int step)
	{
		std::vector<PlayerInputData> inputs(input_slots::count);
		PlayerInputData& keys = inputs[static_cast<std::size_t>(input_slots::keyboard_and_mouse)];
		keys.connected = true;
		keys.x_movement = (step / 90) % 3 == 0 ? 1.0f : (step / 90) % 3 == 1 ? -1.0f : 0.0f;
		keys.jump_held = step % 75 < 12;
		keys.jump_pressed = step % 75 == 0;
		keys.primary_shoot = step % 40 < 25;
		return inputs;
	}

	// Steps the match until `done`, the way GameLevel does: the director's
	// hooks, then its clock's updates. Returns the steps taken.
	template <typename Done>
	int drive(DebugDirector& director, CpuMatch& match, float dt, int limit, Done done,
		int suspend_at = -1)
	{
		DebugDirectorImpl& impl = *director.impl();
		int step = 0;
		bool suspended = false;
		for (; step < limit && !done(); ++step)
		{
			if (director.before_match_update(match, dt))
			{
				continue;
			}
			if (!suspended && suspend_at >= 0 && impl.level_updates() == suspend_at)
			{
				match.suspend();
				suspended = true;
			}
			director.update_match(match, keyboard_step(impl.level_updates()), dt);
		}
		return step;
	}
	}

	std::string check_replay_cpu_match(const std::filesystem::path& game_directory)
	{
		ScratchContent scratch(game_directory, L"replay");
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
		const DebugLog& log = impl.log();
		const float dt = 1.0f / 60.0f;
		const int length = 600;

		// +seed 7 +stage movement_course +players 4 +bot all paint, built directly.
		require(impl.execute("seed 7").ok && impl.execute("bot all paint").ok, "seed and bot in the menus");
		MatchRequest roster;
		roster.stage = "movement_course";
		roster.players = 4;
		roster.visual = PlayerVisualMode::authored;
		const MatchPlan plan = MatchPlanner(&world.stages).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [] { return 0u; });
		require(plan.ok, "The CPU roster resolves");
		CpuMatch match(world, director);
		match.settings = plan.settings;
		director.attach_match(match);
		match.build();
		require(impl.status_strip().find("BOTS 3x paint") != std::string::npos, "Three paint bots");

		// Record 600 updates, with a suspend (a console, say) at update 300.
		require(impl.execute("record cpu 600").ok && impl.request().restart, "record queues a fresh build");
		drive(director, match, dt, 2000, [&] { return impl.recording() || match.builds >= 2; });
		require(impl.recording() && impl.level_updates() == 0, "Recording starts at a fresh build");
		const int recorded_steps = drive(director, match, dt, 2000, [&] { return !impl.recording(); }, 300);
		require(!impl.recording() && recorded_steps >= length && log_contains(log, "record: saved", DebugSeverity::info) &&
			log_contains(log, "(600 updates, checksum", DebugSeverity::info), "record stops by itself after 600 updates");
		const std::filesystem::path saved = std::filesystem::path(labrador::widen(
			directory_consts::replay_directory())) / L"cpu.json";
		const ReplayFile file = ReplayFile::read(saved);
		require(file.updates == length && file.checkpoints.size() == 10 && file.suspends == std::vector<int>{ 300 } &&
			file.inputs.size() == 4 && file.seed == 7u && file.dt_bits == std::bit_cast<std::uint32_t>(dt),
			"The file holds 600 updates, 10 checkpoints, the suspend, four seats, the seed and dt");
		require(file.final_print.team_a != 0 || file.final_print.team_b != 0, "The bots painted");
		// Recorded seats are post-filter: the keyboard seat as scripted, the bots' own.
		require(file.inputs[0].size() == file.inputs[1].size() && file.inputs[1].size() > 300 &&
			std::any_of(file.inputs[2].begin(), file.inputs[2].end(), [](const PlayerInputData& input)
				{ return input.primary_shoot; }), "Every seat's input is recorded, the bots' included");

		// The same seeded bot match again, unrecorded, ends identically.
		impl.execute("restart");
		drive(director, match, dt, 2000, [&] { return impl.level_updates() >= length; }, 300);
		const std::optional<MatchFingerprint> again = impl.fingerprint();
		require(again && again->hash == file.final_print.hash,
			"The same seed, bots and keys give the same match: " + MatchFingerprint::difference(file.final_print, *again));

		// Replays.
		const auto play = [&](const std::string& line)
			{
				require(impl.execute(line).ok, line + " is accepted");
				require(director.before_match_update(match, dt) && impl.playing(),
					line + " rebuilds the match into the replay");
				require(!impl.clock().paused(), line + " resumes the clock the last replay held");
				drive(director, match, dt, 4000, [&] { return !impl.playing() || impl.level_updates() >= length; });
			};
		// One after another, with no `pause off` between: each ends holding
		// its last frame, and the next play resumes from a paused clock.
		play("play cpu");
		require(log_contains(log, "replay cpu: MATCH (600 updates, 10 checkpoints", DebugSeverity::info),
			"The replay matches the recording");
		require(impl.status_strip().find("PLAY cpu done (MATCH)") != std::string::npos,
			"A finished replay's strip says done and how it ended, got '" + impl.status_strip() + "'");
		require(match.seat_labels.size() == 4 && std::all_of(match.seat_labels.begin(),
			match.seat_labels.end(), [](const std::wstring& label) { return label == L"Replay"; }),
			"Every seat's HUD says Replay while a replay drives the match");
		require(impl.execute("god on").ok && impl.execute("god off").ok,
			"Once the replay has ended, cheats are accepted again");

		impl.execute("timescale 4");
		play("play cpu");
		impl.execute("timescale 1");
		require(count_of_lines(log, "replay cpu: MATCH") == 2, "At timescale 4 the replay still matches");

		world.resolution.set_resolution(mattmath::Vector2F(1280.0f, 720.0f));
		play("play cpu");
		require(count_of_lines(log, "replay cpu: MATCH") == 3,
			"At another window size the replay still matches (the camera only feeds recorded aim)");
		require(impl.clock().paused() && !log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A held last frame and a paused rebuild are not suspends");

		// A pause the user chose stays, and stepping the replay one update at
		// a time is neutral: the paused clock pauses voices, not players.
		require(impl.execute("pause off").ok && impl.execute("pause on").ok && impl.execute("play cpu").ok &&
			director.before_match_update(match, dt) && impl.playing() && impl.clock().paused(),
			"play keeps a pause the user set");
		const int matched = count_of_lines(log, "replay cpu: MATCH");
		for (int step = 0; step < length + 10 && impl.playing() && !impl.replay_finished(); ++step)
		{
			// What `step 1` (F7) does, without a log line per update.
			impl.clock().request_steps(1);
			director.update_match(match, keyboard_step(step), dt);
		}
		require(count_of_lines(log, "replay cpu: MATCH") == matched + 1 &&
			!log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A replay stepped one update at a time still matches");
		impl.execute("pause off");

		// During a replay the simulation's side doors are shut.
		require(impl.execute("play cpu").ok && director.before_match_update(match, dt) && impl.playing(),
			"play again");
		drive(director, match, dt, 20, [] { return false; });
		require(!impl.execute("god on").ok && !impl.execute("respawn 1").ok &&
			!impl.execute("tune tuning.player.gravity next").ok && !impl.execute("bot all walk").ok &&
			!impl.execute("tune.save").ok && !impl.execute("tune.reset").ok,
			"god, respawn, tune, tune.save, tune.reset and bot are refused while a replay runs");
		require(log_contains(log, "tune.save: replaying 'cpu' would diverge", DebugSeverity::error),
			"tune.save says why");
		// A script runs under a replay exactly as typed lines do: wait and echo
		// pass, a command that would change the simulation is refused, and that
		// refusal stops the script at its line.
		{
			const std::filesystem::path scripts = std::filesystem::temp_directory_path() /
				(L"colourwars_replay_scripts_" + std::to_wstring(_getpid()));
			std::filesystem::create_directories(scripts);
			write_text(scripts / L"under_replay.cfg", "echo under-replay-1\nwait 3\necho under-replay-2\n"
				"god on\necho under-replay-3\n");
			impl.set_script_directories({ scripts });
			require(impl.execute("exec under_replay").ok, "exec under a replay");
			drive(director, match, dt, 10, [&] { return log_contains(log, "under_replay.cfg:4 stopped",
				DebugSeverity::error); });
			require(log_contains(log, "under-replay-2", DebugSeverity::info) &&
				!log_contains(log, "under-replay-3", DebugSeverity::info) &&
				log_contains(log, "under_replay.cfg:4 stopped: god: replaying 'cpu' would diverge",
					DebugSeverity::error) && impl.playing(),
				"Under a replay, wait and echo run and god stops the script at its line");
			impl.set_script_directories({});
			std::error_code ignored;
			std::filesystem::remove_all(scripts, ignored);
		}
		require(director.drives_slot(input_slots::keyboard_and_mouse), "A replay drives every recorded slot");
		match.suspend();
		require(log_contains(log, "which the recording was not", DebugSeverity::warning),
			"A suspend the recording lacks is reported as the cause of a divergence");
		require(impl.execute("play stop").ok && !impl.playing(), "play stop");

		// Other tuning: refused, then forced and diverged.
		require(impl.execute("tune tuning.player.gravity " + tune_format(std::stod(
			impl.tune_token("tuning.player.gravity")) + 500.0, impl.tune_token("tuning.player.gravity"))).ok,
			"tune gravity");
		require(!impl.execute("play cpu").ok && log_contains(log, "recorded with other tuning", DebugSeverity::error),
			"A replay with other tuning is refused");
		play("play cpu force");
		require(log_contains(log, "replay cpu: DIVERGED at update", DebugSeverity::error),
			"Forced with other gravity it diverges, at a checkpoint");
		impl.execute("tune.reset");

		// Another step size: refused at the first update. God is on before
		// play (the recording had it off), so the refusal must put it back.
		require(impl.execute("god on").ok && match.god, "god on before the wrong-dt play");
		const bool wrong_dt_played = impl.execute("play cpu").ok;
		const bool wrong_dt_built = director.before_match_update(match, dt);
		require(wrong_dt_played && wrong_dt_built && impl.playing(),
			"play with the wrong dt: " + log.lines().back().text);


		director.update_match(match, keyboard_step(0), 0.0166666f);
		require(!impl.playing() && log_contains(log, "refused: recorded at dt", DebugSeverity::error),
			"A replay at another dt (the executable's StepTimer gives 0.0166666) is refused");
		require(match.god && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"The refused replay hands back the cheats and the bots, as play stop does: god " +
			std::to_string(match.god) + ", strip '" + impl.status_strip() + "'");
		require(std::none_of(match.seat_labels.begin(), match.seat_labels.end(),
			[](const std::wstring& label) { return label == L"Replay"; }) &&
			std::count(match.seat_labels.begin(), match.seat_labels.end(), L"Bot: paint") == 3,
			"The refused replay's HUDs say Bot again, not Replay");
		require(impl.execute("god off").ok && !match.god, "god off");
		require(!impl.execute("play cpu").ok && log_contains(log, "a replay never matches across step sizes",
			DebugSeverity::error), "and play itself refuses once the step is known");
		director.update_match(match, keyboard_step(0), dt);

		// A replay armed and then left behind does not take over the next match.
		require(impl.execute("play cpu").ok && impl.execute("menu").ok &&
			director.before_match_update(match, dt) && !impl.playing() &&
			log_contains(log, "play cpu: disarmed (the match was left)", DebugSeverity::warning),
			"Leaving the match disarms the replay it queued");
		match.build();
		require(!impl.playing() && impl.status_strip().find("BOTS 3x paint") != std::string::npos,
			"The next match is a live one, with its bots");

		// From the menus: play starts the match with the recorded settings.
		director.detach_match(match);
		require(impl.execute("play cpu").ok && impl.request().stage == std::optional<std::string>("movement_course"),
			"play in the menus queues the recorded stage");
		FakePage page;
		page.start_result = true;
		require(director.before_page_update(page, dt) && page.starts == 1 &&
			page.started.player_count() == 4 && page.started.stage() == "movement_course" &&
			page.started.workshop_visual_mode() == PlayerVisualMode::authored,
			"and the main menu starts it with the recorded settings");
		CpuMatch fresh(world, director);
		fresh.settings = page.started;
		director.attach_match(fresh);
		fresh.build();
		require(impl.playing(), "The match started from the menus is the replay");
		const int matched_before = count_of_lines(log, "replay cpu: MATCH");
		drive(director, fresh, dt, 4000, [&] { return !impl.playing() || impl.level_updates() >= length; });
		require(count_of_lines(log, "replay cpu: MATCH") == matched_before + 1, "and it matches");
		director.detach_match(fresh);
		impl.execute("bot off");
		impl.execute("seed off");
		std::error_code ignored;
		std::filesystem::remove(saved, ignored);
		return "real Level";
	}

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
		require(about(played, tuning.match().duration) && level.state() == LevelState::zoom_out &&
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
}
#endif
