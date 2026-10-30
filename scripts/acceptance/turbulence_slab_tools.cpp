#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_tools/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>
#include <process.h>

int main(int argc, char** argv)
{
    return acceptance::run_probe("lower-approach slab tools", [&]
    {
        using namespace turbulence_slab_tools_acceptance;
        require(argc == 3, "turbulence_slab_tools <staged-content> <qualified-private-slab.json>");
        const auto evidence = fs::absolute(argv[0]).parent_path() /
            ("turbulence-slab-tools-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing earlier lower-approach slab tool receipts");
        fs::create_directories(evidence);
        run(fs::absolute(argv[1]), fs::absolute(argv[2]), evidence);
        std::cout << "PASS bounded public lower-approach Sniper measurements; receipts=" << evidence.string()
            << ". Controlled stations do not establish earned access, opposed survival or human acceptance\n";
        return 0;
    });
}
