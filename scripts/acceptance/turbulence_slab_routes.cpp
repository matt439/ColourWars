#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_routes/fixtures.h"
#include <iostream>
#include <process.h>
#include <string_view>

int main(int argc, char** argv)
{
    try
    {
        using namespace turbulence_slab_routes_acceptance;
        require(argc == 4 || (argc == 5 && std::string_view(argv[4]) == "--replay"),
            "turbulence_slab_routes <staged-content> <private-slab.json> <private-shoulder.json> [--replay]");
        const auto evidence = fs::absolute(argv[0]).parent_path() /
            ("turbulence-slab-routes-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing earlier public slab Roller-route receipts");
        fs::create_directories(evidence);
        if (argc == 5)
        {
            run_replay(fs::absolute(argv[1]), fs::absolute(argv[2]), fs::absolute(argv[3]), evidence);
            std::cout << "PASS seeded public slab Roller replay; receipts=" << evidence.string()
                << ". Unfiltered countdown priming is explicit; no human or cross-build acceptance\n";
            return 0;
        }
        run(fs::absolute(argv[1]), fs::absolute(argv[2]), fs::absolute(argv[3]), evidence);
        std::cout << "PASS public driven Roller geometry comparison; receipts=" << evidence.string()
            << ". Local route/paint samples do not establish opposed balance or human acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL public slab Roller routes: " << error.what() << '\n'; return 1; }
}
