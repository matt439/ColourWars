#include "game/pch.h"
#include "game/objects/projectile.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/paint_audio.h"
#include "engine/collision/narrow_phase.h"
#include <array>
#include <limits>

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
    // The projectile can grow as well as move. Clip the interval during which
    // its four linearly moving edges overlap the obstacle, so diffusion does
    // not turn the sweep into an inflated, prematurely colliding box.
    std::optional<std::pair<float, float>> sweep_interval(
        const RectangleF& start, const RectangleF& finish,
        const RectangleF& obstacle)
    {
        float entry = 0.0f;
        float exit = 1.0f;
        const auto clip_positive = [&](float before, float after)
        {
            const float change = after - before;
            if (change == 0.0f)
            {
                return before > 0.0f;
            }
            const float crossing = -before / change;
            if (change > 0.0f)
            {
                entry = std::max(entry, crossing);
            }
            else
            {
                exit = std::min(exit, crossing);
            }
            return entry < exit;
        };
        if (!clip_positive(start.right() - obstacle.left(),
                finish.right() - obstacle.left()) ||
            !clip_positive(obstacle.right() - start.left(),
                obstacle.right() - finish.left()) ||
            !clip_positive(start.bottom() - obstacle.top(),
                finish.bottom() - obstacle.top()) ||
            !clip_positive(obstacle.bottom() - start.top(),
                obstacle.bottom() - finish.top()))
        {
            return std::nullopt;
        }
        return std::pair{ entry, exit };
    }

    bool launch_leaves_overlap(const RectangleF& start,
        const RectangleF& finish, const RectangleF& obstacle)
    {
        // A bucket's volume is wider than its owner. Starting that volume at
        // the player's centre can overlap a wall behind the shot even though
        // its muzzle is clear. Allow an exit on the same side as the origin;
        // exiting on the opposite side still crossed cover and must collide.
        const Vector2F origin = start.center();
        return start.intersects(obstacle) &&
            ((origin.x <= obstacle.left() && finish.right() <= obstacle.left()) ||
             (origin.x >= obstacle.right() && finish.left() >= obstacle.right()) ||
             (origin.y <= obstacle.top() && finish.bottom() <= obstacle.top()) ||
             (origin.y >= obstacle.bottom() && finish.top() >= obstacle.bottom()));
    }

    bool launch_leaves_slab(const RectangleF& start, const RectangleF& finish,
        const FixedSlabGeometry& slab)
    {
        // Retain the launch exemption only when a volume overlapping behind
        // the origin leaves the SAME separating side. A clear finish on the
        // far side crossed solid cover and must still collide.
        if (!slab.overlaps(start)) { return false; }
        const std::array<Vector2F, 4> axes{ Vector2F(1, 0), Vector2F(0, 1),
            slab.tangent(), slab.inward_normal() };
        for (const Vector2F& axis : axes)
        {
            const auto dot = [&axis](const Vector2F& p)
            { return static_cast<double>(p.x) * axis.x + static_cast<double>(p.y) * axis.y; };
            double low = std::numeric_limits<double>::infinity(), high = -low;
            for (const auto& p : slab.shape().points())
            { const double v = dot(p); low = std::min(low, v); high = std::max(high, v); }
            const double middle = dot(finish.center());
            const double radius = std::abs(static_cast<double>(axis.x)) * finish.width / 2.0 +
                std::abs(static_cast<double>(axis.y)) * finish.height / 2.0;
            const double origin = dot(start.center());
            if ((origin <= low && middle + radius <= low) ||
                (origin >= high && middle - radius >= high)) { return true; }
        }
        return false;
    }

    RectangleF rectangle_between(const RectangleF& start,
        const RectangleF& finish, float fraction)
    {
        return RectangleF(
            start.x + (finish.x - start.x) * fraction,
            start.y + (finish.y - start.y) * fraction,
            start.width + (finish.width - start.width) * fraction,
            start.height + (finish.height - start.height) * fraction);
    }

    // Relative edges retain an accepted impact independently of rounding its
    // final-world response rectangle. These values are not proxy engine objects.
    struct RelativeRectangle
    {
        double left, top, right, bottom;
    };

    RelativeRectangle relative_rectangle_at(const RectangleF& start,
        const RectangleF& finish, double time, double offset)
    {
        const auto edge = [time](float before, float after)
        { return static_cast<double>(before) +
            (static_cast<double>(after) - before) * time; };
        return { edge(start.left(), finish.left()) - offset,
            edge(start.top(), finish.top()),
            edge(start.right(), finish.right()) - offset,
            edge(start.bottom(), finish.bottom()) };
    }

    RelativeRectangle relative_between(const RelativeRectangle& start,
        const RelativeRectangle& finish, double fraction)
    {
        const auto edge = [fraction](double before, double after)
        { return before + (after - before) * fraction; };
        return { edge(start.left, finish.left), edge(start.top, finish.top),
            edge(start.right, finish.right), edge(start.bottom, finish.bottom) };
    }

    RectangleF relative_shape(const RelativeRectangle& rectangle)
    {
        return RectangleF(static_cast<float>(rectangle.left),
            static_cast<float>(rectangle.top),
            static_cast<float>(rectangle.right - rectangle.left),
            static_cast<float>(rectangle.bottom - rectangle.top));
    }

    std::optional<std::pair<double, double>> relative_sweep_interval(
        const RelativeRectangle& start, const RelativeRectangle& finish,
        const RectangleF& obstacle)
    {
        double entry = 0.0;
        double exit = 1.0;
        const auto clip_positive = [&](double before, double after)
        {
            if (!std::isfinite(before) || !std::isfinite(after)) { return false; }
            const double change = after - before;
            if (change == 0.0) { return before > 0.0; }
            const double crossing = -before / change;
            if (change > 0.0) { entry = std::max(entry, crossing); }
            else { exit = std::min(exit, crossing); }
            return entry < exit;
        };
        if (!clip_positive(start.right - obstacle.left(), finish.right - obstacle.left()) ||
            !clip_positive(obstacle.right() - start.left, obstacle.right() - finish.left) ||
            !clip_positive(start.bottom - obstacle.top(), finish.bottom - obstacle.top()) ||
            !clip_positive(obstacle.bottom() - start.top, obstacle.bottom() - finish.top))
        { return std::nullopt; }
        return std::pair{ entry, exit };
    }

    HorizontalPaintImpact relative_impact(const RelativeRectangle& start,
        const RelativeRectangle& finish, const RectangleF& anchor,
        const HorizontalMotionSegment& segment, const std::pair<double, double>& interval)
    {
        const double travel = std::max({ std::abs(finish.left - start.left),
            std::abs(finish.right - start.right), std::abs(finish.top - start.top),
            std::abs(finish.bottom - start.bottom) });
        const double inset = travel > 0.0 ? 0.01 / travel : 0.0;
        const double fraction = interval.first +
            std::min(inset, (interval.second - interval.first) / 2.0);
        const RelativeRectangle rectangle = relative_between(start, finish, fraction);
        // This accepted double interval has positive inset overlap. Measure
        // cardinal rectangle SAT here; a second rounded world query must not
        // erase a genuine thin/diffusing relative contact.
        const double forwards_x = rectangle.right - anchor.left();
        const double backwards_x = anchor.right() - rectangle.left;
        const double forwards_y = rectangle.bottom - anchor.top();
        const double backwards_y = anchor.bottom() - rectangle.top;
        const double depth_x = std::min(forwards_x, backwards_x);
        const double depth_y = std::min(forwards_y, backwards_y);
        HorizontalPaintImpact impact;
        impact.anchor_rectangle = relative_shape(rectangle);
        double depth = depth_x;
        impact.normal_to_projectile = backwards_x < forwards_x ?
            Vector2F(1.0f, 0.0f) : Vector2F(-1.0f, 0.0f);
        if (depth_y < depth_x)
        {
            depth = depth_y;
            impact.normal_to_projectile = backwards_y < forwards_y ?
                Vector2F(0.0f, 1.0f) : Vector2F(0.0f, -1.0f);
        }
        impact.penetration = static_cast<float>(depth);
        impact.global_entry = segment.begin +
            (segment.end - segment.begin) * interval.first;
        return impact;
    }
}

