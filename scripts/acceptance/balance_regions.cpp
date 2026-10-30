#include "game/pch.h"
#include "scripts/acceptance/balance_regions/metrics.h"
#include "scripts/acceptance/support/probe.h"
#include <iostream>
#include <stdexcept>

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("balance regions", [&]
    {
        if (argc != 2) { throw std::invalid_argument("usage: balance_regions <content>"); }
        balance_regions::self_tests(std::filesystem::absolute(argv[1]));
        std::cout << "PASS balance regions: convex-quad band clipping, authored score authority, outside pieces, "
            "shared edges, independent faces, the Turbulence family x_band partitions, validated/uncredited ownership, source-float rounding/order, "
            "invalid inputs and deterministic locale-independent companion bytes. Actual runner pairing remains an integration gate.\n";
        return 0;
    });
}
