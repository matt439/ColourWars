#include "game/pch.h"
#include "game/objects/horizontal_motion.h"
#include "game/simulation.h"
#include "game/horizontal_motion_contract.h"
#include <cmath>
#include <stdexcept>

using namespace mattmath;

namespace
{
    horizontal_motion_contract::Spec contract_spec(const HorizontalMotionSpec& spec)
    {
        return { { spec.anchor.x, spec.anchor.y, spec.anchor.width, spec.anchor.height },
            spec.travel_x, spec.period_seconds, spec.initial_phase };
    }
}

void HorizontalMotion::validate_spec(const HorizontalMotionSpec& spec)
{
    horizontal_motion_contract::validate_spec(contract_spec(spec));
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
    this->state_.current_offset = { offset_at(spec, this->state_.phase), 0.0f };
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
    if (offset != 0.0f) { result.x += offset; }
    return result;
}

RectangleF HorizontalMotion::bounds() const
{
    return at_offset(this->state_.spec, this->state_.current_offset.x);
}

HorizontalPreparedStep HorizontalMotion::prepare(float dt) const
{
    validate_dt(dt);
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
    if (unwrapped > turn)
    {
        // The spec/step bound admits at most one interior turn. Keep both
        // positive-time intervals even when their float offsets coincide.
        const double split = std::min(std::nextafter(1.0, 0.0),
            (turn - before) / advance);
        const float turn_offset = offset_at(this->state_.spec, turn);
        step.segments[0] = { 0.0, split, this->state_.current_offset.x, turn_offset };
        step.segments[1] = { split, 1.0, turn_offset, after_offset };
        step.segment_count = 2;
    }
    else
    {
        step.segments[0] = { 0.0, 1.0, this->state_.current_offset.x, after_offset };
        step.segment_count = 1;
    }
    prepared.after_state.phase = after;
    prepared.after_state.direction = after < 0.5 ? 1 : -1;
    prepared.after_state.previous_offset = this->state_.current_offset;
    prepared.after_state.current_offset = { after_offset, 0.0f };
    step.after = at_offset(this->state_.spec, after_offset);
    return prepared;
}

void HorizontalMotion::apply(const HorizontalPreparedStep& prepared) noexcept
{
    this->state_ = prepared.after_state;
}
