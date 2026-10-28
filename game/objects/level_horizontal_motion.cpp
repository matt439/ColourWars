#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/structure.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/simulation.h"
#include "game/horizontal_motion_contract.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace mattmath;
using namespace labrador;

namespace
{
    using HorizontalBox = horizontal_motion_contract::Box;

    double horizontal_finite(double value)
    {
        return horizontal_motion_contract::finite(value);
    }

    double horizontal_up(double value)
    {
        return horizontal_motion_contract::up(value);
    }

    // A full upward float spacing covers nearest-rounding error on either sign.
    // Refuse extreme coordinates instead of deriving an infinite/vacuous bound.
    double horizontal_ulp(double magnitude)
    {
        return horizontal_motion_contract::ulp(magnitude);
    }

    HorizontalBox horizontal_box(const RectangleF& value)
    {
        return horizontal_motion_contract::box({ value.x, value.y, value.width, value.height });
    }

    RectangleF horizontal_pose(const HorizontalMotionSpec& spec, float offset)
    {
        RectangleF result = spec.anchor;
        if (offset != 0.0f) { result.x += offset; }
        horizontal_box(result);
        return result;
    }

    HorizontalBox horizontal_path(const TranslatingStructurePaintable& mover)
    {
        const HorizontalMotionSpec spec = mover.motion_state().spec;
        return horizontal_motion_contract::path({
            { spec.anchor.x, spec.anchor.y, spec.anchor.width, spec.anchor.height },
            spec.travel_x, spec.period_seconds, spec.initial_phase });
    }

    bool horizontal_contains(const HorizontalBox& outer, const HorizontalBox& inner)
    {
        return horizontal_motion_contract::contains(outer, inner);
    }

    bool horizontal_intersects(const HorizontalBox& a, const HorizontalBox& b)
    {
        // Positive interiors only. A coplanar dock's top touches the rider's
        // feet boundary; it is not a raised lip/ceiling exemption.
        return horizontal_motion_contract::intersects(a, b);
    }

    struct HorizontalPlayerSpeed { double x, y; };

