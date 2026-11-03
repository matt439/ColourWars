#include "game/pch.h"
#include "game/objects/level_structures.h"
#include "game/objects/collider.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/config/tuning.h"
#include "engine/scene/scene.h"
#include "game/contracts/simulation.h"
#include "game/contracts/horizontal_motion_contract.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <stdexcept>

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

    // A scalar offset along the mover's own axis: x, or y for a lift.
    RectangleF horizontal_pose(const HorizontalMotionSpec& spec, float offset)
    {
        RectangleF result = spec.anchor;
        if (offset != 0.0f)
        {
            if (spec.vertical()) { result.y += offset; }
            else { result.x += offset; }
        }
        horizontal_box(result);
        return result;
    }

    float axis_part(const HorizontalMotionSpec& spec, const Vector2F& offset)
    {
        return spec.vertical() ? offset.y : offset.x;
    }

    HorizontalBox horizontal_path(const TranslatingStructurePaintable& mover)
    {
        // The path's arithmetic reads only the anchor, travel, period and phase.
        return horizontal_motion_contract::path(contract_spec(mover.motion_state().spec));
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

bool LevelStructures::has_horizontal_motion() const
{
    return !this->horizontal_structures_.empty();
}

void LevelStructures::admit(const Scene& scene, std::span<Player* const> players,
    const PlayerTuning& player_tuning)
{
    this->admit_horizontal(scene, players, player_tuning);
    for (CollisionObject* object : scene.collision_objects())
    {
        Structure* structure = Collider::of(*object).structure();
        if (TeamGateStructurePaintable* gate = structure != nullptr ? structure->gate() : nullptr)
        {
            this->team_gates_.push_back(gate);
            this->team_gates_view_.push_back(gate);
        }
    }
    for (CollisionObject* object : scene.collision_objects())
    {
        if (Collider::of(*object).projectile() == nullptr)
        {
            this->projectile_targets_.push_back(object);
        }
    }
}

void LevelStructures::admit_horizontal(const Scene& scene, std::span<Player* const> players,
    const PlayerTuning& player_tuning)
{
    // Leave static Level arithmetic, allocation and construction order alone.
    bool moving = false;
    for (CollisionObject* object : scene.collision_objects())
    {
        const Structure* surface = Collider::of(*object).structure();
        moving |= surface != nullptr && surface->mover() != nullptr;
    }
    if (!moving) { return; }

    std::uint64_t support_order = 0;
    for (CollisionObject* object : scene.collision_objects())
    {
        Structure* surface = Collider::of(*object).structure();
        if (surface == nullptr) { continue; }
        if (support_order > (std::numeric_limits<std::uint32_t>::max)())
        { throw std::invalid_argument("Horizontal motion has too many canonical support identities."); }
        HorizontalMotionId id = 0;
        if (TranslatingStructurePaintable* mover = surface->mover())
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
    this->validate_admission(scene, players);
    this->validate_budget(nullptr, scene, players, player_tuning);
    // All counts/geometry were checked before the first binding operation.
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    {
        TranslatingStructurePaintable& mover = *this->horizontal_structures_[index];
        mover.bind_motion_id(static_cast<HorizontalMotionId>(index + 1));
        // A lift's sign stays within the column its admission just checked:
        // the lift plus the narrowest body's width each side (ROAD#28).
        if (!mover.motion_state().spec.vertical()) { continue; }
        float body = (std::numeric_limits<float>::max)();
        for (const Player* player : players) { body = std::min(body, player->rectangle().width); }
        mover.fit_cue_to_column(mover.motion_state().spec.anchor.width + 2.0f * body);
    }
}

void LevelStructures::validate_admission(const Scene& scene, std::span<Player* const> players) const
{
    if (!this->has_horizontal_motion()) { return; }
    if (players.empty() || !scene.bounds())
    { throw std::invalid_argument("Horizontal motion admission requires actual Players and finite Scene bounds."); }
    const HorizontalBox world = horizontal_box(*scene.bounds());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalBox path = horizontal_path(*mover);
        if (!horizontal_contains(world, path))
        { throw std::invalid_argument("Horizontal motion platform envelope leaves Scene bounds."); }
        for (const Player* player : players)
        {
            if (player == nullptr)
            { throw std::invalid_argument("Horizontal motion admission received a null Player."); }
            const RectangleF& body = player->rectangle();
            horizontal_box(body);
            const HorizontalBox standing = horizontal_motion_contract::standing(path, body.width, body.height);
            if (!horizontal_contains(world, standing))
            { throw std::invalid_argument("Horizontal motion standing envelope leaves Scene bounds."); }
            // A lift's lowest pose leaves a full body height beneath it (E3a as
            // amended): no floor meets it flush and no player stands under it.
            const bool lift = mover->motion_state().spec.vertical();
            const HorizontalBox underside = lift ?
                horizontal_motion_contract::underside(path, body.width, body.height) : HorizontalBox{};
            if (lift && !horizontal_contains(world, underside))
            { throw std::invalid_argument("Vertical lift clearance beneath its lowest pose leaves Scene bounds."); }
            // Beside its shaft too: a rider overhangs the lift's edge by up to a
            // body, so no geometry may sit within a body of the shaft anywhere
            // between the corridor and the clearance.
            const HorizontalBox column = lift ?
                horizontal_motion_contract::column(path, body.width, body.height) : HorizontalBox{};
            for (const CollisionObject* object : scene.collision_objects())
            {
                const Structure* surface = Collider::of(*object).structure();
                if (object == mover || surface == nullptr) { continue; }
                if (object->shape() == nullptr)
                { throw std::invalid_argument("Horizontal motion blocking geometry has no shape."); }
                const RectangleF actual = object->shape()->bounding_box();
                const HorizontalBox obstacle = surface->mover() != nullptr ?
                    horizontal_path(*surface->mover()) : horizontal_box(actual);
                // Positive, finite sums must not turn the engine's raw float
                // extent addition into an infinite, automatically passing bound.
                horizontal_finite(static_cast<double>(body.width) + actual.width);
                horizontal_finite(static_cast<double>(body.height) + actual.height);
                if (horizontal_intersects(path, obstacle))
                { throw std::invalid_argument("Horizontal motion platform envelope intersects blocking geometry."); }
                if (horizontal_intersects(standing, obstacle))
                { throw std::invalid_argument("Horizontal motion standing envelope intersects blocking geometry."); }
                if (lift && horizontal_intersects(underside, obstacle))
                { throw std::invalid_argument("Vertical lift needs a full body height clear beneath its lowest pose."); }
                if (lift && horizontal_intersects(column, obstacle))
                { throw std::invalid_argument("Vertical lift needs a body width clear beside its whole shaft."); }
            }
        }
    }
}

void LevelStructures::validate_budget(const Frame* prepared, const Scene& scene,
    std::span<Player* const> players, const PlayerTuning& player_tuning) const
{
    if (!this->has_horizontal_motion()) { return; }
    if (prepared != nullptr && prepared->motions.size() != this->horizontal_structures_.size())
    { throw std::invalid_argument("Horizontal motion preparation cardinality differs from admission."); }
    const double step = static_cast<double>(SECONDS_PER_STEP);
    double maximum_path = 0.0;
    double maximum_lift = 0.0;
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        const HorizontalMotionSpec spec = mover->motion_state().spec;
        HorizontalMotion::validate_spec(spec);
        double& maximum = spec.vertical() ? maximum_lift : maximum_path;
        maximum = std::max(maximum, horizontal_up(
            2.0 * static_cast<double>(spec.travel()) / static_cast<double>(spec.period_seconds) * step));
    }
    const HorizontalBox world = horizontal_box(*scene.bounds());
    for (const Player* player : players)
    {
        const RectangleF& body = player->rectangle();
        const HorizontalBox actual = horizontal_box(body);
        const HorizontalPlayerSpeed speed = horizontal_player_speed(*player, player_tuning);
        const double personal_x = horizontal_up(speed.x * step);
        const double personal_y = horizontal_up(speed.y * step);
        const double x_scale = horizontal_finite(std::max({ std::abs(world.left), std::abs(world.right),
            std::abs(actual.left), std::abs(actual.right) }) + personal_x + maximum_path);
        // A horizontal-only level adds a zero lift term, so its arithmetic is unchanged.
        const double y_scale = horizontal_finite(std::max({ std::abs(world.top), std::abs(world.bottom),
            std::abs(actual.top), std::abs(actual.bottom) }) + personal_y + maximum_lift);
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
            const bool lift = spec.vertical();
            double observed_path = 0.0;
            for (std::uint32_t segment = 0; segment < collision.segment_count; ++segment)
            {
                const HorizontalMotionSegment& part = collision.segments[segment];
                const RectangleF from = horizontal_pose(spec, part.from);
                const RectangleF to = horizontal_pose(spec, part.to);
                observed_path = horizontal_finite(observed_path + (lift ?
                    std::abs(static_cast<double>(to.y) - from.y) : std::abs(static_cast<double>(to.x) - from.x)));
            }
            const float carry = lift ? collision.after.y - collision.before.y :
                collision.after.x - collision.before.x;
            horizontal_finite(carry);
            const double ideal = horizontal_up(2.0 * static_cast<double>(spec.travel()) /
                static_cast<double>(spec.period_seconds) * step);
            // Four segment endpoints each incur offset/pose rounding; reserve
            // full ulps also for a copied carry subtraction and body addition.
            // The actual prepared WORLD path is included if it is larger.
            const double movement = horizontal_up(std::max({ ideal, observed_path,
                std::abs(static_cast<double>(carry)) }) + 16.0 * (lift ? y_ulp : x_ulp));
            const double mover_width = static_cast<double>(spec.anchor.width) - 2.0 * x_ulp;
            const double mover_height = static_cast<double>(spec.anchor.height) - 2.0 * y_ulp;
            // A lift's travel joins the vertical bound beside the player's own
            // speed (E6); a horizontal mover's joins the horizontal one.
            if (!(personal + (lift ? 16.0 * x_ulp : movement) < std::min(body_width, mover_width)))
            { throw std::invalid_argument("Horizontal motion relative horizontal displacement is unsafe."); }
            const double height_bound = horizontal_finite(body_height + mover_height);
            const double relative_vertical = lift ? horizontal_up(vertical + movement) : vertical;
            if (!(body_height > 0.0) || !(mover_height > 0.0) || !(relative_vertical < height_bound))
            { throw std::invalid_argument("Horizontal motion relative vertical displacement is unsafe."); }
        }
    }
}

