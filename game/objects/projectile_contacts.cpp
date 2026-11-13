#include "game/pch.h"
#include "game/objects/projectile.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/fixed_slab_geometry.h"
#include "game/objects/paint_audio.h"
#include "engine/collision/narrow_phase.h"
#include <array>
#include <limits>

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

    // ROAD#44: the stretch of a disc's travel, its centre moving from
    // start_centre to finish_centre, that overlaps obstacle, as fractions of
    // the travel; empty when the disc never enters it. The disc overlaps the
    // box exactly where its centre lies in the box rounded by its radius: the
    // union of the box widened by the radius in x, the box widened by it in
    // y and the four corner discs. That union is convex, so the travel
    // crosses it in one stretch, from the earliest entry of any of the six
    // pieces to the latest exit. Double precision throughout; a touch with no
    // positive overlap is no entry, as for sweep_interval.
    std::optional<std::pair<float, float>> circle_sweep_interval(
        const Vector2F& start_centre, const Vector2F& finish_centre,
        float radius, const RectangleF& obstacle)
    {
        const double x0 = start_centre.x, y0 = start_centre.y;
        const double dx = static_cast<double>(finish_centre.x) - x0;
        const double dy = static_cast<double>(finish_centre.y) - y0;
        const double r = radius;
        double entry = std::numeric_limits<double>::infinity();
        double exit = -entry;
        const auto keep = [&](double low, double high)
        {
            low = std::max(low, 0.0);
            high = std::min(high, 1.0);
            if (low < high) { entry = std::min(entry, low); exit = std::max(exit, high); }
        };
        // The centre's travel inside the box [left, right] x [top, bottom].
        const auto box = [&](double left, double top, double right, double bottom)
        {
            double low = -std::numeric_limits<double>::infinity();
            double high = -low;
            const auto axis = [&](double from, double change, double min, double max)
            {
                if (change == 0.0) { return from > min && from < max; }
                double a = (min - from) / change, b = (max - from) / change;
                if (a > b) { std::swap(a, b); }
                low = std::max(low, a);
                high = std::min(high, b);
                return low < high;
            };
            if (axis(x0, dx, left, right) && axis(y0, dy, top, bottom)) { keep(low, high); }
        };
        // The centre's travel inside the corner disc at (cx, cy).
        const auto corner = [&](double cx, double cy)
        {
            const double fx = x0 - cx, fy = y0 - cy;
            const double a = dx * dx + dy * dy;
            const double c = fx * fx + fy * fy - r * r;
            if (a == 0.0) { if (c < 0.0) { keep(0.0, 1.0); } return; }
            const double b = fx * dx + fy * dy;
            const double discriminant = b * b - a * c;
            if (discriminant <= 0.0) { return; }
            const double root = std::sqrt(discriminant);
            keep((-b - root) / a, (-b + root) / a);
        };
        const double left = obstacle.left(), top = obstacle.top();
        const double right = obstacle.right(), bottom = obstacle.bottom();
        box(left - r, top, right + r, bottom);
        box(left, top - r, right, bottom + r);
        corner(left, top);
        corner(right, top);
        corner(left, bottom);
        corner(right, bottom);
        if (!(entry < exit)) { return std::nullopt; }
        return std::pair{ static_cast<float>(entry), static_cast<float>(exit) };
    }

    // The stretch of a shot's travel from start to finish that overlaps
    // obstacle: its box's (sweep_interval), or with a radius its inscribed
    // disc's (circle_sweep_interval, ROAD#44).
    std::optional<std::pair<float, float>> shot_interval(
        const RectangleF& start, const RectangleF& finish,
        const RectangleF& obstacle, std::optional<float> radius)
    {
        if (!radius) { return sweep_interval(start, finish, obstacle); }
        return circle_sweep_interval(start.center(), finish.center(), *radius, obstacle);
    }

    // The first breakable (Level's planted subs) this shot's travel from
    // start to finish enters: an enemy one that takes hits now, the earliest
    // entry winning and the earlier in the list on a tie. An empty list does
    // no arithmetic, so a scene without a planted sub sweeps as it always has.
    struct BreakableHit
    {
        Projectile* target;
        std::pair<float, float> interval;
    };

    std::optional<BreakableHit> first_breakable(const Projectile& shot,
        const RectangleF& start, const RectangleF& finish,
        std::span<Projectile* const> breakables, std::optional<float> radius)
    {
        std::optional<BreakableHit> first;
        for (Projectile* target : breakables)
        {
            // Asked now, so one broken earlier this tick stops nothing more.
            if (target == &shot || !target->takes_hits() ||
                target->team() == shot.team()) { continue; }
            const auto interval = shot_interval(start, finish, target->bounds(), radius);
            if (interval && (!first || interval->first < first->interval.first))
            {
                first = BreakableHit{ target, *interval };
            }
        }
        return first;
    }

    // Whether a breakable entered at `entry` stops the shot before the
    // obstruction the sweep chose (none: `closest` is the end of the tick).
    // Cover keeps a tied entry, as it does against a player; a breakable
    // takes a tie from a player, which it stands in front of.
    bool breakable_first(double entry, double closest, const CollisionObject* obstruction)
    {
        return entry < closest || (entry == closest && obstruction != nullptr &&
            obstruction->layer() != collision_layers::structure);
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

    // The mover's offset comes off the projectile's edges on the mover's own
    // axis: left/right for a horizontal mover, top/bottom for a lift.
    RelativeRectangle relative_rectangle_at(const RectangleF& start,
        const RectangleF& finish, double time, double offset, bool vertical)
    {
        const auto edge = [time](float before, float after)
        { return static_cast<double>(before) +
            (static_cast<double>(after) - before) * time; };
        const double x = vertical ? 0.0 : offset;
        const double y = vertical ? offset : 0.0;
        return { edge(start.left(), finish.left()) - x,
            edge(start.top(), finish.top()) - y,
            edge(start.right(), finish.right()) - x,
            edge(start.bottom(), finish.bottom()) - y };
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
        const std::pair<double, double>& interval)
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
        return impact;
    }
}

