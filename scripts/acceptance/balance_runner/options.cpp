#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "engine/core/step_timer.h"
#include "engine/core/file_path.h"
#include <algorithm>
#include <bit>
#include <atomic>
#include <cstring>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

namespace balance_runner
{
    namespace
    {
        std::string ascii(const std::wstring& value)
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t character : value)
            {
                if (static_cast<unsigned long>(character) > 127UL)
                { throw std::invalid_argument("Option names, identifiers and numbers must be ASCII; file paths may be Unicode"); }
                result.push_back(static_cast<char>(character));
            }
            return result;
        }
        std::uint32_t unsigned_number(const std::string& value)
        {
            if (value.empty() || !std::all_of(value.begin(), value.end(),
                [](char c) { return c >= '0' && c <= '9'; }))
            { throw std::invalid_argument("Expected an unsigned decimal integer: " + value); }
            std::uint32_t result = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
            { throw std::invalid_argument("Unsigned integer is outside uint32: " + value); }
            return result;
        }
        float seconds(const std::string& value)
        {
            float result = 0.0f;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result,
                std::chars_format::general);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(result))
            { throw std::invalid_argument("Expected finite duration seconds: " + value); }
            return result;
        }
        WeaponType parse_tool(const std::string& name)
        {
            if (name == "sprayer") { return WeaponType::sprayer; }
            if (name == "sniper") { return WeaponType::sniper; }
            if (name == "roller") { return WeaponType::roller; }
            if (name == "mister") { return WeaponType::mister; }
            if (name == "bucket") { return WeaponType::bucket; }
            throw std::invalid_argument("Unknown concrete primary tool: " + name);
        }
        std::vector<WeaponType> parse_tools(const std::string& value)
        {
            std::vector<WeaponType> result;
            std::size_t begin = 0;
            for (;;)
            {
                const std::size_t end = value.find(',', begin);
                result.push_back(parse_tool(value.substr(begin,
                    end == std::string::npos ? end : end - begin)));
                if (end == std::string::npos) { return result; }
                begin = end + 1;
            }
        }
        std::string field(std::string_view value)
        {
            std::string result = "\"";
            for (char c : value)
            {
                if (c == '"') { result += '"'; }
                result += c;
            }
            return result + '"';
        }
        const char* team_name(PlayerTeam team)
        {
            if (team == PlayerTeam::a) { return "a"; }
            if (team == PlayerTeam::b) { return "b"; }
            return "none";
        }
        std::runtime_error file_error(const char* action, const std::filesystem::path& path, DWORD code)
        { return std::runtime_error(std::string(action) + " (Windows code " + std::to_string(code) + "): " + labrador::path_to_utf8(path)); }
        bool same_name(const std::wstring& first, const std::wstring& second)
        {
            const int comparison = CompareStringOrdinal(first.c_str(), -1, second.c_str(), -1, TRUE);
            if (comparison == 0) { throw std::invalid_argument("Cannot compare CSV output names"); }
            return comparison == CSTR_EQUAL;
        }
        void regular_csv_leaf(const std::filesystem::path& path)
        {
            // Tilde leaves can be DOS8.3 aliases of a different long output name.
            const std::wstring leaf = path.filename().native();
            if (leaf.empty() || leaf == L"." || leaf == L".." || leaf.back() == L'.' || leaf.back() == L' ' ||
                leaf.find_first_of(L"<>:\"/\\|?*~") != std::wstring::npos ||
                std::any_of(leaf.begin(), leaf.end(), [](wchar_t value) { return value < 32; }))
            { throw std::invalid_argument("CSV output needs an ordinary unambiguous Windows file leaf: " + labrador::path_to_utf8(path)); }
            std::wstring base = leaf.substr(0, leaf.find(L'.'));
            while (!base.empty() && (base.back() == L' ' || base.back() == L'.')) { base.pop_back(); }
            for (const wchar_t* reserved : { L"CON", L"PRN", L"AUX", L"NUL", L"CONIN$", L"CONOUT$",
                L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
                L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9",
                L"COM\u00b9", L"COM\u00b2", L"COM\u00b3", L"LPT\u00b9", L"LPT\u00b2", L"LPT\u00b3" })
            { if (same_name(base, reserved)) { throw std::invalid_argument("CSV output cannot use a Windows device name"); } }
        }
        void raw_missing_parent_suffix(const std::filesystem::path& path)
        {
            // Only uncreated suffix names need lexical restrictions. Existing
            // parent aliases are resolved later by their actual directory IDs.
            for (auto parent = path.parent_path(); !parent.empty();)
            {
                std::error_code error; const auto status = std::filesystem::status(parent, error);
                if (error && error != std::errc::no_such_file_or_directory)
                { throw std::invalid_argument("Cannot inspect CSV parent suffix: " + labrador::path_to_utf8(parent)); }
                if (std::filesystem::exists(status))
                {
                    if (!std::filesystem::is_directory(status)) { throw std::invalid_argument("CSV parent is not a directory"); }
                    return;
                }
                regular_csv_leaf(parent);
                const auto next = parent.parent_path(); if (next == parent) { break; } parent = next;
            }
        }
        std::filesystem::path output_path(const std::filesystem::path& input)
        {
            if (input.empty() || input.native().find(L'\0') != std::wstring::npos)
            { throw std::invalid_argument("CSV output path must be nonempty without embedded NULs"); }
            auto preferred = input; preferred.make_preferred();
            const auto filesystem_namespace = [](const std::wstring& native)
            {
                if (native.starts_with(LR"(\\.\)") || native.starts_with(LR"(\??\)"))
                { throw std::invalid_argument("CSV paths cannot name Windows device namespaces"); }
                if (native.starts_with(LR"(\\?\)"))
                {
                    const std::wstring tail = native.substr(4);
                    const bool drive = tail.size() >= 3 && ((tail[0] >= L'A' && tail[0] <= L'Z') ||
                        (tail[0] >= L'a' && tail[0] <= L'z')) && tail[1] == L':' && tail[2] == L'\\';
                    const bool unc = tail.size() > 4 && same_name(tail.substr(0, 4), L"UNC\\");
                    if (!drive && !unc) { throw std::invalid_argument("Extended CSV paths must name a drive or UNC filesystem"); }
                }
            };
            filesystem_namespace(preferred.native());
            regular_csv_leaf(preferred); // Reject ambiguous leaves before Windows can normalize them.
            raw_missing_parent_suffix(preferred);
            auto path = std::filesystem::absolute(preferred).lexically_normal(); path.make_preferred();
            filesystem_namespace(path.native());
            regular_csv_leaf(path);
            (void)labrador::path_to_utf8(path); // Validate the complete wide path at the text boundary.
            return path;
        }
        void require_new(const std::filesystem::path& path)
        {
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            { throw std::invalid_argument("CSV destination already exists: " + labrador::path_to_utf8(path)); }
            const DWORD code = GetLastError();
            if (code != ERROR_FILE_NOT_FOUND && code != ERROR_PATH_NOT_FOUND)
            { throw std::invalid_argument("Cannot inspect new CSV destination (Windows code " + std::to_string(code) + "): " + labrador::path_to_utf8(path)); }
        }
        FILE_ID_INFO directory_identity(const std::filesystem::path& path)
        {
            const HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (handle == INVALID_HANDLE_VALUE) { throw file_error("Cannot identify CSV parent", path, GetLastError()); }
            BY_HANDLE_FILE_INFORMATION attributes{};
            FILE_ID_INFO identity{};
            DWORD error = ERROR_SUCCESS;
            if (GetFileType(handle) != FILE_TYPE_DISK) { error = ERROR_INVALID_FUNCTION; }
            else if (!GetFileInformationByHandle(handle, &attributes)) { error = GetLastError(); }
            else if ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) { error = ERROR_DIRECTORY; }
            else if (!GetFileInformationByHandleEx(handle, FileIdInfo, &identity, static_cast<DWORD>(sizeof(identity)))) { error = GetLastError(); }
            if (!CloseHandle(handle) && error == ERROR_SUCCESS) { error = GetLastError(); }
            if (error != ERROR_SUCCESS) { throw file_error("Cannot identify CSV parent", path, error); }
            return identity;
        }
        void distinct_parents(const std::filesystem::path& main, const std::filesystem::path& paint)
        {
            const FILE_ID_INFO first = directory_identity(main.parent_path()), second = directory_identity(paint.parent_path());
            if (first.VolumeSerialNumber == second.VolumeSerialNumber &&
                std::memcmp(first.FileId.Identifier, second.FileId.Identifier, sizeof(first.FileId.Identifier)) == 0 &&
                same_name(main.filename().native(), paint.filename().native()))
            { throw std::invalid_argument("Main and paint CSV paths identify the same destination"); }
        }
        struct OutputPaths { std::filesystem::path main, paint; };
        struct FutureTarget
        {
            FILE_ID_INFO existing_parent{};
            std::vector<std::wstring> suffix; // Complete absent target relative to its existing directory anchor.
        };
        FutureTarget future_target(const std::filesystem::path& path)
        {
            std::vector<std::wstring> reversed{ path.filename().native() };
            for (auto parent = path.parent_path(); !parent.empty();)
            {
                std::error_code error; const auto status = std::filesystem::status(parent, error);
                if (error && error != std::errc::no_such_file_or_directory)
                { throw std::invalid_argument("Cannot inspect future CSV parent: " + labrador::path_to_utf8(parent)); }
                if (std::filesystem::exists(status))
                {
                    if (!std::filesystem::is_directory(status)) { throw std::invalid_argument("CSV parent is not a directory"); }
                    std::reverse(reversed.begin(), reversed.end());
                    return { directory_identity(parent), std::move(reversed) };
                }
                regular_csv_leaf(parent);
                reversed.push_back(parent.filename().native());
                const auto next = parent.parent_path(); if (next == parent) { break; } parent = next;
            }
            throw std::invalid_argument("CSV output has no accessible existing directory anchor");
        }
        OutputPaths checked_paths(const std::filesystem::path& main, const std::filesystem::path& paint)
        {
            OutputPaths result{ output_path(main), output_path(paint) };
            if (same_name(result.main.native(), result.paint.native()))
            { throw std::invalid_argument("Main and paint CSV destinations must be distinct"); }
            require_new(result.main); require_new(result.paint);
            const auto first = future_target(result.main), second = future_target(result.paint);
            if (first.existing_parent.VolumeSerialNumber == second.existing_parent.VolumeSerialNumber &&
                std::memcmp(first.existing_parent.FileId.Identifier, second.existing_parent.FileId.Identifier,
                    sizeof(first.existing_parent.FileId.Identifier)) == 0)
            {
                const auto count = std::min(first.suffix.size(), second.suffix.size());
                bool common = true;
                for (std::size_t index = 0; index < count; ++index)
                { common = common && same_name(first.suffix[index], second.suffix[index]); }
                if (common) { throw std::invalid_argument("CSV destinations must not alias or be an ancestor of one another"); }
            }
            return result;
        }
        class StagedCsv final
        {
        public:
            ~StagedCsv() { if (this->handle_ != INVALID_HANDLE_VALUE) { CloseHandle(this->handle_); } }
            StagedCsv() = default;
            StagedCsv(const StagedCsv&) = delete;
            StagedCsv& operator=(const StagedCsv&) = delete;
            const std::filesystem::path& path() const { return this->path_; }
            void reserve(const std::filesystem::path& parent, const std::wstring& main_leaf, const std::wstring& paint_leaf)
            {
                static std::atomic<std::uint64_t> sequence{ 0 }; // File names only; no gameplay RNG or row bytes.
                for (int attempt = 0; attempt < 1024; ++attempt)
                {
                    const std::wstring leaf = L".colourwars-balance-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                        std::to_wstring(sequence.fetch_add(1, std::memory_order_relaxed)) + L".tmp";
                    if (same_name(leaf, main_leaf) || same_name(leaf, paint_leaf)) { continue; }
                    auto candidate = parent / leaf;
                    const HANDLE handle = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (handle != INVALID_HANDLE_VALUE) { this->handle_ = handle; this->path_ = std::move(candidate); return; }
                    const DWORD code = GetLastError();
                    if (code != ERROR_FILE_EXISTS && code != ERROR_ALREADY_EXISTS) { throw file_error("Cannot exclusively reserve CSV stage", candidate, code); }
                }
                throw std::runtime_error("Cannot reserve a fresh CSV stage after 1024 collisions");
            }
            void write_and_close(const std::string& bytes)
            {
                if (this->handle_ == INVALID_HANDLE_VALUE) { throw std::invalid_argument("CSV stage is not open"); }
                for (std::size_t position = 0; position < bytes.size();)
                {
                    const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - position, 1024U * 1024U));
                    DWORD written = 0;
                    if (!WriteFile(this->handle_, bytes.data() + position, chunk, &written, nullptr))
                    { throw file_error("CSV stage write failed; preserve partial stage", this->path_, GetLastError()); }
                    if (written != chunk) { throw file_error("CSV stage write was incomplete; preserve partial stage", this->path_, ERROR_WRITE_FAULT); }
                    position += written;
                }
                if (!FlushFileBuffers(this->handle_)) { throw file_error("CSV stage flush failed; preserve stage", this->path_, GetLastError()); }
                const HANDLE handle = this->handle_; this->handle_ = INVALID_HANDLE_VALUE;
                if (!CloseHandle(handle)) { throw file_error("CSV stage close failed; preserve stage", this->path_, GetLastError()); }
            }
        private:
            HANDLE handle_ = INVALID_HANDLE_VALUE;
            std::filesystem::path path_;
        };
        double ratio(float numerator, double denominator)
        {
            // A zero denominator means no painted area, not an inferred 50% score.
            return denominator > 0.0 ? static_cast<double>(numerator) / denominator : 0.0;
        }
    }

    float fixed_dt()
    {
        return static_cast<float>(labrador::StepTimer::TicksToSeconds(
            labrador::StepTimer::TicksPerSecond / 60));
    }

    std::string tool_name(WeaponType tool)
    {
        switch (tool)
        {
        case WeaponType::sprayer: return "sprayer";
        case WeaponType::sniper: return "sniper";
        case WeaponType::roller: return "roller";
        case WeaponType::mister: return "mister";
        case WeaponType::bucket: return "bucket";
        case WeaponType::none: return "none";
        default: throw std::invalid_argument("Runner needs a concrete primary tool");
        }
    }

    void validate_options(const Options& options)
    {
        if (!std::filesystem::is_directory(options.content))
        { throw std::invalid_argument("Content directory does not exist"); }
        if (options.players < 1 || options.players > 4)
        { throw std::invalid_argument("Players must be in 1..4"); }
        if (!std::isfinite(options.duration) || options.duration < 0.05f || options.duration > 600.0f)
        { throw std::invalid_argument("Duration must be finite and in 0.05..600 seconds"); }
        if (options.tools.size() != static_cast<std::size_t>(options.players))
        { throw std::invalid_argument("Supply exactly one concrete tool per player"); }
        for (WeaponType tool : options.tools)
        {
            if (tool < WeaponType::sprayer || tool >= WeaponType::random_primary)
            { throw std::invalid_argument("Runner needs concrete primary tools only"); }
        }
        if (!Bot::is_behaviour(options.bot))
        { throw std::invalid_argument("Unknown bot behaviour: " + options.bot); }
        if (options.stages.empty() || options.stages.size() > 16 || options.seeds.empty() || options.seeds.size() > 64)
        { throw std::invalid_argument("A sweep needs 1..16 stages and 1..64 seeds"); }
        const std::set<std::string> stages(options.stages.begin(), options.stages.end());
        const std::set<std::uint32_t> seeds(options.seeds.begin(), options.seeds.end());
        if (stages.size() != options.stages.size() || seeds.size() != options.seeds.size())
        { throw std::invalid_argument("Duplicate stages or seeds would duplicate observations"); }
        if (std::any_of(options.stages.begin(), options.stages.end(),
            [](const std::string& stage) { return stage.empty(); }))
        { throw std::invalid_argument("Stage identifiers must not be empty"); }
    }

    Options parse_options(const std::filesystem::path& content,
        const std::vector<std::wstring>& arguments)
    {
        Options options;
        options.content = std::filesystem::absolute(content);
        std::set<std::string> seen;
        bool stages_started = false, seeds_started = false;
        for (std::size_t at = 0; at < arguments.size(); ++at)
        {
            const std::string option = ascii(arguments[at]);
            if (option != "--stage" && option != "--seed" && option != "--players" &&
                option != "--duration" && option != "--tools" && option != "--bot" && option != "--csv" && option != "--paint-csv")
            { throw std::invalid_argument("Unknown option: " + option); }
            if (at + 1 >= arguments.size()) { throw std::invalid_argument("Missing value for " + option); }
            if (option != "--stage" && option != "--seed" && !seen.insert(option).second)
            { throw std::invalid_argument("Duplicate option: " + option); }
            const std::wstring& wide_value = arguments[++at];
            if (option == "--csv" || option == "--paint-csv")
            {
                if (wide_value.empty()) { throw std::invalid_argument("CSV path must not be empty"); }
                const std::filesystem::path path(wide_value);
                if (option == "--csv") { options.csv = path; } else { options.paint_csv = path; }
                continue;
            }
            const std::string value = ascii(wide_value);
            if (option == "--stage")
            {
                if (!stages_started) { options.stages.clear(); stages_started = true; }
                options.stages.push_back(value);
            }
            else if (option == "--seed")
            {
                if (!seeds_started) { options.seeds.clear(); seeds_started = true; }
                options.seeds.push_back(unsigned_number(value));
            }
            else if (option == "--players")
            {
                const std::uint32_t count = unsigned_number(value);
                if (count < 1 || count > 4) { throw std::invalid_argument("Players must be in 1..4"); }
                options.players = static_cast<int>(count);
            }
            else if (option == "--duration") { options.duration = seconds(value); }
            else if (option == "--tools") { options.tools = parse_tools(value); }
            else if (option == "--bot") { options.bot = value; }
        }
        if (options.tools.empty())
        { options.tools.assign(static_cast<std::size_t>(options.players), WeaponType::sprayer); }
        validate_options(options);
        if (options.csv.empty() || options.csv.filename().empty())
        { throw std::invalid_argument("CLI runs require --csv with a new output file"); }
        if (options.paint_csv.empty())
        { options.paint_csv = options.csv.parent_path() / (options.csv.stem().native() + L".paint.csv"); }
        const OutputPaths paths = checked_paths(options.csv, options.paint_csv);
        options.csv = paths.main; options.paint_csv = paths.paint;
        return options;
    }

    std::string csv_header()
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << "schema_version,stage,seed,players,bot,requested_active_seconds,fixed_step_ticks,"
            "fixed_step_float_bits,active_ticks,simulated_active_seconds,team_a_paint_area,team_b_paint_area,"
            "painted_area,paintable_area,team_a_share_of_painted,team_b_share_of_painted,"
            "team_a_coverage_of_arena,team_b_coverage_of_arena,final_fingerprint";
        for (int seat = 1; seat <= 4; ++seat)
        {
            const std::string prefix = ",p" + std::to_string(seat) + '_';
            for (const char* name : { "team", "tool", "paint_owner", "paint_area", "share_of_painted",
                "coverage_of_arena", "alive_ticks", "ammo_starved_ticks", "fire_starved_ticks",
                "observed_death_edges", "final_health", "final_ammo" })
            { out << prefix << name; }
        }
        out << ",credited_enemy_eliminations,uncredited_projectile_lethals";
        for (int seat = 1; seat <= 4; ++seat)
        {
            const std::string prefix = ",p" + std::to_string(seat) + '_';
            for (const char* name : { "credited_enemy_eliminations", "credited_enemy_projectile_deaths",
                "uncredited_projectile_lethals" })
            { out << prefix << name; }
        }
        return out.str();
    }

    std::string csv_row(const Result& result)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        const double painted = static_cast<double>(result.team_a_area) + result.team_b_area;
        out << 2 << ',' << field(result.stage) << ',' << result.seed << ',' << result.players << ','
            << field(result.bot) << ',' << result.duration << ',' << labrador::StepTimer::TicksPerSecond / 60 << ','
            << std::bit_cast<std::uint32_t>(fixed_dt()) << ',' << result.active_ticks << ','
            << static_cast<double>(result.active_ticks) * fixed_dt() << ',' << result.team_a_area << ','
            << result.team_b_area << ',' << painted << ',' << result.paintable_area << ','
            << ratio(result.team_a_area, painted) << ',' << ratio(result.team_b_area, painted) << ','
            << ratio(result.team_a_area, result.paintable_area) << ','
            << ratio(result.team_b_area, result.paintable_area) << ',' << result.final_fingerprint;
        for (const SeatResult& seat : result.seats)
        {
            out << ',' << team_name(seat.team) << ',' << tool_name(seat.tool) << ',' << seat.paint_owner << ','
                << seat.paint_area << ',' << ratio(seat.paint_area, painted) << ','
                << ratio(seat.paint_area, result.paintable_area) << ',' << seat.alive_ticks << ','
                << seat.ammo_starved_ticks << ',' << seat.fire_starved_ticks << ',' << seat.observed_death_edges << ','
                << seat.final_health << ',' << seat.final_ammo;
        }
        out << ',' << result.credited_enemy_eliminations << ',' << result.uncredited_projectile_lethals;
        for (const SeatResult& seat : result.seats)
        {
            out << ',' << seat.credited_enemy_eliminations << ',' << seat.credited_enemy_projectile_deaths << ','
                << seat.uncredited_projectile_lethals;
        }
        return out.str();
    }

    std::string csv_text(const std::vector<Result>& results)
    {
        std::string output = csv_header() + '\n';
        for (const Result& result : results) { output += csv_row(result) + '\n'; }
        return output;
    }

    std::string paint_csv_text(const std::vector<Result>& results)
    {
        if (results.empty()) { throw std::invalid_argument("Paired CSV needs completed match results"); }
        std::vector<balance_regions::Record> records;
        records.reserve(results.size());
        for (std::size_t index = 0; index < results.size(); ++index)
        {
            const Result& result = results[index];
            if (!result.paint_report || result.players < 1 || result.players > 4)
            { throw std::invalid_argument("Completed match is missing its final paint report"); }
            const auto& report = *result.paint_report;
            const auto& roster = report.roster();
            const PaintTotal& runtime = report.runtime_totals();
            const auto equal = [](float first, float second) { return std::bit_cast<std::uint32_t>(first) == std::bit_cast<std::uint32_t>(second); };
            if (roster.size() != static_cast<std::size_t>(result.players) || !equal(runtime.total, result.paintable_area) ||
                !equal(runtime.team_a, result.team_a_area) || !equal(runtime.team_b, result.team_b_area))
            { throw std::invalid_argument("Paint report does not match its main result scores/roster"); }
            for (std::size_t view = 0; view < roster.size(); ++view)
            {
                const SeatResult& seat = result.seats[view];
                if (roster[view].team != seat.team || roster[view].paint_owner != seat.paint_owner || roster[view].tool != tool_name(seat.tool) ||
                    !equal(runtime.player_areas.at(static_cast<std::size_t>(seat.paint_owner)), seat.paint_area))
                { throw std::invalid_argument("Paint report does not match its main seat ownership/tool"); }
            }
            records.push_back({ { index + 1, result.stage, result.seed, result.bot, result.duration, result.final_fingerprint }, report });
        }
        return balance_regions::csv_text(records);
    }

    class CsvOutputs::Impl final
    {
    public:
        Impl(const std::filesystem::path& main, const std::filesystem::path& paint) : paths_(checked_paths(main, paint))
        {
            for (const auto& parent : { this->paths_.main.parent_path(), this->paths_.paint.parent_path() })
            {
                std::error_code error; std::filesystem::create_directories(parent, error);
                if (error) { throw std::runtime_error("Cannot prepare CSV parent (filesystem code " + std::to_string(error.value()) + "): " + labrador::path_to_utf8(parent)); }
            }
            distinct_parents(this->paths_.main, this->paths_.paint);
            require_new(this->paths_.main); require_new(this->paths_.paint);
            const auto main_leaf = this->paths_.main.filename().native(), paint_leaf = this->paths_.paint.filename().native();
            try { this->main_.reserve(this->paths_.main.parent_path(), main_leaf, paint_leaf); this->paint_.reserve(this->paths_.paint.parent_path(), main_leaf, paint_leaf); }
            catch (const std::exception& error)
            { throw std::runtime_error(std::string(error.what()) + "\nRetained owned stage paths (empty means not created): " +
                labrador::path_to_utf8(this->main_.path()) + " ; " + labrador::path_to_utf8(this->paint_.path())); }
        }
        void stage(const std::vector<Result>& results)
        {
            if (this->state_ != State::reserved) { throw std::invalid_argument("CSV pair is not awaiting staging"); }
            try
            {
                const std::string main = csv_text(results), paint = paint_csv_text(results);
                this->main_.write_and_close(main); this->paint_.write_and_close(paint);
                this->state_ = State::staged;
            }
            catch (...) { this->state_ = State::failed; throw; }
        }
        void publish()
        {
            if (this->state_ != State::staged) { throw std::invalid_argument("Both CSV stages must be complete before publication"); }
            try
            {
                require_new(this->paths_.main); require_new(this->paths_.paint);
                if (!MoveFileExW(this->paint_.path().c_str(), this->paths_.paint.c_str(), MOVEFILE_WRITE_THROUGH))
                { throw file_error("Paint companion publication failed; retain both stages", this->paths_.paint, GetLastError()); }
                if (!MoveFileExW(this->main_.path().c_str(), this->paths_.main.c_str(), MOVEFILE_WRITE_THROUGH))
                {
                    const DWORD error = GetLastError();
                    throw std::runtime_error("Partial pair: companion published at " + labrador::path_to_utf8(this->paths_.paint) +
                        "; main publication failed (Windows code " + std::to_string(error) + ") at " + labrador::path_to_utf8(this->paths_.main) +
                        ". Main stage retained at " + labrador::path_to_utf8(this->main_.path()) + ". No main completion marker was published by this runner.");
                }
                this->state_ = State::published;
            }
            catch (...) { this->state_ = State::failed; throw; }
        }
        const std::filesystem::path& main_stage_path() const { return this->main_.path(); }
        const std::filesystem::path& paint_stage_path() const { return this->paint_.path(); }
    private:
        enum class State { reserved, staged, published, failed };
        OutputPaths paths_;
        StagedCsv main_, paint_;
        State state_ = State::reserved;
    };
    CsvOutputs::CsvOutputs(const std::filesystem::path& main, const std::filesystem::path& paint) : impl_(std::make_unique<Impl>(main, paint)) {}
    CsvOutputs::~CsvOutputs() = default;
    void CsvOutputs::stage(const std::vector<Result>& results) { this->impl_->stage(results); }
    void CsvOutputs::publish() { this->impl_->publish(); }
    const std::filesystem::path& CsvOutputs::main_stage_path() const { return this->impl_->main_stage_path(); }
    const std::filesystem::path& CsvOutputs::paint_stage_path() const { return this->impl_->paint_stage_path(); }
}
#endif
