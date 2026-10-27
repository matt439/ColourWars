#include "game/pch.h"
#include "game/content_validation.h"
#include "game/directory_consts.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/sound_groups.h"
#include "game/stage_loading.h"
#include "game/tuning.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/file_path.h"
#include "engine/render/sprite_font_file.h"

#include <algorithm>
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
	void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
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
		require(shipped.stages.size() == 10, "User-stage acceptance expects the ten shipped stages");
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
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == 10,
				"A missing user stage folder must add nothing and say nothing");
		}
		{
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, fresh(scratch / "empty"), shipped);
			require(result.accepted.empty() && result.refused.empty() && shipped.stages.size() == 10,
				"An empty user stage folder must add nothing and say nothing");
		}
		{
			const std::filesystem::path file = scratch / "not-a-folder";
			std::filesystem::remove_all(file);
			write_text(file, "x");
			Shipped shipped;
			const UserStageScan result = scan(fixture, content, file, shipped);
			require(result.accepted.empty() && result.refused.size() == 1 &&
				result.refused.front().find("is not a folder") != std::string::npos && shipped.stages.size() == 10,
				"A file where the folder should be must be one refusal line and no stages");
		}
		std::cout << "PASS missing, empty and non-folder user stage paths leave the ten shipped stages\n";
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

		require(shipped.stages.size() == 12, "Stage Select must offer ten shipped and two user stages");
		for (size_t index = 0; index < 10; ++index)
		{
			require(!shipped.stages[index].user, "Shipped stages must stay first and unmarked");
		}
		const Stage& mine = shipped.stages[10];
		require(mine.name == "my_arena" && mine.display_name == "My Arena" && mine.user && !mine.include_in_random,
			"A user stage must follow the shipped ones, marked and explicit-only");
		require(shipped.stages[11].name == "zz-second" && shipped.stages[11].user,
			"The second user stage must follow the first");
		require(shipped.levels.contains("my_arena") && shipped.levels.contains("zz-second") &&
			!shipped.levels.contains("broken") && !shipped.levels.contains("Crossfeed"),
			"Only accepted user stages may reach the level registry");
		require(shipped.levels.get("crossfeed")->display_name() == "Crossfeed",
			"A colliding user stage must not replace the shipped stage");
		const std::vector<size_t> random = random_stage_indices(shipped.stages);
		require(random.size() == 4 && std::none_of(random.begin(), random.end(),
			[](size_t index) { return index >= 10; }), "User stages must not join the Random pool");

		// The startup path after the scan: the whole list, user stages
		// included, passes the same checks and joins the sound groups.
		validate_game_content(*fixture.tuning, *fixture.presentation, &fixture.world->render,
			&fixture.world->audio, shipped.levels, shipped.stages, fixture.play_settings);
		AudioResources audio;
		audio.add_sound_bank("paint_exe", SoundBank::silent());
		sound_groups::assign(audio, *fixture.tuning, *fixture.presentation, shipped.levels, shipped.stages);
		std::cout << "PASS a folder of ten files loads its two valid stages after the shipped ten and refuses "
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
			refused(twice, "twin.json", "another user stage's ('Twin')") && shipped.stages.size() == 11,
			"A user stage may not take another user stage's name");
		std::cout << "PASS a user stage cannot take another user stage's name\n";
	}

	void check_hint(const Presentation& presentation)
	{
		const MenuStageText& text = presentation.menu_text().stage;
		require(!text.user_hint.empty() && text.user_hint != text.browse_hint,
			"Stage Select needs its own user-stage hint in menu_text.stage.user_hint");
		std::cout << "PASS menu_text.stage.user_hint marks a user stage in Stage Select\n";
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: user_stages.exe <staged game directory>");
		const std::filesystem::path content(argv[1]);
		const std::filesystem::path scratch = fresh(
			std::filesystem::absolute(argv[0]).parent_path() / "user-stages-fixtures");
		check_directory(scratch);
		World world(content);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		const Presentation presentation((content / "presentation.json").string().c_str());
		presentation.validate_resources(world.render, world.audio);
		const Fixture fixture{ &world, &tuning, &presentation, PlaySettings{} };
		check_hint(presentation);
		check_empty(fixture, content, scratch);
		check_mixed(fixture, content, scratch);
		check_user_collision(fixture, content, scratch);
		std::cout << "PASS user stages: per-user folder, per-file validation and refusal, "
			"no shipped or user name collisions, Stage Select order and marker, outside Random.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL user stages: " << error.what() << '\n';
		return 1;
	}
}