bool ProjectileTargetFlags::any_mover(std::span<CollisionObject* const> objects)
{
    return std::any_of(objects.begin(), objects.end(),
        [](const CollisionObject* object)
        {
            const Structure* structure = Collider::of(*object).structure();
            return structure != nullptr && structure->mover() != nullptr;
        });
}

bool ProjectileTargetFlags::any_fixed_slab(std::span<CollisionObject* const> objects)
{
    return std::any_of(objects.begin(), objects.end(), [](const CollisionObject* object)
        { return to_collision_type(object->tag()) == CollisionObjectType::structure_fixed_slab; });
}

void Projectile::resolve_launch_obstruction(const Vector2F& origin,
    std::span<CollisionObject* const> objects,
    std::optional<ProjectileTargetFlags> known)
{
    const RectangleF finish = this->shape()->bounding_box();
    RectangleF start = finish;
    start.set_position_at_center(origin);
    this->resolve_sweep(start, finish, objects, true,
        known ? std::optional<bool>(known->fixed_slabs) : std::nullopt);
}

void Projectile::resolve_movement_contacts(
    std::span<CollisionObject* const> objects,
    std::optional<ProjectileTargetFlags> known,
    std::span<Projectile* const> breakables)
{
    if (this->for_deletion_ || !this->first_update_done_) { return; }
    this->resolve_travel(objects, known, breakables);
    // Level supplies one complete target span, once, for this actual update.
    this->moving_contact_ready_ = false;
}

void Projectile::resolve_travel(std::span<CollisionObject* const> objects,
    std::optional<ProjectileTargetFlags> known, std::span<Projectile* const> breakables)
{
    if (!is_swept(this->type_)) { return; }
    // Asked only when the caller has no answer, and only for a swept kind.
    const bool translating = known ? known->movers :
        ProjectileTargetFlags::any_mover(objects);
    if (translating)
    {
        this->resolve_translating_contacts(this->previous_rectangle_,
            this->shape()->bounding_box(), objects, this->moving_contact_ready_, breakables);
    }
    else
    {
        // The complete original static path retains its float arithmetic.
        this->resolve_sweep(this->previous_rectangle_,
            this->shape()->bounding_box(), objects, false,
            known ? std::optional<bool>(known->fixed_slabs) : std::nullopt, breakables);
    }
}

