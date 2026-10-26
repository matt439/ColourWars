#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/input_slots.h"
#include "game/paint_owners.h"
#include "game/objects/player.h"
#include "engine/core/file_path.h"
#include <algorithm>
#include <cmath>
#include <charconv>
#include <cstdlib>
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
        std::vector<std::string> fields(const std::string& row)
        {
            std::vector<std::string> values(1); bool quoted = false;
            for (std::size_t at = 0; at < row.size(); ++at)
            {
                const char value = row[at];
                if (value == '"')
                {
                    if (quoted && at + 1 < row.size() && row[at + 1] == '"') { values.back() += '"'; ++at; }
                    else { quoted = !quoted; }
                }
                else if (value == ',' && !quoted) { values.emplace_back(); }
                else { values.back() += value; }
            }
            require(!quoted, "Unclosed CSV text field"); return values;
        }
        std::size_t column(const std::vector<std::string>& header, const char* name)
        {
            const auto found = std::find(header.begin(), header.end(), name);
            require(found != header.end(), std::string("Missing CSV join column: ") + name);
            return static_cast<std::size_t>(std::distance(header.begin(), found));
        }
        void paint_joins(const std::vector<Result>& rows)
        {
            const std::vector<std::string> main_header = fields(csv_header());
            const std::string paint = paint_csv_text(rows);
            const auto header_end = paint.find('\n');
            require(header_end != std::string::npos && paint.find('\r') == std::string::npos, "Companion needs a complete LF header");
            const auto header = fields(paint.substr(0, header_end));
            require(main_header.size() == 81 && header.size() == 29, "Main 81/companion 29 column contract changed");
            std::vector<bool> seen(rows.size(), false);
            for (std::size_t start = header_end + 1; start < paint.size();)
            {
                const auto end = paint.find('\n', start); require(end != std::string::npos, "Incomplete companion row");
                const auto row = fields(paint.substr(start, end - start));
                require(row.size() == header.size(), "Companion row does not have 29 fields");
                const std::string& number = row.at(column(header, "match_row"));
                std::size_t match = 0; const auto parsed = std::from_chars(number.data(), number.data() + number.size(), match);
                require(parsed.ec == std::errc{} && parsed.ptr == number.data() + number.size() && match > 0 && match <= rows.size(),
                    "Companion main-row join is invalid");
                seen[match - 1] = true;
                const auto main = fields(csv_row(rows[match - 1]));
                for (const char* name : { "stage", "seed", "players", "bot", "requested_active_seconds", "final_fingerprint" })
                { require(row.at(column(header, name)) == main.at(column(main_header, name)), std::string("Companion metadata differs from main row: ") + name); }
                require(row.at(column(header, "region_schema_version")) == "1" && row.at(column(header, "main_schema_version")) == "2",
                    "Companion schema/main-schema join changed");
                start = end + 1;
            }
            require(std::all_of(seen.begin(), seen.end(), [](bool value) { return value; }), "A main match has no companion cells");
        }
        void fresh_bytes(const std::filesystem::path& path, const std::string& bytes)
        {
            const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            require(file != INVALID_HANDLE_VALUE, "Cannot exclusively create private sentinel");
            DWORD written = 0;
            const bool complete = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
                written == bytes.size() && FlushFileBuffers(file);
            const bool closed = CloseHandle(file) != FALSE;
            require(complete && closed, "Cannot finish private sentinel");
        }
        class StageReadLock final
        {
        public:
            explicit StageReadLock(const std::filesystem::path& path) : file_(CreateFileW(path.c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
            { require(this->file_ != INVALID_HANDLE_VALUE, "Cannot hold actual main stage without delete sharing"); }
            ~StageReadLock() { if (this->file_ != INVALID_HANDLE_VALUE) { CloseHandle(this->file_); } }
            StageReadLock(const StageReadLock&) = delete;
            StageReadLock& operator=(const StageReadLock&) = delete;
        private:
            HANDLE file_ = INVALID_HANDLE_VALUE;
        };
        void resolved_alias_check(const std::filesystem::path& content)
        {
            const auto logical = content.parent_path().parent_path().parent_path();
            if (logical.filename() != L"out" || !std::filesystem::is_directory(logical))
            { std::cout << "LIMIT resolved-parent alias fixture: staged content is not under the known out directory.\n"; return; }
            const HANDLE directory = CreateFileW(logical.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (directory == INVALID_HANDLE_VALUE)
            { std::cout << "LIMIT resolved-parent alias fixture: directory handle unavailable (Windows code " << GetLastError() << ").\n"; return; }
            const DWORD count = GetFinalPathNameByHandleW(directory, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
            std::vector<wchar_t> buffer(static_cast<std::size_t>(count) + 1, L'\0');
            const DWORD written = count ? GetFinalPathNameByHandleW(directory, buffer.data(), static_cast<DWORD>(buffer.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS) : 0;
            const DWORD error = written ? ERROR_SUCCESS : GetLastError();
            const bool closed = CloseHandle(directory) != FALSE;
            if (!closed || written == 0 || written >= buffer.size())
            { std::cout << "LIMIT resolved-parent alias fixture: physical name unavailable (Windows code " << error << ").\n"; return; }
            const std::filesystem::path physical(std::wstring(buffer.data(), written));
            std::filesystem::path first, second;
            for (unsigned int suffix = 0; suffix < 1000; ++suffix)
            {
                const auto leaf = L"colourwars_balance_alias_unused_" + std::to_wstring(_getpid()) + L"_" + std::to_wstring(suffix) + L".csv";
                first = logical / leaf; second = physical / leaf;
                if (!std::filesystem::exists(first) && !std::filesystem::exists(second)) { break; }
            }
            require(!std::filesystem::exists(first) && !std::filesystem::exists(second), "Cannot select new alias-control leaves");
            refuses([&] { CsvOutputs alias(first, second); }, "Resolved parent-directory aliases accepted as distinct destinations");
            require(!std::filesystem::exists(first) && !std::filesystem::exists(second), "Alias refusal created an output");
            refuses([&] { CsvOutputs ancestor(first, second / L"nested" / L"paint.csv"); },
                "Resolved parent alias hid a final-as-parent pair");
            require(!std::filesystem::exists(first) && !std::filesystem::exists(second),
                "Aliased ancestor refusal created a final/parent directory");
            std::cout << "PASS resolved-parent alias refusal via existing directory handle; no junction mutation.\n";
        }
        void publication_checks(const std::filesystem::path& evidence, const std::vector<Result>& rows)
        {
            const auto main = evidence / L"rows-\u6e2c\u8a66.csv", paint = evidence / L"paint-\u6e2c\u8a66.csv";
            const std::string expected = csv_text(rows), expected_paint = paint_csv_text(rows);
            std::srand(5731u); const int expected_random = std::rand(); std::srand(5731u);
            CsvOutputs outputs(main, paint);
            require(!std::filesystem::exists(main) && !std::filesystem::exists(paint), "Preflight exposed an incomplete final CSV");
            // The prior publisher's diagnostic path establishes the next
            // single-threaded stage sequence, with no mutable/public test hook.
            const std::wstring stamp = outputs.paint_stage_path().stem().native();
            const auto dash = stamp.rfind(L'-');
            require(dash != std::wstring::npos, "Unexpected diagnostic stage name");
            const auto sequence = std::stoull(stamp.substr(dash + 1));
            require(sequence < (std::numeric_limits<unsigned long long>::max)() - 2, "Stage sequence cannot form collision control");
            const std::wstring prefix = stamp.substr(0, dash + 1), extension = outputs.paint_stage_path().extension().native();
            std::wstring main_leaf = prefix + std::to_wstring(sequence + 1) + extension;
            if (main_leaf.size() > 1 && main_leaf[1] == L'c') { main_leaf[1] = L'C'; } // Exercise ordinal case-insensitive exclusion.
            const auto collision_main = evidence / main_leaf;
            const auto collision_paint = evidence / (prefix + std::to_wstring(sequence + 2) + extension);
            CsvOutputs colliding(collision_main, collision_paint);
            require(!std::filesystem::exists(collision_main) && !std::filesystem::exists(collision_paint),
                "Private stage reservation exposed a requested final before simulation/publication");
            colliding.stage(rows); colliding.publish();
            require(read_bytes(collision_main) == expected && read_bytes(collision_paint) == expected_paint,
                "Excluding both predicted final leaves prevented valid paired publication");
            refuses([&] { outputs.publish(); }, "Unstaged CSV pair was published");
            outputs.stage(rows);
            require(read_bytes(outputs.main_stage_path()) == expected && read_bytes(outputs.paint_stage_path()) == expected_paint &&
                !std::filesystem::exists(main) && !std::filesystem::exists(paint), "Both stages must be exact and closed before either final is visible");
            outputs.publish();
            require(read_bytes(main) == expected && read_bytes(paint) == expected_paint && std::rand() == expected_random,
                "Unicode paired publication changed row bytes or global RNG");
            refuses([&] { CsvOutputs duplicate(main, evidence / L"unused-paint.csv"); }, "Existing main CSV was accepted");
            refuses([&] { CsvOutputs duplicate(evidence / L"unused-main.csv", paint); }, "Existing paint CSV was accepted");
            require(read_bytes(main) == expected && read_bytes(paint) == expected_paint, "No-overwrite refusal altered completed files");
            refuses([&] { CsvOutputs alias(evidence / L"same.csv", evidence / L"SAME.CSV"); }, "Case aliases accepted as distinct CSV destinations");
            for (const wchar_t* leaf : { L"trim.csv.", L"trim.csv ", L"stream.csv:other", L"CON.csv", L"COM1.csv", L"LPT\u00b2.csv", L"CONIN$.csv", L"ROWS~1.CSV" })
            { refuses([&] { CsvOutputs invalid(evidence / leaf, evidence / L"invalid-paint.csv"); }, "Ambiguous Windows output leaf was accepted"); }
            refuses([&] { CsvOutputs invalid(std::filesystem::path(std::wstring(L"nul\0.csv", 8)), evidence / L"invalid-paint.csv"); }, "Embedded NUL output path was accepted");
            refuses([&] { CsvOutputs invalid(std::filesystem::path(LR"(\\.\NUL)"), evidence / L"invalid-paint.csv"); }, "Device namespace output accepted");
            refuses([&] { CsvOutputs invalid(std::filesystem::path(LR"(\\?\GLOBALROOT\Device\HarddiskVolume1\rows.csv)"), evidence / L"invalid-paint.csv"); }, "GLOBALROOT device output accepted");
            const auto ancestor = evidence / L"ancestor-final.csv";
            refuses([&] { CsvOutputs invalid(ancestor, ancestor / L"nested" / L"paint.csv"); }, "Main final accepted as second output ancestor");
            require(!std::filesystem::exists(ancestor), "Malformed pair created a directory at its main final");
            refuses([&] { CsvOutputs invalid(ancestor / L"nested" / L"main.csv", ancestor); }, "Paint final accepted as main output ancestor");
            require(!std::filesystem::exists(ancestor), "Inverse malformed pair created its paint final/ancestor");
            for (const wchar_t* ambiguous : { L"ancestor-final.csv.", L"ancestor-final.csv ", L"ANCEST~1" })
            {
                refuses([&] { CsvOutputs invalid(ancestor, evidence / ambiguous / L"paint.csv"); }, "Ambiguous missing parent suffix was accepted");
                require(!std::filesystem::exists(ancestor) && !std::filesystem::exists(evidence / ambiguous),
                    "Ambiguous parent refusal created a final or suffix directory");
            }
            const auto blocked = evidence / L"blocking-parent";
            fresh_bytes(blocked, "parent-sentinel");
            refuses([&] { CsvOutputs invalid(evidence / L"invalid-main.csv", blocked / L"paint.csv"); }, "Invalid second parent was accepted");
            require(!std::filesystem::exists(evidence / L"invalid-main.csv") && read_bytes(blocked) == "parent-sentinel", "Invalid second target touched the first output or parent sentinel");
            const auto raced = evidence / L"raced-main.csv", untouched = evidence / L"raced-paint.csv";
            CsvOutputs late(raced, untouched); late.stage(rows); fresh_bytes(raced, "late-sentinel");
            refuses([&] { late.publish(); }, "A destination created after preflight was replaced");
            require(read_bytes(raced) == "late-sentinel" && !std::filesystem::exists(untouched), "Late-race refusal changed a final destination");
            const auto failed_main = evidence / L"partial-main.csv", orphan = evidence / L"orphan-paint.csv";
            CsvOutputs partial(failed_main, orphan); partial.stage(rows);
            StageReadLock locked(partial.main_stage_path());
            bool failed = false;
            try { partial.publish(); }
            catch (const std::runtime_error& error) { failed = std::string(error.what()).find("Partial pair:") != std::string::npos; }
            require(failed && !std::filesystem::exists(failed_main) && read_bytes(orphan) == expected_paint &&
                read_bytes(partial.main_stage_path()) == expected, "Actual sharing failure did not preserve explicit orphan companion and complete main stage");
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
            require(result.paint_report.has_value() && result.paint_report->sample_totals().total > 0.0 &&
                std::isfinite(result.paint_report->allocation_tolerance()), "Completed real match has no valid final paint snapshot report");
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
        require(zero.paint_csv == zero.csv.parent_path() / (zero.csv.stem().native() + L".paint.csv"),
            "Default paint companion must be the main stem sibling");
        const auto explicit_paint = parser_csv.parent_path() / (parser_csv.stem().native() + L".explicit.paint.csv");
        const Options separate = parsed({ L"--paint-csv", explicit_paint.native() });
        require(separate.paint_csv == std::filesystem::absolute(explicit_paint).lexically_normal(),
            "Explicit wide companion path was lost");
        refuses([&] { (void)parsed({ L"--paint-csv", parser_csv.native() }); }, "Same main/paint target accepted");
        refuses([&] { (void)parsed({ L"--paint-csv", (content / L"manifest.json").native() }); }, "Existing second target accepted");
        refuses([&] { (void)parsed({ L"--paint-csv", explicit_paint.native(), L"--paint-csv", explicit_paint.native() }); }, "Duplicate companion option accepted");
        refuses([&] { (void)parsed({ L"--paint-csv", L"" }); }, "Empty explicit companion option accepted");
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
        require(paint_csv_text(first) == paint_csv_text(repeated), "Fresh same-seed matches did not reproduce companion CSV bytes");
        paint_joins(first);


        Options sweep = zero;
        sweep.duration = 0.25f; sweep.seeds = { 0, std::numeric_limits<std::uint32_t>::max() };
        const std::vector<Result> rows = run_sweep(sweep);
        require(rows.size() == 2 && rows[0].seed == 0 &&
            rows[1].seed == std::numeric_limits<std::uint32_t>::max(),
            "Short Crossfeed sweep lost seed order or uint32 boundary seeds");
        for (const Result& row : rows) { result_invariants(row); }
        paint_joins(rows);
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
        publication_checks(evidence, rows);
        resolved_alias_check(content);
        std::cout << "PASS balance runner: 4 real CPU matches, nontrivial active state, whole-row seeded byte identity, "
            "2/4-player device-less owner mapping, 0/uint32-max seeds, bounded Crossfeed sweep, CLI refusals, "
            "exact Unicode paired-file publication, no-overwrite and actual partial-failure preservation. "
            "Metrics are held paint, pre-step full-shot empty readiness, sampled death edges and validated enemy credit; "
            "final region/face ownership is measured; physical device, listening, statistical reports and balance acceptance remain open.\n"
            << "Private CSV evidence: " << labrador::path_to_utf8(evidence) << '\n';
    }
}
#endif
