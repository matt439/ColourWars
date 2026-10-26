#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <iostream>
#include <process.h>
#include <string_view>

int main(int argc, char** argv)
{
    using namespace turbulence_movers_acceptance;
    try
    {
        require(argc >= 2, "turbulence_movers <content> [trial.json] [--routes|--replay] | <content> --author <static.json> <tuning.json> <new-output.json>");
        const fs::path content = fs::absolute(argv[1]);
        if (argc == 6 && std::string_view(argv[2]) == "--author")
        {
            author_trial(fs::absolute(argv[3]), fs::absolute(argv[4]), fs::absolute(argv[5]));
            return 0;
        }
        require(argc <= 4, "Unexpected mover-trial argument count");
        fs::path stage = content / "levels/turbulence_mover_trial.json";
        bool full_matrix = false;
        bool replay_requested = false;
        bool explicit_stage = false;
        for (int index = 2; index < argc; ++index)
        {
            if (std::string_view(argv[index]) == "--routes") { full_matrix = true; }
            else if (std::string_view(argv[index]) == "--replay") { replay_requested = true; }
            else { stage = fs::absolute(argv[index]); explicit_stage = true; }
        }
        require(static_cast<int>(full_matrix) + static_cast<int>(replay_requested) <= 1,
            "Select only one explicit mover-trial mode");
        const fs::path evidence = fs::absolute(argv[0]).parent_path() /
            ("turbulence-movers-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing to overwrite previous mover-trial receipts");
        fs::create_directories(evidence);
        std::cout << "Mover trial receipts: " << evidence.string() << '\n';
        if (!explicit_stage && !fs::exists(stage))
        {
            stage = evidence / "turbulence_mover_trial.json";
            author_trial(content / "levels/turbulence_blockout.json", content / "tuning.json", stage);
        }
        World world(content);
        Context context{ world, content, stage, evidence };
        geometry(context);
        if (replay_requested) { replay(context); return 0; }
        routes(context, full_matrix);
        std::cout << "PASS isolated horizontal mover placement: editor-authored source, public 1/4 loads, "
            "local score and bounded driven boarding/carry/reversal/escape. "
            "Five-tool role, seeded replay, native composition and attended acceptance remain separate\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL Turbulence mover trial: " << error.what() << '\n'; return 1; }
}
