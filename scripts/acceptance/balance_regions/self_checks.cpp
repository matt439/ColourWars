#include "game/pch.h"
#include "scripts/acceptance/balance_regions/metrics.h"
#include "game/objects/level_loaded_info.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/probe.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <locale>
#include <stdexcept>
#include <utility>

namespace balance_regions
{
    namespace
    {
        using acceptance::require;
        void require_near(double actual, double expected, double tolerance = 1e-10)
        { require(std::abs(actual - expected) <= tolerance, "Clipped area disagrees with an independent analytical oracle"); }
        template<class Action> void rejected(Action action)
        {
            bool refused = false;
            try { action(); } catch (const std::invalid_argument&) { refused = true; }
            require(refused, "Invalid metric input was accepted");
        }
        std::vector<RosterSeat> roster()
        { return { { PlayerTeam::a, 5, "sprayer" }, { PlayerTeam::b, 6, "sniper" },
            { PlayerTeam::a, 7, "roller" }, { PlayerTeam::b, 8, "bucket" } }; }
        Partition bands()
        { return { "fixture", 0, 20, { { "a", 0, 10 }, { "b", 10, 20 }, { "c", 20, 30 } } }; }
        PaintSurfaceSample rectangle(float x, float y, float width, float height, float area,
            PlayerTeam team = PlayerTeam::a, int owner = 5, std::size_t source = 0,
            PaintSurfaceFace face = PaintSurfaceFace::top)
        {
            PaintSurfaceSample result;
            result.corners = { mattmath::Vector2F(x, y), mattmath::Vector2F(x + width, y),
                mattmath::Vector2F(x + width, y + height), mattmath::Vector2F(x, y + height) };
            result.scored_area = area; result.team = team; result.paint_owner = owner;
            result.source_index = source; result.face = face;
            return result;
        }
        double capacity(const Measurement& metrics, std::size_t region, PaintSurfaceFace face = PaintSurfaceFace::top)
        {
            for (const Cell& cell : metrics.cells())
            { if (cell.region == region && cell.face == face) { return cell.region_face_sample_paintable_area; } }
            throw std::runtime_error("Expected zero-capacity neutral denominator row is missing");
        }
        double status_area(const Measurement& metrics, OwnerStatus status)
        {
            double result = 0.0;
            for (const Cell& cell : metrics.cells()) { if (cell.owner_status == status) { result += cell.held_sample_area; } }
            return result;
        }
        std::vector<std::uint64_t> snapshot_bits(const std::vector<PaintSurfaceSample>& samples)
        {
            std::vector<std::uint64_t> result;
            for (const auto& sample : samples)
            {
                for (const auto& point : sample.corners)
                { result.push_back(std::bit_cast<std::uint32_t>(point.x)); result.push_back(std::bit_cast<std::uint32_t>(point.y)); }
                result.push_back(static_cast<std::uint64_t>(sample.face)); result.push_back(static_cast<std::uint64_t>(sample.team));
                result.push_back(static_cast<std::uint32_t>(sample.paint_owner)); result.push_back(std::bit_cast<std::uint32_t>(sample.scored_area));
                result.push_back(sample.source_index);
            }
            return result;
        }
        void geometry_checks()
        {
            std::vector<PaintSurfaceSample> samples{ rectangle(0, 0, 30, 10, 300) };
            Measurement across(samples, roster(), bands());
            require_near(capacity(across, 0), 100); require_near(capacity(across, 1), 100); require_near(capacity(across, 2), 100); require_near(capacity(across, 3), 0);
            require_near(across.sample_totals().seat_areas[0], 300);
            // A 45-degree rigid quad has geometric area200, versus AABB area400.
            // Its authoritative score20 must split10/10 without being redefined.
            PaintSurfaceSample diamond = rectangle(0, 0, 1, 1, 20, PlayerTeam::a, 5, 0, PaintSurfaceFace::left);
            diamond.corners = { mattmath::Vector2F(10, 0), mattmath::Vector2F(20, 10), mattmath::Vector2F(10, 20), mattmath::Vector2F(0, 10) };
            Measurement rotated(std::array{ diamond }, roster(), bands());
            require_near(capacity(rotated, 0, PaintSurfaceFace::left), 10); require_near(capacity(rotated, 1, PaintSurfaceFace::left), 10);
            require_near(capacity(rotated, 0), 0); require_near(rotated.sample_totals().total, 20);
            // Shift 5 right: geometric side triangles are 25 each out of 200,
            // so score 20 splits 2.5/15/2.5. AABB weighting gives 5/10/5 instead.
            // Local winding, authored left face and scored_area remain unchanged.
            auto asymmetric = diamond;
            for (auto& corner : asymmetric.corners) { corner.x += 5.0f; }
            Measurement offset(std::array{ asymmetric }, roster(), bands());
            require_near(capacity(offset, 0, PaintSurfaceFace::left), 2.5);
            require_near(capacity(offset, 1, PaintSurfaceFace::left), 15.0);
            require_near(capacity(offset, 2, PaintSurfaceFace::left), 2.5);
            require_near(capacity(offset, 3, PaintSurfaceFace::left), 0.0);
            require_near(offset.sample_totals().total, 20.0);
            for (auto& corner : diamond.corners) { corner.x += 100.0f; }
            Measurement translated(std::array{ diamond }, roster(), bands());
            require_near(capacity(translated, 3, PaintSurfaceFace::left), 20); require_near(capacity(translated, 0, PaintSurfaceFace::left), 0);
            Measurement partial(std::array{ rectangle(-5, 0, 10, 10, 100) }, roster(), bands());
            require_near(capacity(partial, 0), 50); require_near(capacity(partial, 3), 50);
            Measurement above(std::array{ rectangle(0, -5, 10, 10, 100) }, roster(), bands());
            require_near(capacity(above, 0), 50); require_near(capacity(above, 3), 50);
            Measurement edge(std::array{ rectangle(10, 0, 10, 10, 100) }, roster(), bands());
            require_near(capacity(edge, 0), 0); require_near(capacity(edge, 1), 100); require_near(capacity(edge, 2), 0);
            Measurement touching(std::array{ rectangle(30, 20, 10, 10, 100) }, roster(), bands());
            require_near(capacity(touching, 2), 0); require_near(capacity(touching, 3), 100);
            std::vector<PaintSurfaceSample> overlapping{ rectangle(0, 0, 10, 10, 100),
                rectangle(0, 0, 10, 10, 100, PlayerTeam::a, 5, 0, PaintSurfaceFace::left),
                rectangle(0, 0, 10, 10, 100, PlayerTeam::none, -1, 1) };
            Measurement independent(overlapping, roster(), bands());
            require_near(independent.sample_totals().total, 300); require_near(capacity(independent, 0), 200);
            require_near(capacity(independent, 0, PaintSurfaceFace::left), 100); require_near(status_area(independent, OwnerStatus::neutral), 100);
            Measurement fractional(std::array{ rectangle(5, 0, 10, 10, 0.1f) }, roster(), bands());
            require_near(fractional.sample_totals().total, static_cast<double>(0.1f), 0);
            require_near(capacity(fractional, 0), static_cast<double>(0.1f) / 2.0, 1e-17);
            require_near(capacity(fractional, 1), static_cast<double>(0.1f) / 2.0, 1e-17);
            // Preserve producer float grouping, while recording exact tile-float
            // mass in double. Within-source permutation can change legacy sums.
            std::vector<PaintSurfaceSample> rounding{ rectangle(0, 0, 1, 1, 16777216.0f), rectangle(0, 0, 1, 1, 1), rectangle(0, 0, 1, 1, 1) };
            Measurement ordered(rounding, roster(), bands());
            require_near(ordered.runtime_totals().total, 16777216.0, 0); require_near(ordered.sample_totals().total, 16777218.0, 0);
            std::rotate(rounding.begin(), rounding.begin() + 1, rounding.end());
            Measurement reordered(rounding, roster(), bands());
            require_near(reordered.runtime_totals().total, 16777218.0, 0); require_near(reordered.sample_totals().total, 16777218.0, 0);
            std::vector<PaintSurfaceSample> hierarchical{ rectangle(0, 0, 1, 1, 16777216.0f),
                rectangle(0, 0, 1, 1, 1), rectangle(0, 0, 1, 1, 1),
                rectangle(0, 0, 1, 1, 1, PlayerTeam::a, 5, 1), rectangle(0, 0, 1, 1, 1, PlayerTeam::a, 5, 1) };
            Measurement grouped(hierarchical, roster(), bands());
            require_near(grouped.runtime_totals().total, 16777218.0, 0); require_near(grouped.sample_totals().total, 16777220.0, 0);
            require_near(grouped.runtime_totals().player_areas[5], 16777218.0, 0);
            rounding[0].source_index = 1; rounding[1].source_index = 0;
            rejected([&] { Measurement invalid(rounding, roster(), bands()); });
            PaintTotal expected = across.runtime_totals(); across.verify_runtime(expected);
            expected.team_a += 1.0f; rejected([&] { across.verify_runtime(expected); });
            Measurement empty(std::span<const PaintSurfaceSample>{}, roster(), bands());
            require(empty.cells().size() == 16 && empty.sample_totals().total == 0.0 && empty.allocation_tolerance() == 0.0,
                "Empty geometry did not retain all four zero-denominator face cells per region");
        }
        void ownership_checks()
        {
            std::vector<PaintSurfaceSample> samples;
            for (const auto& [team, owner] : std::array{ std::pair{ PlayerTeam::a, 5 }, std::pair{ PlayerTeam::a, 7 },
                std::pair{ PlayerTeam::b, 6 }, std::pair{ PlayerTeam::a, -1 }, std::pair{ PlayerTeam::a, 0 },
                std::pair{ PlayerTeam::b, 5 }, std::pair{ PlayerTeam::a, paint_owners::count }, std::pair{ PlayerTeam::none, -1 } })
            { samples.push_back(rectangle(0, 0, 2, 2, 4, team, owner)); }
            Measurement metrics(samples, roster(), bands());
            require_near(metrics.sample_totals().total, 32); require_near(metrics.sample_totals().team_a, 20); require_near(metrics.sample_totals().team_b, 8);
            require_near(metrics.sample_totals().seat_areas[0], 4); require_near(metrics.sample_totals().seat_areas[1], 4); require_near(metrics.sample_totals().seat_areas[2], 4);
            require_near(metrics.sample_totals().raw_owner_areas[5], 8); require_near(metrics.sample_totals().uncredited_painted, 16);
            require_near(status_area(metrics, OwnerStatus::invalid_owner), 8); require_near(status_area(metrics, OwnerStatus::absent_owner), 4);
            require_near(status_area(metrics, OwnerStatus::team_mismatch), 4); require_near(status_area(metrics, OwnerStatus::neutral), 4);
            for (const Cell& cell : metrics.cells())
            {
                if (cell.owner_status != OwnerStatus::seat) { require(cell.seat == 0, "Unknown/friendly-mismatched raw owner acquired a roster seat"); }
                if (cell.paint_owner == 7 && cell.owner_status == OwnerStatus::seat) { require(cell.seat == 3, "Friendly actual owner was merged with another same-team seat"); }
            }
        }
        void invalid_checks()
        {
            const auto valid = rectangle(0, 0, 10, 10, 100);
            const auto bad_quad = [&](auto change) { auto bad = valid; change(bad); rejected([&] { Measurement invalid(std::array{ bad }, roster(), bands()); }); };
            bad_quad([](auto& value) { value.corners[2] = value.corners[1]; });
            bad_quad([](auto& value) { value.corners[2] = mattmath::Vector2F(2, 2); });
            bad_quad([](auto& value) { std::swap(value.corners[1], value.corners[3]); });
            bad_quad([](auto& value) { std::swap(value.corners[2], value.corners[3]); });
            bad_quad([](auto& value) { value.corners[0].x = std::numeric_limits<float>::quiet_NaN(); });
            for (float area : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
            { bad_quad([&](auto& value) { value.scored_area = area; }); }
            bad_quad([](auto& value) { value.face = static_cast<PaintSurfaceFace>(99); });
            bad_quad([](auto& value) { value.team = static_cast<PlayerTeam>(99); });
            bad_quad([](auto& value) { value.team = PlayerTeam::none; });
            const auto bad_partition = [&](auto change) { auto bad = bands(); change(bad); rejected([&] { Measurement invalid(std::array{ valid }, roster(), bad); }); };
            bad_partition([](auto& value) { value.bands[1].left = 9; });
            bad_partition([](auto& value) { value.bands[1].left = 11; });
            bad_partition([](auto& value) { value.bands[0].right = value.bands[0].left; });
            bad_partition([](auto& value) { value.bands[1].name = value.bands[0].name; });
            bad_partition([](auto& value) { value.bands[0].name = "outside_partition"; });
            bad_partition([](auto& value) { value.bands.clear(); });
            bad_partition([](auto& value) { value.bottom = value.top; });
            bad_partition([](auto& value) { value.top = std::numeric_limits<double>::quiet_NaN(); });
            bad_partition([](auto& value) { value.bands[0].left = -std::numeric_limits<double>::infinity(); });
            bad_partition([](auto& value) { value.name.clear(); });
            const auto bad_roster = [&](auto change) { auto bad = roster(); change(bad); rejected([&] { Measurement invalid(std::array{ valid }, bad, bands()); }); };
            bad_roster([](auto& value) { value[1].paint_owner = value[0].paint_owner; });
            bad_roster([](auto& value) { value[0].paint_owner = -1; });
            bad_roster([](auto& value) { value[0].team = PlayerTeam::none; });
            bad_roster([](auto& value) { value[0].tool = "random_primary"; });
            bad_roster([](auto& value) { value.push_back(value[0]); });
            bad_roster([](auto& value) { value.clear(); });
            rejected([] { (void)stage_partition("press_tower", mattmath::RectangleF(std::numeric_limits<float>::quiet_NaN(), 0, 10, 10)); });
        }
        class CommaDecimal final : public std::numpunct<char>
        { protected: char do_decimal_point() const override { return ','; } };
        void csv_checks()
        {
            std::vector<PaintSurfaceSample> samples{ rectangle(5, 0, 10, 10, 0.1f) };
            const auto before = snapshot_bits(samples);
            std::srand(731u); const int expected_random = std::rand(); std::srand(731u);
            const MatchMetadata metadata{ 1, "fixture", 0, "paint,\"quoted\"", 0.25f, 123 };
            std::vector<Record> records{ Record{ metadata, Measurement(samples, roster(), bands()) } };
            const std::string first = csv_text(records);
            const std::string second = csv_text(std::vector<Record>{ Record{ metadata, Measurement(samples, roster(), bands()) } });
            require(first == second && snapshot_bits(samples) == before && std::rand() == expected_random,
                "Nontrivial metric collection/serialization changed input, RNG or repeated bytes");
            require(!first.empty() && first.back() == '\n' && first.find('\r') == std::string::npos &&
                first.starts_with("region_schema_version,match_row,main_schema_version,"), "Companion encoding/schema/newlines are not stable");
            require(first.find("\"paint,\"\"quoted\"\"\"") != std::string::npos && first.find("1,1,2,\"fixture\",0,4,") != std::string::npos,
                "Companion lost quoted metadata, valid seed0 or main-row/schema join");
            const auto header_end = first.find('\n');
            require(std::count(first.begin(), first.begin() + static_cast<std::ptrdiff_t>(header_end), ',') == 28,
                "Companion header no longer has 29 declared columns");
            // Count data fields independently, honoring quoted commas and doubled quotes.
            for (std::size_t begin = 0; begin < first.size();)
            {
                const std::size_t end = first.find('\n', begin);
                require(end != std::string::npos, "Companion has an unterminated record");
                bool quoted = false; int columns = 1;
                for (std::size_t at = begin; at < end; ++at)
                {
                    if (first[at] == '"')
                    {
                        if (quoted && at + 1 < end && first[at + 1] == '"') { ++at; }
                        else { quoted = !quoted; }
                    }
                    else if (first[at] == ',' && !quoted) { ++columns; }
                }
                require(!quoted && columns == 29, "Companion data row has invalid quoting or column count");
                begin = end + 1;
            }
            const std::locale previous = std::locale();
            std::locale::global(std::locale(std::locale::classic(), new CommaDecimal));
            std::string localized;
            try { localized = csv_text(records); } catch (...) { std::locale::global(previous); throw; }
            std::locale::global(previous);
            require(first == localized, "Ambient decimal locale changed companion bytes");
            auto bad = records; bad[0].match.main_row = 0; rejected([&] { (void)csv_text(bad); });
            bad = records; bad[0].match.duration = 0.0f; rejected([&] { (void)csv_text(bad); });
            bad = records; bad[0].match.bot = "paint\ninvalid"; rejected([&] { (void)csv_text(bad); });
            bad = records; bad.push_back(records[0]); rejected([&] { (void)csv_text(bad); });
        }
        void partition_checks(const std::filesystem::path& content)
        {
            const LevelLoadedInfo crossfeed(labrador::path_to_utf8(content / "levels/crossfeed.json").c_str());
            const LevelLoadedInfo tower(labrador::path_to_utf8(content / "levels/press_tower.json").c_str());
            crossfeed.validate(); tower.validate();
            const auto cross = stage_partition("crossfeed", crossfeed.out_of_bounds_rectangle());
            const auto press = stage_partition("press_tower", tower.out_of_bounds_rectangle());
            require(cross.bands.size() == 5 && press.bands.size() == 7 && cross.bands[0].name == "left_home" &&
                cross.bands[2].name == "centre" && cross.bands[4].name == "right_home" && press.bands[0].name == "home_a" &&
                press.bands[3].name == "crown" && press.bands[6].name == "home_b", "Current authored region names changed");
            const std::array<const char*, 7> press_names{ "home_a", "approach_a", "hoist_a", "crown", "hoist_b", "approach_b", "home_b" };
            const std::array<const char*, 5> cross_names{ "left_home", "left_contest", "centre", "right_contest", "right_home" };
            for (std::size_t index = 0; index < press_names.size(); ++index) { require(press.bands[index].name == press_names[index], "Press Tower authored band name changed"); }
            for (std::size_t index = 0; index < cross_names.size(); ++index) { require(cross.bands[index].name == cross_names[index], "Crossfeed authored band name changed"); }
            require_near(press.top, tower.out_of_bounds_rectangle().top(), 0); require_near(press.bottom, tower.out_of_bounds_rectangle().bottom(), 0);
            require_near(cross.top, crossfeed.out_of_bounds_rectangle().top(), 0); require_near(cross.bottom, crossfeed.out_of_bounds_rectangle().bottom(), 0);
            const std::array<double, 8> press_cuts{ 0, 520, 1480, 1640, 1960, 2120, 3080, 3600 };
            for (std::size_t index = 0; index < press.bands.size(); ++index)
            { require_near(press.bands[index].left, press_cuts[index], 0); require_near(press.bands[index].right, press_cuts[index + 1], 0); }
            const std::array<double, 6> cross_cuts{ crossfeed.out_of_bounds_rectangle().left(), 620, 1400, 2200, 2980, crossfeed.out_of_bounds_rectangle().right() };
            for (std::size_t index = 0; index < cross.bands.size(); ++index)
            { require_near(cross.bands[index].left, cross_cuts[index], 0); require_near(cross.bands[index].right, cross_cuts[index + 1], 0); }
            const std::array tail{ rectangle(-50, 0, 10, 10, 100) };
            Measurement cross_tail(tail, roster(), cross), press_tail(tail, roster(), press);
            require_near(capacity(cross_tail, 0), 100); require_near(capacity(press_tail, 7), 100);
            const auto whole = stage_partition("other_stage", mattmath::RectangleF(0, 0, 30, 20));
            require(whole.name == "whole_arena" && whole.bands.size() == 1, "Generic stage lost its explicit whole-arena fallback");
            const LevelLoadedInfo yard(labrador::path_to_utf8(content / "levels/turbulence_blockout.json").c_str());
            yard.validate();
            const auto turbulence = stage_partition("turbulence_blockout", yard.out_of_bounds_rectangle());
            const std::array<double, 6> yard_cuts{ 0, 600, 1100, 2900, 3400, 4000 };
            const std::array<const char*, 5> yard_names{ "home_a", "approach_a", "contest", "approach_b", "home_b" };
            require(turbulence.name == "x_band" && turbulence.bands.size() == yard_names.size(),
                "Turbulence must expose geographic bands rather than a whole-arena or object-role label");
            require_near(turbulence.top, yard.out_of_bounds_rectangle().top(), 0);
            require_near(turbulence.bottom, yard.out_of_bounds_rectangle().bottom(), 0);
            for (std::size_t index = 0; index < yard_names.size(); ++index)
            {
                require(turbulence.bands[index].name == yard_names[index], "Turbulence geographic band name changed");
                require_near(turbulence.bands[index].left, yard_cuts[index], 0);
                require_near(turbulence.bands[index].right, yard_cuts[index + 1], 0);
            }
            const std::array yard_edges{ rectangle(-10, 0, 20, 10, 200), rectangle(590, 0, 20, 10, 200),
                rectangle(1090, 0, 20, 10, 200), rectangle(2890, 0, 20, 10, 200),
                rectangle(3390, 0, 20, 10, 200), rectangle(3990, 0, 20, 10, 200) };
            const Measurement yard_split(yard_edges, roster(), turbulence);
            require_near(capacity(yard_split, 0), 200); require_near(capacity(yard_split, 1), 200);
            require_near(capacity(yard_split, 2), 200); require_near(capacity(yard_split, 3), 200);
            require_near(capacity(yard_split, 4), 200); require_near(capacity(yard_split, 5), 200);
            // The bands cover the blockout's camera extent exactly; only the
            // out-of-bounds margin outside it is `outside_partition`.
            const mattmath::RectangleF yard_bounds = yard.out_of_bounds_rectangle();
            const mattmath::RectangleF yard_camera = yard.camera_bounds_rectangle();
            require_near(turbulence.bands.front().left, yard_camera.left(), 0);
            require_near(turbulence.bands.back().right, yard_camera.right(), 0);
            // The four trials keep the blockout's bounds, so they resolve to the
            // same x_band partition, band for band.
            const std::array<const char*, 4> trials{ "turbulence_mover_trial", "turbulence_gate_trial",
                "turbulence_ferry_trial", "turbulence_slab_trial" };
            for (const char* trial : trials)
            {
                const LevelLoadedInfo info(labrador::path_to_utf8(content / "levels" / (std::string(trial) + ".json")).c_str());
                info.validate();
                const mattmath::RectangleF bounds = info.out_of_bounds_rectangle();
                const mattmath::RectangleF camera = info.camera_bounds_rectangle();
                require(bounds.left() == yard_bounds.left() && bounds.right() == yard_bounds.right() &&
                    bounds.top() == yard_bounds.top() && bounds.bottom() == yard_bounds.bottom(),
                    "A Turbulence trial's out-of-bounds rectangle no longer matches the blockout's");
                require(camera.left() == yard_camera.left() && camera.right() == yard_camera.right(),
                    "A Turbulence trial's camera X extent no longer matches the blockout's");
                const auto bands = stage_partition(trial, bounds);
                require(bands.name == "x_band" && bands.bands.size() == turbulence.bands.size(),
                    "A Turbulence trial lost the blockout's x_band partition");
                require_near(bands.top, turbulence.top, 0); require_near(bands.bottom, turbulence.bottom, 0);
                for (std::size_t index = 0; index < bands.bands.size(); ++index)
                {
                    require(bands.bands[index].name == turbulence.bands[index].name, "A Turbulence trial band name differs from the blockout's");
                    require_near(bands.bands[index].left, turbulence.bands[index].left, 0);
                    require_near(bands.bands[index].right, turbulence.bands[index].right, 0);
                }
                const Measurement trial_split(yard_edges, roster(), bands);
                for (std::size_t region = 0; region <= bands.bands.size(); ++region)
                { require_near(capacity(trial_split, region), capacity(yard_split, region)); }
            }
            // The 6200-wide candidate is not scoped yet and keeps the fallback.
            const LevelLoadedInfo candidate(labrador::path_to_utf8(content / "levels/turbulence.json").c_str());
            candidate.validate();
            const auto whole_candidate = stage_partition("turbulence", candidate.out_of_bounds_rectangle());
            require(whole_candidate.name == "whole_arena" && whole_candidate.bands.size() == 1,
                "The Turbulence candidate gained bands before they were scoped");
        }
    }
    void self_tests(const std::filesystem::path& content)
    { geometry_checks(); ownership_checks(); invalid_checks(); csv_checks(); partition_checks(content); }
}
