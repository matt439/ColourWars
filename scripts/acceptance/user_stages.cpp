#include "game/pch.h"
#include "game/states/menu_content_validation.h"
#include "game/config/content_validation.h"
#include "game/directory_consts.h"
#include "game/objects/stage.h"
#include "game/config/play_settings.h"
#include "game/config/presentation/presentation.h"
#include "game/config/sound_groups.h"
#include "game/config/stage_catalog.h"
#include "game/config/stage_loading.h"
#include "game/config/stage_scan_diagnostics.h"
#include "game/states/menu_context.h"
#include "game/config/tuning.h"
#include "game/contracts/level_format.h"
#include "game/objects/level_object_builder.h"
#include "game/contracts/user_stage_contract.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/file_path.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/content_world.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/scratch.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <vector>

// ROAD#16: stages from the player's own folder, each validated on its own, a
// broken one refused with one line while the rest load. Every folder here is
// a scratch folder beside this executable; the real LOCALAPPDATA is never
// read or written (the directory check points LOCALAPPDATA at scratch first).

using namespace labrador;

namespace
{
	using acceptance::require;

	void check_name_contract()

	{
		// Baseline first: this is a pure string check, before creating scratch or
		// constructing any reserved destination. Never exercise a device path.
		require(!user_stage_contract::usable_name("CON"), "Reserved Windows stage name was accepted");
		const char* reserved[] = { "CON", "PRN", "AUX", "NUL",
			"COM0", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
			"LPT0", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
		for (const char* name : reserved)
		{
			std::string lower(name), mixed(name);
			for (std::size_t index = 0; index < lower.size(); ++index)
			{
				if (lower[index] >= 'A' && lower[index] <= 'Z')
				{
					lower[index] = static_cast<char>(lower[index] + ('a' - 'A'));
					if (index % 2 != 0) { mixed[index] = lower[index]; }
				}
			}
			for (const std::string& spelling : { std::string(name), lower, mixed })
			{
				require(!user_stage_contract::usable_name(spelling),
					"A reserved Windows stage stem was accepted: " + spelling);
			}
		}
		// COM0 and LPT0 are refused like the registration helper refuses them
		// (F5: a user stage registers unchanged), so its records follow it.
		// These are identifier checks only, especially COM10/LPT10; no path or
		// filesystem operation is used to decide what Windows would open.
		for (const char* name : { "console", "console_map", "nul_map", "con_1", "prn1", "auxiliary",
			"COM10", "LPT10", "my_con", "_lpt9", "lpt_9", "A0b9_-", "a-", "_", "con_", "nul-stage", "COM1_copy" })
		{
			require(user_stage_contract::usable_name(name), "A valid neighboring stage identifier was refused: " + std::string(name));
		}
		require(user_stage_contract::usable_name(std::string(64, 'a')) &&
			!user_stage_contract::usable_name(std::string(65, 'a')), "Stage name length boundaries changed");
		for (const std::string& name : { std::string(), std::string("-arena"), std::string("bad name"),
			std::string("arena.v1"), std::string("arena/name"), std::string("caf\xC3\xA9"), std::string(1, '\0') })
		{
			require(!user_stage_contract::usable_name(name), "Existing ASCII/leading-character stage-name refusal changed");
		}
		const std::string rule = user_stage_contract::name_rule();
		std::string explanation = rule;
		for (char& character : explanation)
			if (character >= 'A' && character <= 'Z') { character = static_cast<char>(character + ('a' - 'A')); }
		require(explanation.find("ascii") != std::string::npos && explanation.find("64") != std::string::npos &&
			(explanation.find("reserv") != std::string::npos || explanation.find("device") != std::string::npos),
			"The shared name-rule diagnostic does not explain reserved stems and existing character limits");
		std::cout << "PASS stage name contract: 24 reserved ASCII stems in three cases, valid neighbors, "
			"length/ASCII/leading rules and explanatory diagnostics; no reserved path I/O\n";
	}

	struct World
	{
		RenderResources render;
		AudioResources audio;

		explicit World(const std::filesystem::path& content)
		{
			acceptance::load_cpu_assets(content, render, audio);
		}
	};

	// What wWinMain's "level" kind produces from the staged manifest.
	struct Shipped
	{
		StageCatalog catalog;
		const Registry<LevelLoadedInfo>& levels = catalog.level_infos();
		const StageList& stages = catalog.stages();
		// How many stages the manifest lists: the user stages follow them.
		size_t count = 0;
	};

	void load_shipped(const std::filesystem::path& content, Shipped& shipped)
	{
		for (const acceptance::ManifestEntry& level : acceptance::manifest_level_paths(content))
		{
			shipped.catalog.add(read_stage_definition(path_to_utf8(level.path), level.name));
		}
		shipped.count = shipped.stages.size();
	}

	using acceptance::read_document;



	std::string with_source_metadata(const std::string& stage, const std::string& member)
	{
		const std::size_t end = stage.find_last_of('}');
		require(end != std::string::npos, "Complete-source fixture needs the shipped stage's closing root");
		return stage.substr(0, end) + ',' + member + stage.substr(end);
	}

	void check_runtime_source(const std::filesystem::path& content, const std::filesystem::path& scratch)
	{
		const auto original = content / "levels/close_quarters.json";
		const std::string stage = acceptance::read_text(original);
		const auto path = scratch / "complete-source.json";
		const std::string name = path_to_utf8(path);
		acceptance::write_text(path, stage);
		const LevelLoadedInfo pristine(name.c_str());
		pristine.validate();
		const auto refusal = [&](const std::string& bytes)
		{
			acceptance::write_text(path, bytes);
			std::string diagnostic;
			try { const LevelLoadedInfo loaded(name.c_str()); }
			catch (const std::runtime_error& error) { diagnostic = error.what(); }
			require(acceptance::read_text(path) == bytes, "Runtime admission changed its private source bytes");
			return diagnostic;
		};
		const auto named_offset = [&](const std::string& diagnostic, std::size_t offset)
		{
			require(diagnostic.find(name) != std::string::npos &&
				diagnostic.find("(offset " + std::to_string(offset) + ')') != std::string::npos,
				"Runtime source refusal lost its source or original-snapshot byte offset: " + diagnostic);
		};

		// First new regression: a real complete stage was previously accepted as
		// its prefix when the bounded input contained a raw NUL and more bytes.
		const std::string hidden_suffix = stage + std::string(1, '\0') + R"({"hidden":true})";
		const std::string baseline = refusal(hidden_suffix);
		require(!baseline.empty(), "Runtime accepted a raw NUL JSON suffix.");
		named_offset(baseline, stage.size());
		for (const std::string& prefix : { std::string(), std::string("\xef\xbb\xbf") })
		{
			for (const std::string& suffix : { std::string(1, '\0'),
				std::string(" \r\n") + '\0' + "trailing bytes" })
			{
				const std::string bytes = prefix + stage + suffix;
				const std::string diagnostic = refusal(bytes);
				require(diagnostic.find("NUL") != std::string::npos, "Runtime did not identify a raw NUL suffix");
				named_offset(diagnostic, bytes.find('\0'));
			}
		}

		const std::array<std::string, 6> malformed{ std::string("\x80"), std::string("\xc0\xaf"),
			std::string("\xc2"), std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80"), std::string("\xff") };
		std::size_t malformed_cases = 0;
		for (const std::string& prefix : { std::string(), std::string("\xef\xbb\xbf") })
		{
			for (const std::string& invalid : malformed)
			{
				for (bool key : { false, true })
				{
					const std::string member = key ? "\"unknown-" + invalid + "\":true" :
						"\"unknown\":{\"text\":\"" + invalid + "\"}";
					const std::string bytes = prefix + with_source_metadata(stage, member);
					const std::string diagnostic = refusal(bytes);
					require(diagnostic.find("encoding") != std::string::npos,
						"Runtime accepted malformed UTF-8 in an unknown value or key");
					named_offset(diagnostic, prefix.size() + stage.find_last_of('}') + 1 + member.find(invalid));
					++malformed_cases;
				}
			}
		}

		// Preserve diagnostics already owned by the engine syntax parser, even
		// when a later byte would also fail the new complete-source check.
		for (const std::string& bytes : { std::string("{\"format\":2,}"),
			std::string("{\"format\":1,\"text\":\"") + '\0' + "\"}",
			std::string("{\"format\":2,\"unknown\":\"") + malformed[1] + "\",}" })
		{
			std::string expected;
			try { const auto document = parse_json(bytes, name); }
			catch (const std::runtime_error& error) { expected = error.what(); }
			require(!expected.empty() && refusal(bytes) == expected,
				"Complete-source validation replaced an earlier engine syntax diagnostic");
		}
		for (const std::string& format : { std::string("2"), std::string("true") })
		{
			const std::string base = "{\"format\":" + format + '}';
			const std::string expected = refusal(base);
			// A raw NUL is a syntax error the engine parser owns (v0.3.0), so it
			// outranks the format check, like the syntax cases above.
			const std::string nul_bytes = base + '\0' + "suffix";
			std::string nul_expected;
			try { const auto document = parse_json(nul_bytes, name); }
			catch (const std::runtime_error& error) { nul_expected = error.what(); }
			require(expected.find("format") != std::string::npos &&
				!nul_expected.empty() && refusal(nul_bytes) == nul_expected &&
				refusal("{\"format\":" + format + ",\"unknown\":\"" + malformed[1] + "\"}") == expected,
				"Complete-source validation changed unsupported/malformed format diagnostic precedence");
		}

		const auto hash = [&](const std::string& bytes)
		{
			acceptance::write_text(path, bytes);
			const LevelLoadedInfo loaded(name.c_str());
			loaded.validate();
			require(acceptance::read_text(path) == bytes, "Loading a valid snapshot changed its bytes");
			return loaded.semantic_hash();
		};
		const std::string metadata = with_source_metadata(stage,
			std::string(R"("unknown":{"text":"A\u0000B","utf8":"caf)") + "\xc3\xa9" +
			R"(","number":1,"order":{"first":true,"second":null}})");
		const std::string equivalent = with_source_metadata(stage,
			R"("unknown":{"text":"\u0041\u0000B","utf8":"caf\u00e9","number":1e0,"order":{"first":true,"second":null}})");
		const std::uint64_t valid_hash = hash(metadata);
		require(hash(equivalent) == valid_hash && hash("\xef\xbb\xbf" + equivalent + " \r\n\t") == valid_hash &&
			hash(stage) == hash("\xef\xbb\xbf" + stage),
			"Complete-source admission changed valid UTF-8/BOM/escaped-NUL/equal-number semantic meaning");
		require(hash(with_source_metadata(stage,
			R"("unknown":{"text":"A\u0000B","utf8":"caf\u00e9","number":1,"order":{"second":null,"first":true}})")) != valid_hash,
			"Runtime fingerprint lost authored unknown-member order");
		acceptance::write_text(path, metadata);
		const LevelLoadedInfo cached(name.c_str());
		acceptance::write_text(path, hidden_suffix);
		require(cached.semantic_hash() == valid_hash && !cached.display_name().empty(),
			"An admitted runtime definition reread subsequently malformed disk bytes");
		require(acceptance::read_text(original) == stage, "Runtime source checks changed the staged input asset");
		std::cout << "PASS complete runtime source: raw NUL suffixes, " << malformed_cases <<
			" malformed UTF-8 key/value/BOM cases, original offsets, syntax/format precedence, "
			"valid decoded/hash equivalence and cached private snapshot\n";
	}

	template<class Edit>
	void write_level(const rapidjson::Document& base, const std::filesystem::path& path, Edit edit)
	{
		rapidjson::Document document;
		document.CopyFrom(base, document.GetAllocator());
		edit(document);
		acceptance::write_document(document, path);
	}

	void set_string(rapidjson::Document& document, rapidjson::Value& owner, const char* key, const char* value)
	{
		owner[key].SetString(value, document.GetAllocator());
	}

	std::filesystem::path fresh(const std::filesystem::path& path)
	{
		std::filesystem::remove_all(path);
		std::filesystem::create_directories(path);
		return path;
	}

	struct Fixture
	{
		World* world;
		const GameTuning* tuning;
		const Presentation* presentation;
		PlaySettings play_settings;

		UserStageChecks checks() const
		{
			return UserStageChecks{ tuning, presentation, &world->render,
				&world->audio, &play_settings };
		}
	};

	UserStageScan scan(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& folder, Shipped& shipped)
	{
		load_shipped(content, shipped);
		return load_user_stages(folder, fixture.checks(), shipped.catalog);
	}

	bool refused(const UserStageScan& result, const std::string& file, const std::string& reason)
	{
		const std::string prefix = "user stage refused: " + file + ": ";
		return std::any_of(result.refused.begin(), result.refused.end(), [&](const std::string& line)
		{
			return line.rfind(prefix, 0) == 0 && line.find(reason, prefix.size()) != std::string::npos;
		});
	}

	void check_runtime_source_scan(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		const auto folder = fresh(scratch / "complete-source-scan");
		const std::string stage = acceptance::read_text(content / "levels/close_quarters.json");
		acceptance::write_text(folder / "a-valid.json", stage);
		acceptance::write_text(folder / "b-nul.json", stage + '\0' + "unparsed suffix");
		acceptance::write_text(folder / "c-value.json", with_source_metadata(stage, "\"unknown\":\"\xc0\xaf\""));
		acceptance::write_text(folder / "d-key.json", with_source_metadata(stage, "\"unknown-\xed\xa0\x80\":true"));
		Shipped shipped;
		const UserStageScan result = scan(fixture, content, folder, shipped);
		require(result.accepted == std::vector<std::string>{ "a-valid" } && result.refused.size() == 3 &&
			shipped.stages.size() == shipped.count + 1 && shipped.levels.contains("a-valid") &&
			!shipped.levels.contains("b-nul") && !shipped.levels.contains("c-value") && !shipped.levels.contains("d-key"),
			"Mixed runtime source scan lost its valid sibling or published malformed definitions");
		require(refused(result, "b-nul.json", "NUL") && refused(result, "c-value.json", "encoding") &&
			refused(result, "d-key.json", "encoding"), "Mixed runtime source scan lost per-file refusal causes");
		for (const std::string& diagnostic : result.refused)
		{
			require(diagnostic.find("(offset ") != std::string::npos &&
				diagnostic.find('\n') == std::string::npos && diagnostic.find('\r') == std::string::npos,
				"Mixed runtime source refusal lost its byte location or one-line boundary");
		}
		std::cout << "PASS complete runtime source scan: valid sibling retained, raw NUL and malformed "
			"UTF-8 value/key refused independently before registry publication\n";
	}

	void check_directory(const std::filesystem::path& scratch)
	{
		// Point LOCALAPPDATA at scratch, never the player's profile.
		const std::filesystem::path profile = fresh(scratch / "profile");
		require(_wputenv_s(L"LOCALAPPDATA", profile.c_str()) == 0, "Cannot isolate LOCALAPPDATA");
		require(path_from_utf8(directory_consts::user_stage_directory()) == profile / "ColourWars" / "stages",
			"User stages must live in %LOCALAPPDATA%\\ColourWars\\stages, beside the save");
		require(path_from_utf8(directory_consts::save_file_path()).parent_path() ==
			path_from_utf8(directory_consts::user_stage_directory()).parent_path(),
			"The user stage folder must sit beside the save");
		require(!std::filesystem::exists(profile / "ColourWars"), "Asking for the folder must not create it");
		std::cout << "PASS user stage folder is %LOCALAPPDATA%\\ColourWars\\stages and is not created on lookup\n";
	}

	void check_empty(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		{
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, scratch / "no-such-folder", shipped);
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == shipped.count,
				"A missing user stage folder must add nothing and say nothing");
		}
		{
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, fresh(scratch / "empty"), shipped);
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == shipped.count,
				"An empty user stage folder must add nothing and say nothing");
		}
		{
			const std::filesystem::path file = scratch / "not-a-folder";
			std::filesystem::remove_all(file);
			acceptance::write_text(file, "x");
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, file, shipped);
			require(result.accepted.empty() && result.refused.size() == 1 &&
				result.refused.front().find("is not a folder") != std::string::npos && shipped.stages.size() == shipped.count,
				"A file where the folder should be must be one refusal line and no stages");
		}
		std::cout << "PASS missing, empty and non-folder user stage paths leave the shipped stages\n";
	}

	void check_mixed(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		// A non-ASCII folder name: paths cross the engine as UTF-8.
		const std::filesystem::path folder = fresh(scratch / L"stäges");
		const rapidjson::Document base = read_document(content / "levels" / "close_quarters.json");

		write_level(base, folder / "my_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "My Arena");
			// Asks for Random, so it joins it; zz-second leaves the key out
			// and, unlike a shipped stage without it, stays explicit-only.
			d.AddMember("include_in_random", true, d.GetAllocator());
		});
		write_level(base, folder / "zz-second.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "Second Of Mine");
		});
		acceptance::write_text(folder / "broken.json", "{ \"display_name\": \"Broken\", ");
		write_level(base, folder / "no_spawns.json", [](rapidjson::Document& d)
		{
			d.RemoveMember("spawn_locations");
		});
		write_level(base, folder / "missing_frame.json", [](rapidjson::Document& d)
		{
			set_string(d, d["collision_objects"][0], "frame_name", "no_such_frame_anywhere");
		});
		write_level(base, folder / "loud_music.json", [](rapidjson::Document& d)
		{
			// match.go is a match effect in the SFX group; as music it would
			// sit under two volume settings.
			set_string(d, d["sound"], "music_name", "match.go");
		});
		write_level(base, folder / "Crossfeed.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "Not Crossfeed");
		});
		write_level(base, folder / "bad name.json", [](rapidjson::Document&) {});
		write_level(base, folder / "-leading.json", [](rapidjson::Document&) {});
		write_level(base, folder / "emoji_name.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "Paint \xF0\x9F\x8E\xA8");
		});
		acceptance::write_text(folder / "notes.txt", "not a stage");
		std::filesystem::create_directories(folder / "nested.json");

		Shipped shipped;
		const UserStageScan result = scan(fixture, content, folder, shipped);
		require(result.accepted == std::vector<std::string>{ "my_arena", "zz-second" },
			"Exactly the two valid user stages must load, in file-name order");
		require(result.refused.size() == 8, "Each of the eight bad files must be refused once; got " +
			std::to_string(result.refused.size()));
		for (const std::string& line : result.refused)
		{
			require(line.find('\n') == std::string::npos && line.find('\r') == std::string::npos,
				"A refusal must be one line: " + line);
			std::cout << "  " << line << '\n';
		}
		require(refused(result, "broken.json", "broken.json"), "Broken JSON was not refused by file");
		require(refused(result, "no_spawns.json", "spawn_locations"), "A level without spawns was not refused by key");
		require(refused(result, "missing_frame.json", "no_such_frame_anywhere"),
			"A missing sprite frame was not refused by name");
		require(refused(result, "loud_music.json", "match.go"), "A sound-group conflict was not refused");
		require(refused(result, "Crossfeed.json", "a shipped stage's ('crossfeed')"),
			"A case-only collision with a shipped stage was not refused");
		require(refused(result, "bad name.json", "file name"), "A file name with a space was not refused");
		require(refused(result, "-leading.json", "file name"), "A leading hyphen was not refused");
		require(refused(result, "emoji_name.json", "display_name"), "An undrawable display name was not refused");

		require(shipped.stages.size() == shipped.count + 2, "Stage Select must offer the shipped stages and two user stages");
		for (size_t index = 0; index < shipped.count; ++index)
		{
			require(!shipped.stages[index].user, "Shipped stages must stay first and unmarked");
		}
		const Stage& mine = shipped.stages[shipped.count];
		require(mine.name == "my_arena" && mine.display_name == "My Arena" && mine.user && mine.include_in_random,
			"A user stage asking for Random must follow the shipped ones, marked and a Random candidate");
		// menu_text.stage_names names only shipped stages: a user stage's
		// title is its own display_name, which no locale reaches.
		require(fixture.presentation->menu_text().stage_name("my_arena") == nullptr &&
			fixture.presentation->menu_text().stage_title(mine.name, mine.display_name, "my_arena") == L"My Arena",
			"A user stage's title must be its own display_name");
		require(shipped.stages[shipped.count + 1].name == "zz-second" && shipped.stages[shipped.count + 1].user &&
			!shipped.stages[shipped.count + 1].include_in_random,
			"The second user stage must follow the first, explicit-only without include_in_random");
		require(shipped.levels.contains("my_arena") && shipped.levels.contains("zz-second") &&
			!shipped.levels.contains("broken") && !shipped.levels.contains("Crossfeed"),
			"Only accepted user stages may reach the level registry");
		require(shipped.levels.get("crossfeed")->display_name() == "Crossfeed",
			"A colliding user stage must not replace the shipped stage");
		// The opted-in user stage joins the shipped Random pool, which the
		// hint counts; the one without the key does not.
		const StageList shipped_only(shipped.stages.begin(), shipped.stages.begin() + static_cast<std::ptrdiff_t>(shipped.count));
		const std::vector<size_t> shipped_random = random_stage_indices(shipped_only);
		std::vector<size_t> expected_random = shipped_random;
		expected_random.push_back(shipped.count);
		require(!shipped_random.empty() && random_stage_indices(shipped.stages) == expected_random,
			"Only the user stage asking for Random may join the Random pool, after the shipped candidates");

		// The startup path after the scan: the whole list, user stages
		// included, passes the same checks and joins the sound groups.
		validate_shell_content(*fixture.tuning, *fixture.presentation, &fixture.world->render,
			&fixture.world->audio, shipped.levels, shipped.stages, fixture.play_settings);
		AudioResources audio;
		audio.add_sound_bank("paint_exe", SoundBank::silent());
		sound_groups::assign(audio, *fixture.tuning, *fixture.presentation, shipped.levels, shipped.stages);
		std::cout << "PASS a folder of ten files loads its two valid stages after the shipped stages and refuses "
			"broken JSON, a missing key, a missing frame, a sound conflict, a shipped-name collision, "
			"two bad file names and an undrawable name, one line each\n";
	}

	// presentation.json's menu_text.stage_names against the stage list
	// (validate_stage_names) and its English against the levels
	// (check_stage_names_agree): exactly the shipped stages, each its level's
	// display_name; user stages never in it.
	void check_stage_names(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		Shipped shipped;
		load_shipped(content, shipped);
		validate_stage_names(*fixture.presentation, shipped.stages);
		const std::string base_path = (content / "presentation.json").string();
		check_stage_names_agree(base_path, shipped.stages);
		const auto refuses = [](const char* what, std::initializer_list<const char*> fragments, auto action)
		{
			std::string message = "accepted";
			try { action(); }
			catch (const std::exception& error)
			{
				message = error.what();
				bool all = true;
				for (const char* fragment : fragments) { all = all && message.find(fragment) != std::string::npos; }
				if (all) { return; }
			}
			throw std::runtime_error(std::string(what) + " was not refused as expected: " + message);
		};
		const auto edited = [&](const char* name, auto edit)
		{
			rapidjson::Document document = read_document(content / "presentation.json");
			edit(document);
			const std::filesystem::path path = scratch / (std::string("stage-names-") + name + ".json");
			acceptance::write_document(document, path);
			return path.string();
		};
		const std::string missing = edited("missing",
			[](rapidjson::Document& d) { d["menu_text"]["stage_names"].RemoveMember("crossfeed"); });
		refuses("A shipped stage stage_names lacks", { "menu_text.stage_names", "'crossfeed'", "manifest" },
			[&] { validate_stage_names(Presentation(missing.c_str()), shipped.stages); });
		const std::string extra = edited("extra", [](rapidjson::Document& d)
			{ d["menu_text"]["stage_names"].AddMember("nowhere", "Nowhere", d.GetAllocator()); });
		refuses("A stage_names entry the manifest lacks", { "menu_text.stage_names", "'nowhere'", "not a stage" },
			[&] { validate_stage_names(Presentation(extra.c_str()), shipped.stages); });
		// Startup's whole check refuses both before the first menu.
		refuses("validate_shell_content with a missing stage name", { "'crossfeed'" }, [&]
			{
				validate_shell_content(*fixture.tuning, Presentation(missing.c_str()), &fixture.world->render,
					&fixture.world->audio, shipped.levels, shipped.stages, fixture.play_settings);
			});

		StageList listed = shipped.stages;
		// A user stage is never in the table, and an entry for one is refused.
		listed.push_back({ "my_arena", "My Arena", "stage_close_quarters", false, true });
		validate_stage_names(*fixture.presentation, listed);
		const std::string user_named = edited("user", [](rapidjson::Document& d)
			{ d["menu_text"]["stage_names"].AddMember("my_arena", "My Arena", d.GetAllocator()); });
		refuses("A stage_names entry for a user stage", { "'my_arena'", "not a stage" },
			[&] { validate_stage_names(Presentation(user_named.c_str()), listed); });
		StageList hidden = shipped.stages;
		hidden.push_back({ "_hidden", "Hidden", "stage_close_quarters", true, false });
		refuses("A shipped stage named like a comment", { "'_hidden'", "'_'" },
			[&] { validate_stage_names(*fixture.presentation, hidden); });

		// The base English must be the level's display_name, byte for byte.
		StageList drifted = shipped.stages;
		const auto crossfeed = std::find_if(drifted.begin(), drifted.end(),
			[](const Stage& stage) { return stage.name == "crossfeed"; });
		require(crossfeed != drifted.end(), "The stage-names fixture needs Crossfeed");
		crossfeed->display_name = "Crossfeed ";
		refuses("A stage_names entry that differs from its level", { "'crossfeed'", "'Crossfeed'", "'Crossfeed '",
			"levels/crossfeed.json", "agree" }, [&] { check_stage_names_agree(base_path, drifted); });
		// A user stage's own display_name is never compared.
		check_stage_names_agree(base_path, listed);
		std::cout << "PASS stage names: presentation.json names exactly the shipped stages, each its level's "
			"display_name; a missing, extra, user or '_' entry and a drifted display_name are refused\n";
	}

	void check_user_collision(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		// Two user stages differing only in case: the first by file name wins.
		// NTFS folders are case-insensitive, so this needs a second folder scan.
		const std::filesystem::path first = fresh(scratch / "first");
		const std::filesystem::path second = fresh(scratch / "second");
		const rapidjson::Document base = read_document(content / "levels" / "close_quarters.json");
		write_level(base, first / "Twin.json", [](rapidjson::Document&) {});
		write_level(base, second / "twin.json", [](rapidjson::Document&) {});
		Shipped shipped;
		const UserStageScan once = scan(fixture, content, first, shipped);
		const UserStageScan twice = load_user_stages(second, fixture.checks(), shipped.catalog);
		require(once.accepted == std::vector<std::string>{ "Twin" } && twice.accepted.empty() &&
			refused(twice, "twin.json", "another user stage's ('Twin')") && shipped.stages.size() == shipped.count + 1,
			"A user stage may not take another user stage's name");
		std::cout << "PASS a user stage cannot take another user stage's name\n";
	}

	std::vector<std::string> names(const StageList& stages)
	{
		std::vector<std::string> result;
		for (const Stage& stage : stages) { result.push_back(stage.name); }
		return result;
	}

	void check_rescan(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		// What Stage Select does as it opens from the main menu: reread the
		// folder in place of the user stages, leaving the shipped stages alone.
		const std::filesystem::path folder = fresh(scratch / "rescan");
		const rapidjson::Document base = read_document(content / "levels" / "close_quarters.json");
		Shipped shipped;
		load_shipped(content, shipped);
		const std::vector<std::string> manifest = names(shipped.stages);

		UserStageFolder unattached;
		require(unattached.scan().accepted.empty() && unattached.rescan().accepted.empty() &&
			!unattached.attached() && unattached.latest_scan().refused.empty(),
			"An unattached folder (every probe's GameMenu) must do nothing");

		UserStageFolder session;
		session.attach(folder, fixture.checks(), &fixture.world->audio, &shipped.catalog);
		write_level(base, folder / "b_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "B Arena");
		});
		require(session.scan().accepted == std::vector<std::string>{ "b_arena" } && shipped.stages.size() == manifest.size() + 1,
			"The startup scan must load the folder's one stage");
		require(session.latest_scan().accepted == std::vector<std::string>{ "b_arena" } &&
			session.latest_scan().refused.empty(), "Startup scan details were not retained");

		// Saved while the game runs: a new stage that sorts first, an edit and
		// a broken file.
		write_level(base, folder / "a_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "A Arena");
			d.AddMember("include_in_random", false, d.GetAllocator());
		});
		write_level(base, folder / "b_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "B Arena Edited");
		});
		acceptance::write_text(folder / "broken.json", "{");
		const UserStageScan second = session.rescan();
		require(session.latest_scan().accepted == second.accepted && session.latest_scan().refused == second.refused,
			"The retained rescan outcome lost a refused file");
		require(second.accepted == std::vector<std::string>{ "a_arena", "b_arena" } &&
			second.refused.size() == 1 && refused(second, "broken.json", "broken.json"),
			"A rescan must add a new stage, reread an edited one and refuse a broken one");
		std::vector<std::string> expected = manifest;
		expected.push_back("a_arena");
		expected.push_back("b_arena");
		require(names(shipped.stages) == expected,
			"After a rescan the shipped stages stay first and user stages follow in file-name order");
		require(shipped.stages[manifest.size() + 1].display_name == "B Arena Edited" &&
			shipped.levels.get("b_arena")->display_name() == "B Arena Edited" && shipped.stages[manifest.size() + 1].user &&
			!shipped.stages[manifest.size() + 1].include_in_random, "An edited user stage must play its new definition");
		require(shipped.stages[manifest.size()].name == "a_arena" && !shipped.stages[manifest.size()].include_in_random,
			"A user stage saying include_in_random false must stay explicit-only");

		// Nothing changed: the same list, and the same refusal.
		const UserStageScan same = session.rescan();
		require(session.latest_scan().refused == second.refused, "An unchanged refusal disappeared from details");
		require(same.accepted == second.accepted && same.refused == second.refused &&
			names(shipped.stages) == expected, "A rescan with no change must give the same list");

		// One file deleted and the other broken since: both leave the list.
		std::filesystem::remove(folder / "a_arena.json");
		write_level(base, folder / "b_arena.json", [](rapidjson::Document& d)
		{
			d.RemoveMember("spawn_locations");
		});
		const UserStageScan third = session.rescan();
		require(third.accepted.empty() && third.refused.size() == 2 &&
			refused(third, "b_arena.json", "spawn_locations") && names(shipped.stages) == manifest,
			"A vanished or now-broken user stage must leave the list, keeping the shipped stages");
		require(shipped.levels.get("crossfeed")->display_name() == "Crossfeed",
			"A rescan must not touch a shipped definition");
		require(session.latest_scan().refused == third.refused, "Changed refusal details were not refreshed");

		const MenuContext ownerless(nullptr, nullptr, nullptr);
		require(ownerless.stage_scan() == nullptr, "Unattached menu context gained scan details");
		// The main menu's owner lends the folder; this one counts its rescans.
		struct FolderOwner final : MenuOwner
		{
			explicit FolderOwner(UserStageFolder& stage_folder) : user_stages(&stage_folder) {}
			UserStageFolder* user_stages;
			int rescans = 0;
			bool detached = false;
			void update_menu_music(float) override {}
			void rescan_stages() override { ++this->rescans; }
			const UserStageScan* stage_scan() const override
			{
				return this->detached ? nullptr : &this->user_stages->latest_scan();
			}
		};
		FolderOwner owner(session);
		MenuContext menu(nullptr, nullptr, nullptr, &owner);
		const int& rescans = owner.rescans;
		require(menu.stage_scan() == &session.latest_scan() && menu.stage_scan()->refused == third.refused && rescans == 0,
			"Reading scan details performed a rescan or lost the session result");
		menu.rescan_stages();
		require(rescans == 1, "Details callback changed the existing explicit rescan boundary");

		write_level(base, folder / "b_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "B Arena Fixed");
		});
		const UserStageScan fixed = session.rescan();
		require(fixed.accepted == std::vector<std::string>{ "b_arena" } && fixed.refused.size() == 1 &&
			session.latest_scan().refused == fixed.refused && shipped.levels.get("b_arena")->display_name() == "B Arena Fixed",
			"Fixing a refused stage did not update the retained details and playable definition");
		std::filesystem::remove(folder / "b_arena.json");
		std::filesystem::remove(folder / "broken.json");
		const UserStageScan cleared = session.rescan();
		require(cleared.accepted.empty() && cleared.refused.empty() && menu.stage_scan()->refused.empty() &&
			names(shipped.stages) == manifest, "A clean rescan retained stale refusal details");
		owner.detached = true;
		require(menu.stage_scan() == nullptr, "Detaching scan details left a stale result");
		// The whole list still passes what startup checks.
		validate_shell_content(*fixture.tuning, *fixture.presentation, &fixture.world->render,
			&fixture.world->audio, shipped.levels, shipped.stages, fixture.play_settings);
		std::cout << "PASS a rescan adds a new user stage, rereads an edited one and drops a deleted or broken one, "
			"keeping the shipped stages first\n";
	}

	// REV#2: the format's upper limits (level_format::limit). A stage past one
	// is refused with one line naming the rule, the same reader refuses it as a
	// shipped stage, and a folder past its file limit is read for its first
	// files by name.
	void check_limits(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		namespace limit = level_format::limit;
		const std::filesystem::path folder = fresh(scratch / "limits");
		const rapidjson::Document base = read_document(content / "levels" / "close_quarters.json");
		const std::string stage = acceptance::read_text(content / "levels" / "close_quarters.json");
		// close_quarters' floor: a StructurePaintable with a material and its top face on.
		constexpr rapidjson::SizeType floor_index = 4;
		require(std::string(base["collision_objects"][floor_index]["type"].GetString()) == "StructurePaintable" &&
			base["collision_objects"][floor_index].HasMember("material") &&
			base["collision_objects"][floor_index]["paintable_faces"]["top"].GetBool(),
			"The limits fixture needs close_quarters' paintable floor");

		write_level(base, folder / "wide_bounds.json", [](rapidjson::Document& d)
		{
			// The record's case: a width typed with four extra digits.
			d["level_bounds"]["out_of_bounds"]["width"].SetDouble(60000000.0);
		});
		write_level(base, folder / "wide_object.json", [&](rapidjson::Document& d)
		{
			// Every edge inside the coordinate limit; only the width is over.
			d["collision_objects"][floor_index]["rectangle"]["x"].SetDouble(-limit::max_coordinate + 1000.0);
			d["collision_objects"][floor_index]["rectangle"]["width"].SetDouble(limit::max_object_extent + 500.0);
		});
		write_level(base, folder / "far_object.json", [&](rapidjson::Document& d)
		{
			// An ordinary width a billion units out, where a float's spacing
			// is wider than the terrain outline's step.
			d["collision_objects"][floor_index]["rectangle"]["x"].SetDouble(1.0e9);
		});
		write_level(base, folder / "far_ramp.json", [&](rapidjson::Document& d)
		{
			rapidjson::Value ramp(rapidjson::kObjectType);
			ramp.CopyFrom(d["collision_objects"][0], d.GetAllocator());
			ramp["type"].SetString("StructureRamp");
			ramp["collision_type"].SetString("STRUCTURE_RAMP_RIGHT");
			ramp["material"].SetString("set_square");
			ramp.RemoveMember("rectangle");
			rapidjson::Value triangle(rapidjson::kObjectType);
			triangle.AddMember("x1", 1.0e9, d.GetAllocator()); triangle.AddMember("y1", 5000.0, d.GetAllocator());
			triangle.AddMember("x2", 1.0e9 + 512.0, d.GetAllocator()); triangle.AddMember("y2", 5000.0, d.GetAllocator());
			triangle.AddMember("x3", 1.0e9 + 512.0, d.GetAllocator()); triangle.AddMember("y3", 4800.0, d.GetAllocator());
			ramp.AddMember("triangle", triangle, d.GetAllocator());
			d["collision_objects"].PushBack(ramp, d.GetAllocator());
		});
		const auto decorations = [](rapidjson::Document& d, std::size_t count)
		{
			rapidjson::Value& visuals = d["non_collision_objects"];
			while (visuals.Size() > count) { visuals.PopBack(); }
			while (visuals.Size() < count)
			{
				rapidjson::Value copy(rapidjson::kObjectType);
				copy.CopyFrom(visuals[0], d.GetAllocator());
				visuals.PushBack(copy, d.GetAllocator());
			}
		};
		write_level(base, folder / "many_objects.json",
			[&](rapidjson::Document& d) { decorations(d, limit::max_objects + 1); });
		write_level(base, folder / "most_objects.json",
			[&](rapidjson::Document& d) { decorations(d, limit::max_objects); });
		write_level(base, folder / "long_paint.json", [&](rapidjson::Document& d)
		{
			// One face inside the object limit and past the paint limit.
			const double width = (limit::max_paint_length + limit::max_object_extent) / 2.0;
			require(width > limit::max_paint_length && width < limit::max_object_extent,
				"The paint limit must sit below the object limit for this fixture");
			d["collision_objects"][floor_index]["rectangle"]["x"].SetDouble(-width / 2.0);
			d["collision_objects"][floor_index]["rectangle"]["width"].SetDouble(width);
		});
		require(stage.size() < limit::max_file_bytes, "The shipped stage must be inside the file limit");
		acceptance::write_text(folder / "file_at_limit.json",
			stage + std::string(limit::max_file_bytes - stage.size(), ' '));
		acceptance::write_text(folder / "file_over_limit.json",
			stage + std::string(limit::max_file_bytes + 1 - stage.size(), ' '));

		// A member nothing reads, nested under the root object: the root is
		// one level, so this many arrays reach the limit and one more passes
		// it. The third is far past where the recursive parsers overflow.
		const auto nested = [&stage](std::size_t arrays)
		{
			const std::size_t root = stage.find('{');
			require(root != std::string::npos, "The shipped stage has no root object");
			return stage.substr(0, root + 1) + "\"zz_nested\":" + std::string(arrays, '[') +
				std::string(arrays, ']') + "," + stage.substr(root + 1);
		};
		acceptance::write_text(folder / "nested_at_limit.json", nested(limit::max_nesting - 1));
		acceptance::write_text(folder / "nested_over_limit.json", nested(limit::max_nesting));
		acceptance::write_text(folder / "nested_deep.json", nested(100000));

		Shipped shipped;
		const UserStageScan result = scan(fixture, content, folder, shipped);
		for (const std::string& line : result.refused)
		{
			require(line.rfind("user stage refused: ", 0) == 0 && line.find('\n') == std::string::npos &&
				line.find('\r') == std::string::npos, "A limit refusal must be one refusal line: " + line);
			std::cout << "  " << line << '\n';
		}
		require(result.accepted == std::vector<std::string>{ "file_at_limit", "most_objects", "nested_at_limit" } &&
			result.refused.size() == 9 && shipped.stages.size() == shipped.count + 3,
			"A stage at the file, object and nesting limits must each load, and the nine past a limit must not; got " +
			std::to_string(result.accepted.size()) + " accepted and " + std::to_string(result.refused.size()) + " refused");
		const std::string coordinate = "coordinate limit of +/-" + limit::whole(limit::max_coordinate);
		const std::string extent = "wider or taller than the level format's limit of " + limit::whole(limit::max_bounds_extent);
		require(refused(result, "wide_bounds.json", "level_bounds") && refused(result, "wide_bounds.json", coordinate),
			"Over-wide bounds were not refused naming level_bounds and the limit");
		require(refused(result, "wide_object.json", "collision_objects") && refused(result, "wide_object.json", extent),
			"An over-size object was not refused naming collision_objects and the limit");
		require(refused(result, "far_object.json", "collision_objects") && refused(result, "far_object.json", coordinate),
			"An object far from the origin was not refused naming collision_objects and the limit");
		require(refused(result, "far_ramp.json", "triangle") && refused(result, "far_ramp.json", coordinate),
			"A ramp far from the origin was not refused naming its triangle and the limit");
		require(refused(result, "many_objects.json", "non_collision_objects") &&
			refused(result, "many_objects.json", "holds " + std::to_string(limit::max_objects + 1) + " objects"),
			"Too many objects were not refused naming the array and its count");
		require(refused(result, "long_paint.json", "collision_objects") &&
			refused(result, "long_paint.json", limit::whole(limit::max_paint_length) + " world units of paint"),
			"Too much paintable face was not refused naming the paint limit");
		require(refused(result, "file_over_limit.json", std::to_string(limit::max_file_bytes) + " bytes"),
			"A file one byte over the limit was not refused naming the limit");
		const std::string nesting = "more than " + std::to_string(limit::max_nesting) + " deep";
		require(refused(result, "nested_over_limit.json", nesting) && refused(result, "nested_deep.json", nesting),
			"A file nested past the limit was not refused naming the limit");

		// The manifest's reader (wWinMain's "level" kind): a shipped stage past
		// a limit throws, which fails startup like any content bug. The object
		// and paint limits throw from the content check every stage then gets.
		const LevelObjectBuilder builder(&fixture.world->render, &fixture.play_settings, &fixture.tuning->paint());
		for (const char* name : { "wide_bounds", "many_objects", "file_over_limit", "wide_object", "far_object",
			"far_ramp", "long_paint", "nested_over_limit", "nested_deep" })
		{
			bool thrown = false;
			try
			{
				const StageDefinition definition = read_stage_definition(
					path_to_utf8(folder / (std::string(name) + ".json")), name);
				validate_stage_objects(*fixture.tuning, builder, fixture.world->render, fixture.world->audio,
					*definition.info);
			}
			catch (const std::runtime_error&) { thrown = true; }
			require(thrown, std::string("A shipped stage past a limit did not fail its startup checks: ") + name);
		}

		// One file more than the folder limit: the first by name are read and
		// the last is neither loaded nor refused by name.
		const std::filesystem::path crowded = fresh(scratch / "limits-count");
		acceptance::write_text(crowded / "a-first.json", stage);
		for (std::size_t index = 0; index + 2 < limit::max_user_stage_files + 1; ++index)
		{
			acceptance::write_text(crowded / ("m-" + std::to_string(1000 + index) + ".json"), "{");
		}
		acceptance::write_text(crowded / "zz-last.json", stage);
		Shipped counted;
		const UserStageScan count = scan(fixture, content, crowded, counted);
		const std::size_t files = limit::max_user_stage_files + 1;
		require(count.accepted == std::vector<std::string>{ "a-first" } &&
			count.refused.size() == limit::max_user_stage_files && !counted.levels.contains("zz-last"),
			"A folder one file past the limit must load its first stage, refuse the broken files it read "
			"and leave the last file unread");
		require(refused(count, path_to_utf8(crowded), "holds " + std::to_string(files) + " .json files") &&
			refused(count, path_to_utf8(crowded), "first " + std::to_string(limit::max_user_stage_files) + " by file name"),
			"The folder line must give the folder's count and the limit");
		require(std::none_of(count.refused.begin(), count.refused.end(),
			[](const std::string& line) { return line.find("zz-last") != std::string::npos; }),
			"The file past the folder limit must not be refused by name");
		std::cout << "PASS format limits: bounds, object size and distance, ramp distance, object count, paint length, "
			"file size and nesting refused with one line each and by the manifest's reader; a stage at the object, file "
			"and nesting limits loads; a folder of " << files << " files is read for its first " << limit::max_user_stage_files << "\n";
	}

	void check_diagnostics(const Fixture& fixture)
	{
		using stage_scan_diagnostics::display_text;
		using stage_scan_diagnostics::pages;
		require(display_text("broken.json: missing spawn_locations") == L"broken.json: missing spawn_locations",
			"Printable refusal copy changed");
		require(display_text("caf\xC3\xA9.json: \xF0\x9F\x8E\xA8") == L"caf\\u00E9.json: \\uD83C\\uDFA8",
			"A non-ASCII filename or supplementary character was replaced instead of escaped");
		require(display_text(std::string("\xC3(", 2)) == L"\\xC3(" &&
			display_text(std::string("a\0b\n", 4)) == L"a\\u0000b\\u000A",
			"Malformed UTF-8 or diagnostic controls lost their byte/unit explanation");
		std::vector<Glyph> glyphs;
		for (char32_t character = U' '; character <= U'~'; ++character)
		{
			glyphs.push_back({ character, labrador::RectangleI(0, 0, 10, 8), 0.0f, 0.0f, 0.0f });
		}
		const Font fixed(TextureHandle{}, std::move(glyphs), 10.0f);
		const Font proportional(TextureHandle{}, std::vector<Glyph>{
			{ U'a', labrador::RectangleI(0, 0, 1, 8), 0.0f, 0.0f, 0.0f },
			{ U' ', labrador::RectangleI(0, 0, 1, 8), 0.0f, 0.0f, 0.0f },
			{ U'b', labrador::RectangleI(0, 0, 3, 8), 0.0f, 0.0f, 0.5f },
			{ U'X', labrador::RectangleI(0, 0, 5, 8), 0.0f, 0.0f, 0.0f } }, 10.0f);
		require(pages(proportional, "a bbX", 10.0f, 10.0f) ==
			std::vector<std::wstring>{ L"a ", L"bb", L"X" },
			"A wide final proportional glyph left an overwide suffix after word wrapping");
		require(pages(fixed, "abcdefgh", 20.0f, 20.0f) ==
			std::vector<std::wstring>{ L"ab\ncd", L"ef\ngh" }, "Exact diagnostic width/height edges did not paginate inclusively");
		require(pages(fixed, "", 20.0f, 20.0f) == std::vector<std::wstring>{ L"" },
			"An empty diagnostic did not provide a safe empty page");
		for (const auto area : { labrador::Vector2F(0.0f, 20.0f), labrador::Vector2F(20.0f, -1.0f),
			labrador::Vector2F(9.0f, 20.0f), labrador::Vector2F(20.0f, 9.0f) })
		{
			bool rejected = false;
			try { pages(fixed, "W", area.x, area.y); }
			catch (const std::invalid_argument&) { rejected = true; }
			require(rejected, "An unusable diagnostic area was accepted");
		}
		const std::string long_message = "user stage refused: " + std::string(2001, 'W') +
			".json: a long unbroken key and caf\xC3\xA9.json need all of their content";
		for (const char* name : { "lato_black_24", "lato_black_36", "courier_prime_bold_16" })
		{
			const Font& font = *fixture.world->render.font(fixture.world->render.resolve_sprite_font(name));
			const auto laid_out = pages(font, long_message, 400.0f, 120.0f);
			require(laid_out.size() > 1, "Long refusal fixture did not exercise paging");
			std::wstring restored;
			for (const std::wstring& page : laid_out)
			{
				const auto size = font.measure(page);
				require(size.x <= 400.0f && size.y <= 120.0f, "A diagnostic page escaped its actual font bounds");
				for (const wchar_t character : page) { if (character != L'\n') { restored.push_back(character); } }
			}
			require(restored == display_text(long_message), "Wrapping/paging lost refusal filename or reason text");
		}
		const auto& look = fixture.presentation->main_menu().stage_scan;
		const auto& copy = fixture.presentation->menu_text().stage_scan;
		const Font& source_font = *fixture.world->render.font(fixture.world->render.resolve_sprite_font(look.font));
		std::vector<Glyph> tall_glyphs;
		for (char32_t character = U' '; character <= U'~'; ++character)
		{
			Glyph glyph = *source_font.find(character);
			if (character == U'X' || character == U'9') { glyph.subrect.height = 100; }
			tall_glyphs.push_back(glyph);
		}
		const Font tall(TextureHandle{}, std::move(tall_glyphs), source_font.line_spacing());
		const auto layout = stage_scan_diagnostics::layout(look, fixture.presentation->main_menu().art,
			fixture.presentation->menu().footer_y, tall, copy);
		require(layout.unicode_hint.height >= tall.measure(copy.unicode_hint).y * look.text_scale &&
			layout.progress.height >= tall.measure(copy.progress(9, 9, 9, 9)).y * look.text_scale &&
			layout.unicode_hint.bottom() < layout.body.y && layout.body.bottom() < layout.progress.y,
			"A font's tall hint/counter glyph escaped its row into adjacent diagnostic text");
		std::cout << "PASS refusal display: exact ASCII, UTF-16/invalid-byte escapes, inclusive independent font edges, "
			"actual proportional/monospace long-token pages and lossless reconstruction\n";
	}

	void check_hint(const Presentation& presentation)
	{
		const MenuStageText& text = presentation.menu_text().stage;
		require(!text.user_hint.empty() && text.user_hint != text.browse_hint,
			"Stage Select needs its own user-stage hint in menu_text.stage.user_hint");
		std::cout << "PASS menu_text.stage.user_hint marks a user stage in Stage Select\n";
	}

	void check_diagnostic_presentation(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		const auto base = read_document(content / "presentation.json");
		const auto check = [&](const char* name, auto edit, const std::string& refusal)
		{
			const auto path = scratch / (std::string("diagnostic-") + name + ".json");
			write_level(base, path, edit);
			try
			{
				const Presentation candidate(path_to_utf8(path).c_str());
				candidate.validate_resources(fixture.world->render, fixture.world->audio);
				require(refusal.empty(), std::string(name) + " unexpectedly passed");
			}
			catch (const std::exception& error)
			{
				require(!refusal.empty() && std::string(error.what()).find(refusal) != std::string::npos,
					std::string(name) + ": " + error.what());
			}
		};
		check("legacy-defaults", [](auto& d) {
			d["main_menu"].RemoveMember("stage_scan"); d["menu_text"].RemoveMember("stage_scan"); }, "");
		check("missing-counter-token", [](auto& d) {
			d["menu_text"]["stage_scan"]["counter"].SetString("File {file} of {files} {page}"); }, "stage_scan");
		check("duplicate-summary-token", [](auto& d) {
			d["menu_text"]["stage_scan"]["status"].SetString("{count} {count}"); }, "stage_scan");
		// Copy errors name the copy's section; font, layout and fit errors,
		// before and after the copy checks, name the page's look.
		check("blank-heading", [](auto& d) {
			d["menu_text"]["stage_scan"]["heading"].SetString(" "); }, "menu_text.stage_scan");
		check("unknown-font", [](auto& d) {
			d["main_menu"]["stage_scan"]["font"].SetString("absent_diagnostic_font"); }, "main_menu.stage_scan");
		check("thin-paper", [](auto& d) {
			d["main_menu"]["stage_scan"]["box_size"]["x"].SetFloat(50.0f); }, "main_menu.stage_scan");
		check("huge-text", [](auto& d) {
			d["main_menu"]["stage_scan"]["text_scale"].SetFloat(500.0f); }, "main_menu.stage_scan");
		check("overlapping-status", [](auto& d) {
			d["main_menu"]["stage_scan"]["status"]["position"]["y"].SetFloat(40.0f); }, "main_menu.stage_scan");
		check("outside-legend", [](auto& d) {
			d["main_menu"]["stage_scan"]["prompt_position"]["x"].SetFloat(1900.0f); }, "main_menu.stage_scan");
		check("long-legend", [](auto& d) {
			const std::string long_legend(500, 'W');
			d["menu_text"]["stage_scan"]["details_prompt"].SetString(long_legend.c_str(), d.GetAllocator());
		}, "main_menu.stage_scan");
		const auto& copy = fixture.presentation->menu_text().stage_scan;
		require(copy.summary(0) == copy.no_refusals && copy.summary(37).find(L"37") != std::wstring::npos &&
			copy.progress(12, 345, 6, 78) == L"Issue 12 of 345   Page 6 of 78", "Diagnostic counters changed their domains");
		for (const auto indices : { std::array<size_t, 4>{ 0, 1, 1, 1 }, { 2, 1, 1, 1 }, { 1, 1, 0, 1 }, { 1, 1, 2, 1 } })
		{
			bool refused = false;
			try { copy.progress(indices[0], indices[1], indices[2], indices[3]); }
			catch (const std::invalid_argument&) { refused = true; }
			require(refused, "An out-of-domain diagnostic counter was accepted");
		}
		std::cout << "PASS diagnostic presentation: legacy defaults, exact counters, bad copy/font/layout refusals before navigation\n";
	}
}

