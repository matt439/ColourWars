#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("paint surfaces", [&]
    {
        paint_surfaces_acceptance::require(argc == 2, "usage: paint_surfaces <content>");
        paint_surfaces_acceptance::World world(std::filesystem::absolute(argv[1]));
        paint_surfaces_acceptance::check_surfaces(world);
        paint_surfaces_acceptance::check_levels(world);
        std::cout << "PASS paint surfaces: actual scored quads, four authored faces, neutral/captured/friendly/enemy "
            "ownership, clipped/fractional/buried tiles, exact source-group score reconciliation, and active "
            "Crossfeed/Press Tower read-only state/fingerprint/RNG invariance. No rigid-motion or human-balance acceptance.\n";
        return 0;
    });
}
