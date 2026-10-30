#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include <objbase.h>
#include <bit>
#include <cstdint>
#include <fstream>

namespace debug_console_acceptance
{
	namespace
	{
		struct ReplayScratch
		{
			std::filesystem::path directory;
			ReplayScratch()
			{
				GUID guid{};
				wchar_t name[40]{};
				require(SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, name, 40) > 0,
					"Cannot name isolated bot replay directory");
				this->directory = std::filesystem::temp_directory_path() /
					(std::wstring(L"ColourWars-bot-replay-") + name);
				require(std::filesystem::create_directory(this->directory), "Cannot create isolated bot replay directory");
			}
			~ReplayScratch()
			{
				std::error_code ignored;
				std::filesystem::remove(this->directory / L"recording.json", ignored);
				std::filesystem::remove(this->directory / L"absent" / L"recording.json", ignored);
				std::filesystem::remove(this->directory / L"absent", ignored);
				std::filesystem::remove(this->directory, ignored);
			}
		};

		std::string replace_once(std::string text, const std::string& before, const std::string& after)
		{
			const std::size_t index = text.find(before);
			require(index != std::string::npos, "Missing replay mutation target: " + before);
			text.replace(index, before.size(), after);
			return text;
		}

		ReplayFile bot_recording(int count, int bot_mask, BotDifficulty difficulty)
		{
			ReplayFile file;
			file.build = "bot metadata";
			file.dt_bits = std::bit_cast<std::uint32_t>(1.0f / 60.0f);
			file.settings = planned(count);
			file.settings.begin_input_claims(input_slots::keyboard_and_mouse);
			for (const int slot : { 2, 0, 3 })
			{
				if (!file.settings.input_roster_complete())
				{ require(file.settings.claim_input_slot(slot), "Cannot claim replay fixture human"); }
			}
			for (int seat = 1; seat < count; ++seat)
			{
				if ((bot_mask & (1 << seat)) != 0) { file.settings.set_player_bot(seat, difficulty); }
			}
			file.settings.set_screen_layout(MatchPlanner::layout_for(file.settings.human_count()));
			file.updates = 2;
			file.final_print.updates = 2;
			for (int seat = 0; seat < count; ++seat)
			{
				PlayerInputData input;
				input.connected = true;
				input.primary_shoot = seat % 2 == 0;
				input.jump_pressed = seat % 2 != 0;
				input.x_movement = seat == 0 ? -0.0f : static_cast<float>(seat) / 4.0f;
				input.shoot_direction = { 0.25f, -0.5f };
				file.inputs.push_back({ input, PlayerInputData() });
			}
			return file;
		}

