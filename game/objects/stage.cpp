#include "game/pch.h"
#include "game/objects/stage.h"

std::vector<size_t> random_stage_indices(const StageList& stages)
{
	std::vector<size_t> indices;
	for (size_t index = 0; index < stages.size(); ++index)
	{
		if (stages[index].include_in_random)
		{
			indices.push_back(index);
		}
	}
	return indices;
}

size_t random_stage_index(const StageList& stages, size_t random_value)
{
	const std::vector<size_t> indices = random_stage_indices(stages);
	if (indices.empty())
	{
		throw std::runtime_error("Random stage selection has no eligible arenas; "
			"choose a stage explicitly or enable include_in_random in a level.");
	}
	return indices[random_value % indices.size()];
}
