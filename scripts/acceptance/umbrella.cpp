#include "game/pch.h"
#include "scripts/acceptance/umbrella/fixtures.h"
#include "scripts/acceptance/umbrella/balance.h"

int main(int argc, char** argv)
{
    using namespace umbrella_acceptance;
    return acceptance::run_probe("Umbrella", [&]
    {
        if (argc > 2 && std::string_view(argv[2]) == "--balance") { return umbrella_balance::run(argc, argv); }
        require(argc == 2, "usage: umbrella <content>");
        World world(argv[1], "umbrella", "Umbrella requires support '", "crossfeed");
        check_weapon(world);
        check_sweeps(world);
        check_match(world);
        std::cout << "PASS Umbrella fan, shared tank, held canopy, break and repair, swept hits, "
            "planted-wall ties, roller passage, respawn, movement and match scoring\n";
        return 0;
    });
}
