#pragma once

#include "game/debug/dev_tools.h"
#include "scripts/acceptance/support/probe.h"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace skirmish_acceptance
{
	using acceptance::require;
#if COLOURWARS_DEV_TOOLS
	void check_policy();
	void check_sequences();
	void check_actual_observations(const std::filesystem::path& content);
#endif
}
