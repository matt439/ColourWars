#include "game/pch.h"
#include "scripts/acceptance/balance_runner/runner.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "engine/core/step_timer.h"
#include "engine/core/file_path.h"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

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
                option != "--duration" && option != "--tools" && option != "--bot" && option != "--csv")
            { throw std::invalid_argument("Unknown option: " + option); }
            if (at + 1 >= arguments.size()) { throw std::invalid_argument("Missing value for " + option); }
            if (option != "--stage" && option != "--seed" && !seen.insert(option).second)
            { throw std::invalid_argument("Duplicate option: " + option); }
            const std::wstring& wide_value = arguments[++at];
            if (option == "--csv")
            {
                if (wide_value.empty()) { throw std::invalid_argument("CSV path must not be empty"); }
                options.csv = std::filesystem::absolute(std::filesystem::path(wide_value));
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
        if (std::filesystem::exists(options.csv))
        { throw std::invalid_argument("CSV output already exists; select a new path"); }
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
        return out.str();
    }

    std::string csv_row(const Result& result)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        const double painted = static_cast<double>(result.team_a_area) + result.team_b_area;
        out << 1 << ',' << field(result.stage) << ',' << result.seed << ',' << result.players << ','
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
        return out.str();
    }

    std::string csv_text(const std::vector<Result>& results)
    {
        std::string output = csv_header() + '\n';
        for (const Result& result : results) { output += csv_row(result) + '\n'; }
        return output;
    }

    void write_csv(const std::filesystem::path& path, const std::vector<Result>& results)
    {
        if (path.empty() || std::filesystem::exists(path))
        { throw std::invalid_argument("CSV output must be a new file"); }
        const std::string output = csv_text(results);
        if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }
        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file) { throw std::runtime_error("Cannot create CSV: " + labrador::path_to_utf8(path)); }
        file.write(output.data(), static_cast<std::streamsize>(output.size()));
        file.close();
        if (!file) { throw std::runtime_error("CSV write failed; preserve partial file: " + labrador::path_to_utf8(path)); }
    }
}
#endif
