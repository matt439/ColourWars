#include "game/pch.h"
#include "game/config/play_settings.h"
#include "game/config/presentation/presentation.h"
#include "game/config/stage_catalog.h"
#include "game/config/stage_loading.h"
#include "game/config/tuning.h"
#include "game/contracts/level_format.h"
#include "game/directory_consts.h"
#include "game/match/match_setup.h"
#include "game/objects/level_end_info.h"
#include "game/objects/stage.h"
#include "game/save/last_match_setup.h"
#include "game/save/save.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/isolated_save.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"
#include "scripts/acceptance/support/stage_catalog.h"
#include "scripts/acceptance/support/stderr_capture.h"

#include <array>
#include <atomic>
#include <chrono>
#include <crtdbg.h>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

// REV#62: the two readers that take files from outside the build, fed files
// nobody wrote by hand. Every shipped level and three saves are mutated with
// a fixed seed and a fixed count, and each mutant goes through the game
// library's own doors: load_user_stages for a level, Save::load_save_file for
// a save. A mutant must load or be refused. A crash, an exception the reader's
// contract does not allow, or a hang fails the case.
//
// Deterministic: the generator is the SplitMix64 below with a constant seed,
// and a mutant depends on its source and iteration alone, never on the clock
// or on the mutants before it. A failure names both, so it can be rebuilt.
//
// Private: one IsolatedSave root under %TEMP% holds the save and the stage
// folder. The player's save and stage folder are never opened, and nothing is
// written beside the shipped levels.
//
// Hangs: a watchdog thread in this process fails the case when one mutant
// takes longer than mutant_deadline, naming it; the runner's own deadline
// (-RunTimeoutSeconds) is only the backstop. Crashes: the unhandled-exception
// filter names the mutant, and the runner fails on the exit code.
//
// Not covered: no Level is built or stepped from an accepted mutant, and the
// replay codec is out of scope.

using namespace labrador;

namespace
{
	using acceptance::require;

	constexpr std::uint64_t seed = 0x5245563632ull;	// "REV62"
	// Mutants per source; iteration 0 of each is the source itself.
	constexpr int level_iterations = 64;
	constexpr int save_iterations = 200;
	constexpr std::chrono::seconds mutant_deadline(120);

	// Depths past the level format's limit: just over it, and far past where a
	// recursive parser overflows. A level is refused by the format's own
	// nesting limit before a parser sees it; a save reaches the engine's
	// iterative parse, which loads it or refuses it (engine report 36).
	constexpr std::size_t deep_nesting[] = { level_format::limit::max_nesting, level_format::limit::max_nesting + 1,
		100, 1000, 10000, 100000 };

	struct Random
	{
		std::uint64_t state;

		static std::uint64_t mix(std::uint64_t value)
		{
			value += 0x9e3779b97f4a7c15ull;
			value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
			value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
			return value ^ (value >> 31);
		}

		// The generator of one mutant: its source and iteration, nothing else.
		Random(std::size_t source, int iteration) :
			state(mix(mix(seed ^ mix(source)) ^ static_cast<std::uint64_t>(iteration)))
		{
		}

		std::uint64_t next()
		{
			this->state += 0x9e3779b97f4a7c15ull;
			std::uint64_t value = this->state;
			value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
			value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
			return value ^ (value >> 31);
		}

		// 0 for an empty range.
		std::size_t below(std::size_t count)
		{
			return count == 0 ? 0 : static_cast<std::size_t>(this->next() % count);
		}
	};

	enum class Operator { none, bytes, truncate, number, remove_key, duplicate_key, nest, count };

	constexpr std::array<const char*, static_cast<std::size_t>(Operator::count)> operator_names{
		"unchanged", "byte edits", "truncation", "number replaced", "key removed", "key duplicated", "nesting" };

	// What a number token is replaced with: the edges of the game's int and
	// float, of the level format's limits (level_format::limit), spellings
	// JSON does not allow, and values of another type.
	const char* const replacement_numbers[] = {
		"0", "-0", "-0.0", "-1", "1", "0.0000001", "1e-320", "0.5", "16777217", "65536", "2147483647", "2147483648",
		"-2147483648", "-2147483649", "4294967295", "4294967296", "9223372036854775807", "9223372036854775808",
		"18446744073709551615", "18446744073709551616", "3.4e38", "3.5e38", "-3.5e38", "1e39", "-1e39", "1e308",
		"1e309", "-1e309", "999999999999999999999999999999", "0.00000000000000000000000000000000000000000000001",
		// level_format::limit: coordinates and extents at 1048576, paint at
		// 524288, objects at 512; each side of each.
		"511", "512", "513", "4096", "100000", "524287", "524288", "524289", "1048575", "1048576", "1048576.5",
		"1048577", "-1048576", "-1048577", "2097152", "1e6", "1e7",
		"NaN", "nan", "Infinity", "-Infinity", "0x10", "1e", "--1", "01", "1.", ".5", "+1",
		"null", "true", "false", "\"12\"", "\"\"", "[]", "{}", "[1]"
	};

