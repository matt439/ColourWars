#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"
#include "scripts/acceptance/support/probe.h"

// Baseline evidence for the authored Press Tower stage on current tuning:
// geometry, paint budget and sightline claims from the blueprint, then driven
// routes, tool contact and live fire at the spawns. No graphics/audio device,
// player save or position injection after a Run's construction.
int main(int argc, char** argv)
{
    using namespace press_tower_acceptance;
    return acceptance::run_probe("Press Tower", [&]
    {
        require(argc == 2, "Usage: press_tower.exe <content directory>");
        Services services(argv[1]);
        std::cout << "NOTE Press Tower baseline dt=" << dt <<
            "; actual authored Player/Scene; current tuning; no devices or player save\n";
        geometry_checks(services, std::filesystem::absolute(argv[0]).parent_path());
        spawn_checks(services);
        route_checks(services);
        tool_checks(services);
        painting_checks(services);
        live_fire_checks(services);
        std::cout << "PASS Press Tower baseline: mirrored authored geometry, independent paint budget, "
            "summit sightline claims, live-fire spawn safety, spawn respawn safety, jump-free required "
            "crossing, optional climbs and five-tool paint contact. Attended play, camera framing, balance "
            "and physical-pad acceptance remain open\n";
        return 0;
    });
}