    HorizontalPlayerSpeed horizontal_player_speed(const Player& actual,
        const PlayerTuning& values)
    {
        const Vector2F velocity = actual.velocity();
        // Check operands before max(): a NaN in a later argument could otherwise
        // be ignored by std::max rather than poison the derived budget.
        for (float value : { values.max_velocity.x, values.max_velocity.y,
            values.x_initial_velocity, values.x_acceleration, values.x_deacceleration,
            values.gravity, values.ramp_gravity_multiplier, values.jump_launch_velocity,
            values.drop_down_velocity, values.jump_acceleration,
            values.paint_underfoot.own_speed_multiplier, values.paint_underfoot.enemy_speed_multiplier })
        { horizontal_finite(static_cast<double>(value)); }
        horizontal_finite(velocity.x); horizontal_finite(velocity.y);
        if (!(values.max_velocity.x > 0.0f) || !(values.max_velocity.y > 0.0f) ||
            !(values.x_initial_velocity > 0.0f) || !(values.x_acceleration >= 0.0f) ||
            !(values.x_deacceleration > 0.0f) || !(values.ramp_gravity_multiplier >= 0.0f) ||
            !(values.paint_underfoot.own_speed_multiplier >= 1.0f) ||
            !(values.paint_underfoot.enemy_speed_multiplier > 0.0f &&
                values.paint_underfoot.enemy_speed_multiplier <= 1.0f))
        { throw std::invalid_argument("Horizontal motion requires supported player rate magnitudes."); }
        const double factor = values.paint_underfoot.enabled ?
            std::max(1.0, static_cast<double>(values.paint_underfoot.own_speed_multiplier)) : 1.0;
        const double initial = horizontal_up(std::abs(static_cast<double>(values.x_initial_velocity)) * factor);
        const double cap = horizontal_up(std::abs(static_cast<double>(values.max_velocity.x)) * factor);
        const double acceleration = horizontal_up(std::abs(static_cast<double>(values.x_acceleration)) * factor);
        const double braking = horizontal_up(std::abs(static_cast<double>(values.x_deacceleration)) * factor);
        // These float intermediates exist before any clamp/stop-time branch.
        horizontal_up(acceleration + braking);
        const double step = static_cast<double>(SECONDS_PER_STEP);
        double reachable_x = std::max({ std::abs(static_cast<double>(velocity.x)), initial, cap });
        double acceleration_step = 0.0;
        double least_braking = (std::numeric_limits<double>::max)();
        const std::array<float, 3> factors{ 1.0f,
            values.paint_underfoot.own_speed_multiplier,
            values.paint_underfoot.enemy_speed_multiplier };
        const std::size_t factor_count = values.paint_underfoot.enabled ? factors.size() : 1;
        const auto effective_rate = [](float base, float multiplier, bool positive)
        {
            // Mirror Player's ==1 branch and actual float scaling, after a
            // conservative double product check. In particular the denominator
            // below must not use a rounded-UP estimate of a tiny braking rate.
            horizontal_up(static_cast<double>(base) * static_cast<double>(multiplier));
            const float result = multiplier == 1.0f ? base : base * multiplier;
            if (!std::isfinite(result) || !(positive ? result > 0.0f : result >= 0.0f))
            { throw std::invalid_argument("Horizontal motion effective horizontal rates are non-finite or unusable."); }
            return result;
        };
        for (std::size_t index = 0; index < factor_count; ++index)
        {
            const float multiplier = factors[index];
            const float effective_initial = effective_rate(values.x_initial_velocity, multiplier, true);
            const float effective_cap = effective_rate(values.max_velocity.x, multiplier, true);
            const float effective_acceleration = effective_rate(values.x_acceleration, multiplier, false);
            const float effective_braking = effective_rate(values.x_deacceleration, multiplier, true);
            reachable_x = std::max({ reachable_x, static_cast<double>(effective_initial),
                static_cast<double>(effective_cap) });
            // |input|<=1 cannot increase either product. Check the opposing
            // sum, then acceleration*dt (before its multiply by input).
            horizontal_up(static_cast<double>(effective_braking) + effective_acceleration);
            acceleration_step = std::max(acceleration_step,
                horizontal_up(static_cast<double>(effective_acceleration) * step));
            least_braking = std::min(least_braking, static_cast<double>(effective_braking));
        }
        const double preclamp = reachable_x + acceleration_step;
        if (!std::isfinite(preclamp) || preclamp > static_cast<double>((std::numeric_limits<float>::max)()))
        { throw std::invalid_argument("Horizontal motion horizontal pre-clamp addition is unsafe."); }
        horizontal_up(preclamp);
        // braking_rate + acceleration*|input| is at least this ACTUAL positive
        // scaled rate for every enabled class. Retained/reachable vx can enter
        // that opposing-input division before any cap recovers its magnitude.
        const double stop_time = reachable_x / least_braking;
        if (!std::isfinite(stop_time) || stop_time > static_cast<double>((std::numeric_limits<float>::max)()))
        { throw std::invalid_argument("Horizontal motion horizontal stop-time division is unsafe."); }
        horizontal_up(stop_time);
        const double gravity = std::abs(static_cast<double>(values.gravity));
        const double ramp_gravity = horizontal_up(gravity *
            std::abs(static_cast<double>(values.ramp_gravity_multiplier)));
        double vertical = std::max({ std::abs(static_cast<double>(velocity.y)),
            std::abs(static_cast<double>(values.max_velocity.y)),
            std::abs(static_cast<double>(values.jump_launch_velocity)) });
        vertical = horizontal_up(vertical + horizontal_up(std::max(gravity, ramp_gravity) * step));
        vertical = horizontal_up(vertical + std::abs(static_cast<double>(values.drop_down_velocity)));
        vertical = horizontal_up(vertical + horizontal_up(
            std::abs(static_cast<double>(values.jump_acceleration)) * step));
        return { reachable_x, vertical };
    }
}

bool Level::has_horizontal_motion() const
{
    return !this->horizontal_structures_.empty();
}

void Level::admit_horizontal_structures()
{
    // Leave static Level arithmetic, allocation and construction order alone.
    bool moving = false;
    for (CollisionObject* object : this->scene_->collision_objects())
    { moving |= dynamic_cast<TranslatingStructurePaintable*>(object) != nullptr; }
    if (!moving) { return; }

    std::uint64_t support_order = 0;
    for (CollisionObject* object : this->scene_->collision_objects())
    {
        auto* surface = dynamic_cast<Structure*>(object);
        if (surface == nullptr) { continue; }
        if (support_order > (std::numeric_limits<std::uint32_t>::max)())
        { throw std::invalid_argument("Horizontal motion has too many canonical support identities."); }
        HorizontalMotionId id = 0;
        if (auto* mover = dynamic_cast<TranslatingStructurePaintable*>(surface))
        {
            if (mover->motion_id() != 0 || this->horizontal_structures_.size() >=
                static_cast<std::size_t>((std::numeric_limits<int>::max)()))
            { throw std::invalid_argument("Horizontal motion identities must be bound once within INT_MAX."); }
            this->horizontal_structures_.push_back(mover);
            id = static_cast<HorizontalMotionId>(this->horizontal_structures_.size());
        }
        this->horizontal_supports_.push_back({ surface,
            static_cast<std::uint32_t>(support_order++), id });
    }
    this->validate_horizontal_admission();
    this->validate_horizontal_budget(nullptr);
    // All counts/geometry were checked before the first binding operation.
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    { this->horizontal_structures_[index]->bind_motion_id(static_cast<HorizontalMotionId>(index + 1)); }
}

