#pragma once

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/horizontal_motion_contract.h"
#include "game/objects/paintable_object.h"
#include <array>
#include <cstdint>
#include <vector>

class Structure;
class TranslatingStructurePaintable;

// Stable within one Level. Zero is no moving carrier, never a device or
// snapshot-local paint-source index. Level binds positive IDs once on admission.
using HorizontalMotionId = std::uint32_t;

// What advances the phase. Defined in the engine-free contract so the level
// editor checks an authored drive by the same rule the game loads it by.
using HorizontalDrive = horizontal_motion_contract::Drive;

struct HorizontalMotionSpec
{
    mattmath::RectangleF anchor = mattmath::RectangleF::ZERO;
    float travel_x = 0.0f;
    float period_seconds = 0.0f;
    float initial_phase = 0.0f;
    HorizontalDrive drive = HorizontalDrive::cycle;
    // Paint drive only: +1 when team A's paint carries the mover right, -1
    // when it carries it left. Team B's paint always heads the other way.
    int team_a_heading = 1;
};

struct HorizontalMotionSegment
{
    double begin = 0.0;
    double end = 0.0;
    float from_x = 0.0f;
    float to_x = 0.0f;
};

struct HorizontalMotionStep
{
    float dt = 0.0f;
    mattmath::RectangleF before = mattmath::RectangleF::ZERO;
    mattmath::RectangleF after = mattmath::RectangleF::ZERO;
    std::array<HorizontalMotionSegment, 2> segments{};
    // Positive-duration intervals survive even if float endpoint offsets agree.
    // A zero step has no intervals and before == after == the current pose.
    std::uint32_t segment_count = 0;
};

struct HorizontalMotionState
{
    HorizontalMotionSpec spec;
    double phase = 0.0;
    int direction = 1; // Outgoing: +1 at phase0, -1 at phase0.5.
    mattmath::Vector2F previous_offset = mattmath::Vector2F::ZERO;
    mattmath::Vector2F current_offset = mattmath::Vector2F::ZERO;
};

struct HorizontalPreparedStep
{
    HorizontalMotionState after_state;
    HorizontalMotionStep collision;
    // Paint demand sampled when this token was prepared; later contacts must
    // not rewrite the committed step's cosmetic explanation.
    int paint_heading = 0;
};

struct HorizontalCarryStep
{
    HorizontalMotionId id = 0;
    mattmath::RectangleF previous_top = mattmath::RectangleF::ZERO;
    mattmath::RectangleF current_top = mattmath::RectangleF::ZERO;
    float displacement_x = 0.0f;
};

struct HorizontalSupportRef
{
    const Structure* surface = nullptr; // Contact-phase borrow only in Player.
    std::uint32_t order = 0;
    HorizontalMotionId motion = 0; // Static supports still have a stable order.
};

struct HorizontalPaintImpact
{
    mattmath::RectangleF anchor_rectangle = mattmath::RectangleF::ZERO;
    mattmath::Vector2F normal_to_projectile = mattmath::Vector2F::ZERO;
    float penetration = 0.0f;
    double global_entry = 0.0;
};

struct HorizontalStructureState
{
    HorizontalMotionId id = 0;
    HorizontalMotionState motion;
    mattmath::RectangleF previous_bounds = mattmath::RectangleF::ZERO;
    mattmath::RectangleF current_bounds = mattmath::RectangleF::ZERO;
    std::vector<PaintSurfaceSample> local_paint;
};

// One bounded horizontal triangular trajectory. A Level prepares every token
// before any fallible/input work, then its concrete owners commit copies only.
class HorizontalMotion final
{
public:
    explicit HorizontalMotion(const HorizontalMotionSpec& spec);
    static void validate_spec(const HorizontalMotionSpec& spec);
    static void validate_dt(float dt);
    HorizontalMotionState state() const;
    // heading is read only by a paint drive: +1 right, -1 left, 0 hold.
    HorizontalPreparedStep prepare(float dt, int heading = 0) const;
    mattmath::RectangleF bounds() const;
    // The paint drive's heading for this paint, before any end stop.
    static int paint_heading(const HorizontalMotionSpec& spec, float team_a, float team_b);

private:
    friend class TranslatingStructurePaintable;
    void apply(const HorizontalPreparedStep& prepared) noexcept;
    static float offset_at(const HorizontalMotionSpec& spec, double phase);
    static mattmath::RectangleF at_offset(const HorizontalMotionSpec& spec, float offset);
    HorizontalPreparedStep prepare_driven(float dt, int heading) const;
    HorizontalMotionState state_;
};