LevelStructures::Frame LevelStructures::prepare(float dt, const Scene& scene,
    std::span<Player* const> players, const PlayerTuning& player_tuning) const
{
    HorizontalMotion::validate_dt(dt);
    this->validate_admission(scene, players);
    Frame frame;
    frame.motions.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { frame.motions.push_back(mover->prepare_motion(dt)); }
    this->hold_closing_lifts(frame, dt, scene, players);
    frame.carriers.resize(players.size());
    this->validate_budget(&frame, scene, players, player_tuning);
    for (std::size_t player = 0; player < players.size(); ++player)
    {
        const HorizontalMotionId selected = players[player]->carrier_id();
        if (selected == 0) { continue; }
        for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
        {
            if (this->horizontal_structures_[index]->motion_id() != selected) { continue; }
            const HorizontalMotionStep& step = frame.motions[index].collision;
            frame.carriers[player] = HorizontalCarryStep{ selected, step.before, step.after,
                step.after.x - step.before.x, step.after.y - step.before.y };
            break;
        }
    }
    return frame;
}

void LevelStructures::hold_closing_lifts(Frame& frame, float dt, const Scene& scene,
    std::span<Player* const> players) const
{
    if (!(dt > 0.0f)) { return; }
    std::optional<HorizontalBox> world;
    std::vector<std::pair<const CollisionObject*, HorizontalBox>> blocking;
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    {
        const TranslatingStructurePaintable& lift = *this->horizontal_structures_[index];
        const HorizontalMotionStep& step = frame.motions[index].collision;
        if (!lift.motion_state().spec.vertical() || step.after.y == step.before.y) { continue; }
        if (!world)
        {
            // Settled from the state before the step, as a gate and a paint
            // ferry's heading are: every other structure where it stands now,
            // and every other mover across its whole path.
            world = horizontal_box(*scene.bounds());
            for (const CollisionObject* object : scene.collision_objects())
            {
                const Structure* surface = Collider::of(*object).structure();
                if (surface == nullptr || object->shape() == nullptr) { continue; }
                blocking.emplace_back(object, surface->mover() != nullptr ?
                    horizontal_path(*surface->mover()) : horizontal_box(object->shape()->bounding_box()));
            }
        }
        std::vector<HorizontalBox> others;
        others.reserve(blocking.size());
        for (const auto& [object, box] : blocking)
        {
            if (object != &lift) { others.push_back(box); }
        }
        const HorizontalBox before = horizontal_box(step.before);
        const HorizontalBox after = horizontal_box(step.after);
        for (const Player* player : players)
        {
            if (player->state() != PlayerState::alive || !(player->health() > 0.0f)) { continue; }
            const bool rider = lift.motion_id() != 0 && player->carrier_id() == lift.motion_id();
            if (horizontal_motion_contract::closes_on(before, after,
                horizontal_box(player->rectangle()), rider, *world, others))
            {
                frame.motions[index] = lift.hold_motion(dt);
                break;
            }
        }
    }
}