void Projectile::resolve_launch_obstruction(const Vector2F& origin,
    std::span<CollisionObject* const> objects)
{
    const RectangleF finish = this->shape()->bounding_box();
    RectangleF start = finish;
    start.set_position_at_center(origin);
    this->resolve_sweep(start, finish, objects, true);
}

void Projectile::resolve_movement_contacts(
    std::span<CollisionObject* const> objects)
{
    if (this->for_deletion_ || !this->first_update_done_) { return; }
    if (this->type_ == ProjectileType::rolling)
    {
        if (this->moving_contact_ready_)
        { this->resolve_rolling_translating_contacts(objects); }
        this->moving_contact_ready_ = false;
        return;
    }
    if (!is_swept(this->type_)) { return; }
    const bool translating = std::any_of(objects.begin(), objects.end(),
        [](const CollisionObject* object)
        { return dynamic_cast<const TranslatingStructurePaintable*>(object) != nullptr; });
    if (translating)
    {
        this->resolve_translating_contacts(this->previous_rectangle_,
            this->shape()->bounding_box(), objects, this->moving_contact_ready_);
    }
    else
    {
        // The complete original static path retains its float arithmetic.
        this->resolve_sweep(this->previous_rectangle_,
            this->shape()->bounding_box(), objects, false);
    }
    // Level supplies one complete target span, once, for this actual update.
    this->moving_contact_ready_ = false;
}

