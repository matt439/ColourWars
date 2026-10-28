#pragma once

// Engine-free rules for a team gate (ROAD#15), shared by the game's loader,
// its startup validation and the level editor, so the editor refuses exactly
// the gates the game refuses, in the game's words. Each caller adds its own
// location to the message.
//
// A gate is a solid paintable wall that players of one team walk through: the
// team holding more of its own paint, if that team also holds at least the
// tuned share of its paintable area. Everyone else, including both teams on a
// tie or an unpainted gate, collides with it. Shots of both teams always hit
// and paint it.
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace team_gate_contract
{
    struct Rectangle
    {
        float x, y, width, height;
    };

    // What an authored record says about itself, read before it is built.
    struct Record
    {
        // The text of "gate", nullopt when absent.
        std::optional<std::string> gate;
        bool structure_paintable = false;
        // STRUCTURE_PAINTABLE collision: one-way supports cannot be gates.
        bool solid = false;
        bool moving = false;
        bool any_paintable_face = false;
    };

    // True when the record is a gate; throws when it names a gate it cannot be.
    inline bool decode(const Record& record)
    {
        if (!record.gate) { return false; }
        if (*record.gate != "paint") { throw std::invalid_argument("gate must be 'paint'"); }
        if (!record.structure_paintable) { throw std::invalid_argument("gate belongs only on a StructurePaintable"); }
        if (!record.solid) { throw std::invalid_argument("a gate is solid: use STRUCTURE_PAINTABLE collision"); }
        if (record.moving) { throw std::invalid_argument("a gate does not move: remove its motion"); }
        if (!record.any_paintable_face)
        { throw std::invalid_argument("a gate needs a paintable face, or no paint could open it"); }
        return true;
    }

    inline void validate_open_share(float share)
    {
        if (!std::isfinite(share) || share < 0.0f || share > 1.0f)
        { throw std::invalid_argument("A gate's open share must be a fraction from 0 to 1."); }
    }

    enum class Leader : std::uint8_t { none, team_a, team_b };

    // Which team the paint opens the gate for. Areas are the gate's own held
    // paint and its whole paintable area, over all its faces.
    inline Leader leader(float team_a, float team_b, float total, float open_share)
    {
        const float needed = open_share * total;
        if (team_a > team_b && team_a > 0.0f && team_a >= needed) { return Leader::team_a; }
        if (team_b > team_a && team_b > 0.0f && team_b >= needed) { return Leader::team_b; }
        return Leader::none;
    }

    // Open intervals: touching edges do not overlap.
    inline bool overlaps(const Rectangle& a, const Rectangle& b)
    {
        return a.x < b.x + b.width && b.x < a.x + a.width &&
            a.y < b.y + b.height && b.y < a.y + a.height;
    }

    // A spawn body inside a gate would be pushed out of it, or through it,
    // whenever the gate is closed to its team.
    inline void require_clear_of_spawn(const Rectangle& gate, const Rectangle& spawn_body)
    {
        if (overlaps(gate, spawn_body))
        { throw std::invalid_argument("a gate must not overlap a spawn body"); }
    }
}
