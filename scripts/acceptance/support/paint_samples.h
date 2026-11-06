#pragma once

// Exact reads of what a paintable reports to scoring: its surface samples
// and the totals built from them, compared bit for bit. Shared by the cases
// that pin paint arithmetic (paint_surfaces, moving_structure).

#include "game/objects/paintable_object.h"
#include "scripts/acceptance/support/probe.h"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace acceptance
{
    inline std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }

    // The samples `object` adds to scoring, under source index `source`.
    inline std::vector<PaintSurfaceSample> snapshot(const PaintableObject& object, std::size_t source = 0)
    {
        std::vector<PaintSurfaceSample> samples;
        object.append_paint_surfaces(samples, source);
        return samples;
    }

    // Every field of every sample, floats as their bits: equal states are
    // equal vectors.
    inline std::vector<std::uint64_t> sample_state(const std::vector<PaintSurfaceSample>& samples)
    {
        std::vector<std::uint64_t> result{ static_cast<std::uint64_t>(samples.size()) };
        for (const PaintSurfaceSample& sample : samples)
        {
            for (const auto& corner : sample.corners)
            { result.push_back(bits(corner.x)); result.push_back(bits(corner.y)); }
            result.push_back(static_cast<std::uint64_t>(sample.face));
            result.push_back(static_cast<std::uint64_t>(sample.team));
            result.push_back(static_cast<std::uint32_t>(sample.paint_owner));
            result.push_back(bits(sample.scored_area)); result.push_back(sample.source_index);
        }
        return result;
    }

    // Two totals agree to the bit: overall, per team and per paint owner.
    inline void same_total(const PaintTotal& actual, const PaintTotal& expected)
    {
        require(bits(actual.total) == bits(expected.total) && bits(actual.team_a) == bits(expected.team_a) &&
            bits(actual.team_b) == bits(expected.team_b), "Paint totals changed exact scoring arithmetic");
        for (std::size_t owner = 0; owner < actual.player_areas.size(); ++owner)
        {
            require(bits(actual.player_areas[owner]) == bits(expected.player_areas[owner]),
                "A paint owner's area changed exact scoring arithmetic");
        }
    }
}
