#pragma once

#include "game/paint_owners.h"
#include "game/objects/player_team.h"
#include "engine/math/vector2f.h"
#include <array>
#include <cstddef>
#include <vector>

// Match-long capture tallies by paint owner, for the results awards. A tile
// changing colour credits its area as laid; taking it from the other team
// credits it as recaptured too. Friendly repaint and neutral answers add nothing.
struct PaintCredit
{
	std::array<float, paint_owners::count> laid{};
	std::array<float, paint_owners::count> recaptured{};
	void add(int owner, PlayerTeam before, PlayerTeam after, float area)
	{
		if (before == after || after == PlayerTeam::none ||
			owner < 0 || owner >= paint_owners::count)
		{
			return;
		}
		this->laid[static_cast<std::size_t>(owner)] += area;
		if (before != PlayerTeam::none)
		{
			this->recaptured[static_cast<std::size_t>(owner)] += area;
		}
	}
	void merge(const PaintCredit& other)
	{
		for (std::size_t owner = 0; owner < this->laid.size(); ++owner)
		{
			this->laid[owner] += other.laid[owner];
			this->recaptured[owner] += other.recaptured[owner];
		}
	}
};

struct PaintTotal
{
	float team_a = 0.0f;
	float team_b = 0.0f;
	// Held area by paint owner: device slots, then device-less views.
	std::array<float, paint_owners::count> player_areas{};

	// All paintable area, including neutral tiles, for the arena coverage score.
	float total = 0.0f;

	// What was captured over the match, not what is held now.
	PaintCredit credit;
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
// Report current held area, including neutral area in total, and the match's
// capture tallies; no other paint events.
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
