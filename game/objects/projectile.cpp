#include "game/pch.h"
#include "game/objects/projectile.h"

using namespace mattmath;
using namespace labrador;

RenderResources::SheetStrip resolve_projectile_strip(
    const ProjectileTuning& tuning, const RenderResources& render_resources)
{
    return render_resources.resolve_sheet_strip(tuning.details.sheet_name,
        tuning.details.animation_strip_name);
}

Projectile::Projectile(const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    ProjectileType type,
    RenderResources* render_resources,
    RenderResources::SheetStrip strip,
    const ProjectileTuning* tuning,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    colourwars::MovingObject(velocity, rotation),
    colourwars::AnimationObject(strip, render_resources, color, rotation, origin, flip, layer_depth),
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
    RenderResources::SheetStrip strip,
    const ProjectileTuning* tuning,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    Projectile(velocity, team, player_num,
        team_colour, type,
        render_resources, strip, tuning, color, rotation,
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
void Projectile::touch(Collider& receiver, const Vector2F& normal, float penetration) const
{
    receiver.on_projectile_contact(*this, normal, penetration);
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
    if (dt > 0.0f)
    {
        this->moving_contact_ready_ = true;
        this->moving_paint_completed_.clear();
    }
    this->previous_rectangle_ = this->shape()->bounding_box();

    const Vector2F& max_velocity = this->max_velocity();

    colourwars::MovingObject::alter_velocity_y(gravity * dt);
    if (colourwars::MovingObject::velocity_y() > max_velocity.y)
	{
        colourwars::MovingObject::set_velocity_y(max_velocity.y);
	}
	else if (colourwars::MovingObject::velocity_y() < -max_velocity.y)
	{
        colourwars::MovingObject::set_velocity_y(-max_velocity.y);
	}

    if (std::fabs(colourwars::MovingObject::velocity_x()) > wind_resistance * this->tuning().wind_stop_factor)
    {
        if (colourwars::MovingObject::velocity_x() > 0.0f)
        {
            colourwars::MovingObject::alter_velocity_x(-wind_resistance * dt);
        }
        else
        {
            colourwars::MovingObject::alter_velocity_x(wind_resistance * dt);
        }
    }
	else
	{
        colourwars::MovingObject::set_velocity_x(0.0f);
	}
    if (colourwars::MovingObject::velocity_x() > max_velocity.x)
    {
        colourwars::MovingObject::set_velocity_x(max_velocity.x);
    }
    else if (colourwars::MovingObject::velocity_x() < -max_velocity.x)
    {
        colourwars::MovingObject::set_velocity_x(-max_velocity.x);
    }

    const Vector2F displacement = colourwars::MovingObject::velocity() * dt;

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
