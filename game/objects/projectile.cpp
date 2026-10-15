#include "game/pch.h"
#include "game/objects/projectile.h"
#include "game/objects/paint_audio.h"
#include "engine/collision/narrow_phase.h"

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

    RectangleF rectangle_between(const RectangleF& start,
        const RectangleF& finish, float fraction)
    {
        return RectangleF(
            start.x + (finish.x - start.x) * fraction,
            start.y + (finish.y - start.y) * fraction,
            start.width + (finish.width - start.width) * fraction,
            start.height + (finish.height - start.height) * fraction);
    }
}

Projectile::Projectile(const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    ProjectileType type,
    RenderResources* render_resources,
    const ProjectileTuning* tuning,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    MovingObject(velocity, rotation),
    AnimationObject(tuning->details.sheet_name,
                    tuning->details.animation_strip_name,
                    render_resources, color, rotation, origin, flip, layer_depth),
    tuning_(tuning),
    player_num_(player_num),
    team_colour_(team_colour),
    type_(type),
    team_(team)
{

}

DiffusingProjectile::DiffusingProjectile(
    const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    ProjectileType type,
    RenderResources* render_resources,
    const ProjectileTuning* tuning,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    Projectile(velocity, team, player_num,
        team_colour, type,
        render_resources, tuning, color, rotation,
        origin, flip, layer_depth)
{

}
const DiffusionDetails& DiffusingProjectile::diffusion_details() const
{
    return this->tuning().diffusion;
}
Vector2F DiffusingProjectile::calculate_diffusion_scale() const
{
    const float time = this->timer();
    const DiffusionDetails& details = this->diffusion_details();
    Vector2F multiple;
    if (details.type == DiffusionType::no_diffusion ||
        time < details.start_time)
    {
        multiple = Vector2F::ONE;
    }
    else if (time >= details.end_time)
    {
        multiple = details.end_scale;
    }
    else
    {
        float diff_length = details.end_time -
            details.start_time;
        float diff_time = time -
            details.start_time;

        float ratio = diff_time / diff_length;

        // Interpolate to end_scale exactly so the sprite and any scaled collider
        // remain continuous at end_time.
        multiple = Vector2F::ONE +
            (details.end_scale - Vector2F::ONE) * ratio;
    }
    return multiple;
}
RectangleF DiffusingProjectile::sprite_rectangle(
    const RectangleF& collision_rectangle) const
{
    const Vector2F size = this->details().size *
        this->calculate_diffusion_scale();
    return RectangleF(collision_rectangle.center() - size / 2.0f, size);
}
void DiffusingProjectile::diffuse_collision_rectangle(RectangleF& rectangle) const
{
    if (this->diffusion_details().type != DiffusionType::linear_size_and_col_rect)
    {
        return;
    }

    const Vector2F size = this->details().col_rect_size *
        this->calculate_diffusion_scale();

    // Split growth between both sides to preserve the cloud's centre.
    rectangle.x -= (size.x - rectangle.width) / 2.0f;
    rectangle.y -= (size.y - rectangle.height) / 2.0f;
    rectangle.set_size(size);
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
    if (this->type_ == ProjectileType::rolling || this->for_deletion_ ||
        !this->first_update_done_)
    {
        return;
    }
    this->resolve_sweep(this->previous_rectangle_,
        this->shape()->bounding_box(), objects, false);
}

void Projectile::resolve_sweep(const RectangleF& start,
    const RectangleF& finish, std::span<CollisionObject* const> objects,
    bool launch)
{
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
        if (std::abs(normal.x) >= std::abs(normal.y))
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
CollisionLayer Projectile::layer() const
{
    return collision_layer(this->collision_type());
}
CollisionMask Projectile::mask() const
{
    return collision_mask(this->collision_type());
}
CollisionTag Projectile::tag() const
{
    return to_collision_tag(this->collision_type());
}
CollisionObjectType Projectile::collision_type() const
{
    ProjectileType type = this->type();
    switch (this->team())
    {
    case PlayerTeam::a:
        switch (type)
        {
        case ProjectileType::spray:
			return CollisionObjectType::projectile_spray_team_a;
        case ProjectileType::mist:
            return CollisionObjectType::projectile_mist_team_a;
        case ProjectileType::jet:
            return CollisionObjectType::projectile_jet_team_a;
        case ProjectileType::rolling:
            return CollisionObjectType::projectile_rolling_team_a;
        case ProjectileType::ball:
            return CollisionObjectType::projectile_ball_team_a;
        default:
            throw std::exception("Invalid ProjectileType value.");
        }
    case PlayerTeam::b:
        switch (type)
        {
        case ProjectileType::spray:
            return CollisionObjectType::projectile_spray_team_b;
        case ProjectileType::mist:
            return CollisionObjectType::projectile_mist_team_b;
        case ProjectileType::jet:
            return CollisionObjectType::projectile_jet_team_b;
        case ProjectileType::rolling:
            return CollisionObjectType::projectile_rolling_team_b;
        case ProjectileType::ball:
            return CollisionObjectType::projectile_ball_team_b;
        default:
            throw std::exception("Invalid ProjectileType value.");
        }
    default:
        throw std::exception("Invalid PlayerTeam value.");
    }
}
PlayerTeam Projectile::team() const
{
    return this->team_;
}
bool Projectile::for_deletion() const
{
    return this->for_deletion_;
}
void Projectile::set_for_deletion(bool for_deletion)
{
    this->for_deletion_ = for_deletion;
}
int Projectile::player_num() const
{
    return this->player_num_;
}
const Colour& Projectile::team_colour() const
{
    return this->team_colour_;
}
ProjectileType Projectile::type() const
{
    return this->type_;
}
float Projectile::timer() const
{
    return this->timer_;
}
void Projectile::alter_timer(float time)
{
    this->timer_ += time;
}
Vector2F Projectile::update_movement(float gravity, float wind_resistance, float dt)
{
    this->previous_rectangle_ = this->shape()->bounding_box();

    const Vector2F& max_velocity = this->max_velocity();

    MovingObject::alter_velocity_y(gravity * dt);
    if (MovingObject::velocity_y() > max_velocity.y)
	{
        MovingObject::set_velocity_y(max_velocity.y);
	}
	else if (MovingObject::velocity_y() < -max_velocity.y)
	{
        MovingObject::set_velocity_y(-max_velocity.y);
	}

    if (std::fabs(MovingObject::velocity_x()) > wind_resistance * 4.0f)
    {
        if (MovingObject::velocity_x() > 0.0f)
        {
            MovingObject::alter_velocity_x(-wind_resistance * dt);
        }
        else
        {
            MovingObject::alter_velocity_x(wind_resistance * dt);
        }
    }
	else
	{
        MovingObject::set_velocity_x(0.0f);
	}
    if (MovingObject::velocity_x() > max_velocity.x)
    {
        MovingObject::set_velocity_x(max_velocity.x);
    }
    else if (MovingObject::velocity_x() < -max_velocity.x)
    {
        MovingObject::set_velocity_x(-max_velocity.x);
    }

    const Vector2F displacement = MovingObject::velocity() * dt;

    // Advance age before testing the lifetime.
    this->alter_timer(dt);

    // Allow the first collision sweep even when lifetime is shorter than one step.
    // Scene admits a newly spawned projectile at end_tick; its first update runs
    // before its first resolve, which skips objects marked for deletion.
    // The roller relies on this to act as a one-step contact volume.
    if (!this->first_update_done_)
    {
        this->first_update_done_ = true;
    }
    else if (this->timer() > this->delete_timer())
    {
        this->set_for_deletion(true);
    }

    return displacement;
}

float Projectile::delete_timer() const
{
	return this->details().delete_timer;
}

const ProjectileTuning& Projectile::tuning() const
{
	return *this->tuning_;
}

const ProjectileDetails& Projectile::details() const
{
	return this->tuning_->details;
}

const Vector2F& Projectile::max_velocity() const
{
	return this->tuning_->max_velocity;
}
