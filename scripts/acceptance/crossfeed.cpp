#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include "scripts/acceptance/support/probe.h"

int main(int argc, char** argv)
{
    using namespace crossfeed_acceptance;
    return acceptance::run_probe("Crossfeed", [&]
    {
        require(argc == 2, "Usage: crossfeed.exe <content directory>");
        Services services(argv[1]);
        std::cout << "NOTE Crossfeed H1-H8 baseline dt=" << dt <<
            "; actual authored Player/Scene; no devices, player save or position injection after construction\n";
        geometry_checks(services, std::filesystem::absolute(argv[0]).parent_path());
        spawn_checks(services);
        route_checks(services);
        tool_checks(services);
        combat_checks(services);
        pressure_checks(services);
        reciprocal_checks(services);
        trajectory_checks(services);
        supply_checks(services);
        painting_checks(services);
        repaint_checks(services);
        home_loop_checks(services);
        opposed_loop_checks(services);
        opening_checks(services);
        std::cout << "PASS Crossfeed baseline feasibility and measured score/tool access. "
            "Human pressure, camera warning, weapon balance and physical-pad acceptance remain open\n";
        return 0;
    });
}
