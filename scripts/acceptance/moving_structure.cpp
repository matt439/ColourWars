#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include <iostream>

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc != 2) { throw std::invalid_argument("moving_structure <content>"); }
        moving_structure_acceptance::World world(std::filesystem::absolute(argv[1]));
        moving_structure_acceptance::require(!world.tuning.player().paint_underfoot.enabled,
            "POL36 prototype must continue to ship disabled");
        moving_structure_acceptance::check_motion(world);
        moving_structure_acceptance::check_loader(world);
        moving_structure_acceptance::check_paint(world);
        moving_structure_acceptance::check_support(world);
        moving_structure_acceptance::check_projectiles(world);
        moving_structure_acceptance::check_draw_cpu(world);
        moving_structure_acceptance::check_replay(world);
        moving_structure_acceptance::check_public_replay(world);
        std::cout << "PASS horizontal moving structure CPU checks: motion/owned paint/support/projectile/bounds; "
            "runtime loader construction qualified; actual renderer pixels, editor/map and human acceptance remain separate\n";
#if COLOURWARS_DEV_TOOLS
        std::cout << "Debug manual and public LevelBuilder replay checks completed\n";
#else
        std::cout << "Developer replay checks are Debug-only\n";
#endif
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL horizontal moving structure: " << error.what() << '\n'; return 1; }
}