void Level::validate_horizontal_admission() const
{
    if (!this->has_horizontal_motion()) { return; }
    if (this->players_.empty() || !this->scene_->bounds())
    { throw std::invalid_argument("Horizontal motion admission requires actual Players and finite Scene bounds."); }
    const HorizontalBox world = horizontal_box(*this->scene_->bounds());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalBox path = horizontal_path(*mover);
        if (!horizontal_contains(world, path))
        { throw std::invalid_argument("Horizontal motion platform envelope leaves Scene bounds."); }
        for (const Player* player : this->players_)
        {
            if (player == nullptr)
            { throw std::invalid_argument("Horizontal motion admission received a null Player."); }
            const RectangleF& body = player->rectangle();
            horizontal_box(body);
            const HorizontalBox standing = horizontal_motion_contract::standing(path, body.width, body.height);
            if (!horizontal_contains(world, standing))
            { throw std::invalid_argument("Horizontal motion standing envelope leaves Scene bounds."); }
            for (const CollisionObject* object : this->scene_->collision_objects())
            {
                if (object == mover || !is_structure(to_collision_type(object->tag()))) { continue; }
                if (object->shape() == nullptr)
                { throw std::invalid_argument("Horizontal motion blocking geometry has no shape."); }
                const RectangleF actual = object->shape()->bounding_box();
                const HorizontalBox obstacle = dynamic_cast<const TranslatingStructurePaintable*>(object) != nullptr ?
                    horizontal_path(*static_cast<const TranslatingStructurePaintable*>(object)) : horizontal_box(actual);
                // Positive, finite sums must not turn the engine's raw float
                // extent addition into an infinite, automatically passing bound.
                horizontal_finite(static_cast<double>(body.width) + actual.width);
                horizontal_finite(static_cast<double>(body.height) + actual.height);
                if (horizontal_intersects(path, obstacle))
                { throw std::invalid_argument("Horizontal motion platform envelope intersects blocking geometry."); }
                if (horizontal_intersects(standing, obstacle))
                { throw std::invalid_argument("Horizontal motion standing envelope intersects blocking geometry."); }
            }
        }
    }
}

