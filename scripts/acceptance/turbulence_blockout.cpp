#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"
#include <string_view>

int main(int argc, char** argv)
{
    using namespace turbulence_blockout_acceptance;
    try
    {
        require(argc == 2 || (argc == 3 && (std::string_view(argv[2]) == "--tools" ||
            std::string_view(argv[2]) == "--escape" || std::string_view(argv[2]) == "--exchange")),
            "turbulence_blockout <content directory> [--tools|--escape|--exchange]");
        Services services(argv[1]);
        if (argc == 3)
        {
            if (std::string_view(argv[2]) == "--tools") { tools(services, std::filesystem::absolute(argv[0]).parent_path()); }
            else if (std::string_view(argv[2]) == "--escape") { escape_trials(services, std::filesystem::absolute(argv[0]).parent_path()); }
            else { exchange_trials(services, std::filesystem::absolute(argv[0]).parent_path()); }
            return 0;
        }
        geometry(services, std::filesystem::absolute(argv[0]).parent_path());
        public_load(services, std::filesystem::path(argv[1]));
        routes(services);
        std::cout << "PASS Turbulence static skeleton: production object loader, mirrored geometry, "
            "independent face/region score, public 1/4-player LevelBuilder loads, eight spawns/respawns, "
            "jump-free required walking and driven optional/recovery edges. "
            "Tool/escape and native composition use separate focused modes; mover/rotation, attended play and pad acceptance remain open\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL Turbulence blockout: " << error.what() << '\n'; return 1; }
}
