#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"
#include "scripts/acceptance/debug_console/cpu_level.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay/bot.h"
#include "game/debug/replay/match_fingerprint.h"
#include "game/debug/replay/replay.h"
#include "game/debug/tune/tune_document.h"
#include "game/debug/tune/tune_scope.h"
#include "game/directory_consts.h"
#include "engine/render/text_encoding.h"
#include "engine/core/file_path.h"
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <process.h>

// Replay: the codec and the file, written and read back in a temp folder.
namespace debug_console_acceptance
{
	void check_bot_replay_metadata();
	namespace
	{
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

	bool same_gaps(const std::vector<RoundTape::Gap>& a, const std::vector<RoundTape::Gap>& b)
	{
		if (a.size() != b.size()) { return false; }
		for (std::size_t i = 0; i < a.size(); ++i)
		{
			if (a[i].step != b[i].step || a[i].resume != b[i].resume ||
				!same_inputs({ a[i].seats }, { b[i].seats }))
			{
				return false;
			}
		}
		return true;
	}
	}

	void check_replay_codec()
	{
		check_bot_replay_metadata();
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
		// Files from before seats lost their pause and intro-skip signals still
		// read; those two flags are dropped and never written again.
		require(replay_codec::decode("1c0|0|0|0|0|0|0|0|0", decoded) && decoded.connected &&
			replay_codec::encode(decoded).rfind("80|", 0) == 0,
			"A recorded seat's old pause and skip flags read and are dropped");
		// ROAD#11: the Sub action's press is flag 512, the one bit past them.
		{
			PlayerInputData thrown;
			thrown.connected = thrown.sub_pressed = true;
			PlayerInputData read;
			require(replay_codec::encode(thrown).rfind("280|", 0) == 0 &&
				replay_codec::decode(replay_codec::encode(thrown), read) && read.sub_pressed &&
				replay_codec::same(read, thrown) && !replay_codec::decode("400|0|0|0|0|0|0|0|0", read),
				"The Sub press round-trips as flag 512, and no flag past it reads");
		}

		std::vector<PlayerInputData> stream(5, input);
		stream.push_back(PlayerInputData());
		stream.insert(stream.end(), 3, input);
		const std::vector<std::string> entries = replay_codec::run_length(stream);
		require(entries.size() == 3 && entries[0].rfind("5*", 0) == 0 && entries[2].rfind("3*", 0) == 0,
			"Runs compress");
		const std::vector<PlayerInputData> expanded = replay_codec::expand(entries);
		require(same_inputs({ expanded }, { stream }), "Runs expand back");
		// DBG#15: pure replay-name values only; no new file/record/play IO.
		{
			const std::string first = replay_name("CON");
			require(first == "_CON", "DBG#15 reserved replay stem was not protected: " + first);
			const std::vector<std::string> reserved = reserved_windows_stems();
			std::size_t protected_names = 1;
			for (const std::string& stem : reserved)
				for (int spelling = 0; spelling < 3; ++spelling)
				{
					std::string name = stem;
					for (std::size_t index = 0; index < name.size(); ++index)
						if ((spelling == 1 || (spelling == 2 && index % 2 == 1)) && name[index] >= 'A' && name[index] <= 'Z')
							name[index] = static_cast<char>(name[index] - 'A' + 'a');
					for (const char* suffix : { "", ".json", "_json" })
					{
						const std::string requested = name + suffix;
						const std::string actual = replay_name(requested);
						require(actual == "_" + name, "DBG#15 reserved replay stem changed: " + requested + " -> " + actual);
						++protected_names;
					}
				}
			require(protected_names == 199, "DBG#15 reserved matrix did not visit every spelling and suffix");
			for (const char* neighbor : { "COM0", "COM10", "LPT0", "LPT10", "CON_", "_CON", "-CON", "CONSOLE" })
				for (const char* suffix : { "", ".json", "_json" })
				{
					const std::string name(neighbor);
					require(replay_name(name + suffix) == name, "DBG#15 valid neighboring replay stem changed: " + name + suffix);
				}
			require(replay_name("CON.JSON") == "CON_JSON" && replay_name("CON.JsOn") == "CON_JsOn" &&
				replay_name("CON.") == "CON_" && replay_name("dir/CON") == "dir_CON" &&
				replay_name("CON\xC3\xA9") == "CON__" && replay_name("_CON") == "_CON" &&
				replay_name(".json") == "_json" && replay_name("CON_json_json") == "CON_json",
				"DBG#15 replay protection changed existing suffix case, sanitization or a protected stem");
			std::printf("PASS DBG#15 replay stems: 199 reserved stem/case/suffix values, 24 valid neighbors, "
				"existing case-sensitive suffix and sanitizer controls; pure strings, no record/play or device-path access\n");
		}

		require(replay_name("my run!") == "my_run_" && replay_name("").rfind("replay-", 0) == 0,
			"Replay names are safe file stems");

		const std::filesystem::path directory = std::filesystem::temp_directory_path() /
			(L"colourwars_replay_codec_" + std::to_wstring(_getpid()));
		const std::filesystem::path path = directory / L"round.json";
		ReplayFile file;
		MatchSetup setup;
		require(!file.tape.settings().duration() && !setup.duration(), "New setup follows the tuning duration");
		for (const float invalid : { 0.0f, -1.0f,
			std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
		{
			bool rejected = false;
			try { setup.set_duration(invalid); }
			catch (const std::invalid_argument&) { rejected = true; }
			require(rejected && !setup.duration(), "Invalid durations cannot change setup");
		}
		file.build = "test \"build\"";
		file.dt_bits = std::bit_cast<std::uint32_t>(1.0f / 60.0f);
		file.seed = 4000000000u;
		file.tuning_hash = 0x0123456789abcdefull;
		file.presentation_hash = 42;
		file.tune_diff = { "tuning.player.gravity 2000.0 -> 2100.0" };
		setup = planned(3);
		setup.set_duration(120.12345f);
		setup.set_variant(MatchVariant::endless_paint);
		setup.set_player_weapon(1, WeaponType::roller);
		file.tape.start(0, setup);
		file.tape.set_cheats(true, false);
		file.updates = 120;
		file.tape.record_suspend();
		for (std::size_t step = 0; step < stream.size(); ++step)
		{
			if (step == 6)
			{
				file.tape.record_suspend();
				file.tape.record_resume({ input, PlayerInputData(), stream[5] });
			}
			file.tape.record_step({ stream[step], PlayerInputData(), expanded[step] }, 1.0f / 60.0f);
		}
		file.gap_updates = { 0, 40, 40 };
		file.checkpoints = { { ReplayFile::checkpoint_interval, 0xfeedfacecafebeefull },
			{ 2 * ReplayFile::checkpoint_interval, 7 } };
		file.final_print.updates = 120;
		file.final_print.hash = 99;
		file.final_print.team_a = std::bit_cast<std::uint32_t>(12.5f);
		file.final_print.seats.push_back({ 1, 2, 3, 4, 5, 6, 7, 2, 1 });
		file.write(path);
		const ReplayFile read = ReplayFile::read(path);
		require(read.build == file.build && read.dt_bits == file.dt_bits && read.seed == file.seed &&
			read.tuning_hash == file.tuning_hash && read.presentation_hash == 42 &&
			read.tune_diff == file.tune_diff && read.tape.damage_immune() && !read.tape.infinite_ammo() &&
			read.updates == 120 && same_gaps(read.tape.gaps(), file.tape.gaps()) &&
			read.gap_updates == file.gap_updates &&
			read.tape.steps() == 9 && same_inputs(read.inputs_by_seat(), file.inputs_by_seat()) &&
			read.checkpoints.size() == 2 &&
			read.checkpoints[0].hash == 0xfeedfacecafebeefull &&
			MatchFingerprint::difference(file.final_print, read.final_print).empty() &&
			read.final_print.hash == 99, "A replay file round-trips");
        // The optional active state round-trips; disabled files retain exactly
        // the legacy encoding and contain no paint-underfoot fields.
        const std::string legacy_encoding = read_text(path);
        file.final_print.seats[0].paint_underfoot = true;
        file.final_print.seats[0].support_paint_team = static_cast<int>(PlayerTeam::a);
        file.final_print.seats[0].step_paint_team = static_cast<int>(PlayerTeam::b);
        file.write(path);
        const ReplayFile active = ReplayFile::read(path);
        require(active.final_print.seats[0].paint_underfoot &&
            active.final_print.seats[0].support_paint_team == static_cast<int>(PlayerTeam::a) &&
            active.final_print.seats[0].step_paint_team == static_cast<int>(PlayerTeam::b) &&
            MatchFingerprint::difference(file.final_print, active.final_print).empty(),
            "Active underfoot state did not survive actual replay encoding/decoding");
        file.final_print.seats[0].paint_underfoot = false;
        file.write(path);
        require(read_text(path) == legacy_encoding, "Off replay serialization appended active state or changed legacy bytes");
		// A replay is replaced whole (replace_file_atomically): a refused write
		// throws, keeps the earlier replay and leaves no temporary beside it.
		// The lock shares writing, so an in-place truncating writer would get
		// through it; the write carries different bytes so that would show.
		{
			acceptance::ReplacementLock lock(path);
			require(lock.hold(), "Cannot lock the replay fixture");
			file.final_print.seats[0].paint_underfoot = true;
			std::string refused;
			try { file.write(path); } catch (const std::exception& error) { refused = error.what(); }
			file.final_print.seats[0].paint_underfoot = false;
			require(refused.find("could not be written") != std::string::npos,
				"A replay that cannot replace its file is reported: " + refused);
			require(lock.release() && read_text(path) == legacy_encoding && !acceptance::temporary_beside(path),
				"A refused replay write kept the earlier replay whole and left no temporary");
			// Refused before HYG#63 too (the stream failed to open); the
			// create_directories error is no longer ignored on the way there.
			const std::filesystem::path not_a_folder = directory / L"not-a-folder";
			write_text(not_a_folder, "a file where the replay folder should be");
			refused.clear();
			try { file.write(not_a_folder / L"x.json"); } catch (const std::exception& error) { refused = error.what(); }
			require(refused.find("could not be written") != std::string::npos,
				"A replay whose folder cannot be created is refused: " + refused);
		}
		const std::vector<SeatSetup> seats = read.tape.settings().player_settings();
		require(seats.size() == 3 && seats[0].num == input_slots::keyboard_and_mouse && seats[1].num == -1 &&
			seats[1].weapon == WeaponType::roller && seats[1].team == PlayerTeam::b &&
			read.tape.settings().stage() == "x" &&
			read.tape.settings().screen_layout() == labrador::ScreenLayout::three_player &&
			read.tape.settings().duration() == file.tape.settings().duration() &&
			read.tape.settings().variant() == MatchVariant::endless_paint,
			"The recorded settings rebuild verbatim");

		std::string text;
		std::string message;
		for (const int rejected_version : { 2, 3, 4, 5, 6, 7, 8, 9, 11 })
		{
			file.write(path);
			text = read_text(path);
			text.replace(text.find("\"version\": 10"), 13,
				"\"version\": " + std::to_string(rejected_version));
			write_text(path, text);
			message.clear();
			try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("format version " + std::to_string(rejected_version) +
				"; this build reads only 10") != std::string::npos,
				"A previous workshop recording, an older replay or another format is refused: " + message);
		}
		// A missing, malformed or unknown option may not silently replay as
		// Standard: that would change ammunition and the bot's refill decisions.
		for (const char* replacement : { "", "\"variant\": \"roulette\",",
			"\"variant\": true,", "\"variant\": 1," })
		{
			file.write(path);
			text = read_text(path);
			const std::string variant = "\"variant\": \"endless_paint\",";
			text.replace(text.find(variant), variant.size(), replacement);
			write_text(path, text);
			message.clear();
			try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
			require(!message.empty(), "A replay with missing or invalid match variant was accepted");
		}
		// Every participant's inputs cover every step.
		file.write(path);
		text = read_text(path);
		const std::size_t inputs_key = text.find("\"inputs\": [");
		const std::size_t first_entry = text.find('"', text.find("[", text.find("[", inputs_key) + 1) + 1);
		text.insert(first_entry, "\"80|0|0|0|0|0|0|0|0\", ");
		write_text(path, text);
		message.clear();
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("seat 2 has 9 steps, seat 1 10") != std::string::npos,
			"A replay whose seats differ in length is refused: " + message);
		// Every step is taken at the file's dt.
		{
			ReplayFile uneven = file;
			uneven.tape.start(0, file.tape.settings());
			uneven.tape.record_step({ input, input, input }, 1.0f / 30.0f);
			message.clear();
			try { uneven.write(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("step 0 was taken at dt") != std::string::npos,
				"A tape stepped at another dt is not written: " + message);
		}
		// Every gap has the Level update playback repeats it at, in order.
		{
			ReplayFile unplaced = file;
			unplaced.gap_updates.pop_back();
			message.clear();
			try { unplaced.write(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("3 gaps but 2 gap updates") != std::string::npos,
				"A gap without its update is not written: " + message);
			unplaced.gap_updates = { 0, 40, 39 };
			message.clear();
			try { unplaced.write(path); } catch (const std::exception& error) { message = error.what(); }
			require(message.find("gap 3's update 39") != std::string::npos,
				"Gap updates out of order are not written: " + message);
		}
		write_text(path, "{ \"version\": 3, ");
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
		// A seat's team is a, b or none; any other word is refused, not read as none.
		file.write(path);
		text = read_text(path);
		text.replace(text.find("\"team\": \"b\""), 11, "\"team\": \"c\"");
		write_text(path, text);
		message.clear();
		try { ReplayFile::read(path); } catch (const std::exception& error) { message = error.what(); }
		require(message.find("has an unknown team 'c'") != std::string::npos,
			"A replay with an unknown team word is refused: " + message);
		std::error_code ignored;
		std::filesystem::remove_all(directory, ignored);
	}
}
#endif
