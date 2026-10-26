#pragma once

#include "game/debug/dev_tools.h"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace skirmish_acceptance
{
	inline void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}
#if COLOURWARS_DEV_TOOLS
	void check_policy();
	void check_sequences();
	void check_actual_observations(const std::filesystem::path& content);
#endif
}
