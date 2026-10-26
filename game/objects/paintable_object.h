#pragma once

#include "game/paint_owners.h"
#include "game/objects/player_team.h"
#include "engine/math/vector2f.h"
#include <array>
#include <cstddef>
#include <vector>

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	// Held area by paint owner: device slots, then device-less views.
	std::array<float, paint_owners::count> player_areas{};

	// All paintable area, including neutral tiles, for the arena coverage score.
	float total = 0.0f;
};

// Authored/local face identity; a future rigid pose may change its world normal.
enum class PaintSurfaceFace { top, bottom, left, right };

// A value-only view of one actual scoring tile, including neutral tiles.
// Corners follow local top-left, top-right, bottom-right, bottom-left winding
// in world space. The current producer is static and axis aligned; this quad
// contract can also represent a rigidly transformed tile, without claiming
// that moving/rotated scoring exists yet. Never replace it with a world AABB.
struct PaintSurfaceSample
{
    std::array<mattmath::Vector2F, 4> corners{};
    PaintSurfaceFace face = PaintSurfaceFace::top;
    PlayerTeam team = PlayerTeam::none;
    int paint_owner = -1; // Raw paint-owner index; -1 may also mean owned but uncredited.
    // Authoritative area from the scored tile, not recomputed corner arithmetic.
    // Rigid transforms preserve this area; a future shape/scale needs its own rule.
    float scored_area = 0.0f;
    // Snapshot-local paintable-role ordinal supplied by the collector. Keep
    // source groups and tile order to reproduce existing float summation exactly.
    std::size_t source_index = 0;
};
// A scoring role independent of the object's collision/rendering hierarchy.
// Report current held area, including neutral area in total, not paint events.
// Level discovers this role on its collision objects, independently of their
// collision tag: solid and jump-through paintable supports both contribute.
class PaintableObject
{
public:
	virtual ~PaintableObject() = default;
	virtual PaintTotal paint_total() const = 0;
	// Append owned values in the same tile order used by paint_total().
	// Only the caller's output changes; no scoring, presentation or RNG advances.
	virtual void append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
		std::size_t source_index = 0) const = 0;
};
