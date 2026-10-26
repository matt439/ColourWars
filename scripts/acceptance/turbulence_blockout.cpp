#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"

int main(int argc, char** argv)
{
    using namespace turbulence_blockout_acceptance;
    try
    {
        require(argc == 2, "turbulence_blockout <content directory>");
        Services services(argv[1]);
        geometry(services, std::filesystem::absolute(argv[0]).parent_path());
        public_load(services, std::filesystem::path(argv[1]));
        routes(services);
        std::cout << "PASS Turbulence static skeleton: production object loader, mirrored geometry, "
            "independent face/region score, public 1/4-player LevelBuilder loads, eight spawns/respawns, "
            "jump-free required walking and driven optional/recovery edges. "
            "Mover/rotation, five-tool pressure, native framing, attended play and pad acceptance remain open\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL Turbulence blockout: " << error.what() << '\n'; return 1; }
}
