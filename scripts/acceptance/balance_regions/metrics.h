#pragma once

#include "game/objects/paintable_object.h"
#include "engine/math/rectanglef.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace balance_regions
{
    struct Band
    {
        std::string name;
        double left = 0.0;
        double right = 0.0;
    };
    // Finite, contiguous, ordered vertical rectangles. Their complement is
    // always reported as outside_partition, including area above/below them.
    struct Partition
    {
        std::string name;
        double top = 0.0;
        double bottom = 0.0;
        std::vector<Band> bands;
    };
    Partition stage_partition(std::string_view stage, const mattmath::RectangleF& arena);
    // Measurement's exact allocation of one tile: bands in order, then
    // outside_partition. The pieces sum to the tile's authoritative scored_area.
    std::vector<double> region_pieces(const PaintSurfaceSample& sample, const Partition& partition);

    struct RosterSeat
    {
        PlayerTeam team = PlayerTeam::none;
        int paint_owner = -1;
        std::string tool;
    };
    enum class OwnerStatus { neutral, seat, invalid_owner, absent_owner, team_mismatch };
    struct SampleTotals
    {
        double total = 0.0;
        double team_a = 0.0;
        double team_b = 0.0;
        std::array<double, paint_owners::count> raw_owner_areas{};
        std::array<double, 4> seat_areas{};
        double uncredited_painted = 0.0;
    };
    struct Cell
    {
        std::size_t region = 0; // bands.size() names outside_partition.
        PaintSurfaceFace face = PaintSurfaceFace::top; // Authored/local, not world normal.
        PlayerTeam team = PlayerTeam::none;
        int seat = 0; // One-based actual roster; zero means neutral/uncredited.
        int paint_owner = -1;
        OwnerStatus owner_status = OwnerStatus::neutral;
        double held_sample_area = 0.0;
        double region_face_sample_paintable_area = 0.0;
        double region_face_sample_painted_area = 0.0;
    };

    // Pure, owned measurement. No world/RNG/timing references are retained.
    // Input must retain producer source and tile order. This is essential to
    // reproduce legacy float score addition; arbitrary permutations are not a
    // replay contract. Output cells have a fixed canonical key order.
    class Measurement final
    {
    public:
        Measurement(std::span<const PaintSurfaceSample> samples,
            std::vector<RosterSeat> roster, Partition partition);
        const Partition& partition() const { return this->partition_; }
        const std::vector<RosterSeat>& roster() const { return this->roster_; }
        // Existing per-source float arithmetic, preserved independently from
        // compensated-double sums of the authoritative individual tile floats.
        const PaintTotal& runtime_totals() const { return this->runtime_; }
        const SampleTotals& sample_totals() const { return this->sample_; }
        const std::vector<Cell>& cells() const { return this->cells_; }
        double allocation_tolerance() const { return this->allocation_tolerance_; }
        void verify_runtime(const PaintTotal& expected) const;
    private:
        Partition partition_;
        std::vector<RosterSeat> roster_;
        PaintTotal runtime_;
        SampleTotals sample_;
        std::vector<Cell> cells_;
        double allocation_tolerance_ = 0.0;
    };

    struct MatchMetadata
    {
        std::size_t main_row = 0; // First main CSV data row is one, excluding its header.
        std::string stage;
        std::uint32_t seed = 0;
        std::string bot;
        float duration = 0.0f;
        std::uint64_t final_fingerprint = 0;
    };
    struct Record
    {
        MatchMetadata match;
        Measurement metrics;
    };
    // Independent companion schema1. Does not change/serialize the main row.
    // Records must follow their main rows contiguously, starting at one.
    // UTF-8-compatible bytes, quoted text fields, classic locale, max_digits10,
    // LF terminators and no wall clock/file I/O. Parent owns atomic output policy.
    std::string csv_text(std::span<const Record> records);
    void self_tests(const std::filesystem::path& content);
}