bool Projectile::moving_paint_completed(HorizontalMotionId id) const
{
    return id != 0 && std::find(this->moving_paint_completed_.begin(),
        this->moving_paint_completed_.end(), id) != this->moving_paint_completed_.end();
}

void Projectile::resolve_translating_contacts(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects, bool moving_ready,
    std::span<Projectile* const> breakables)
{
    CollisionObject* obstruction = nullptr;
    TranslatingStructurePaintable* moving_obstruction = nullptr;
    HorizontalPaintImpact closest_impact;
    double closest_entry = 1.0;
    for (CollisionObject* object : objects)
    {
        if (object->for_deletion() || !layers_collide(this->layer(), this->mask(),
            object->layer(), object->mask())) { continue; }
        const bool structure = object->layer() == collision_layers::structure;
        Structure* surface = Collider::of(*object).structure();
        TranslatingStructurePaintable* moving = surface != nullptr ? surface->mover() : nullptr;
        if (moving != nullptr)
        {
            if (!moving_ready) { continue; }
            const HorizontalMotionStep step = moving->collision_step();
            const RectangleF anchor = moving->motion_state().spec.anchor;
            const bool vertical = moving->motion_state().spec.vertical();
            for (std::uint32_t index = 0; index < step.segment_count; ++index)
            {
                const HorizontalMotionSegment& segment = step.segments[index];
                const RelativeRectangle before = relative_rectangle_at(start, finish,
                    segment.begin, segment.from, vertical);
                const RelativeRectangle after = relative_rectangle_at(start, finish,
                    segment.end, segment.to, vertical);
                const auto interval = relative_sweep_interval(before, after, anchor);
                if (!interval) { continue; }
                const double entry = segment.begin +
                    (segment.end - segment.begin) * interval->first;
                if (entry < closest_entry || (entry == closest_entry && structure &&
                    obstruction != nullptr && obstruction->layer() != collision_layers::structure))
                {
                    obstruction = object;
                    moving_obstruction = moving;
                    closest_entry = entry;
                    closest_impact = relative_impact(before, after, anchor, *interval);
                }
            }
            continue;
        }
        if (const FixedSlabGeometry* slab = surface != nullptr ? surface->slab_geometry() : nullptr)
        {
            const auto hit = slab->sweep(start, finish);
            if (hit && hit->positive_overlap && (hit->entry < closest_entry ||
                (hit->entry == closest_entry && obstruction != nullptr &&
                    obstruction->layer() != collision_layers::structure)))
            {
                obstruction = object;
                moving_obstruction = nullptr;
                closest_entry = hit->entry;
            }
            continue;
        }
        const auto interval = shot_interval(start, finish,
            object->shape()->bounding_box(), this->sweep_radius());
        if (interval && (interval->first < closest_entry ||
            (interval->first == closest_entry && structure && obstruction != nullptr &&
                obstruction->layer() != collision_layers::structure)))
        {
            obstruction = object;
            moving_obstruction = nullptr;
            closest_entry = interval->first;
        }
    }
    // Planted subs stand still, so their world-frame entry is a fraction of
    // this tick like the movers' segment times.
    if (const auto breakable = first_breakable(*this, start, finish, breakables,
        this->sweep_radius());
        breakable && breakable_first(breakable->interval.first, closest_entry, obstruction))
    {
        this->strike_breakable(start, finish, *breakable->target, breakable->interval);
        return;
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
        Structure* surface = Collider::of(*object).structure();
        TranslatingStructurePaintable* moving = surface != nullptr ? surface->mover() : nullptr;
        if (moving == nullptr || object->for_deletion() ||
            !layers_collide(this->layer(), this->mask(), object->layer(), object->mask()) ||
            this->moving_paint_completed(moving->motion_id())) { continue; }
        const HorizontalMotionStep step = moving->collision_step();
        const RectangleF anchor = moving->motion_state().spec.anchor;
        const bool vertical = moving->motion_state().spec.vertical();
        std::optional<RelativeRectangle> footprint;
        for (std::uint32_t index = 0; index < step.segment_count; ++index)
        {
            const HorizontalMotionSegment& segment = step.segments[index];
            const RelativeRectangle before = relative_rectangle_at(volume, volume,
                segment.begin, segment.from, vertical);
            const RelativeRectangle after = relative_rectangle_at(volume, volume,
                segment.end, segment.to, vertical);
            const auto interval = relative_sweep_interval(before, after, anchor);
            if (!interval) { continue; }
            const RelativeRectangle entered = relative_between(before, after, interval->first);
            const RelativeRectangle exited = relative_between(before, after, interval->second);
            // The visited footprint grows along the mover's own axis.
            const RelativeRectangle visited = vertical ?
                RelativeRectangle{ entered.left, std::min(entered.top, exited.top),
                    entered.right, std::max(entered.bottom, exited.bottom) } :
                RelativeRectangle{ std::min(entered.left, exited.left), entered.top,
                    std::max(entered.right, exited.right), entered.bottom };
            if (!footprint) { footprint = visited; }
            else if (vertical)
            {
                footprint->top = std::min(footprint->top, visited.top);
                footprint->bottom = std::max(footprint->bottom, visited.bottom);
            }
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
    bool launch, std::optional<bool> fixed_slabs, std::span<Projectile* const> breakables)
{
    if (fixed_slabs ? *fixed_slabs : ProjectileTargetFlags::any_fixed_slab(objects))
    {
        this->resolve_fixed_slab_sweep(start, finish, objects, launch, breakables);
        return;
    }
    // A round shot built as a circle sweeps its disc (ROAD#44); every other
    // shot, and every round one by default, keeps the box arithmetic as is.
    const std::optional<float> radius = this->sweep_radius();
    CollisionObject* obstruction = nullptr;
    std::pair<float, float> closest{ 1.0f, 1.0f };
    for (CollisionObject* object : objects)
    {
        // Flying paint can hit an enemy between its owner and muzzle just
        // as it can hit cover there. A roller keeps its one-step volume:
        // enemy contacts belong to its ordinary multi-contact sweep, not an
        // extra launch hit. Masks retain ramp pass-through, friendly-fire
        // exclusion and dead players' empty collision mask.
        const bool structure = object->layer() == collision_layers::structure;
        if ((launch && !this->launch_hits_players() && !structure) ||
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
        const auto interval = shot_interval(start, finish, obstacle, radius);
        // Cover wins a tied entry time, including a launch already touching
        // both participants, independently of their order in the scene.
        if (interval && (interval->first < closest.first ||
            (interval->first == closest.first && structure &&
                obstruction != nullptr &&
                obstruction->layer() != collision_layers::structure)))
        {
            obstruction = object;
            closest = *interval;
        }
    }
    if (const auto breakable = first_breakable(*this, start, finish, breakables,
        this->sweep_radius());
        breakable && breakable_first(breakable->interval.first, closest.first, obstruction))
    {
        this->strike_breakable(start, finish, *breakable->target, breakable->interval);
        return;
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
    else if (radius)
    {
        // A disc that only grazed a corner in float arithmetic has no contact
        // to answer; it flies on to where its travel ends rather than stall.
        this->set_rectangle(finish);
    }
}

void Projectile::resolve_fixed_slab_sweep(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects, bool launch,
    std::span<Projectile* const> breakables)
{
    // Only scenes containing the new geometry enter this selection path.
    // The legacy rectangle participants still compute their original float
    // intervals; slab intervals retain double precision through ordering.
    CollisionObject* obstruction = nullptr;
    CollisionObject* winning_slab = nullptr;
    const FixedSlabGeometry* winning_geometry = nullptr;
    FixedSlabSweep winning_hit;
    double closest = 1.0;
    for (CollisionObject* object : objects)
    {
        const bool structure = object->layer() == collision_layers::structure;
        if ((launch && !this->launch_hits_players() && !structure) ||
            object->for_deletion() || !layers_collide(this->layer(), this->mask(), object->layer(), object->mask()))
        { continue; }
        const Structure* surface = Collider::of(*object).structure();
        const FixedSlabGeometry* slab = surface != nullptr ? surface->slab_geometry() : nullptr;
        std::optional<double> entry;
        std::optional<FixedSlabSweep> hit;
        if (slab != nullptr)
        {
            if (launch && launch_leaves_slab(start, finish, *slab)) { continue; }
            hit = slab->sweep(start, finish);
            if (hit && hit->positive_overlap) { entry = hit->entry; }
        }
        else
        {
            const RectangleF obstacle = object->shape()->bounding_box();
            if (launch && structure && launch_leaves_overlap(start, finish, obstacle)) { continue; }
            if (const auto interval = shot_interval(start, finish, obstacle, this->sweep_radius())) { entry = interval->first; }
        }
        if (entry && (*entry < closest || (*entry == closest && structure && obstruction != nullptr &&
            obstruction->layer() != collision_layers::structure)))
        {
            closest = *entry;
            obstruction = object;
            winning_slab = slab != nullptr ? object : nullptr;
            winning_geometry = slab;
            if (hit) { winning_hit = *hit; }
        }
    }
    if (const auto breakable = first_breakable(*this, start, finish, breakables,
        this->sweep_radius());
        breakable && breakable_first(breakable->interval.first, closest, obstruction))
    {
        this->strike_breakable(start, finish, *breakable->target, breakable->interval);
        return;
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
    if (!winning_geometry->overlaps(response))
    { response = at((winning_hit.entry + winning_hit.exit) / 2.0); }
    if (!winning_geometry->overlaps(response))
    { throw std::runtime_error("Fixed slab impact has no representable overlapping response rectangle"); }
    this->set_rectangle(response);
    const auto contact = narrow_phase(*winning_slab->shape(), *this->shape());
    const Vector2F normal = contact ? contact->normal : winning_hit.normal_to_rectangle;
    const float penetration = contact ? contact->penetration : 0.0f;
    winning_slab->on_contact(*this, normal, penetration);
    this->on_contact(*winning_slab, -normal, penetration);
}

void Projectile::strike_breakable(const RectangleF& start,
    const RectangleF& finish, Projectile& target, const std::pair<float, float>& interval)
{
    // The same face inset resolve_sweep gives cover, so a grenade struck
    // here bursts at the face it reached, not past it.
    const float edge_travel = std::max({
        std::abs(finish.left() - start.left()),
        std::abs(finish.right() - start.right()),
        std::abs(finish.top() - start.top()),
        std::abs(finish.bottom() - start.bottom()) });
    const float inset = edge_travel > 0.0f ? 0.01f / edge_travel : 0.0f;
    const float fraction = interval.first +
        std::min(inset, (interval.second - interval.first) / 2.0f);
    this->set_rectangle(rectangle_between(start, finish, fraction));
    // Spent on it, with no paint, damage or blocked-shot cue, as when Level
    // struck it after the tick's contacts.
    this->note_retire_cause(ShotRetireCause::breakable);
    this->set_for_deletion(true);
    target.take_hit();
}

void Projectile::on_structure_contact(const Structure& other,
    const Vector2F& normal, float /*penetration*/)
{
    this->geometry_contact_ |= other.layer() == collision_layers::structure;
    if (!this->for_deletion_ && other.layer() == collision_layers::structure)
    {
        // The cover says where on its face the shot stopped.
        const Vector2F center = this->shape()->bounding_box().center();
        this->blocked_contact_ = BlockedShotContact{
            other.cover_point(center, normal), -normal, this->team_colour_, this->player_num_ };
    }
    // Flying projectiles die on contact with flat geometry or enemy players.
    // Rollers override both to process every contact in their one-step volume.
    this->note_retire_cause(ShotRetireCause::cover);
    this->set_for_deletion(true);
}

void Projectile::on_player_contact(const Player& /*other*/,
    const Vector2F& /*normal*/, float /*penetration*/)
{
    this->note_retire_cause(ShotRetireCause::player);
    this->set_for_deletion(true);
}

void Projectile::note_retire_cause(ShotRetireCause cause)
{
    if (!this->for_deletion_ && this->retire_cause_ == ShotRetireCause::none)
    {
        this->retire_cause_ = cause;
    }
}

void Projectile::answer_paint_contact(const Projectile& toucher,
    bool painted, bool captured)
{
    const_cast<Projectile&>(toucher).note_paint_contact(painted, captured);
}

void Projectile::note_paint_contact(bool painted, bool captured)
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
        (this->geometry_contact_ && this->for_deletion() && this->duds_on_bare_cover()))
    {
        audio.contact(this->type_, this->player_num_,
            this->painted_contact_, this->captured_contact_);
    }
    this->geometry_contact_ = false;
    this->painted_contact_ = false;
    this->captured_contact_ = false;
}