		void require_same_recording(const ReplayFile& expected, const ReplayFile& actual)
		{
			require(expected.settings.player_count() == actual.settings.player_count() &&
				expected.settings.screen_layout() == actual.settings.screen_layout() &&
				expected.settings.stage() == actual.settings.stage() && expected.updates == actual.updates &&
				expected.inputs.size() == actual.inputs.size(), "Bot replay header or participant count changed");
			for (std::size_t seat = 0; seat < expected.settings.player_settings().size(); ++seat)
			{
				const SeatSetup& source = expected.settings.player_settings()[seat];
				const SeatSetup& read = actual.settings.player_settings()[seat];
				require(source.controller == read.controller && source.num == read.num && source.team == read.team &&
					source.weapon == read.weapon && read.difficulty == (source.controller == PlayerController::bot ?
						source.difficulty : BotDifficulty::normal), "Bot replay changed seat identity/difficulty or choices");
				require(expected.inputs[seat].size() == actual.inputs[seat].size(), "Bot replay lost recorded input steps");
				for (std::size_t step = 0; step < expected.inputs[seat].size(); ++step)
				{
					require(replay_codec::same(expected.inputs[seat][step], actual.inputs[seat][step]),
						"Bot replay changed a final recorded participant input");
				}
			}
		}
	}

	void check_bot_replay_metadata()
	{
		ReplayScratch scratch;
		const std::filesystem::path path = scratch.directory / L"recording.json";
		for (int count = 1; count <= 4; ++count)
		{
			for (int mask = 0; mask < (1 << count); mask += 2)
			{
				for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
				{
					const ReplayFile file = bot_recording(count, mask, difficulty);
					file.write(path);
					require_same_recording(file, ReplayFile::read(path));
				}
			}
		}

		ReplayFile sparse = bot_recording(4, 1 << 2, BotDifficulty::hard);
		require(sparse.settings.release_input_slot(2), "Cannot make the sparse human replay gap");
		sparse.write(path);
		require_same_recording(sparse, ReplayFile::read(path));
		require(ReplayFile::read(path).settings.player_settings()[1].controller == PlayerController::human,
			"The unclaimed human beside a bot became a bot");

		ReplayFile converted = bot_recording(2, 1 << 1, BotDifficulty::hard);
		converted.settings.set_player_human(1);
		converted.settings.set_screen_layout(labrador::ScreenLayout::two_player);
		converted.write(path);
		require_same_recording(converted, ReplayFile::read(path));

		const ReplayFile humans = bot_recording(3, 0, BotDifficulty::normal);
		humans.write(path);
		std::string legacy = replace_once(read_text(path), "\"version\": 4", "\"version\": 3");
		const std::string metadata = ", \"controller\": \"human\", \"difficulty\": \"normal\"";
		while (legacy.find(metadata) != std::string::npos) { legacy = replace_once(legacy, metadata, ""); }
		write_text(path, legacy);
		require_same_recording(humans, ReplayFile::read(path));

		ReplayFile mixed = bot_recording(3, 1 << 1, BotDifficulty::hard);
		mixed.write(path);
		const std::string valid = read_text(path);
		const auto rejects = [&](const std::string& bytes, const std::string& reason)
		{
			write_text(path, bytes);
			std::string error;
			try { ReplayFile::read(path); } catch (const std::exception& failure) { error = failure.what(); }
			require(!error.empty(), "Malformed bot replay was accepted: " + reason);
		};
		rejects(replace_once(valid, "\"controller\": \"human\"", "\"controller\": \"bot\""), "bot host");
		rejects(replace_once(valid, "\"controller\": \"bot\"", "\"controller\": \"other\""), "unknown controller");
		rejects(replace_once(valid, "\"controller\": \"bot\", ", ""), "missing controller");
		rejects(replace_once(valid, ", \"difficulty\": \"hard\"", ""), "missing difficulty");
		rejects(replace_once(valid, "\"difficulty\": \"hard\"", "\"difficulty\": false"), "wrong difficulty type");
		rejects(replace_once(valid, "\"difficulty\": \"hard\"", "\"difficulty\": \"expert\""), "unknown difficulty");
		rejects(replace_once(valid, "\"difficulty\": \"normal\"", "\"difficulty\": \"easy\""), "human difficulty");
		rejects(replace_once(valid, "\"slot\": -1", "\"slot\": 1"), "bot with device");
		rejects(replace_once(valid, "\"slot\": 0", "\"slot\": 4"), "duplicate human device");
		rejects(replace_once(valid, "\"slot\": 0", "\"slot\": 5"), "out-of-range human device");
		rejects(replace_once(valid, "\"slot\": 0", "\"slot\": -2"), "invalid unclaimed human device");
		rejects(replace_once(valid, "\"layout\": 1", "\"layout\": 2"), "participant layout instead of human layout");
		rejects(replace_once(valid, "\"version\": 4", "\"version\": 3"), "new metadata in old schema");

		write_text(path, "existing recording");
		mixed.settings.set_screen_layout(labrador::ScreenLayout::three_player);
		std::string write_error;
		try { mixed.write(path); } catch (const std::exception& failure) { write_error = failure.what(); }
		require(!write_error.empty() && read_text(path) == "existing recording",
			"Invalid bot metadata truncated an existing recording");
		const std::filesystem::path absent = scratch.directory / L"absent" / L"recording.json";
		write_error.clear();
		try { mixed.write(absent); } catch (const std::exception& failure) { write_error = failure.what(); }
		require(!write_error.empty() && !std::filesystem::exists(absent.parent_path()),
			"Invalid bot metadata created a replay directory");
	}

	void check_bot_replay_inputs(const Level& level)
	{
		const ReplayFile expected = bot_recording(3, 1 << 1, BotDifficulty::hard);
		class FinalInputs final : public SeatInputFilter
		{
		public:
			explicit FinalInputs(const ReplayFile& file) : file_(file) {}
			void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
			{
				for (std::size_t seat = 0; seat < seats.size(); ++seat)
				{ seats[seat] = this->file_.inputs[seat][this->step_]; }
				++this->step_;
			}
		private:
			const ReplayFile& file_;
			std::size_t step_ = 0;
		};
		FinalInputs final_inputs(expected);
		ReplayFile header = expected;
		header.inputs.clear();
		ReplayRecorder recorder("mixed", header, &final_inputs, 2);
		std::vector<PlayerInputData> seats(3);
		for (int step = 0; step < 2; ++step) { recorder.filter_seats(seats, level); }
		require_same_recording(expected, recorder.file());
		ReplayPlayer player("mixed", recorder.file());
		for (std::size_t step = 0; step < 3; ++step)
		{
			for (PlayerInputData& input : seats) { input.toggle_debug = true; input.primary_shoot = true; }
			player.filter_seats(seats, level);
			for (std::size_t seat = 0; seat < seats.size(); ++seat)
			{
				PlayerInputData end;
				end.connected = true;
				require(replay_codec::same(seats[seat], step < 2 ? expected.inputs[seat][step] : end),
					"Replay did not replace live human/bot inputs or hold neutral at recording end");
			}
		}
	}
}
#endif