bool Projectile::moving_paint_completed(HorizontalMotionId id) const
{
    return id != 0 && std::find(this->moving_paint_completed_.begin(),
        this->moving_paint_completed_.end(), id) != this->moving_paint_completed_.end();
}

void Projectile::resolve_translating_contacts(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects, bool moving_ready)
{
    CollisionObject* obstruction = nullptr;
    TranslatingStructurePaintable* moving_obstruction = nullptr;
    HorizontalPaintImpact closest_impact;
    double closest_entry = 1.0;
    for (CollisionObject* object : objects)
    {
        if (object->for_deletion() || !layers_collide(this->layer(), this->mask(),
            object->layer(), object->mask())) { continue; }
        const bool structure = object->layer() == collision_layers::STRUCTURE;
        auto* moving = dynamic_cast<TranslatingStructurePaintable*>(object);
        if (moving != nullptr)
        {
            if (!moving_ready) { continue; }
            const HorizontalMotionStep step = moving->collision_step();
            const RectangleF anchor = moving->motion_state().spec.anchor;
            for (std::uint32_t index = 0; index < step.segment_count; ++index)
            {
                const HorizontalMotionSegment& segment = step.segments[index];
                const RelativeRectangle before = relative_rectangle_at(start, finish,
                    segment.begin, segment.from_x);
                const RelativeRectangle after = relative_rectangle_at(start, finish,
                    segment.end, segment.to_x);
                const auto interval = relative_sweep_interval(before, after, anchor);
                if (!interval) { continue; }
                const double entry = segment.begin +
                    (segment.end - segment.begin) * interval->first;
                if (entry < closest_entry || (entry == closest_entry && structure &&
                    obstruction != nullptr && obstruction->layer() != collision_layers::STRUCTURE))
                {
                    obstruction = object;
                    moving_obstruction = moving;
                    closest_entry = entry;
                    closest_impact = relative_impact(before, after, anchor, segment, *interval);
                }
            }
            continue;
        }
        if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(object))
        {
            const auto hit = slab->geometry().sweep(start, finish);
            if (hit && hit->positive_overlap && (hit->entry < closest_entry ||
                (hit->entry == closest_entry && obstruction != nullptr &&
                    obstruction->layer() != collision_layers::STRUCTURE)))
            {
                obstruction = object;
                moving_obstruction = nullptr;
                closest_entry = hit->entry;
            }
            continue;
        }
        const auto interval = sweep_interval(start, finish, object->shape()->bounding_box());
        if (interval && (interval->first < closest_entry ||
            (interval->first == closest_entry && structure && obstruction != nullptr &&
                obstruction->layer() != collision_layers::STRUCTURE)))
        {
            obstruction = object;
            moving_obstruction = nullptr;
            closest_entry = interval->first;
        }
    }
    if (moving_obstruction != nullptr)
    {
        moving_obstruction->paint_projectile_at(*this, closest_impact.anchor_rectangle);
        RectangleF response = closest_impact.anchor_rectangle;
        response.offset(moving_obstruction->translation());
        this->set_rectangle(response);
        this->on_contact(*moving_obstruction, -closest_impact.normal_to_projectile,
            closest_impact.penetration);
    }
    else if (obstruction != nullptr)
    {
        // Moving selection is complete. Reuse the original float sweep and
        // response verbatim for this winning static participant.
        CollisionObject* target = obstruction;
        this->resolve_sweep(start, finish, std::span<CollisionObject* const>(&target, 1), false);
    }
}

