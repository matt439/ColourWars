#include "game/pch.h"
#include "scripts/acceptance/skirmish_bot/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <fstream>
#include <iostream>
#include <iterator>

int wmain(int argc, wchar_t** argv)
{
	return acceptance::run_probe("skirmish bot", [&]
	{
		using namespace skirmish_acceptance;
		require(argc == 2, "Usage: skirmish_bot <staged-content-directory>");
		const std::filesystem::path content(argv[1]);
		std::ifstream game(content / "ColourWars.exe", std::ios::binary);
		require(game.good(), "Could not inspect the actual game executable");
		const std::string bytes{ std::istreambuf_iterator<char>(game), std::istreambuf_iterator<char>() };
		const bool present = bytes.find(".?AVSkirmishBot@@") != std::string::npos;
		require(present == dev_tools::enabled, "SkirmishBot Debug/Release RTTI exclusion failed");
#if COLOURWARS_DEV_TOOLS
		check_policy();
		check_sequences();
		check_actual_observations(content);
		std::cout << "PASS skirmish bot policy, deterministic sequences, lifecycle and actual tool observations\n";
#else
		std::cout << "PASS SkirmishBot absent from Release\n";
#endif
		return 0;
	});
}
