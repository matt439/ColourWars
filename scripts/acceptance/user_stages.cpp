#include "game/pch.h"
#include "game/content_validation.h"
#include "game/states/menu_content_validation.h"
#include "game/directory_consts.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/sound_groups.h"
#include "game/stage_loading.h"
#include "game/stage_scan_diagnostics.h"
#include "game/states/menu_context.h"
#include "game/tuning.h"
#include "game/user_stage_contract.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/file_path.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

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
			"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
			"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
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
		// These are identifier checks only, especially COM10/LPT0; no path or
		// filesystem operation is used to decide what Windows would open.
		for (const char* name : { "console", "console_map", "nul_map", "con_1", "prn1", "auxiliary",
			"COM10", "LPT0", "my_con", "_lpt9", "lpt_9", "A0b9_-", "a-", "_", "con_", "nul-stage", "COM1_copy" })
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
		std::cout << "PASS stage name contract: 22 reserved ASCII stems in three cases, valid neighbors, "
			"length/ASCII/leading rules and explanatory diagnostics; no reserved path I/O\n";
	}

	struct World
	{
		RenderResources render;
		AudioResources audio;

		explicit World(const std::filesystem::path& content)
		{
			for (const char* name : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
			{
				render.add_sprite_sheet(name, read_sprite_sheet(
					(content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
			}
			for (const char* name : { "bowlby_one_144", "lato_black_24", "lato_black_36", "lato_black_48",
				"lato_black_72", "lato_black_144", "courier_prime_bold_16" })
			{
				const SpriteFontFile file = read_sprite_font_file(
					(content / "fonts" / (std::string(name) + ".spritefont")).string());
				auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
				if (font->contains(U'?')) { font->set_stand_in(U'?'); }
				render.add_font(name, std::move(font));
			}
			audio.add_sound_bank("paint_exe", SoundBank::silent());
		}
	};

	// What wWinMain's "level" kind produces from the staged manifest.
	struct Shipped
	{
		Registry<LevelLoadedInfo> levels{ "Level" };
		StageList stages;
	};

	void load_shipped(const std::filesystem::path& content, Shipped& shipped)
	{
		const JsonDocument manifest = read_json_file((content / "manifest.json").string().c_str());
		const JsonValue assets = manifest.root().array("assets");
		for (size_t group = 0; group < assets.size(); ++group)
		{
			const JsonValue asset = assets.at(group);
			if (asset.string("kind") != "level") { continue; }
			const JsonValue names = asset.array("names");
			for (size_t index = 0; index < names.size(); ++index)
			{
				const std::string name = names.at(index).as_string();
				StageDefinition definition = read_stage_definition(
					(content / asset.string("directory") / (name + ".json")).string(), name);
				shipped.stages.push_back(std::move(definition.stage));
				shipped.levels.add(name, std::move(definition.info));
			}
		}
		require(shipped.stages.size() == 11, "User-stage acceptance expects the eleven shipped stages");
	}

	rapidjson::Document read_document(const std::filesystem::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		require(input.good(), "Cannot read " + path.string());
		const std::string source{ std::istreambuf_iterator<char>(input), {} };
		rapidjson::Document document;
		document.Parse(source.c_str());
		require(!document.HasParseError(), "Cannot parse " + path.string());
		return document;
	}

	void write_text(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		output << text;
		output.close();
		require(output.good(), "Cannot write fixture " + path_to_utf8(path));
	}

	std::string read_bytes(const std::filesystem::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		require(input.good(), "Cannot read source fixture " + path_to_utf8(path));
		const std::string text{ std::istreambuf_iterator<char>(input), {} };
		require(!input.bad(), "Cannot read complete source fixture " + path_to_utf8(path));
		return text;
	}

	std::string with_source_metadata(const std::string& stage, const std::string& member)
	{
		const std::size_t end = stage.find_last_of('}');
		require(end != std::string::npos, "Complete-source fixture needs the shipped stage's closing root");
		return stage.substr(0, end) + ',' + member + stage.substr(end);
	}

	void check_runtime_source(const std::filesystem::path& content, const std::filesystem::path& scratch)
	{
		const auto original = content / "levels/close_quarters.json";
		const std::string stage = read_bytes(original);
		const auto path = scratch / "complete-source.json";
		const std::string name = path_to_utf8(path);
		write_text(path, stage);
		const LevelLoadedInfo pristine(name.c_str());
		pristine.validate();
		const auto refusal = [&](const std::string& bytes)
		{
			write_text(path, bytes);
			std::string diagnostic;
			try { const LevelLoadedInfo loaded(name.c_str()); }
			catch (const std::runtime_error& error) { diagnostic = error.what(); }
			require(read_bytes(path) == bytes, "Runtime admission changed its private source bytes");
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
			require(expected.find("format") != std::string::npos &&
				refusal(base + '\0' + "suffix") == expected &&
				refusal("{\"format\":" + format + ",\"unknown\":\"" + malformed[1] + "\"}") == expected,
				"Complete-source validation changed unsupported/malformed format diagnostic precedence");
		}

		const auto hash = [&](const std::string& bytes)
		{
			write_text(path, bytes);
			const LevelLoadedInfo loaded(name.c_str());
			loaded.validate();
			require(read_bytes(path) == bytes, "Loading a valid snapshot changed its bytes");
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
		write_text(path, metadata);
		const LevelLoadedInfo cached(name.c_str());
		write_text(path, hidden_suffix);
		require(cached.semantic_hash() == valid_hash && !cached.display_name().empty(),
			"An admitted runtime definition reread subsequently malformed disk bytes");
		require(read_bytes(original) == stage, "Runtime source checks changed the staged input asset");
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
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		require(document.Accept(writer), "Cannot encode fixture " + path_to_utf8(path));
		write_text(path, std::string(buffer.GetString(), buffer.GetSize()));
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
		return load_user_stages(folder, fixture.checks(), shipped.levels, shipped.stages);
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
		const std::string stage = read_bytes(content / "levels/close_quarters.json");
		write_text(folder / "a-valid.json", stage);
		write_text(folder / "b-nul.json", stage + '\0' + "unparsed suffix");
		write_text(folder / "c-value.json", with_source_metadata(stage, "\"unknown\":\"\xc0\xaf\""));
		write_text(folder / "d-key.json", with_source_metadata(stage, "\"unknown-\xed\xa0\x80\":true"));
		Shipped shipped;
		const UserStageScan result = scan(fixture, content, folder, shipped);
		require(result.accepted == std::vector<std::string>{ "a-valid" } && result.refused.size() == 3 &&
			shipped.stages.size() == 12 && shipped.levels.contains("a-valid") &&
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
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == 11,
				"A missing user stage folder must add nothing and say nothing");
		}
		{
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, fresh(scratch / "empty"), shipped);
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == 11,
				"An empty user stage folder must add nothing and say nothing");
		}
		{
			const std::filesystem::path file = scratch / "not-a-folder";
			std::filesystem::remove_all(file);
			write_text(file, "x");
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, file, shipped);
			require(result.accepted.empty() && result.refused.size() == 1 &&
				result.refused.front().find("is not a folder") != std::string::npos && shipped.stages.size() == 11,
				"A file where the folder should be must be one refusal line and no stages");
		}
		std::cout << "PASS missing, empty and non-folder user stage paths leave the eleven shipped stages\n";
	}

	void check_mixed(const Fixture& fixture, const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		// A non-ASCII folder name: paths cross the engine as UTF-8.
		const std::filesystem::path folder = fresh(scratch / L"stÃ¤ges");
		const rapidjson::Document base = read_document(content / "levels" / "close_quarters.json");

		write_level(base, folder / "my_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "My Arena");
			// Asks for Random; a user stage is explicit-only regardless.
			d.AddMember("include_in_random", true, d.GetAllocator());
		});
		write_level(base, folder / "zz-second.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "Second Of Mine");
		});
		write_text(folder / "broken.json", "{ \"display_name\": \"Broken\", ");
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
		write_text(folder / "notes.txt", "not a stage");
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

		require(shipped.stages.size() == 13, "Stage Select must offer eleven shipped and two user stages");
		for (size_t index = 0; index < 11; ++index)
		{
			require(!shipped.stages[index].user, "Shipped stages must stay first and unmarked");
		}
		const Stage& mine = shipped.stages[11];
		require(mine.name == "my_arena" && mine.display_name == "My Arena" && mine.user && !mine.include_in_random,
			"A user stage must follow the shipped ones, marked and explicit-only");
		require(shipped.stages[12].name == "zz-second" && shipped.stages[12].user,
			"The second user stage must follow the first");
		require(shipped.levels.contains("my_arena") && shipped.levels.contains("zz-second") &&
			!shipped.levels.contains("broken") && !shipped.levels.contains("Crossfeed"),
			"Only accepted user stages may reach the level registry");
		require(shipped.levels.get("crossfeed")->display_name() == "Crossfeed",
			"A colliding user stage must not replace the shipped stage");
		const std::vector<size_t> random = random_stage_indices(shipped.stages);
		require(random.size() == 4 && std::none_of(random.begin(), random.end(),
			[](size_t index) { return index >= 11; }), "User stages must not join the Random pool");

		// The startup path after the scan: the whole list, user stages
		// included, passes the same checks and joins the sound groups.
		validate_shell_content(*fixture.tuning, *fixture.presentation, &fixture.world->render,
			&fixture.world->audio, shipped.levels, shipped.stages, fixture.play_settings);
		AudioResources audio;
		audio.add_sound_bank("paint_exe", SoundBank::silent());
		sound_groups::assign(audio, *fixture.tuning, *fixture.presentation, shipped.levels, shipped.stages);
		std::cout << "PASS a folder of ten files loads its two valid stages after the shipped eleven and refuses "
			"broken JSON, a missing key, a missing frame, a sound conflict, a shipped-name collision, "
			"two bad file names and an undrawable name, one line each\n";
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
		const UserStageScan twice = load_user_stages(second, fixture.checks(), shipped.levels, shipped.stages);
		require(once.accepted == std::vector<std::string>{ "Twin" } && twice.accepted.empty() &&
			refused(twice, "twin.json", "another user stage's ('Twin')") && shipped.stages.size() == 12,
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
		session.attach(folder, fixture.checks(), &fixture.world->audio, &shipped.levels, &shipped.stages);
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
		});
		write_level(base, folder / "b_arena.json", [](rapidjson::Document& d)
		{
			set_string(d, d, "display_name", "B Arena Edited");
		});
		write_text(folder / "broken.json", "{");
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
			glyphs.push_back({ character, mattmath::RectangleI(0, 0, 10, 8), 0.0f, 0.0f, 0.0f });
		}
		const Font fixed(TextureHandle{}, std::move(glyphs), 10.0f);
		const Font proportional(TextureHandle{}, std::vector<Glyph>{
			{ U'a', mattmath::RectangleI(0, 0, 1, 8), 0.0f, 0.0f, 0.0f },
			{ U' ', mattmath::RectangleI(0, 0, 1, 8), 0.0f, 0.0f, 0.0f },
			{ U'b', mattmath::RectangleI(0, 0, 3, 8), 0.0f, 0.0f, 0.5f },
			{ U'X', mattmath::RectangleI(0, 0, 5, 8), 0.0f, 0.0f, 0.0f } }, 10.0f);
		require(pages(proportional, "a bbX", 10.0f, 10.0f) ==
			std::vector<std::wstring>{ L"a ", L"bb", L"X" },
			"A wide final proportional glyph left an overwide suffix after word wrapping");
		require(pages(fixed, "abcdefgh", 20.0f, 20.0f) ==
			std::vector<std::wstring>{ L"ab\ncd", L"ef\ngh" }, "Exact diagnostic width/height edges did not paginate inclusively");
		require(pages(fixed, "", 20.0f, 20.0f) == std::vector<std::wstring>{ L"" },
			"An empty diagnostic did not provide a safe empty page");
		for (const auto area : { mattmath::Vector2F(0.0f, 20.0f), mattmath::Vector2F(20.0f, -1.0f),
			mattmath::Vector2F(9.0f, 20.0f), mattmath::Vector2F(20.0f, 9.0f) })
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
		check_rescan(fixture, content, scratch);
		std::cout << "PASS user stages: per-user folder, per-file validation and refusal, "
			"no shipped or user name collisions, Stage Select order and marker, outside Random.\n";
		return 0;
	});
}
