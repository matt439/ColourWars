#include "game/pch.h"
#include "scripts/acceptance/cq_blockout/fixtures.h"

// Focused first-blockout evidence on the authored CQ map and current tuning.
// No graphics/audio device, player save, injected position or physical input.
int main(int argc, char** argv)
{
    using namespace cq_blockout_acceptance;
    try
    {
        require(argc == 2, "Usage: cq_blockout.exe <content directory>");
        Services services(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[0]).parent_path();
        std::cout << "NOTE CQ authored Player/Scene trials dt=" << dt <<
            "; current tuning; no devices or player save; both directions are scripted CPU evidence\n";
        geometry_checks(services, output);
        spawn_checks(services);
        route_checks(services);
        tool_checks(services);
        std::cout << "PASS CQ first blockout: mirrored gameplay geometry, spawn clearance/respawn, "
            "exposed regional budget, driven routes and five-tool paint contact. "
            "Attended multiplayer, readability, balance and physical-pad acceptance remain open.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL CQ blockout: " << error.what() << '\n';
        return 1;
    }
}
