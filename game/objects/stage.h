#pragma once

#include <cstddef>
#include <string>
#include <vector>

// A pickable level registered by the manifest. name is its registry key;
// display_name and icon_frame are copied from the level file at load time.
struct Stage
{
	std::string name;
	std::string display_name;
	std::string icon_frame;
	bool include_in_random = true;
};

// The stages a game offers, in the order its manifest names them.
using StageList = std::vector<Stage>;

// Explicit selection always offers every stage; Random offers only authored candidates.
std::vector<size_t> random_stage_indices(const StageList& stages);
size_t random_stage_index(const StageList& stages, size_t random_value);
