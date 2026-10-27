#include "game/pch.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <set>

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	void check_no_candidates(const StageList& stages)
	{
		bool rejected = false;
		try
		{
			random_stage_index(stages, 0);
		}
		catch (const std::runtime_error& error)
		{
			rejected = std::string(error.what()).find("no eligible arenas") != std::string::npos;
		}
		require(rejected, "Random with no candidates must fail clearly rather than pick an excluded stage");
	}

	void check_mapping()
	{
		const StageList mixed{
			{ "first_excluded", "First excluded", "", false },
			{ "first_arena", "First arena", "" },
			{ "middle_excluded", "Middle excluded", "", false },
			{ "second_arena", "Second arena", "", true },
			{ "last_excluded", "Last excluded", "", false }
		};
		require(random_stage_indices(mixed) == std::vector<size_t>{ 1, 3 },
			"Random eligibility must preserve manifest indices and default to inclusion");
		for (size_t draw = 0; draw < 12; ++draw)
		{
			require(random_stage_index(mixed, draw) == (draw % 2 == 0 ? 1u : 3u),
				"Random selection returned an excluded index or skipped an eligible index");
		}
		check_no_candidates({});
		check_no_candidates({ mixed.front(), mixed.back() });
	}

	void check_shipped_stages(const std::filesystem::path& content)
	{
		const std::string manifest_path = (content / "manifest.json").string();
		const labrador::JsonDocument manifest = labrador::read_json_file(manifest_path.c_str());
		const labrador::JsonValue assets = manifest.root().array("assets");
		StageList stages;
		bool course_found = false;
		bool crossfeed_found = false;
		bool press_tower_found = false;
		bool turbulence_blockout_found = false;
		bool turbulence_mover_found = false;
		bool turbulence_slab_found = false;
		bool turbulence_ferry_found = false;
		std::set<std::string> names_seen;
		size_t crossfeed_index = 0;
		size_t defaulted_arenas = 0;
		for (size_t group = 0; group < assets.size(); ++group)
		{
			const labrador::JsonValue asset = assets.at(group);
			if (asset.string("kind") != "level") { continue; }
			const labrador::JsonValue names = asset.array("names");
			for (size_t index = 0; index < names.size(); ++index)
			{
				const std::string name = names.at(index).as_string();
				require(names_seen.insert(name).second, "Shipped stage names must be unique");
				const std::string path = (content / asset.string("directory") / (name + ".json")).string();
				const LevelLoadedInfo info(path.c_str());
				info.validate();
				stages.push_back({ name, info.display_name(), info.icon_frame(), info.include_in_random() });
				if (name == "movement_course")
				{
					course_found = true;
					require(!info.include_in_random(), "Movement Course must require an explicit stage choice");
				}
				if (name == "crossfeed")
				{
					crossfeed_found = true;
					crossfeed_index = stages.size() - 1;
					require(info.include_in_random(), "Accepted Crossfeed must be eligible for Random");
				}
				if (name == "press_tower")
				{
					press_tower_found = true;
					require(!info.include_in_random(), "Press Tower must require an explicit stage choice while provisional");
				}
				if (name == "turbulence_blockout")
				{
					turbulence_blockout_found = true;
					require(!info.include_in_random(), "Turbulence blockout must remain explicit-only while provisional");
				}
				if (name == "turbulence_mover_trial")
				{
					turbulence_mover_found = true;
					require(!info.include_in_random(), "Turbulence mover comparison must remain explicit-only while provisional");
				}
				if (name == "turbulence_slab_trial")
				{
					turbulence_slab_found = true;
					require(!info.include_in_random(), "Turbulence scored-slab comparison must remain explicit-only while provisional");
				}
				if (name == "turbulence_ferry_trial")
				{
					turbulence_ferry_found = true;
					require(!info.include_in_random(), "Turbulence paint-ferry trial must remain explicit-only while provisional");
				}
				const labrador::JsonDocument source = labrador::read_json_file(path.c_str());
				if (!source.root().has("include_in_random"))
				{
					++defaulted_arenas;
					require(info.include_in_random(), "Existing stages without metadata must stay eligible");
				}
			}
		}
		require(course_found && crossfeed_found && press_tower_found && turbulence_blockout_found && turbulence_mover_found &&
			turbulence_slab_found && turbulence_ferry_found && defaulted_arenas == 3,
			"Acceptance needs the explicit course, Press Tower and Turbulence comparisons, eligible Crossfeed and three unchanged arenas");
		const std::set<std::string> expected_names{ "king_of_the_hill", "turbulence", "close_quarters", "movement_course",
			"crossfeed", "press_tower", "turbulence_blockout", "turbulence_mover_trial", "turbulence_slab_trial", "turbulence_ferry_trial" };
		require(stages.size() == 10 && names_seen == expected_names, "Stage selection must expose exactly the ten registered stages");
		const std::vector<size_t> candidates = random_stage_indices(stages);
		require(candidates.size() == 4, "The three legacy arenas and accepted Crossfeed must remain the four Random candidates");
		std::set<std::string> candidate_names;
		for (const size_t index : candidates) { candidate_names.insert(stages[index].name); }
		require(candidate_names == std::set<std::string>{ "king_of_the_hill", "turbulence", "close_quarters", "crossfeed" },
			"Random changed its four accepted arena identities");
		require(std::find(candidates.begin(), candidates.end(), crossfeed_index) != candidates.end(),
			"Shipped Random selection omitted accepted Crossfeed");
		for (size_t draw = 0; draw < candidates.size(); ++draw)
		{
			const size_t index = random_stage_index(stages, draw);
			require(index == candidates[draw] && stages[index].include_in_random &&
				stages[index].name != "movement_course" && stages[index].name != "press_tower" &&
				stages[index].name != "turbulence_blockout" && stages[index].name != "turbulence_mover_trial" &&
				stages[index].name != "turbulence_slab_trial" && stages[index].name != "turbulence_ferry_trial",
				"Shipped Random selection admitted an explicit-only stage");
		}
		std::cout << "Shipped stage selection: " << stages.size() << " explicit stages; "
			<< candidates.size() << " Random candidates.\n";
	}

	void check_invalid_metadata(const std::filesystem::path& content,
		const std::filesystem::path& scratch)
	{
		std::ifstream input(content / "levels/movement_course.json", std::ios::binary);
		require(input.good(), "Could not read the course for metadata rejection acceptance");
		std::string source{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
		const std::string field = "\"include_in_random\": false";
		const size_t position = source.find(field);
		require(position != std::string::npos, "Course metadata fixture needs its authored boolean field");
		source.replace(position, field.size(), "\"include_in_random\": \"false\"");
		const std::filesystem::path fixture = scratch / "stage-selection-invalid.json";
		{
			std::ofstream output(fixture, std::ios::binary);
			output << source;
			require(output.good(), "Could not write isolated invalid stage metadata");
		}
		bool rejected = false;
		try
		{
			const LevelLoadedInfo invalid(fixture.string().c_str());
			invalid.validate();
		}
		catch (const std::runtime_error& error)
		{
			rejected = std::string(error.what()).find("include_in_random") != std::string::npos;
		}
		require(rejected, "Startup validation must reject non-boolean stage eligibility by field name");
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: stage_selection.exe <game/content directory>");
		check_mapping();
		check_shipped_stages(argv[1]);
		check_invalid_metadata(argv[1], std::filesystem::absolute(argv[0]).parent_path());
		std::cout << "PASS stage selection: explicit course, Press Tower and Turbulence comparisons, Crossfeed in Random, eligible Random mapping, defaults and rejection guards.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL stage selection: " << error.what() << '\n';
		return 1;
	}
}
