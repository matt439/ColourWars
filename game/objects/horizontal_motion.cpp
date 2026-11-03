#include "game/pch.h"
#include "game/objects/horizontal_motion.h"
#include "game/contracts/simulation.h"
#include "game/contracts/horizontal_motion_contract.h"
#include <cmath>
#include <stdexcept>

using namespace mattmath;

horizontal_motion_contract::Spec contract_spec(const HorizontalMotionSpec& spec)
{
    return { { spec.anchor.x, spec.anchor.y, spec.anchor.width, spec.anchor.height },
        spec.travel_x, spec.period_seconds, spec.initial_phase, spec.drive, spec.team_a_heading,
        spec.travel_y };
}

void HorizontalMotion::validate_spec(const HorizontalMotionSpec& spec)
{
    // The drive rule is in the contract, which the level editor shares.
    horizontal_motion_contract::validate_spec(contract_spec(spec));
}

int HorizontalMotion::paint_heading(const HorizontalMotionSpec& spec, float team_a, float team_b)
{
    if (spec.drive != HorizontalDrive::paint || team_a == team_b) { return 0; }
    return team_a > team_b ? spec.team_a_heading : -spec.team_a_heading;
}

void HorizontalMotion::validate_dt(float dt)
{
    if (!std::isfinite(dt) || dt < 0.0f || dt > SECONDS_PER_STEP)
    {
        throw std::invalid_argument("Horizontal motion supports only zero or a finite positive simulation step <= SECONDS_PER_STEP.");
    }
}

HorizontalMotion::HorizontalMotion(const HorizontalMotionSpec& spec)
{
    validate_spec(spec);
    this->state_.spec = spec;
    this->state_.phase = static_cast<double>(spec.initial_phase);
    this->state_.direction = this->state_.phase < 0.5 ? 1 : -1;
    this->state_.current_offset = spec.axis_offset(offset_at(spec, this->state_.phase));
    this->state_.previous_offset = this->state_.current_offset;
}

HorizontalMotionState HorizontalMotion::state() const
{
    return this->state_;
}

float HorizontalMotion::offset_at(const HorizontalMotionSpec& spec, double phase)
{
    return horizontal_motion_contract::offset_at(contract_spec(spec), phase);
}

RectangleF HorizontalMotion::at_offset(const HorizontalMotionSpec& spec, float offset)
{
    RectangleF result = spec.anchor;
    if (offset != 0.0f)
    {
        if (spec.vertical()) { result.y += offset; }
        else { result.x += offset; }
    }
    return result;
}

namespace
{
    // The scalar offset along the mover's own axis.
    float axis_part(const HorizontalMotionSpec& spec, const Vector2F& offset)
    {
        return spec.vertical() ? offset.y : offset.x;
    }
}

RectangleF HorizontalMotion::bounds() const
{
    return at_offset(this->state_.spec, axis_part(this->state_.spec, this->state_.current_offset));
}

HorizontalPreparedStep HorizontalMotion::prepare(float dt, int heading) const
{
    validate_dt(dt);
    if (this->state_.spec.drive == HorizontalDrive::paint) { return this->prepare_driven(dt, heading); }
    HorizontalPreparedStep prepared;
    prepared.after_state = this->state_;
    HorizontalMotionStep& step = prepared.collision;
    step.dt = dt;
    step.before = this->bounds();
    step.after = step.before;
    if (dt == 0.0f) { return prepared; }

    const double advance = static_cast<double>(dt) /
        static_cast<double>(this->state_.spec.period_seconds);
    const double before = this->state_.phase;
    const double unwrapped = before + advance;
    const double turn = before < 0.5 ? 0.5 : 1.0;
    const double after = unwrapped >= 1.0 ? unwrapped - 1.0 : unwrapped;
    const float after_offset = offset_at(this->state_.spec, after);
    const float current = axis_part(this->state_.spec, this->state_.current_offset);
    if (unwrapped > turn)
    {
        // The spec/step bound admits at most one interior turn. Keep both
        // positive-time intervals even when their float offsets coincide.
        const double split = std::min(std::nextafter(1.0, 0.0),
            (turn - before) / advance);
        const float turn_offset = offset_at(this->state_.spec, turn);
        step.segments[0] = { 0.0, split, current, turn_offset };
        step.segments[1] = { split, 1.0, turn_offset, after_offset };
        step.segment_count = 2;
    }
    else
    {
        step.segments[0] = { 0.0, 1.0, current, after_offset };
        step.segment_count = 1;
    }
    prepared.after_state.phase = after;
    prepared.after_state.direction = after < 0.5 ? 1 : -1;
    prepared.after_state.previous_offset = this->state_.current_offset;
    prepared.after_state.current_offset = this->state_.spec.axis_offset(after_offset);
    step.after = at_offset(this->state_.spec, after_offset);
    return prepared;
}

HorizontalPreparedStep HorizontalMotion::prepare_driven(float dt, int heading) const
{
    HorizontalPreparedStep prepared;
    prepared.paint_heading = heading;
    prepared.after_state = this->state_;
    HorizontalMotionStep& step = prepared.collision;
    step.dt = dt;
    step.before = this->bounds();
    step.after = step.before;
    if (dt == 0.0f) { return prepared; }

    // The same speed as a cycle's leg, so the admission budget still holds.
    const double advance = static_cast<double>(dt) /
        static_cast<double>(this->state_.spec.period_seconds);
    const double before = this->state_.phase;
    const double target = heading > 0 ? 0.5 : heading < 0 ? 0.0 : before;
    const double remaining = std::abs(target - before);
    const float current = axis_part(this->state_.spec, this->state_.current_offset);
    double after = before;
    if (remaining > 0.0)
    {
        after = advance >= remaining ? target : before + (heading > 0 ? advance : -advance);
    }
    const float after_offset = after == before ? current : offset_at(this->state_.spec, after);
    if (remaining > 0.0 && advance > remaining)
    {
        // Reaches its end stop inside the step, then holds there.
        const double split = std::min(std::nextafter(1.0, 0.0), remaining / advance);
        step.segments[0] = { 0.0, split, current, after_offset };
        step.segments[1] = { split, 1.0, after_offset, after_offset };
        step.segment_count = 2;
    }
    else
    {
        // A held mover still publishes its positive-time interval.
        step.segments[0] = { 0.0, 1.0, current, after_offset };
        step.segment_count = 1;
    }
    prepared.after_state.phase = after;
    prepared.after_state.direction = after < 0.5 ? 1 : -1;
    prepared.after_state.previous_offset = this->state_.current_offset;
    prepared.after_state.current_offset = this->state_.spec.axis_offset(after_offset);
    step.after = at_offset(this->state_.spec, after_offset);
    return prepared;
}

HorizontalPreparedStep HorizontalMotion::hold(float dt) const
{
    validate_dt(dt);
    // As a paint ferry holds on a tie: the phase stays, and a positive step
    // still publishes its one positive-time interval at the current pose.
    HorizontalPreparedStep prepared;
    prepared.after_state = this->state_;
    HorizontalMotionStep& step = prepared.collision;
    step.dt = dt;
    step.before = this->bounds();
    step.after = step.before;
    if (dt == 0.0f) { return prepared; }
    const float current = axis_part(this->state_.spec, this->state_.current_offset);
    step.segments[0] = { 0.0, 1.0, current, current };
    step.segment_count = 1;
    prepared.after_state.previous_offset = this->state_.current_offset;
    return prepared;
}

void HorizontalMotion::apply(const HorizontalPreparedStep& prepared) noexcept
{
    this->state_ = prepared.after_state;
}