	// Where each JSON number token lies in `text`, outside strings.
	std::vector<std::pair<std::size_t, std::size_t>> number_tokens(std::string_view text)
	{
		std::vector<std::pair<std::size_t, std::size_t>> tokens;
		bool in_string = false;
		for (std::size_t index = 0; index < text.size(); ++index)
		{
			const char character = text[index];
			if (in_string)
			{
				if (character == '\\') { ++index; }
				else if (character == '"') { in_string = false; }
				continue;
			}
			if (character == '"') { in_string = true; continue; }
			if (character == '-' || (character >= '0' && character <= '9'))
			{
				std::size_t end = index + 1;
				while (end < text.size() && (text[end] == '-' || text[end] == '+' || text[end] == '.' ||
					text[end] == 'e' || text[end] == 'E' || (text[end] >= '0' && text[end] <= '9')))
				{
					++end;
				}
				tokens.emplace_back(index, end);
				index = end - 1;
			}
		}
		return tokens;
	}

	void objects_under(rapidjson::Value& value, std::vector<rapidjson::Value*>& found)
	{
		if (value.IsObject())
		{
			if (value.MemberCount() > 0) { found.push_back(&value); }
			for (auto& member : value.GetObject()) { objects_under(member.value, found); }
		}
		else if (value.IsArray())
		{
			for (auto& item : value.GetArray()) { objects_under(item, found); }
		}
	}

	// Removes one member of one object, or gives one object a second member
	// under a name it already has: its own value again or another member's,
	// placed first (the one FindMember answers with) or last.
	std::string edit_keys(const std::string& source, Random& random, bool duplicate)
	{
		rapidjson::Document document;
		document.Parse(source.data(), source.size());
		require(!document.HasParseError(), "A mutation source is not valid JSON");
		std::vector<rapidjson::Value*> objects;
		objects_under(document, objects);
		require(!objects.empty(), "A mutation source has no object with a member");
		auto& allocator = document.GetAllocator();
		rapidjson::Value& object = *objects[random.below(objects.size())];
		const auto chosen = object.MemberBegin() + static_cast<std::ptrdiff_t>(random.below(object.MemberCount()));
		if (!duplicate)
		{
			object.EraseMember(chosen);
		}
		else
		{
			const auto other = object.MemberBegin() + static_cast<std::ptrdiff_t>(random.below(object.MemberCount()));
			rapidjson::Value name(chosen->name, allocator);
			rapidjson::Value value(random.below(2) == 0 ? chosen->value : other->value, allocator);
			if (random.below(2) == 0)
			{
				object.AddMember(name, value, allocator);
			}
			else
			{
				rapidjson::Value rebuilt(rapidjson::kObjectType);
				rebuilt.AddMember(name, value, allocator);
				for (auto& member : object.GetObject()) { rebuilt.AddMember(member.name, member.value, allocator); }
				object = rebuilt;
			}
		}
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		require(document.Accept(writer), "Cannot encode a key mutant");
		return { buffer.GetString(), buffer.GetSize() };
	}

	// Wraps one number, or a new root member nothing reads, in arrays or
	// objects: as often 1 to the level format's limit of them as one of
	// deep_nesting.
	std::string nest(const std::string& source, Random& random)
	{
		const std::size_t depth = random.below(2) == 0 ?
			deep_nesting[random.below(std::size(deep_nesting))] : 1 + random.below(level_format::limit::max_nesting);
		const bool arrays = random.below(2) == 0;
		std::string open, close;
		for (std::size_t level = 0; level < depth; ++level)
		{
			open += arrays ? "[" : "{\"a\":";
			close += arrays ? "]" : "}";
		}
		const auto tokens = number_tokens(source);
		const std::size_t root = source.find('{');
		if (random.below(2) == 0 || tokens.empty())
		{
			require(root != std::string::npos, "A mutation source has no root object");
			return source.substr(0, root + 1) + "\"zz_nested\":" + open + "0" + close + "," + source.substr(root + 1);
		}
		const auto [begin, end] = tokens[random.below(tokens.size())];
		return source.substr(0, begin) + open + source.substr(begin, end - begin) + close + source.substr(end);
	}

