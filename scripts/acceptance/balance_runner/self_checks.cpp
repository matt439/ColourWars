#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/input_slots.h"
#include "game/paint_owners.h"
#include "engine/core/file_path.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <process.h>
#include <stdexcept>

namespace balance_runner
{
    namespace
    {
        void require(bool condition, const std::string& message)
        {
            if (!condition) { throw std::runtime_error("Self-check: " + message); }
        }
        template<class Work> void refuses(Work work, const std::string& reason)
        {
            bool rejected = false;
            try { work(); }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, reason);
        }
        std::string read_bytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            require(file.good(), "Cannot read private CSV");
            return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        }
        void result_invariants(const Result& result)
        {
            require(result.active_ticks > 0 && result.activity_observed,
                "Real active logic and nontrivial player motion/ammo activity must occur");
            require(std::isfinite(result.paintable_area) && result.paintable_area > 0.0f,
                "Actual stage has no finite paint capacity");
            const double painted = static_cast<double>(result.team_a_area) + result.team_b_area;
            const double tolerance = std::max(0.5, static_cast<double>(result.paintable_area) * 0.00001);
            require(result.team_a_area >= 0.0f && result.team_b_area >= 0.0f &&
                painted <= static_cast<double>(result.paintable_area) + tolerance,
                "Owned area is outside the actual paint capacity");
            double seats = 0.0;
            for (int view = 0; view < result.players; ++view)
            {
                const SeatResult& seat = result.seats.at(static_cast<std::size_t>(view));
                require(seat.paint_owner == paint_owners::index(-1, view) &&
                    seat.paint_owner == input_slots::count + view,
                    "Device-less credit was confused with view or physical input-slot indices");
                require(seat.team == (view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b),
                    "Actual builder changed the declared alternating teams");
                require(seat.fire_starved_ticks <= seat.ammo_starved_ticks &&
                    seat.ammo_starved_ticks <= seat.alive_ticks && seat.alive_ticks <= result.active_ticks,
                    "Pre-step affordability/fire/alive clocks do not nest");
                require(seat.observed_death_edges <= result.active_ticks &&
                    std::isfinite(seat.final_health) && std::isfinite(seat.final_ammo),
                    "Observed state edges or resource samples are invalid");
                require(std::isfinite(seat.paint_area) && seat.paint_area >= 0.0f,
                    "Per-seat held paint is invalid");
                seats += seat.paint_area;
            }
            require(std::fabs(seats - painted) <= tolerance,
                "Actual per-owner paint does not reconcile with team-held area");
            require(csv_row(result).find('\n') == std::string::npos &&
                csv_row(result).find('\r') == std::string::npos,
                "One completed match must occupy one CSV row");
        }
    }

    void self_tests(const std::filesystem::path& content)
    {
        const std::filesystem::path parser_csv = std::filesystem::temp_directory_path() /
            (L"colourwars_balance_parser_unused_" + std::to_wstring(_getpid()) + L".csv");
        const auto parsed = [&](std::vector<std::wstring> arguments)
        {
            arguments.push_back(L"--csv"); arguments.push_back(parser_csv.native());
            return parse_options(content, arguments);
        };
        const Options zero = parsed({ L"--seed", L"0", L"--players", L"4", L"--duration", L"0.25",
            L"--tools", L"sniper,roller,mister,bucket", L"--bot", L"paint" });
        require(zero.seeds.size() == 1 && zero.seeds[0] == 0 && zero.players == 4 &&
            zero.tools[0] == WeaponType::sniper && zero.tools[3] == WeaponType::bucket,
            "Valid zero seed or concrete roster did not survive CLI parsing");
        const Options bounds = parsed({ L"--seed", L"4294967295", L"--stage", L"crossfeed", L"--stage", L"press_tower" });
        require(bounds.seeds[0] == std::numeric_limits<std::uint32_t>::max() && bounds.stages.size() == 2,
            "The complete uint32 seed range and repeated stage options must be supported");
        for (const std::vector<std::wstring>& invalid : std::vector<std::vector<std::wstring>>{
            { L"--players", L"0" }, { L"--players", L"5" }, { L"--seed", L"-1" },
            { L"--seed", L"4294967296" }, { L"--seed", L"1x" }, { L"--duration", L"nan" },
            { L"--duration", L"inf" }, { L"--duration", L"0" }, { L"--duration", L"601" },
            { L"--duration", L"0.05x" }, { L"--tools", L"sprayer" },
            { L"--tools", L"sprayer,random" }, { L"--bot", L"skirmish" },
            { L"--stage", L"crossfeed", L"--stage", L"crossfeed" },
            { L"--seed", L"0", L"--seed", L"0" },
            { L"--players", L"2", L"--players", L"2" }, { L"--unknown", L"value" }, { L"--duration" }
        })
        {
            refuses([&] { (void)parsed(invalid); }, "Invalid CLI request was accepted");
        }
        refuses([&] { (void)parse_options(content, { L"--csv", (content / L"manifest.json").native() }); },
            "An existing output file was accepted");
        refuses([&] { (void)parse_options(content, { L"--seed", L"1" }); }, "A CLI run without a CSV path was accepted");

        Options short_match;
        short_match.content = content; short_match.duration = 1.0f; short_match.seeds = { 12345 };
        short_match.tools = { WeaponType::sprayer, WeaponType::bucket };
        const std::vector<Result> first = run_sweep(short_match);
        const std::vector<Result> repeated = run_sweep(short_match);
        require(first.size() == 1 && repeated.size() == 1 && csv_text(first) == csv_text(repeated),
            "Fresh same-seed matches did not reproduce the complete CSV bytes");
        result_invariants(first.front());


        Options sweep = zero;
        sweep.duration = 0.25f; sweep.seeds = { 0, std::numeric_limits<std::uint32_t>::max() };
        const std::vector<Result> rows = run_sweep(sweep);
        require(rows.size() == 2 && rows[0].seed == 0 &&
            rows[1].seed == std::numeric_limits<std::uint32_t>::max(),
            "Short Crossfeed sweep lost seed order or uint32 boundary seeds");
        for (const Result& row : rows) { result_invariants(row); }
        Options unknown = short_match;
        unknown.stages = { "crossfeed", "not_a_manifest_stage" };
        refuses([&] { (void)run_sweep(unknown); }, "A non-manifest stage was allowed to build");

        // A real Unicode filename and a refused overwrite verify publication
        // independently of string formatting. These small private receipts stay
        // in TEMP, including on failure; no authored data or player save is touched.
        std::filesystem::path evidence;
        for (unsigned int suffix = 0; suffix < 1000; ++suffix)
        {
            const auto candidate = std::filesystem::temp_directory_path() /
                (L"colourwars_balance_\u6e2c\u8a66_" + std::to_wstring(_getpid()) + L"_" + std::to_wstring(suffix));
            if (std::filesystem::create_directory(candidate)) { evidence = candidate; break; }
        }
        require(!evidence.empty(), "Cannot allocate a fresh private CSV receipt directory");
        const auto csv = evidence / L"rows-\u6e2c\u8a66.csv";
        write_csv(csv, rows);
        const std::string expected = csv_text(rows);
        require(read_bytes(csv) == expected && expected.find('\r') == std::string::npos &&
            expected.rfind("schema_version,", 0) == 0,
            "Published CSV must preserve exact UTF-8/no-BOM/LF bytes");
        refuses([&] { write_csv(csv, rows); }, "A previous CSV was overwritten");
        require(read_bytes(csv) == expected, "Refused overwrite altered existing bytes");
        std::cout << "PASS balance runner: 4 real CPU matches, nontrivial active state, whole-row seeded byte identity, "
            "2/4-player device-less owner mapping, 0/uint32-max seeds, bounded Crossfeed sweep, CLI refusals, "
            "exact Unicode-file CSV publication and no-overwrite. "
            "Metrics are held paint, pre-step full-shot empty readiness and sampled death edges; "
            "no killer credit, region/face ownership, physical device, listening or balance acceptance.\n"
            << "Private CSV evidence: " << labrador::path_to_utf8(csv) << '\n';
    }
}
#endif
