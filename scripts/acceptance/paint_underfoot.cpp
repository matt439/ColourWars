#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc != 2) { throw std::invalid_argument("paint_underfoot <content>"); }
        paint_underfoot_acceptance::World world(std::filesystem::absolute(argv[1]));
        paint_underfoot_acceptance::require(!world.tuning.player().paint_underfoot.enabled,
            "POL36 prototype must ship disabled");
        paint_underfoot_acceptance::check_movement(world);
        paint_underfoot_acceptance::check_support(world);
        paint_underfoot_acceptance::check_recovery(world);
        paint_underfoot_acceptance::check_schema_and_bounds(world);
        paint_underfoot_acceptance::check_health_regen(world);
        std::cout << "PASS paint underfoot: actual contact/movement/recovery/schema/bounds; no human balance acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL paint underfoot: " << error.what() << '\n'; return 1; }
}
