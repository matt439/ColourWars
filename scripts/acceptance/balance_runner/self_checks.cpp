#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/input_slots.h"
#include "game/paint_owners.h"
#include "game/objects/player.h"
#include "engine/core/file_path.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
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
            std::uint64_t credits = 0, credited_deaths = 0, uncredited = 0;
            for (const SeatResult& seat : result.seats)
            {
                credits += seat.credited_enemy_eliminations;
                credited_deaths += seat.credited_enemy_projectile_deaths;
                uncredited += seat.uncredited_projectile_lethals;
            }
            require(credits == result.credited_enemy_eliminations && credited_deaths == credits &&
                uncredited == result.uncredited_projectile_lethals, "Credited/uncredited lethal totals do not reconcile");
            require(std::fabs(seats - painted) <= tolerance,
                "Actual per-owner paint does not reconcile with team-held area");
            require(csv_row(result).find('\n') == std::string::npos &&
                csv_row(result).find('\r') == std::string::npos,
                "One completed match must occupy one CSV row");
        }
        void attribution_checks()
        {
            Result result;
            result.players = 4;
            for (int view = 0; view < result.players; ++view)
            {
                SeatResult& seat = result.seats.at(static_cast<std::size_t>(view));
                seat.paint_owner = paint_owners::index(-1, view);
                seat.team = view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
            }
            std::vector<PlayerLethalProjectiles> victims(4);
            const auto owner = [&](int view) { return static_cast<std::size_t>(result.seats.at(static_cast<std::size_t>(view)).paint_owner); };
            victims[1].enemy_by_owner[owner(0)] = 2;
            victims[1].enemy_by_owner[owner(2)] = 1;
            victims[0].enemy_by_owner[owner(1)] = 1;
            victims[2].enemy_by_owner[owner(2)] = 3; // Self provenance cannot earn credit.
            victims[2].enemy_by_owner[owner(0)] = 4; // Same-team provenance cannot earn credit.
            victims[3].enemy_by_owner[0] = 5;        // Valid owner index, absent from this roster.
            victims[3].uncredited = 2;              // Invalid/unowned/non-enemy source metadata.
            attribute_eliminations(result, victims);
            require(result.credited_enemy_eliminations == 4 && result.uncredited_projectile_lethals == 14 &&
                result.seats[0].credited_enemy_eliminations == 2 && result.seats[1].credited_enemy_eliminations == 1 &&
                result.seats[2].credited_enemy_eliminations == 1 && result.seats[3].credited_enemy_eliminations == 0 &&
                result.seats[1].credited_enemy_projectile_deaths == 3 && result.seats[0].credited_enemy_projectile_deaths == 1 &&
                result.seats[2].uncredited_projectile_lethals == 7 && result.seats[3].uncredited_projectile_lethals == 7,
                "Owner/roster/opponent/self validation did not preserve raw lethal provenance");
            const std::string once = csv_row(result);
            attribute_eliminations(result, victims);
            require(csv_row(result) == once, "Repeated lethal snapshots accumulated duplicate credit");
            Result invalid = result;
            invalid.seats[1].paint_owner = invalid.seats[0].paint_owner;
            refuses([&] { attribute_eliminations(invalid, victims); }, "Duplicate roster owners accepted for credit");
            invalid = result; invalid.seats[0].paint_owner = paint_owners::count;
            refuses([&] { attribute_eliminations(invalid, victims); }, "Invalid roster owner accepted for credit");
            const std::string header = csv_header();
            const std::size_t extension = header.find(",credited_enemy_eliminations,uncredited_projectile_lethals");
            require(extension != std::string::npos &&
                std::count(header.begin(), header.begin() + static_cast<std::ptrdiff_t>(extension), ',') == 66 &&
                std::count(header.begin(), header.end(), ',') == 80 && csv_row(result).rfind("2,", 0) == 0,
                "Schema2 must append14 columns after the67-field legacy schema");
        }
    }

    void self_tests(const std::filesystem::path& content)
    {
        attribution_checks();
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
        const Options skirmish = parsed({ L"--seed", L"0", L"--players", L"4", L"--duration", L"0.25",
            L"--tools", L"sniper,roller,mister,bucket", L"--bot", L"skirmish" });
        require(skirmish.bot == "skirmish" && skirmish.seeds == zero.seeds &&
            skirmish.players == zero.players && skirmish.tools == zero.tools && skirmish.duration == zero.duration,
            "The registered skirmish bot did not preserve the CLI zero seed, concrete roster and duration");
        for (const std::vector<std::wstring>& invalid : std::vector<std::vector<std::wstring>>{
            { L"--players", L"0" }, { L"--players", L"5" }, { L"--seed", L"-1" },
            { L"--seed", L"4294967296" }, { L"--seed", L"1x" }, { L"--duration", L"nan" },
            { L"--duration", L"inf" }, { L"--duration", L"0" }, { L"--duration", L"601" },
            { L"--duration", L"0.05x" }, { L"--tools", L"sprayer" },
            { L"--tools", L"sprayer,random" }, { L"--bot", L"not_a_bot" },
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
            "Metrics are held paint, pre-step full-shot empty readiness, sampled death edges and validated enemy credit; "
            "region/face ownership, physical device, listening and balance acceptance remain open.\n"
            << "Private CSV evidence: " << labrador::path_to_utf8(csv) << '\n';
    }
}
#endif