int main(int argc, char** argv)
{
	return acceptance::run_probe("user stages", [&]
	{
		require(argc == 2, "Usage: user_stages.exe <staged game directory>");
		check_name_contract();
		const std::filesystem::path content(argv[1]);
		const std::filesystem::path scratch = fresh(
			std::filesystem::absolute(argv[0]).parent_path() / "user-stages-fixtures");
		check_runtime_source(content, scratch);
		check_directory(scratch);
		World world(content);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		const Presentation presentation((content / "presentation.json").string().c_str());
		presentation.validate_resources(world.render, world.audio);
		const Fixture fixture{ &world, &tuning, &presentation, PlaySettings{} };
		check_hint(presentation);
		check_diagnostics(fixture);
		check_diagnostic_presentation(fixture, content, scratch);
		check_runtime_source_scan(fixture, content, scratch);
		check_empty(fixture, content, scratch);
		check_mixed(fixture, content, scratch);
		check_user_collision(fixture, content, scratch);
		check_stage_names(fixture, content, scratch);
		check_rescan(fixture, content, scratch);
		check_limits(fixture, content, scratch);
		std::cout << "PASS user stages: per-user folder, per-file validation and refusal, "
			"no shipped or user name collisions, Stage Select order and marker, Random only on request.\n";
		return 0;
	});
}