void Level::validate_horizontal_budget(const HorizontalFrame* prepared) const
{
    if (!this->has_horizontal_motion()) { return; }
    if (prepared != nullptr && prepared->motions.size() != this->horizontal_structures_.size())
    { throw std::invalid_argument("Horizontal motion preparation cardinality differs from admission."); }
    const double step = static_cast<double>(SECONDS_PER_STEP);
    double maximum_path = 0.0;
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalMotionSpec spec = mover->motion_state().spec;
        HorizontalMotion::validate_spec(spec);
        maximum_path = std::max(maximum_path, horizontal_up(
            2.0 * static_cast<double>(spec.travel_x) / static_cast<double>(spec.period_seconds) * step));
    }
    const HorizontalBox world = horizontal_box(*this->scene_->bounds());
    for (const Player* player : this->players_)
    {
        const RectangleF& body = player->rectangle();
        const HorizontalBox actual = horizontal_box(body);
        const HorizontalPlayerSpeed speed = horizontal_player_speed(*player, this->tuning_->player());
        const double personal_x = horizontal_up(speed.x * step);
        const double personal_y = horizontal_up(speed.y * step);
        const double x_scale = horizontal_finite(std::max({ std::abs(world.left), std::abs(world.right),
            std::abs(actual.left), std::abs(actual.right) }) + personal_x + maximum_path);
        const double y_scale = horizontal_finite(std::max({ std::abs(world.top), std::abs(world.bottom),
            std::abs(actual.top), std::abs(actual.bottom) }) + personal_y);
        const double x_ulp = horizontal_ulp(horizontal_finite(2.0 * x_scale));
        const double y_ulp = horizontal_ulp(horizontal_finite(2.0 * y_scale));
        const double personal = horizontal_up(personal_x + 4.0 * x_ulp);
        const double vertical = horizontal_up(personal_y + 4.0 * y_ulp);
        const double body_width = std::min(static_cast<double>(body.width), actual.right - actual.left) - 2.0 * x_ulp;
        const double body_height = std::min(static_cast<double>(body.height), actual.bottom - actual.top) - 2.0 * y_ulp;
        for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
        {
            const TranslatingStructurePaintable& mover = *this->horizontal_structures_[index];
            const HorizontalMotionSpec spec = mover.motion_state().spec;
            const HorizontalPreparedStep probe = prepared == nullptr ? mover.prepare_motion(SECONDS_PER_STEP) :
                prepared->motions[index];
            const HorizontalMotionStep& collision = probe.collision;
            horizontal_box(collision.before); horizontal_box(collision.after);
            if (collision.segment_count > collision.segments.size())
            { throw std::invalid_argument("Horizontal motion preparation has too many segments."); }
            double observed_path = 0.0;
            for (std::uint32_t segment = 0; segment < collision.segment_count; ++segment)
            {
                const HorizontalMotionSegment& part = collision.segments[segment];
                const RectangleF from = horizontal_pose(spec, part.from_x);
                const RectangleF to = horizontal_pose(spec, part.to_x);
                observed_path = horizontal_finite(observed_path + std::abs(static_cast<double>(to.x) - from.x));
            }
            const float carry = collision.after.x - collision.before.x;
            horizontal_finite(carry);
            const double ideal = horizontal_up(2.0 * static_cast<double>(spec.travel_x) /
                static_cast<double>(spec.period_seconds) * step);
            // Four segment endpoints each incur offset/pose rounding; reserve
            // full ulps also for a copied carry subtraction and body addition.
            // The actual prepared WORLD path is included if it is larger.
            const double movement = horizontal_up(std::max({ ideal, observed_path,
                std::abs(static_cast<double>(carry)) }) + 16.0 * x_ulp);
            const double mover_width = static_cast<double>(spec.anchor.width) - 2.0 * x_ulp;
            const double mover_height = static_cast<double>(spec.anchor.height) - 2.0 * y_ulp;
            if (!(personal + movement < std::min(body_width, mover_width)))
            { throw std::invalid_argument("Horizontal motion relative horizontal displacement is unsafe."); }
            const double height_bound = horizontal_finite(body_height + mover_height);
            if (!(body_height > 0.0) || !(mover_height > 0.0) || !(vertical < height_bound))
            { throw std::invalid_argument("Horizontal motion relative vertical displacement is unsafe."); }
        }
    }
}

Level::HorizontalFrame Level::prepare_horizontal_frame(float dt) const
{
    HorizontalMotion::validate_dt(dt);
    this->validate_horizontal_admission();
    HorizontalFrame frame;
    frame.motions.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { frame.motions.push_back(mover->prepare_motion(dt)); }
    frame.carriers.resize(this->players_.size());
    this->validate_horizontal_budget(&frame);
    for (std::size_t player = 0; player < this->players_.size(); ++player)
    {
        const HorizontalMotionId selected = this->players_[player]->carrier_id();
        if (selected == 0) { continue; }
        for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
        {
            if (this->horizontal_structures_[index]->motion_id() != selected) { continue; }
            const HorizontalMotionStep& step = frame.motions[index].collision;
            frame.carriers[player] = HorizontalCarryStep{ selected, step.before, step.after,
                step.after.x - step.before.x };
            break;
        }
    }
    return frame;
}

void Level::commit_horizontal_frame(const HorizontalFrame& prepared) noexcept
{
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    { this->horizontal_structures_[index]->commit_motion(prepared.motions[index]); }
    for (std::size_t index = 0; index < this->players_.size(); ++index)
    { this->players_[index]->prepare_carrier_step(prepared.carriers[index]); }
}

void Level::publish_horizontal_zero() noexcept
{
    for (TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { mover->publish_zero_step(); }
    for (Player* player : this->players_)
    {
        std::optional<HorizontalCarryStep> zero;
        for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
        {
            if (mover->motion_id() == player->carrier_id())
            {
                const RectangleF current = mover->bounds();
                zero = HorizontalCarryStep{ mover->motion_id(), current, current, 0.0f };
                break;
            }
        }
        player->prepare_carrier_step(zero);
    }
}

std::vector<HorizontalStructureState> Level::horizontal_structure_states() const
{
    std::vector<HorizontalStructureState> result;
    result.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        HorizontalStructureState state;
        state.id = mover->motion_id();
        state.motion = mover->motion_state();
        // History is persisted pose state, never the transient zero collision step.
        state.previous_bounds = horizontal_pose(state.motion.spec, state.motion.previous_offset.x);
        state.current_bounds = horizontal_pose(state.motion.spec, state.motion.current_offset.x);
        mover->append_anchor_paint_surfaces(state.local_paint);
        result.push_back(std::move(state));
    }
    return result;
}