void LevelStructures::commit(const Frame& prepared, std::span<Player* const> players) noexcept
{
    for (std::size_t index = 0; index < this->horizontal_structures_.size(); ++index)
    { this->horizontal_structures_[index]->commit_motion(prepared.motions[index]); }
    for (std::size_t index = 0; index < players.size(); ++index)
    { players[index]->prepare_carrier_step(prepared.carriers[index]); }
}

void LevelStructures::publish_zero(std::span<Player* const> players) noexcept
{
    for (TranslatingStructurePaintable* mover : this->horizontal_structures_)
    { mover->publish_zero_step(); }
    for (Player* player : players)
    {
        std::optional<HorizontalCarryStep> zero;
        for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
        {
            if (mover->motion_id() == player->carrier_id())
            {
                const RectangleF current = mover->bounds();
                zero = HorizontalCarryStep{ mover->motion_id(), current, current, 0.0f, 0.0f };
                break;
            }
        }
        player->prepare_carrier_step(zero);
    }
}

std::vector<HorizontalStructureState> LevelStructures::horizontal_structure_states() const
{
    std::vector<HorizontalStructureState> result;
    result.reserve(this->horizontal_structures_.size());
    for (const TranslatingStructurePaintable* mover : this->horizontal_structures_)
    {
        HorizontalStructureState state;
        state.id = mover->motion_id();
        state.motion = mover->motion_state();
        // History is persisted pose state, never the transient zero collision step.
        state.previous_bounds = horizontal_pose(state.motion.spec,
            axis_part(state.motion.spec, state.motion.previous_offset));
        state.current_bounds = horizontal_pose(state.motion.spec,
            axis_part(state.motion.spec, state.motion.current_offset));
        mover->append_anchor_paint_surfaces(state.local_paint);
        result.push_back(std::move(state));
    }
    return result;
}

std::span<const HorizontalSupportRef> LevelStructures::supports() const
{
    return this->horizontal_supports_;
}

void LevelStructures::settle_gates(std::span<Player* const> players)
{
    for (TeamGateStructurePaintable* gate : this->team_gates_) { gate->settle(players); }
}

std::span<const TeamGateStructurePaintable* const> LevelStructures::team_gates() const
{
    return this->team_gates_view_;
}

std::span<CollisionObject* const> LevelStructures::projectile_targets() const
{
    return this->projectile_targets_;
}