void Projectile::resolve_rolling_translating_contacts(std::span<CollisionObject* const> objects)
{
    const RectangleF volume = this->shape()->bounding_box(); // POST-launch published volume.
    for (CollisionObject* object : objects)
    {
        auto* moving = dynamic_cast<TranslatingStructurePaintable*>(object);
        if (moving == nullptr || object->for_deletion() ||
            !layers_collide(this->layer(), this->mask(), object->layer(), object->mask()) ||
            this->moving_paint_completed(moving->motion_id())) { continue; }
        const HorizontalMotionStep step = moving->collision_step();
        const RectangleF anchor = moving->motion_state().spec.anchor;
        std::optional<RelativeRectangle> footprint;
        for (std::uint32_t index = 0; index < step.segment_count; ++index)
        {
            const HorizontalMotionSegment& segment = step.segments[index];
            const RelativeRectangle before = relative_rectangle_at(volume, volume,
                segment.begin, segment.from_x);
            const RelativeRectangle after = relative_rectangle_at(volume, volume,
                segment.end, segment.to_x);
            const auto interval = relative_sweep_interval(before, after, anchor);
            if (!interval) { continue; }
            const RelativeRectangle entered = relative_between(before, after, interval->first);
            const RelativeRectangle exited = relative_between(before, after, interval->second);
            const RelativeRectangle visited{ std::min(entered.left, exited.left), entered.top,
                std::max(entered.right, exited.right), entered.bottom };
            if (!footprint) { footprint = visited; }
            else
            {
                footprint->left = std::min(footprint->left, visited.left);
                footprint->right = std::max(footprint->right, visited.right);
            }
        }
        if (footprint)
        {
            moving->paint_projectile_at(*this, relative_shape(*footprint));
            this->moving_paint_completed_.push_back(moving->motion_id());
        }
    }
}
void Projectile::resolve_sweep(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects,
    bool launch)
{
    if (std::any_of(objects.begin(), objects.end(), [](const CollisionObject* object)
        { return to_collision_type(object->tag()) == CollisionObjectType::structure_fixed_slab; }))
    {
        this->resolve_fixed_slab_sweep(start, finish, objects, launch);
        return;
    }
    CollisionObject* obstruction = nullptr;
    std::pair<float, float> closest{ 1.0f, 1.0f };
    for (CollisionObject* object : objects)
    {
        // Flying paint can hit an enemy between its owner and muzzle just
        // as it can hit cover there. A roller keeps its one-step volume:
        // enemy contacts belong to its ordinary multi-contact sweep, not an
        // extra launch hit. Masks retain ramp pass-through, friendly-fire
        // exclusion and dead players' empty collision mask.
        const bool structure = object->layer() == collision_layers::STRUCTURE;
        if ((launch && this->type_ == ProjectileType::rolling && !structure) ||
            object->for_deletion() ||
            !layers_collide(this->layer(), this->mask(),
                object->layer(), object->mask()))
        {
            continue;
        }
        const RectangleF obstacle = object->shape()->bounding_box();
        if (launch && structure &&
            launch_leaves_overlap(start, finish, obstacle))
        {
            continue;
        }
        const auto interval = sweep_interval(start, finish, obstacle);
        // Cover wins a tied entry time, including a launch already touching
        // both participants, independently of their order in the scene.
        if (interval && (interval->first < closest.first ||
            (interval->first == closest.first && structure &&
                obstruction != nullptr &&
                obstruction->layer() != collision_layers::STRUCTURE)))
        {
            obstruction = object;
            closest = *interval;
        }
    }
    if (obstruction == nullptr)
    {
        return;
    }

    // Paint from the entered face, not the endpoint buried inside a wall.
    // A tiny inset gives the ordinary response a real overlap, bounded by
    // the exit time so even a thin obstacle cannot be stepped through here.
    const float edge_travel = std::max({
        std::abs(finish.left() - start.left()),
        std::abs(finish.right() - start.right()),
        std::abs(finish.top() - start.top()),
        std::abs(finish.bottom() - start.bottom()) });
    const float inset = edge_travel > 0.0f ? 0.01f / edge_travel : 0.0f;
    const float fraction = closest.first +
        std::min(inset, (closest.second - closest.first) / 2.0f);
    this->set_rectangle(rectangle_between(start, finish, fraction));
    const auto contact = narrow_phase(*obstruction->shape(), *this->shape());
    if (contact)
    {
        obstruction->on_contact(*this, contact->normal, contact->penetration);
        this->on_contact(*obstruction, -contact->normal, contact->penetration);
    }
}

