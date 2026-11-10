#include "game/pch.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "scripts/acceptance/support/probe.h"
#include "scripts/acceptance/support/stage_catalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <set>

namespace
{
	using acceptance::require;

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
		const std::string last = stages.empty() ? std::string("not_a_stage") : stages.front().name;
		rejected = false;
		try
		{
			random_stage_index(stages, 0, last);
		}
		catch (const std::runtime_error& error)
		{
			rejected = std::string(error.what()).find("no eligible arenas") != std::string::npos;
		}
		require(rejected, "Random with no candidates and a last stage must fail the same way");
	}

	// REV#24: Stage Select's Random leaves out the last stage while another
	// candidate exists, and shares the draw evenly among the rest in manifest order.
	void check_last_stage_mapping(const StageList& mixed)
	{
		for (size_t draw = 0; draw < 12; ++draw)
		{
			require(random_stage_index(mixed, draw, "first_arena") == 3u &&
				random_stage_index(mixed, draw, "second_arena") == 1u,
				"Random repeated the last stage from a pool of two");
			const size_t plain = random_stage_index(mixed, draw);
			require(random_stage_index(mixed, draw, "") == plain &&
				random_stage_index(mixed, draw, "first_excluded") == plain &&
				random_stage_index(mixed, draw, "not_a_stage") == plain,
				"No last stage, or one outside the pool, must draw as the two-argument form");
		}

		const StageList wide{
			{ "arena_a", "Arena A", "" },
			{ "excluded_a", "Excluded A", "", false },
			{ "arena_b", "Arena B", "" },
			{ "arena_c", "Arena C", "" },
			{ "excluded_b", "Excluded B", "", false },
			{ "arena_d", "Arena D", "" }
		};
		const std::vector<size_t> pool = random_stage_indices(wide);
		require(pool == std::vector<size_t>{ 0, 2, 3, 5 }, "The wide fixture needs four candidates");
		for (const size_t last : pool)
		{
			std::vector<size_t> remaining;
			for (const size_t index : pool)
			{
				if (index != last) { remaining.push_back(index); }
			}
			for (size_t draw = 0; draw < 3 * remaining.size(); ++draw)
			{
				require(random_stage_index(wide, draw, wide[last].name) ==
					remaining[draw % remaining.size()],
					"Random must share the draw evenly among the stages other than the last, in manifest order");
			}
		}

		const StageList lone{
			{ "explicit_only", "Explicit only", "", false },
			{ "only_arena", "Only arena", "" }
		};
		for (size_t draw = 0; draw < 4; ++draw)
		{
			require(random_stage_index(lone, draw, "only_arena") == 1u,
				"A Random pool of one must still return its stage after playing it");
		}
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
		check_last_stage_mapping(mixed);
		check_no_candidates({});
		check_no_candidates({ mixed.front(), mixed.back() });
	}

	void check_shipped_stages(const std::filesystem::path& content)
	{
		// The exact pins: the thirteen registered stages, and which of them
		// Random may draw. Every other probe walks the manifest instead.
		const std::map<std::string, bool> expected_random{
			{ "movement_course", false }, { "king_of_the_hill", false }, { "turbulence", false },
			{ "close_quarters", true }, { "crossfeed", true }, { "press_tower", true },
			{ "turbulence_blockout", false }, { "turbulence_mover_trial", false },
			{ "turbulence_slab_trial", true }, { "turbulence_ferry_trial", false },
			{ "turbulence_gate_trial", false }, { "turbulence_lift_trial", false },
			{ "turbulence_paint_lift_trial", false } };
		StageList stages;
		std::set<std::string> names_seen;
		size_t crossfeed_index = 0;
		size_t defaulted_arenas = 0;
		for (const acceptance::ManifestEntry& level : acceptance::manifest_level_paths(content))
		{
			const std::string& name = level.name;
			require(names_seen.insert(name).second, "Shipped stage names must be unique");
			const std::string path = labrador::path_to_utf8(level.path);
			const LevelLoadedInfo info(path.c_str());
			info.validate();
			stages.push_back({ name, info.display_name(), info.icon_frame(), info.include_in_random() });
			const auto expected = expected_random.find(name);
			require(expected != expected_random.end(), "Stage selection gained an unpinned stage: " + name);
			require(info.include_in_random() == expected->second, expected->second ?
				"An accepted Random arena left Random: " + name :
				"An explicit-only stage joined Random: " + name);
			if (name == "crossfeed") { crossfeed_index = stages.size() - 1; }
			const labrador::JsonDocument source = labrador::read_json_file(path.c_str());
			if (!source.root().has("include_in_random"))
			{
				++defaulted_arenas;
				require(info.include_in_random(), "Existing stages without metadata must stay eligible");
			}
		}
		require(defaulted_arenas == 1, "Acceptance needs exactly one arena without include_in_random");
		require(stages.size() == expected_random.size() && stages.size() == 13,
			"Stage selection must expose exactly the thirteen registered stages");
		const std::vector<size_t> candidates = random_stage_indices(stages);
		require(candidates.size() == 4, "Close Quarters, Crossfeed, Press Tower and the slab trial must be the four Random candidates");
		std::set<std::string> candidate_names;
		for (const size_t index : candidates) { candidate_names.insert(stages[index].name); }
		require(candidate_names == std::set<std::string>{ "close_quarters", "crossfeed", "press_tower", "turbulence_slab_trial" },
			"Random changed its four accepted arena identities");
		require(std::find(candidates.begin(), candidates.end(), crossfeed_index) != candidates.end(),
			"Shipped Random selection omitted accepted Crossfeed");
		for (size_t draw = 0; draw < candidates.size(); ++draw)
		{
			const size_t index = random_stage_index(stages, draw);
			require(index == candidates[draw] && stages[index].include_in_random &&
				expected_random.at(stages[index].name),
				"Shipped Random selection admitted an explicit-only stage");
		}
		for (const size_t last : candidates)
		{
			std::map<size_t, int> hits;
			for (size_t draw = 0; draw < 12; ++draw)
			{
				const size_t index = random_stage_index(stages, draw, stages[last].name);
				require(index != last, "Shipped Random repeated the last stage: " + stages[last].name);
				require(stages[index].include_in_random,
					"Shipped Random after a last stage admitted an explicit-only stage");
				++hits[index];
			}
			require(hits.size() == 3, "Shipped Random after a last stage must reach the other three arenas");
			for (const auto& [index, count] : hits)
			{
				require(count == 4, "Shipped Random after a last stage favoured " + stages[index].name);
			}
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
	return acceptance::run_probe("stage selection", [&]
	{
		require(argc == 2, "Usage: stage_selection.exe <game/content directory>");
		check_mapping();
		check_shipped_stages(argv[1]);
		check_invalid_metadata(argv[1], std::filesystem::absolute(argv[0]).parent_path());
		std::cout << "PASS stage selection: explicit course, originals and Turbulence trials, Crossfeed, Press Tower and slab trial in Random, eligible Random mapping, no repeat of the last stage, defaults and rejection guards.\n";
		return 0;
	});
}
