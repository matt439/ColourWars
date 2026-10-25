#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include <iostream>
#if COLOURWARS_DEV_TOOLS
#include "scripts/acceptance/balance_runner/runner.h"
#include "engine/core/file_path.h"
#endif

int wmain(int argc, wchar_t* argv[])
{
#if COLOURWARS_DEV_TOOLS
    try
    {
        if (argc < 2)
        {
            std::cerr << "usage: balance_runner <content> [--stage id ... --seed uint32 ... "
                "--players 1..4 --duration 0.05..600 --tools sprayer,sniper,... "
                "--bot paint|chase|walk|jump|idle --csv new-file]\n"
                "With <content> only, run bounded device-free self-tests.\n";
            return 2;
        }
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        if (argc == 2)
        {
            balance_runner::self_tests(content);
            return 0;
        }
        const std::vector<std::wstring> arguments(argv + 2, argv + argc);
        const balance_runner::Options options = balance_runner::parse_options(content, arguments);
        const std::vector<balance_runner::Result> results = balance_runner::run_sweep(options);
        balance_runner::write_csv(options.csv, results);
        std::cout << "PASS balance runner: " << results.size() << " completed matches; "
            << labrador::path_to_utf8(options.csv) << '\n'
            << "Aggregate held paint, pre-step empty-readiness ticks and observed death edges only. "
            "Credited eliminations, region/face ownership, skirmish AI and attended balance acceptance remain separate.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL balance runner: " << error.what() << '\n';
        return 1;
    }
#else
    (void)argc; (void)argv;
    std::cerr << "balance_runner requires Debug game objects; it is not a Release tool.\n";
    return 2;
#endif
}