void Projectile::resolve_fixed_slab_sweep(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects, bool launch)
{
    // Only scenes containing the new geometry enter this selection path.
    // The legacy rectangle participants still compute their original float
    // intervals; slab intervals retain double precision through ordering.
    CollisionObject* obstruction = nullptr;
    FixedSlabPaintable* winning_slab = nullptr;
    FixedSlabSweep winning_hit;
    double closest = 1.0;
    for (CollisionObject* object : objects)
    {
        const bool structure = object->layer() == collision_layers::STRUCTURE;
        if ((launch && this->type_ == ProjectileType::rolling && !structure) ||
            object->for_deletion() || !layers_collide(this->layer(), this->mask(), object->layer(), object->mask()))
        { continue; }
        auto* slab = dynamic_cast<FixedSlabPaintable*>(object);
        std::optional<double> entry;
        std::optional<FixedSlabSweep> hit;
        if (slab != nullptr)
        {
            if (launch && launch_leaves_slab(start, finish, slab->geometry())) { continue; }
            hit = slab->geometry().sweep(start, finish);
            if (hit && hit->positive_overlap) { entry = hit->entry; }
        }
        else
        {
            const RectangleF obstacle = object->shape()->bounding_box();
            if (launch && structure && launch_leaves_overlap(start, finish, obstacle)) { continue; }
            if (const auto interval = sweep_interval(start, finish, obstacle)) { entry = interval->first; }
        }
        if (entry && (*entry < closest || (*entry == closest && structure && obstruction != nullptr &&
            obstruction->layer() != collision_layers::STRUCTURE)))
        {
            closest = *entry;
            obstruction = object;
            winning_slab = slab;
            if (hit) { winning_hit = *hit; }
        }
    }
    if (obstruction == nullptr) { return; }
    if (winning_slab == nullptr)
    {
        CollisionObject* target = obstruction;
        this->resolve_sweep(start, finish, std::span<CollisionObject* const>(&target, 1), launch);
        return;
    }
    const double edge_travel = std::max({ std::abs(static_cast<double>(finish.left()) - start.left()),
        std::abs(static_cast<double>(finish.right()) - start.right()),
        std::abs(static_cast<double>(finish.top()) - start.top()),
        std::abs(static_cast<double>(finish.bottom()) - start.bottom()) });
    const double inset = edge_travel > 0.0 ? 0.01 / edge_travel : 0.0;
    const auto at = [&](double fraction)
    {
        const auto between = [fraction](float a, float b)
        { return static_cast<float>(static_cast<double>(a) + (static_cast<double>(b) - a) * fraction); };
        return RectangleF(between(start.x, finish.x), between(start.y, finish.y),
            between(start.width, finish.width), between(start.height, finish.height));
    };
    RectangleF response = at(winning_hit.entry + std::min(inset, (winning_hit.exit - winning_hit.entry) / 2.0));
    if (!winning_slab->geometry().overlaps(response))
    { response = at((winning_hit.entry + winning_hit.exit) / 2.0); }
    if (!winning_slab->geometry().overlaps(response))
    { throw std::runtime_error("Fixed slab impact has no representable overlapping response rectangle"); }
    this->set_rectangle(response);
    const auto contact = narrow_phase(*winning_slab->shape(), *this->shape());
    const Vector2F normal = contact ? contact->normal : winning_hit.normal_to_rectangle;
    const float penetration = contact ? contact->penetration : 0.0f;
    winning_slab->on_contact(*this, normal, penetration);
    this->on_contact(*winning_slab, -normal, penetration);
}

