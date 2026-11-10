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
	// A stage name is a registry key and never empty, so nothing is left out.
	return random_stage_index(stages, random_value, std::string());
}

size_t random_stage_index(const StageList& stages, size_t random_value,
	const std::string& last_stage)
{
	std::vector<size_t> indices = random_stage_indices(stages);
	if (indices.size() > 1)
	{
		std::erase_if(indices, [&](size_t index)
		{
			return stages[index].name == last_stage;
		});
	}
	if (indices.empty())
	{
		throw std::runtime_error("Random stage selection has no eligible arenas; "
			"choose a stage explicitly or enable include_in_random in a level.");
	}
	return indices[random_value % indices.size()];
}
