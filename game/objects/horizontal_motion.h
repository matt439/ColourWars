#pragma once

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/contracts/horizontal_motion_contract.h"
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
    labrador::RectangleF anchor = labrador::RectangleF::zero;
    float travel_x = 0.0f;
    float period_seconds = 0.0f;
    float initial_phase = 0.0f;
    HorizontalDrive drive = HorizontalDrive::cycle;
    // Paint drive only: +1 when team A's paint carries the mover right, -1
    // when it carries it left. Team B's paint always heads the other way.
    int team_a_heading = 1;
    // A vertical lift (ROAD#28) travels travel_y down from its topmost anchor
    // and leaves travel_x zero; a horizontal mover leaves this zero.
    float travel_y = 0.0f;
    bool vertical() const { return this->travel_y != 0.0f; }
    // The travel along the mover's own axis.
    float travel() const { return this->vertical() ? this->travel_y : this->travel_x; }
    // A scalar axis offset as a world translation.
    labrador::Vector2F axis_offset(float offset) const
    { return this->vertical() ? labrador::Vector2F(0.0f, offset) : labrador::Vector2F(offset, 0.0f); }
    // A world translation's part along the mover's own axis.
    float axis_part(const labrador::Vector2F& offset) const
    { return this->vertical() ? offset.y : offset.x; }
    // The anchor moved a scalar offset along the mover's own axis. A zero
    // offset returns the anchor exactly.
    labrador::RectangleF pose_at(float offset) const
    {
        labrador::RectangleF result = this->anchor;
        if (offset != 0.0f)
        {
            if (this->vertical()) { result.y += offset; }
            else { result.x += offset; }
        }
        return result;
    }
};

// Offsets along the mover's own axis: x for a horizontal mover, y for a lift.
struct HorizontalMotionSegment
{
    double begin = 0.0;
    double end = 0.0;
    float from = 0.0f;
    float to = 0.0f;
};

struct HorizontalMotionStep
{
    float dt = 0.0f;
    labrador::RectangleF before = labrador::RectangleF::zero;
    labrador::RectangleF after = labrador::RectangleF::zero;
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
    labrador::Vector2F previous_offset = labrador::Vector2F::zero;
    labrador::Vector2F current_offset = labrador::Vector2F::zero;
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
    labrador::RectangleF previous_top = labrador::RectangleF::zero;
    float displacement_x = 0.0f;
    // A lift's rider moves with its top (E2), before its own integration.
    float displacement_y = 0.0f;
};

struct HorizontalSupportRef
{
    const Structure* surface = nullptr; // Contact-phase borrow only in Player.
    std::uint32_t order = 0;
    HorizontalMotionId motion = 0; // Static supports still have a stable order.
};

struct HorizontalPaintImpact
{
    labrador::RectangleF anchor_rectangle = labrador::RectangleF::zero;
    labrador::Vector2F normal_to_projectile = labrador::Vector2F::zero;
    float penetration = 0.0f;
};

struct HorizontalStructureState
{
    HorizontalMotionId id = 0;
    HorizontalMotionState motion;
    labrador::RectangleF previous_bounds = labrador::RectangleF::zero;
    labrador::RectangleF current_bounds = labrador::RectangleF::zero;
    std::vector<PaintSurfaceSample> local_paint;
};

// The engine-free contract's copy of a spec, which the level editor shares.
horizontal_motion_contract::Spec contract_spec(const HorizontalMotionSpec& spec);

// One bounded triangular trajectory on one axis: horizontal, or vertical for
// a lift (ROAD#28). A Level prepares every token before any fallible/input
// work, then its concrete owners commit copies only.
class HorizontalMotion final
{
public:
    explicit HorizontalMotion(const HorizontalMotionSpec& spec);
    static void validate_spec(const HorizontalMotionSpec& spec);
    static void validate_dt(float dt);
    HorizontalMotionState state() const;
    // heading is read only by a paint drive: +1 right, -1 left, 0 hold.
    HorizontalPreparedStep prepare(float dt, int heading = 0) const;
    // The same positive-time step with the phase held: a lift that would
    // close on a player stays where it is for this step (E3b).
    HorizontalPreparedStep hold(float dt) const;
    labrador::RectangleF bounds() const;
    // The paint drive's heading for this paint, before any end stop.
    static int paint_heading(const HorizontalMotionSpec& spec, float team_a, float team_b);

private:
    friend class TranslatingStructurePaintable;
    void apply(const HorizontalPreparedStep& prepared) noexcept;
    static float offset_at(const HorizontalMotionSpec& spec, double phase);
    HorizontalPreparedStep prepare_driven(float dt, int heading) const;
    HorizontalMotionState state_;
};