	struct Mutant
	{
		Operator made = Operator::none;
		std::string bytes;
	};

	enum class Kind { level, save };
	const char* const kind_names[] = { "level", "save" };

	Mutant mutate(const std::string& source, std::size_t source_index, int iteration)
	{
		if (iteration == 0) { return { Operator::none, source }; }
		Random random(source_index, iteration);
		Mutant mutant{ static_cast<Operator>(1 + random.below(static_cast<std::size_t>(Operator::count) - 1)), source };
		switch (mutant.made)
		{
		case Operator::bytes:
		{
			const std::size_t edits = 1 + random.below(4);
			for (std::size_t edit = 0; edit < edits && !mutant.bytes.empty(); ++edit)
			{
				char& byte = mutant.bytes[random.below(mutant.bytes.size())];
				byte = random.below(2) == 0 ? static_cast<char>(byte ^ (1 << random.below(8))) :
					static_cast<char>(random.below(256));
			}
			break;
		}
		case Operator::truncate:
			mutant.bytes.resize(random.below(mutant.bytes.size()));
			break;
		case Operator::number:
		{
			const auto tokens = number_tokens(source);
			require(!tokens.empty(), "A mutation source has no number");
			const auto [begin, end] = tokens[random.below(tokens.size())];
			mutant.bytes = source.substr(0, begin) +
				replacement_numbers[random.below(std::size(replacement_numbers))] + source.substr(end);
			break;
		}
		case Operator::remove_key: mutant.bytes = edit_keys(source, random, false); break;
		case Operator::duplicate_key: mutant.bytes = edit_keys(source, random, true); break;
		case Operator::nest:
			mutant.bytes = nest(source, random);
			break;
		default: break;
		}
		return mutant;
	}

	// The mutant in hand, for the watchdog and the crash filter; a negative
	// iteration is a named regression input. `report` is the probe's own
	// stderr, kept from before any capture.
	std::atomic<int> current_phase{ -1 };
	std::atomic<int> current_source{ 0 };
	std::atomic<int> current_iteration{ 0 };
	std::atomic<long long> current_started{ 0 };
	int report = -1;
	long long ticks()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	void begin_mutant(Kind kind, std::size_t source, int iteration)
	{
		current_source = static_cast<int>(source);
		current_iteration = iteration;
		current_started = ticks();
		current_phase = static_cast<int>(kind);
	}

	void say(const char* what)
	{
		const int phase = current_phase.load();
		char line[256]{};
		const int length = std::snprintf(line, sizeof(line),
			"FAIL reader mutations: %s on %s source %d, iteration %d\n", what,
			phase < 0 ? "no" : kind_names[phase], current_source.load(), current_iteration.load());
		if (report >= 0 && length > 0) { _write(report, line, static_cast<unsigned>(length)); }
	}

	LONG WINAPI crashed(EXCEPTION_POINTERS*)
	{
		say("crashed");
		return EXCEPTION_EXECUTE_HANDLER;
	}

	class Watchdog
	{
	public:
		Watchdog() : thread_([this]
		{
			while (!this->stop_.load())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (current_phase.load() >= 0 && ticks() - current_started.load() >
					std::chrono::duration_cast<std::chrono::milliseconds>(mutant_deadline).count())
				{
					say("the per-mutant deadline passed");
					std::_Exit(1);
				}
			}
		})
		{
		}
		~Watchdog() { this->stop_ = true; this->thread_.join(); }

