#pragma once

#include <cstddef>
#include <string>
#include <vector>

// A pickable level: registered by the manifest, or a user stage found in the
// player's stage folder (game/stage_loading.h). name is its registry key, the
// file name without .json; display_name and icon_frame are copied from the
// level file at load time. A user stage is never a Random candidate.
struct Stage
{
	std::string name;
	std::string display_name;
	std::string icon_frame;
	bool include_in_random = true;
	bool user = false;
};

// The stages a game offers: the manifest's in its order, then the user stages.
using StageList = std::vector<Stage>;

// Explicit selection always offers every stage; Random offers only authored candidates.
std::vector<size_t> random_stage_indices(const StageList& stages);
size_t random_stage_index(const StageList& stages, size_t random_value);
