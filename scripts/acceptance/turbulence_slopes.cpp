#include "game/pch.h"
#include "scripts/acceptance/turbulence_slopes/fixtures.h"
#include <iostream>
#include <process.h>
#include <string_view>

int main(int argc, char** argv)
{
    using namespace turbulence_slopes_acceptance;
    try
    {
        require(argc == 2 || argc == 3 || (argc == 4 &&
            (std::string_view(argv[3]) == "--tools" || std::string_view(argv[3]) == "--comparison")) ||
            (argc == 6 && std::string_view(argv[2]) == "--author"),
            "turbulence_slopes <content> [isolated-trial.json] [--tools|--comparison] | <content> --author <mover-source.json> <tuning.json> <new-output.json>");
        const fs::path content = fs::absolute(argv[1]);
        if (argc == 6)
        {
            author_trial(content, fs::absolute(argv[3]), fs::absolute(argv[4]), fs::absolute(argv[5]));
            return 0;
        }
        const bool tools_requested = (argc == 3 && std::string_view(argv[2]) == "--tools") ||
            (argc == 4 && std::string_view(argv[3]) == "--tools");
        const bool comparison_requested = (argc == 3 && std::string_view(argv[2]) == "--comparison") ||
            (argc == 4 && std::string_view(argv[3]) == "--comparison");
        const auto source = content / "levels/turbulence_mover_trial.json";
        SourceGuard originals(content, content / "tuning.json", source);
        const auto evidence = fs::absolute(argv[0]).parent_path() / ("turbulence-slopes-" + std::to_string(_getpid()));
        require(!fs::exists(evidence), "Refusing earlier shoulder-trial receipts"); fs::create_directories(evidence);
        std::cout << "Shoulder trial receipts: " << evidence.string() << '\n';
        const bool explicit_stage = argc == 4 || (argc == 3 && !tools_requested && !comparison_requested);
        const auto stage = explicit_stage ? fs::absolute(argv[2]) : evidence / "turbulence_shoulder_trial.json";
        if (!explicit_stage) { author_trial(content, source, content / "tuning.json", stage); }
        World world(content); Context context{ world, content, stage, evidence };
        turbulence_slopes_acceptance::geometry(context);
        if (comparison_requested) { turbulence_slopes_acceptance::comparison_checks(context); }
        else if (tools_requested) { turbulence_slopes_acceptance::tool_checks(context); }
        else { turbulence_slopes_acceptance::routes(context); }
        originals.check();
        if (comparison_requested)
        {
            std::cout << "PASS isolated shoulder geometry comparison: matched controlled five-tool health and paint samples across "
                "static, mover and shoulder scenes. Geometry selection and attended balance remain open\n";
        }
        else if (tools_requested)
        {
            std::cout << "PASS isolated shoulder tool controls: public editor/LevelBuilder, independent score, mirrored flat-shoe blocking, "
                "ramp-pass damage controls and reachable top paint. Driven routes, native composition and attended acceptance are separate\n";
        }
        else
        {
            std::cout << "PASS isolated shoulder composite: public editor/LevelBuilder, independent score, driven up/down/mid-slope stops "
                "and lower recovery. Existing ramp passes paint; shoe is flat scored solid. Useful paint/shot/native and attended acceptance remain separate\n";
        }
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL Turbulence shoulders: " << error.what() << '\n'; return 1; }
}
