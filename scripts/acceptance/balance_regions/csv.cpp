#include "game/pch.h"
#include "scripts/acceptance/balance_regions/metrics.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace balance_regions
{
    namespace
    {
        std::string field(const std::string& value)
        {
            if (value.find_first_of("\r\n\0", 0, 3) != std::string::npos)
            { throw std::invalid_argument("Companion metadata must be single-line text"); }
            std::string result = "\"";
            for (char character : value) { if (character == '"') { result += '"'; } result += character; }
            return result + '"';
        }
        const char* team_name(PlayerTeam team)
        {
            if (team == PlayerTeam::a) { return "a"; }
            if (team == PlayerTeam::b) { return "b"; }
            if (team == PlayerTeam::none) { return "none"; }
            throw std::invalid_argument("Invalid metric team");
        }
        const char* face_name(PaintSurfaceFace face)
        {
            switch (face)
            {
            case PaintSurfaceFace::top: return "top";
            case PaintSurfaceFace::bottom: return "bottom";
            case PaintSurfaceFace::left: return "left";
            case PaintSurfaceFace::right: return "right";
            }
            throw std::invalid_argument("Invalid metric face");
        }
        const char* status_name(OwnerStatus status)
        {
            switch (status)
            {
            case OwnerStatus::neutral: return "neutral";
            case OwnerStatus::seat: return "seat";
            case OwnerStatus::invalid_owner: return "invalid_owner";
            case OwnerStatus::absent_owner: return "absent_owner";
            case OwnerStatus::team_mismatch: return "team_mismatch";
            }
            throw std::invalid_argument("Invalid metric owner status");
        }
        std::string roster_text(const std::vector<RosterSeat>& roster)
        {
            std::ostringstream out; out.imbue(std::locale::classic());
            for (std::size_t view = 0; view < roster.size(); ++view)
            {
                if (view != 0) { out << ';'; }
                out << view + 1 << ':' << team_name(roster[view].team) << ':' << roster[view].tool << ':' << roster[view].paint_owner;
            }
            return out.str();
        }
    }

    std::string csv_text(std::span<const Record> records)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        out << "region_schema_version,match_row,main_schema_version,stage,seed,players,bot,requested_active_seconds,"
            "final_fingerprint,roster,partition,region,region_left,region_right,region_top,region_bottom,face,team,seat,"
            "paint_owner,owner_status,held_sample_area,region_face_sample_paintable_area,region_face_sample_painted_area,"
            "arena_sample_paintable_area,arena_runtime_paintable_area,arena_sample_painted_area,arena_runtime_painted_area,"
            "allocation_tolerance\n";
        for (std::size_t index = 0; index < records.size(); ++index)
        {
            const Record& record = records[index];
            const MatchMetadata& match = record.match;
            if (match.main_row != index + 1 || match.stage.empty() || match.bot.empty() ||
                !std::isfinite(match.duration) || match.duration < 0.05f || match.duration > 600.0f)
            { throw std::invalid_argument("Companion records need contiguous main rows, stage/bot and valid active duration"); }
            const Measurement& metrics = record.metrics;
            const Partition& partition = metrics.partition();
            const SampleTotals& sample = metrics.sample_totals();
            const PaintTotal& runtime = metrics.runtime_totals();
            for (const Cell& cell : metrics.cells())
            {
                const bool outside = cell.region == partition.bands.size();
                const std::string region = outside ? "outside_partition" : partition.bands.at(cell.region).name;
                out << 1 << ',' << match.main_row << ',' << 2 << ',' << field(match.stage) << ',' << match.seed << ','
                    << metrics.roster().size() << ',' << field(match.bot) << ',' << match.duration << ','
                    << match.final_fingerprint << ',' << field(roster_text(metrics.roster())) << ',' << field(partition.name)
                    << ',' << field(region) << ',';
                if (!outside)
                {
                    const Band& band = partition.bands[cell.region];
                    out << band.left << ',' << band.right << ',' << partition.top << ',' << partition.bottom;
                }
                else { out << ",,,"; } // The outside region is a complement, not one finite rectangle.
                out << ',' << field(face_name(cell.face)) << ',' << field(team_name(cell.team)) << ',' << cell.seat << ','
                    << cell.paint_owner << ',' << field(status_name(cell.owner_status)) << ',' << cell.held_sample_area << ','
                    << cell.region_face_sample_paintable_area << ',' << cell.region_face_sample_painted_area << ','
                    << sample.total << ',' << runtime.total << ',' << sample.team_a + sample.team_b << ','
                    << static_cast<double>(runtime.team_a) + runtime.team_b << ',' << metrics.allocation_tolerance() << '\n';
            }
        }
        return out.str();
    }
}
