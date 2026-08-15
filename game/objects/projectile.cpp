#include "game/pch.h"
#include "game/objects/projectile.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

Projectile::Projectile(const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    projectile_type type,
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
    projectile_type type,
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
const Vector2F& DiffusingProjectile::base_size() const
{
	return this->details().size;
}

Vector2F DiffusingProjectile::calculate_diffusion_size() const
{
    const float time = this->timer();
    const DiffusionDetails& details = this->diffusion_details();
    const Vector2F& base_size = this->base_size();
    Vector2F multiple;
    if (time < details.start_time)
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

        // Interpolating between ONE and end_scale, not between ONE and
        // ONE + end_scale: the ramp has to arrive at exactly the value the
        // branch above returns at end_time, or the size steps down in one
        // frame as it crosses. That step is not only cosmetic - this size is
        // inflated into the rectangle that is both the sprite and shape(), so
        // the paint footprint jumps with it.
        multiple = Vector2F::ONE +
            (details.end_scale - Vector2F::ONE) * ratio;
    }
    return base_size * multiple;
}
void DiffusingProjectile::diffuse_to_size(RectangleF& rectangle) const
{
    const Vector2F size = this->calculate_diffusion_size();

    // Half the growth off each side keeps the centre where it was, which is
    // what a cloud spreading out does. Setting the size alone would pin the
    // top-left and walk the cloud down and right as it grew.
    rectangle.x -= (size.x - rectangle.width) / 2.0f;
    rectangle.y -= (size.y - rectangle.height) / 2.0f;
    rectangle.set_size(size);
}


void Projectile::on_contact(const CollisionObject& /*other*/,
    const Vector2F& /*normal*/, float /*penetration*/)
{
    // Every contact a projectile can have is fatal to it, so there is nothing
    // to ask. The mask admits flat level geometry and enemy players and
    // nothing else; the two type lists this replaces - one to decide whether
    // to test the pair, one to decide whether to die - were re-deriving that
    // filter twice, and had already disagreed about ramps.
    this->set_for_deletion(true);
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
    projectile_type type = this->type();
    switch (this->team())
    {
    case PlayerTeam::a:
        switch (type)
        {
        case SPRAY:
			return CollisionObjectType::projectile_spray_team_a;
        case MIST:
            return CollisionObjectType::projectile_mist_team_a;
        case JET:
            return CollisionObjectType::projectile_jet_team_a;
        case ROLLING:
            return CollisionObjectType::projectile_rolling_team_a;
        case BALL:
            return CollisionObjectType::projectile_ball_team_a;
        default:
            throw std::exception("Invalid projectile_type value.");
        }
    case PlayerTeam::b:
        switch (type)
        {
        case SPRAY:
            return CollisionObjectType::projectile_spray_team_b;
        case MIST:
            return CollisionObjectType::projectile_mist_team_b;
        case JET:
            return CollisionObjectType::projectile_jet_team_b;
        case ROLLING:
            return CollisionObjectType::projectile_rolling_team_b;
        case BALL:
            return CollisionObjectType::projectile_ball_team_b;
        default:
            throw std::exception("Invalid projectile_type value.");
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
projectile_type Projectile::type() const
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

    const Vector2F& max_velocity = this->max_velocity();

    //gravity
    MovingObject::alter_velocity_y(gravity * dt);
    if (MovingObject::velocity_y() > max_velocity.y)
	{
        MovingObject::set_velocity_y(max_velocity.y);
	}
	else if (MovingObject::velocity_y() < -max_velocity.y)
	{
        MovingObject::set_velocity_y(-max_velocity.y);
	}

    //wind resistance
    if (std::fabs(MovingObject::velocity_x()) > wind_resistance * 4.0f)
    {
        //right
        if (MovingObject::velocity_x() > 0.0f)
        {
            MovingObject::alter_velocity_x(-wind_resistance * dt);
        }
        //left
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

    // Age first, then test: testing before the increment let a projectile
    // survive one frame past its deadline.
    this->alter_timer(dt);

    // Never on the first update, though, or a projectile whose tuned lifetime
    // is shorter than one step dies without ever being tested against
    // anything. Scene::add is deferred: an object created during tick N only
    // joins the scene's collidables at that tick's end_tick(), so the first
    // update it ever gets is tick N+1's - and Scene::update runs before
    // Scene::resolve, whose find_contacts and dispatch_contacts both skip
    // objects already flagged for deletion. Flagging here would retire it at
    // the end of N+1 having touched nothing. The roller is exactly that case:
    // its delete_timer is a millionth of a second because it wants to be a
    // one-step melee volume, and it was painting nothing at all. Surviving one
    // collision sweep is the minimum a projectile is worth spawning for.
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