void Projectile::on_contact(const CollisionObject& other,
    const Vector2F& normal, float /*penetration*/)
{
    this->geometry_contact_ |= other.layer() == collision_layers::STRUCTURE;
    if (!this->for_deletion_ && other.layer() == collision_layers::STRUCTURE &&
        this->type_ != ProjectileType::rolling)
    {
        // The collision normal points from the shot into cover. Anchor the
        // cue on that cover's actual face, with tangential position clamped
        // to the face (including corner hits and very thin walls).
        const RectangleF cover = other.shape()->bounding_box();
        const Vector2F center = this->shape()->bounding_box().center();
        Vector2F point(std::clamp(center.x, cover.left(), cover.right()),
            std::clamp(center.y, cover.top(), cover.bottom()));
        if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(&other))
        {
            const auto& geometry = slab->geometry();
            Vector2F local = geometry.to_local(center);
            local.x = std::clamp(local.x, 0.0f, geometry.length());
            local.y = std::clamp(local.y, 0.0f, FixedSlabGeometry::normal_thickness);
            const float along = normal.dot(geometry.tangent());
            const float across = normal.dot(geometry.inward_normal());
            if (std::abs(along) > std::abs(across)) { local.x = along > 0.0f ? 0.0f : geometry.length(); }
            else { local.y = across > 0.0f ? 0.0f : FixedSlabGeometry::normal_thickness; }
            point = geometry.to_world(local);
        }
        else if (std::abs(normal.x) >= std::abs(normal.y))
        {
            point.x = normal.x > 0.0f ? cover.left() : cover.right();
        }
        else
        {
            point.y = normal.y > 0.0f ? cover.top() : cover.bottom();
        }
        this->blocked_contact_ = BlockedShotContact{
            point, -normal, this->team_colour_, this->player_num_ };
    }
    // Flying projectiles die on contact with flat geometry or enemy players.
    // Rollers override this to process every contact in their one-step volume.
    this->set_for_deletion(true);
}

void Projectile::note_paint_contact(bool painted, bool captured) const
{
    this->geometry_contact_ = true;
    this->painted_contact_ |= painted;
    this->captured_contact_ |= captured;
    // Audio may be flushed first. Keep this classification until the visual
    // is consumed, independently of the audio batch's reset.
    this->wet_contact_ |= painted;
}

std::optional<BlockedShotContact> Projectile::take_blocked_contact()
{
    const std::optional<BlockedShotContact> result =
        this->for_deletion_ && !this->wet_contact_ ? this->blocked_contact_ : std::nullopt;
    this->blocked_contact_.reset();
    this->wet_contact_ = false;
    return result;
}

void Projectile::flush_paint_audio(PaintAudio& audio)
{
    // A roller touching bare geometry is still active and can reach exposed
    // paint in its ordinary sweep. Only retiring flying paint makes a dud.
    if (this->painted_contact_ ||
        (this->geometry_contact_ && this->for_deletion() && this->type_ != ProjectileType::rolling))
    {
        audio.contact(this->type_, this->player_num_,
            this->painted_contact_, this->captured_contact_);
    }
    this->geometry_contact_ = false;
    this->painted_contact_ = false;
    this->captured_contact_ = false;
}