	private:
		std::atomic<bool> stop_{ false };
		std::thread thread_;
	};

	// Nothing here may wait on a person: no fault dialog, no abort dialog and
	// no Debug CRT assertion dialog, which would sit modal until the deadline.
	void fail_without_dialogs()
	{
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#ifdef _DEBUG
		for (const int kind : { _CRT_ASSERT, _CRT_ERROR })
		{
			_CrtSetReportMode(kind, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(kind, _CRTDBG_FILE_STDERR);
		}
#endif
		SetUnhandledExceptionFilter(crashed);
	}

	// Sends stdout's descriptor to NUL while it lives, as StderrCapture sends
	// stderr's to a file.
	class QuietStdout
	{
	public:
		QuietStdout()
		{
			std::cout.flush();
			std::fflush(stdout);
			int nul = -1;
			require(_sopen_s(&nul, "NUL", _O_WRONLY, _SH_DENYNO, 0) == 0 && nul >= 0, "Cannot open NUL");
			this->saved_ = _dup(_fileno(stdout));
			const bool redirected = this->saved_ >= 0 && _dup2(nul, _fileno(stdout)) == 0;
			_close(nul);
			if (!redirected)
			{
				if (this->saved_ >= 0) { _close(this->saved_); this->saved_ = -1; }
				throw std::runtime_error("Cannot quiet stdout");
			}
		}
		QuietStdout(const QuietStdout&) = delete;
		QuietStdout& operator=(const QuietStdout&) = delete;
		~QuietStdout() { this->restore(); }

		void finish() { require(this->restore(), "Cannot restore the probe's stdout"); }

	private:
		bool restore() noexcept
		{
			if (this->saved_ < 0) { return true; }
			std::cout.flush();
			std::fflush(stdout);
			const bool restored = _dup2(this->saved_, _fileno(stdout)) == 0;
			_close(this->saved_); this->saved_ = -1;
			return restored;
		}

		int saved_ = -1;
	};

	struct Source
	{
		std::string name;
		std::string bytes;
	};

	struct Tally
	{
		// Indexed by operator: mutants that loaded, and mutants refused.
		std::array<int, static_cast<std::size_t>(Operator::count)> loaded{};
		std::array<int, static_cast<std::size_t>(Operator::count)> refused{};

		void print(const char* loaded_word, const char* refused_word) const
		{
			for (std::size_t made = 0; made < operator_names.size(); ++made)
			{
				std::cout << "  " << operator_names[made] << ": " << this->loaded[made] << ' ' << loaded_word
					<< ", " << this->refused[made] << ' ' << refused_word << '\n';
			}
		}

		int total(bool of_loaded) const
		{
			int sum = 0;
			for (std::size_t made = 1; made < operator_names.size(); ++made)
			{
				sum += of_loaded ? this->loaded[made] : this->refused[made];
			}
			return sum;
		}
	};

	// Runs `check` on every mutant of every source. A failed check keeps the
	// mutant beside this executable and names it.
	template <class Check>
	Tally sweep(Kind kind, const std::vector<Source>& sources, int iterations,
		const std::filesystem::path& evidence, Check check)
	{
		Tally tally;
		for (std::size_t index = 0; index < sources.size(); ++index)
		{
			for (int iteration = 0; iteration < iterations; ++iteration)
			{
				const Mutant mutant = mutate(sources[index].bytes, index, iteration);
				const std::size_t made = static_cast<std::size_t>(mutant.made);
				begin_mutant(kind, index, iteration);
				try
				{
					const bool loaded = check(mutant.bytes);
					require(loaded || iteration != 0, "the unchanged source was refused");
					++(loaded ? tally.loaded : tally.refused)[made];
				}
				catch (const std::exception& failure)
				{
					current_phase = -1;
					const char* const name = kind_names[static_cast<int>(kind)];
					const std::filesystem::path kept = evidence / ("reader_mutations-failed-" +
						std::string(name) + "-" + sources[index].name + "-" + std::to_string(iteration) + ".json");
					acceptance::write_text(kept, mutant.bytes);
					throw std::runtime_error(std::string(name) + " source " + std::to_string(index) +
						" (" + sources[index].name + "), iteration " + std::to_string(iteration) + " (" +
						operator_names[made] + "): " + failure.what() + "; the mutant is " + acceptance::utf8_path(kept));
				}
				current_phase = -1;
			}
		}
		return tally;
	}

	// A populated save, written by the game's own writer.
	std::string populated_save(const acceptance::IsolatedSave& isolated, const std::string& stage)
	{
		Save save;
		save.load_save_file();
		bool saved = false;
		PlayerProfile profile;
		profile.name = "ALEX";
		profile.boost = 1;
		save.add_profile_and_save(profile, saved);
		require(saved, "Cannot seed a profile");
		LastMatchSetup setup;
		setup.players = { { PlayerTeam::b, WeaponType::bucket }, { PlayerTeam::a, WeaponType::mister } };
		setup.stage = stage;
		setup.layout = ScreenLayout::two_player;
		setup.duration = 98.5f;
		setup.best_of = 3;
		require(save.remember_match_setup_and_save(setup), "Cannot seed a remembered match");
		LevelEndInfo result;
		result.total_area = 200.0f;
		result.team_a_area = 120.0f;
		result.team_b_area = 60.0f;
		save.record_result_and_save(stage, result);
		require(save.set_display_settings_and_save(ScreenResolution::s_1280_720, true, TeamColourPair::blue_yellow),
			"Cannot seed display settings");
		return isolated.save_bytes();
	}

	// The flat version-0 layout, which the reader still takes.
	const char* const flat_save =
		"{\"resolution\":{\"x\":1280,\"y\":720},\"fullscreen\":true,"
		"\"team_palette\":\"blue_yellow\",\"aim_sensitivity\":4,"
		"\"pad_sticks\":[{\"movement_deadzone\":10,\"aim_deadzone\":15},"
		"{\"movement_deadzone\":20,\"aim_deadzone\":25},"
		"{\"movement_deadzone\":30,\"aim_deadzone\":35},"
		"{\"movement_deadzone\":40,\"aim_deadzone\":45}],"
		"\"music_volume\":20,\"sfx_volume\":80,\"view_shake\":false,"
		"\"aim_marker\":false,\"reduce_effects\":true}";
}

