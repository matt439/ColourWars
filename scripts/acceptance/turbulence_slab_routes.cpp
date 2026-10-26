#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_routes/fixtures.h"
#include <iostream>
#include <process.h>

int main(int argc, char** argv)
{
    try
    {
        using namespace turbulence_slab_routes_acceptance;
        require(argc == 4, "turbulence_slab_routes <staged-content> <private-slab.json> <private-shoulder.json>");
        const auto evidence = fs::absolute(argv[0]).parent_path() /
            ("turbulence-slab-routes-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing earlier public slab Roller-route receipts");
        fs::create_directories(evidence);
        run(fs::absolute(argv[1]), fs::absolute(argv[2]), fs::absolute(argv[3]), evidence);
        std::cout << "PASS public driven Roller geometry comparison; receipts=" << evidence.string()
            << ". Local route/paint samples do not establish opposed balance or human acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL public slab Roller routes: " << error.what() << '\n'; return 1; }
}
