#include "game/pch.h"
#include "scripts/acceptance/balance_regions/metrics.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace balance_regions
{
    namespace
    {
        void check(bool condition, const char* reason)
        { if (!condition) { throw std::invalid_argument(reason); } }
        class Sum final
        {
        public:
            void add(double value)
            {
                const double next = this->sum_ + value;
                this->correction_ += std::abs(this->sum_) >= std::abs(value)
                    ? (this->sum_ - next) + value : (value - next) + this->sum_;
                this->sum_ = next;
            }
            double value() const { return this->sum_ + this->correction_; }
        private:
            double sum_ = 0.0, correction_ = 0.0;
        };
        void label(const std::string& value)
        { check(!value.empty() && value.find_first_of("\r\n\0", 0, 3) == std::string::npos, "Metric labels must be nonempty single-line text"); }
        void validate(const Partition& partition)
        {
            label(partition.name);
            check(std::isfinite(partition.top) && std::isfinite(partition.bottom) && partition.top < partition.bottom,
                "Partition needs finite positive vertical extent");
            check(!partition.bands.empty(), "Partition needs at least one vertical band");
            std::set<std::string> names;
            for (std::size_t index = 0; index < partition.bands.size(); ++index)
            {
                const Band& band = partition.bands[index]; label(band.name);
                check(band.name != "outside_partition" && names.insert(band.name).second, "Partition region names must be unique and not reserved");
                check(std::isfinite(band.left) && std::isfinite(band.right) && band.left < band.right,
                    "Partition band needs finite positive width");
                check(index == 0 || partition.bands[index - 1].right == band.left,
                    "Partition bands must be ordered, contiguous and nonoverlapping");
            }
        }
        bool concrete_tool(const std::string& tool)
        { return tool == "sprayer" || tool == "sniper" || tool == "roller" || tool == "mister" || tool == "bucket"; }
        struct Owner { int seat = 0; OwnerStatus status = OwnerStatus::neutral; };
        Owner owner_of(const PaintSurfaceSample& sample, const std::vector<RosterSeat>& roster)
        {
            if (sample.team == PlayerTeam::none) { return {}; }
            if (sample.paint_owner < 0 || sample.paint_owner >= paint_owners::count) { return { 0, OwnerStatus::invalid_owner }; }
            for (std::size_t view = 0; view < roster.size(); ++view)
            {
                if (roster[view].paint_owner != sample.paint_owner) { continue; }
                return roster[view].team == sample.team ? Owner{ static_cast<int>(view) + 1, OwnerStatus::seat }
                    : Owner{ 0, OwnerStatus::team_mismatch };
            }
            return { 0, OwnerStatus::absent_owner };
        }
        struct Point { double x = 0.0, y = 0.0; };
        double cross(const Point& a, const Point& b, const Point& c)
        { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }
        double signed_area(const std::vector<Point>& polygon)
        {
            if (polygon.size() < 3) { return 0.0; }
            Sum twice;
            for (std::size_t index = 1; index + 1 < polygon.size(); ++index)
            { twice.add(cross(polygon[0], polygon[index], polygon[index + 1])); }
            return twice.value() / 2.0;
        }
        std::vector<Point> clip(const std::vector<Point>& polygon, bool x_axis, double edge, bool minimum)
        {
            if (polygon.empty()) { return {}; }
            std::vector<Point> result;
            const auto coordinate = [x_axis](const Point& point) { return x_axis ? point.x : point.y; };
            const auto inside = [&](const Point& point) { return minimum ? coordinate(point) >= edge : coordinate(point) <= edge; };
            Point previous = polygon.back(); bool was_inside = inside(previous);
            for (const Point& current : polygon)
            {
                const bool is_inside = inside(current);
                if (was_inside != is_inside)
                {
                    const double fraction = std::clamp((edge - coordinate(previous)) /
                        (coordinate(current) - coordinate(previous)), 0.0, 1.0);
                    Point intersection{ previous.x + (current.x - previous.x) * fraction,
                        previous.y + (current.y - previous.y) * fraction };
                    if (x_axis) { intersection.x = edge; } else { intersection.y = edge; }
                    result.push_back(intersection);
                }
                if (is_inside) { result.push_back(current); }
                previous = current; was_inside = is_inside;
            }
            return result;
        }
        std::vector<double> split(const PaintSurfaceSample& sample, const Partition& partition)
        {
            const double origin_x = sample.corners[0].x, origin_y = sample.corners[0].y;
            std::vector<Point> polygon;
            for (const auto& point : sample.corners)
            {
                check(std::isfinite(point.x) && std::isfinite(point.y), "Scored quad corners must be finite");
                polygon.push_back({ static_cast<double>(point.x) - origin_x, static_cast<double>(point.y) - origin_y });
            }
            for (std::size_t index = 0; index < polygon.size(); ++index)
            { check(cross(polygon[index], polygon[(index + 1) % 4], polygon[(index + 2) % 4]) > 0.0,
                "Scored quad must be nondegenerate convex in local TL/TR/BR/BL winding"); }
            const double area = signed_area(polygon);
            check(std::isfinite(area) && area > 0.0, "Scored quad needs positive finite geometric area");
            const double geometry_tolerance = 256.0 * std::numeric_limits<double>::epsilon() * area *
                static_cast<double>(partition.bands.size() + 1);
            std::vector<double> pieces(partition.bands.size() + 1, 0.0);
            Sum covered;
            for (std::size_t region = 0; region < partition.bands.size(); ++region)
            {
                const Band& band = partition.bands[region];
                auto clipped = clip(polygon, true, band.left - origin_x, true);
                clipped = clip(clipped, true, band.right - origin_x, false);
                clipped = clip(clipped, false, partition.top - origin_y, true);
                clipped = clip(clipped, false, partition.bottom - origin_y, false);
                const double clipped_area = signed_area(clipped);
                check(std::isfinite(clipped_area) && clipped_area >= -geometry_tolerance,
                    "Clipped quad produced invalid oriented area");
                pieces[region] = std::max(0.0, clipped_area); covered.add(pieces[region]);
            }
            double outside = area - covered.value();
            check(outside >= -geometry_tolerance, "Band clipping exceeds the whole quad area");
            // At machine-error scale the geometric remainder is indistinguishable
            // from clipping arithmetic. The largest-piece correction below absorbs it.
            if (std::abs(outside) <= geometry_tolerance) { outside = 0.0; }
            pieces.back() = std::max(0.0, outside);
            const std::size_t largest = static_cast<std::size_t>(std::distance(pieces.begin(),
                std::max_element(pieces.begin(), pieces.end())));
            check(pieces[largest] > 0.0, "Scored quad lost every partition piece");
            Sum others;
            for (std::size_t region = 0; region < pieces.size(); ++region)
            {
                if (region == largest) { continue; }
                pieces[region] = static_cast<double>(sample.scored_area) * (pieces[region] / area);
                others.add(pieces[region]);
            }
            pieces[largest] = static_cast<double>(sample.scored_area) - others.value();
            check(std::isfinite(pieces[largest]) && pieces[largest] > 0.0,
                "Authoritative tile-area residual is not a positive largest piece");
            return pieces;
        }
        int team_order(PlayerTeam team) { return team == PlayerTeam::none ? 0 : (team == PlayerTeam::a ? 1 : 2); }
        struct Key
        {
            std::size_t region = 0;
            PaintSurfaceFace face = PaintSurfaceFace::top;
            PlayerTeam team = PlayerTeam::none;
            int paint_owner = -1, seat = 0;
            OwnerStatus status = OwnerStatus::neutral;
            bool operator<(const Key& other) const
            {
                return std::tuple(region, face, team_order(team), paint_owner, seat, status) <
                    std::tuple(other.region, other.face, team_order(other.team), other.paint_owner, other.seat, other.status);
            }
        };
        struct CellTotals { Sum total, painted; };
        class Totals final
        {
        public:
            void add(const PaintSurfaceSample& sample, Owner owner, double area)
            {
                this->total_.add(area);
                if (sample.team == PlayerTeam::a) { this->a_.add(area); }
                else if (sample.team == PlayerTeam::b) { this->b_.add(area); }
                if (sample.paint_owner >= 0 && sample.paint_owner < paint_owners::count)
                { this->owners_[static_cast<std::size_t>(sample.paint_owner)].add(area); }
                if (owner.seat > 0) { this->seats_[static_cast<std::size_t>(owner.seat - 1)].add(area); }
                else if (sample.team != PlayerTeam::none) { this->uncredited_.add(area); }
            }
            SampleTotals value() const
            {
                SampleTotals result;
                result.total = this->total_.value(); result.team_a = this->a_.value(); result.team_b = this->b_.value();
                for (std::size_t index = 0; index < this->owners_.size(); ++index) { result.raw_owner_areas[index] = this->owners_[index].value(); }
                for (std::size_t index = 0; index < this->seats_.size(); ++index) { result.seat_areas[index] = this->seats_[index].value(); }
                result.uncredited_painted = this->uncredited_.value();
                return result;
            }
        private:
            Sum total_, a_, b_, uncredited_;
            std::array<Sum, paint_owners::count> owners_;
            std::array<Sum, 4> seats_;
        };
        void add_total(PaintTotal& result, const PaintTotal& part)
        {
            result.total += part.total; result.team_a += part.team_a; result.team_b += part.team_b;
            for (std::size_t owner = 0; owner < result.player_areas.size(); ++owner) { result.player_areas[owner] += part.player_areas[owner]; }
        }
    }

    std::vector<double> region_pieces(const PaintSurfaceSample& sample, const Partition& partition)
    {
        validate(partition);
        check(std::isfinite(sample.scored_area) && sample.scored_area > 0.0f, "Region pieces need a positive finite tile area");
        return split(sample, partition);
    }

    Partition stage_partition(std::string_view stage, const mattmath::RectangleF& arena)
    {
        check(std::isfinite(arena.left()) && std::isfinite(arena.right()) && std::isfinite(arena.top()) &&
            std::isfinite(arena.bottom()) && arena.left() < arena.right() && arena.top() < arena.bottom(),
            "Stage partition needs a valid finite arena rectangle");
        Partition result{ "whole_arena", arena.top(), arena.bottom(), {} };
        if (stage == "crossfeed")
        {
            result.name = "x_band";
            result.bands = { { "left_home", arena.left(), 620.0 }, { "left_contest", 620.0, 1400.0 },
                { "centre", 1400.0, 2200.0 }, { "right_contest", 2200.0, 2980.0 }, { "right_home", 2980.0, arena.right() } };
        }
        else if (stage == "press_tower")
        {
            result.name = "x_band";
            result.bands = { { "home_a", 0.0, 520.0 }, { "approach_a", 520.0, 1480.0 }, { "hoist_a", 1480.0, 1640.0 },
                { "crown", 1640.0, 1960.0 }, { "hoist_b", 1960.0, 2120.0 }, { "approach_b", 2120.0, 3080.0 }, { "home_b", 3080.0, 3600.0 } };
        }
        else if (stage == "turbulence_blockout")
        {
            // Geographic X bands, distinct from the candidate's object-role
            // budget (which keeps high crossover and lower contest separate).
            result.name = "x_band";
            result.bands = { { "home_a", 0.0, 600.0 }, { "approach_a", 600.0, 1100.0 },
                { "contest", 1100.0, 2900.0 }, { "approach_b", 2900.0, 3400.0 }, { "home_b", 3400.0, 4000.0 } };
        }
        else { result.bands = { { "whole_arena", arena.left(), arena.right() } }; }
        validate(result);
        return result;
    }

    Measurement::Measurement(std::span<const PaintSurfaceSample> samples, std::vector<RosterSeat> roster, Partition partition) :
        partition_(std::move(partition)), roster_(std::move(roster))
    {
        validate(this->partition_);
        check(!this->roster_.empty() && this->roster_.size() <= 4, "Region metrics need 1..4 actual roster seats");
        std::set<int> owners;
        for (const RosterSeat& seat : this->roster_)
        {
            check((seat.team == PlayerTeam::a || seat.team == PlayerTeam::b) && seat.paint_owner >= 0 &&
                seat.paint_owner < paint_owners::count && owners.insert(seat.paint_owner).second,
                "Roster needs unique valid paint owners and actual A/B teams");
            check(concrete_tool(seat.tool), "Roster needs canonical concrete tool names");
        }
        std::map<Key, Sum> cells;
        std::vector<std::array<CellTotals, 4>> denominators(this->partition_.bands.size() + 1);
        for (std::size_t region = 0; region < denominators.size(); ++region)
        { for (int face = 0; face < 4; ++face) { cells.try_emplace(Key{ region, static_cast<PaintSurfaceFace>(face), PlayerTeam::none, -1, 0, OwnerStatus::neutral }); } }
        PaintTotal source;
        std::size_t source_index = 0;
        bool started = false;
        Totals sample_totals;
        for (const PaintSurfaceSample& sample : samples)
        {
            check(std::isfinite(sample.scored_area) && sample.scored_area > 0.0f, "Snapshot area must be a positive finite authoritative tile float");
            check(sample.face >= PaintSurfaceFace::top && sample.face <= PaintSurfaceFace::right, "Snapshot has invalid authored face");
            check(sample.team == PlayerTeam::none || sample.team == PlayerTeam::a || sample.team == PlayerTeam::b, "Snapshot has invalid team");
            check(sample.team != PlayerTeam::none || sample.paint_owner == -1, "Neutral snapshot tile cannot carry invented owner credit");
            check(!started || sample.source_index >= source_index, "Snapshot must retain ascending producer source groups");
            if (started && sample.source_index != source_index) { add_total(this->runtime_, source); source = {}; }
            started = true; source_index = sample.source_index;
            source.total += sample.scored_area;
            if (sample.team == PlayerTeam::a) { source.team_a += sample.scored_area; }
            else if (sample.team == PlayerTeam::b) { source.team_b += sample.scored_area; }
            if (sample.paint_owner >= 0 && sample.paint_owner < paint_owners::count)
            { source.player_areas[static_cast<std::size_t>(sample.paint_owner)] += sample.scored_area; }
            const Owner owner = owner_of(sample, this->roster_);
            sample_totals.add(sample, owner, sample.scored_area);
            const std::vector<double> pieces = split(sample, this->partition_);
            for (std::size_t region = 0; region < pieces.size(); ++region)
            {
                if (!(pieces[region] > 0.0)) { continue; }
                cells[Key{ region, sample.face, sample.team, sample.paint_owner, owner.seat, owner.status }].add(pieces[region]);
                CellTotals& total = denominators[region][static_cast<std::size_t>(sample.face)];
                total.total.add(pieces[region]);
                if (sample.team != PlayerTeam::none) { total.painted.add(pieces[region]); }
            }
        }
        if (started) { add_total(this->runtime_, source); }
        this->sample_ = sample_totals.value();
        check(std::isfinite(this->runtime_.total) && std::isfinite(this->runtime_.team_a) && std::isfinite(this->runtime_.team_b),
            "Legacy float source scoring overflowed");
        for (float area : this->runtime_.player_areas) { check(std::isfinite(area), "Legacy owner source scoring overflowed"); }
        this->allocation_tolerance_ = 4096.0 * std::numeric_limits<double>::epsilon() * this->sample_.total *
            (static_cast<double>(samples.size()) + static_cast<double>(this->partition_.bands.size()) + 1.0);
        Totals allocated;
        for (const auto& [key, area] : cells)
        {
            const CellTotals& total = denominators[key.region][static_cast<std::size_t>(key.face)];
            this->cells_.push_back(Cell{ key.region, key.face, key.team, key.seat, key.paint_owner, key.status,
                area.value(), total.total.value(), total.painted.value() });
            PaintSurfaceSample held;
            held.team = key.team; held.paint_owner = key.paint_owner;
            allocated.add(held, Owner{ key.seat, key.status }, area.value());
        }
        const SampleTotals actual = allocated.value();
        const auto reconciles = [&](double a, double b) { check(std::abs(a - b) <= this->allocation_tolerance_, "Clipped metric allocations do not reconcile with authoritative tile totals"); };
        reconciles(actual.total, this->sample_.total); reconciles(actual.team_a, this->sample_.team_a); reconciles(actual.team_b, this->sample_.team_b);
        for (std::size_t owner = 0; owner < actual.raw_owner_areas.size(); ++owner) { reconciles(actual.raw_owner_areas[owner], this->sample_.raw_owner_areas[owner]); }
        for (std::size_t seat = 0; seat < actual.seat_areas.size(); ++seat) { reconciles(actual.seat_areas[seat], this->sample_.seat_areas[seat]); }
        reconciles(actual.uncredited_painted, this->sample_.uncredited_painted);
    }

    void Measurement::verify_runtime(const PaintTotal& expected) const
    {
        const auto equal = [](float first, float second) { return std::bit_cast<std::uint32_t>(first) == std::bit_cast<std::uint32_t>(second); };
        check(equal(this->runtime_.total, expected.total) && equal(this->runtime_.team_a, expected.team_a) &&
            equal(this->runtime_.team_b, expected.team_b), "Snapshot does not reproduce the supplied runtime float score totals");
        for (std::size_t owner = 0; owner < expected.player_areas.size(); ++owner)
        { check(equal(this->runtime_.player_areas[owner], expected.player_areas[owner]), "Snapshot does not reproduce supplied runtime raw-owner scores"); }
    }
}