int main(int argc, char** argv)
{
	return acceptance::run_probe("reader mutations", [&]
	{
		require(argc == 2, "Usage: reader_mutations.exe <staged game directory>");
		report = _dup(_fileno(stderr));
		fail_without_dialogs();
		const std::filesystem::path content(argv[1]);
		const std::filesystem::path evidence = std::filesystem::absolute(argv[0]).parent_path();

		const acceptance::IsolatedSave isolated(L"reader-mutations");
		require(path_from_utf8(directory_consts::save_file_path()) == isolated.save_path(),
			"The save is not the probe's private one");
		const std::filesystem::path stage_folder = isolated.root() / "mutant-stages";
		std::filesystem::create_directory(stage_folder);

		acceptance::ContentWorld world(content);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		const Presentation presentation((content / "presentation.json").string().c_str());
		presentation.validate_resources(world.render, world.audio);
		const PlaySettings play_settings;
		const UserStageChecks checks{ &tuning, &presentation, &world.render, &world.audio, &play_settings };
		StageCatalog catalog = acceptance::manifest_catalog(content);
		const std::size_t shipped = catalog.stages().size();

		std::vector<Source> levels;
		for (const acceptance::ManifestEntry& level : acceptance::manifest_level_paths(content))
		{
			levels.push_back({ level.name, acceptance::read_text(level.path) });
		}

		const Watchdog watchdog;

		// A level mutant is the one file of the stage folder. The scan's
		// contract (stage_loading.h): it never throws for what is in the
		// folder, and a file it leaves out is one line naming it.
		const std::string refusal = "user stage refused: mutant.json: ";
		const auto scan_level = [&](const std::string& bytes)
		{
			acceptance::write_text(stage_folder / "mutant.json", bytes);
			UserStageScan scan;
			try
			{
				scan = load_user_stages(stage_folder, checks, catalog);
			}
			catch (const std::exception& escaped)
			{
				throw std::runtime_error(std::string("load_user_stages threw: ") + escaped.what());
			}
			catch (...)
			{
				throw std::runtime_error("load_user_stages threw something that is not a std::exception");
			}
			require(scan.accepted.size() + scan.refused.size() == 1, "The scan did not answer once for one file");
			const bool accepted = !scan.accepted.empty();
			if (accepted)
			{
				require(scan.accepted[0] == "mutant" && catalog.stages().size() == shipped + 1 &&
					catalog.definition("mutant") != nullptr, "An accepted mutant is not the listed stage");
			}
			else
			{
				const std::string& line = scan.refused[0];
				require(line.compare(0, refusal.size(), refusal) == 0 && line.size() > refusal.size() &&
					line.find('\n') == std::string::npos && line.find('\r') == std::string::npos,
					"A refusal is not one line naming the file and a reason: " + line);
				require(catalog.stages().size() == shipped, "A refused mutant joined the list");
			}
			catalog.remove_user_stages();
			require(catalog.stages().size() == shipped, "The shipped list changed");
			return accepted;
		};

		// Named inputs, kept from what this probe first found and run before
		// the sweep with the same check. Each must be refused.
		//   deep arrays, deep objects: before the level format had a nesting
		//   limit, a level nested this deeply overflowed the stack in the
		//   reader's recursive parse (0xC00000FD) instead of being refused.
		const std::string& first_level = levels.front().bytes;
		const std::size_t root = first_level.find('{');
		require(root != std::string::npos, "The first level has no root object");
		const auto nested_member = [&](const std::string& open, const std::string& close)
		{
			std::string opens, closes;
			for (int level = 0; level < 100000; ++level) { opens += open; closes += close; }
			return first_level.substr(0, root + 1) + "\"zz_nested\":" + opens + "0" + closes + "," +
				first_level.substr(root + 1);
		};
		const std::pair<const char*, std::string> regressions[] = {
			{ "deep arrays", nested_member("[", "]") },
			{ "deep objects", nested_member("{\"a\":", "}") },
		};
		for (std::size_t index = 0; index < std::size(regressions); ++index)
		{
			begin_mutant(Kind::level, 0, -1 - static_cast<int>(index));
			require(!scan_level(regressions[index].second),
				std::string("The regression input '") + regressions[index].first + "' was accepted");
			current_phase = -1;
		}
		std::cout << "regressions: " << std::size(regressions) << " named level inputs refused\n";

		const Tally level_tally = sweep(Kind::level, levels, level_iterations, evidence, scan_level);
		std::cout << "levels: " << levels.size() << " sources, " << level_iterations << " mutants each\n";
		level_tally.print("accepted", "refused");
		require(level_tally.total(true) > 0 && level_tally.total(false) > 0,
			"The level sweep must both accept and refuse mutants, or it tests nothing");

		// Save reports each unreadable file and section on stderr and each
		// write on stdout: thousands of lines, kept out of the case's log.
		acceptance::StderrCapture quiet(isolated.root() / "save-stderr.txt");
		QuietStdout quiet_writes;

		// The save seeds: the defaults a first run writes, a populated save
		// and the flat version-0 layout. Only this first load finds no file;
		// every mutant is on disk before its load, so the missing-file branch,
		// whose failed write raises a dialog, is never reached with one.
		std::vector<Source> saves;
		{
			Save first;
			first.load_save_file();
			saves.push_back({ "defaults", isolated.save_bytes() });
		}
		saves.push_back({ "populated", populated_save(isolated, catalog.stages().front().name) });
		saves.push_back({ "flat", flat_save });

		// A save mutant replaces the private save. Loading never throws: an
		// unreadable file is copied aside and the defaults stand. Whatever was
		// loaded must then be usable by what reads it first, and must write
		// back as a file the reader takes whole.
		const std::filesystem::path aside = isolated.save_path().parent_path() / L"save_data.unreadable-1.json";
		const Tally save_tally = sweep(Kind::save, saves, save_iterations, evidence, [&](const std::string& bytes)
		{
			std::error_code ignored;
			isolated.write_save(bytes);
			Save save;
			try
			{
				save.load_save_file();
			}
			catch (const std::exception& escaped)
			{
				throw std::runtime_error(std::string("Save::load_save_file threw: ") + escaped.what());
			}
			catch (...)
			{
				throw std::runtime_error("Save::load_save_file threw something that is not a std::exception");
			}
			const bool kept_aside = std::filesystem::exists(aside);
			std::filesystem::remove(aside, ignored);
			try
			{
				Save::checked_play_settings(save.saved_play_settings());
				save.saved_input_bindings().validate();
				save.profiles().validate();
				if (save.last_match_setup()) { save.last_match_setup()->restore(catalog.stages()); }
			}
			catch (const std::exception& invalid)
			{
				throw std::runtime_error(std::string("a loaded save holds what its first reader refuses: ") + invalid.what());
			}
			// A save set aside left the defaults, which every other case writes.
			if (kept_aside) { return false; }
			require(save.save_to_file(), "A loaded save could not be written back");
			Save again;
			again.load_save_file();
			const bool unreadable_again = std::filesystem::exists(aside);
			std::filesystem::remove(aside, ignored);
			require(!unreadable_again, "The file written from a loaded save could not be read");
			return true;
		});
		quiet_writes.finish();
		quiet.finish();
		std::cout << "saves: " << saves.size() << " sources, " << save_iterations << " mutants each\n";
		save_tally.print("loaded", "set aside");
		require(save_tally.total(true) > 0 && save_tally.total(false) > 0,
			"The save sweep must both load and set aside mutants, or it tests nothing");

		std::cout << "PASS reader mutations: " << levels.size() * level_iterations << " level mutants through "
			"load_user_stages and " << saves.size() * save_iterations << " save mutants through Save, seed 0x"
			<< std::hex << seed << std::dec << "; each loaded or was refused, none crashed, threw past its reader or outlasted "
			<< mutant_deadline.count() << " s.\n";
		return 0;
	});
}
