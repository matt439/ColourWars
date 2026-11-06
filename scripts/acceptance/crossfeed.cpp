#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include "scripts/acceptance/support/probe.h"
#include "engine/assets/json.h"
#include <fstream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace
{
    // Paint underfoot ships on (POL#30). The repaint, home-loop and opposed-loop
    // comparisons require one matched route whatever the floor's owner and
    // whether anyone fires, so they run on a copy of the shipped tuning with
    // underfoot off; every other check keeps the shipped tuning.
    GameTuning underfoot_off(const std::filesystem::path& content)
    {
        std::ifstream input(content / "tuning.json", std::ios::binary);
        crossfeed_acceptance::require(input.good(), "Cannot read tuning for the underfoot-off repaint copy");
        const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
        rapidjson::Document document; document.Parse(bytes.c_str());
        crossfeed_acceptance::require(!document.HasParseError(), "Cannot parse tuning for the underfoot-off repaint copy");
        document["player"]["paint_underfoot"]["enabled"].SetBool(false);
        rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        crossfeed_acceptance::require(document.Accept(writer), "Cannot encode the underfoot-off repaint copy");
        return GameTuning(labrador::parse_json(buffer.GetString(), "crossfeed-repaint-underfoot-off.json"));
    }
}

int main(int argc, char** argv)
{
    using namespace crossfeed_acceptance;
    return acceptance::run_probe("Crossfeed", [&]
    {
        require(argc == 2, "Usage: crossfeed.exe <content directory>");
        Services services(argv[1], "crossfeed", "Crossfeed");
        std::cout << "NOTE Crossfeed H1-H8 baseline dt=" << dt <<
            "; actual authored Player/Scene; no devices, player save or position injection after construction\n";
        geometry_checks(services, std::filesystem::absolute(argv[0]).parent_path());
        spawn_checks(services);
        route_checks(services);
        tool_checks(services);
        combat_checks(services);
        pressure_checks(services);
        reciprocal_checks(services);
        trajectory_checks(services);
        supply_checks(services);
        painting_checks(services);
        {
            require(services.tuning.player().paint_underfoot.enabled, "Paint underfoot must ship enabled (POL#30)");
            const GameTuning shipped = services.tuning;
            services.tuning = underfoot_off(argv[1]);
            repaint_checks(services);
            home_loop_checks(services);
            opposed_loop_checks(services);
            services.tuning = shipped;
        }
        opening_checks(services);
        std::cout << "PASS Crossfeed baseline feasibility and measured score/tool access. "
            "Human pressure, camera warning, weapon balance and physical pads are outside this probe\n";
        return 0;
    });
}
