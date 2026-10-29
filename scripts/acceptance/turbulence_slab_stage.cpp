#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_stage/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>
#include <process.h>

int main(int argc, char** argv)
{
    return acceptance::run_probe("registered slab stage", [&]
    {
        using namespace turbulence_slab_stage_acceptance;
        require(argc == 3, "turbulence_slab_stage <staged-content> <repository>");
        const auto evidence = fs::absolute(argv[0]).parent_path() /
            ("turbulence-slab-stage-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing earlier registered slab stage receipts");
        fs::create_directories(evidence);
        qualify(fs::absolute(argv[1]), fs::absolute(argv[2]), evidence);
        std::cout << "PASS registered slab stage: source/staged identity, explicit-only selection and public1/2/3/4P short endings. "
            "CPU preconditions only; no native rendering or human balance acceptance. Receipts=" << evidence.string() << '\n';
        return 0;
    });
}
