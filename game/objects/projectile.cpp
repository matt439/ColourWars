#include "game/pch.h"
#include "game/objects/projectile.h"

using namespace DirectX;
using namespace mattmath;

Projectile::Projectile(const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    projectile_type type,
    const float* dt,
    RenderResources* render_resources,
    const ProjectileDetails& details,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    MovingObject(velocity, rotation),
    AnimationObject(dt, details.sheet_name, details.animation_strip_name,
                    render_resources, color, rotation, origin, effects, layer_depth),
    details_(details),
    player_num_(player_num),
    team_colour_(team_colour),
    type_(type),
    team_(team),
    dt_(dt)
{

}

DiffusingProjectile::DiffusingProjectile(
    const Vector2F& velocity,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    projectile_type type,
    const float* dt,
    RenderResources* render_resources,
    const ProjectileDetails& details,
    const DiffusionDetails& diffusion_details,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    Projectile(velocity, team, player_num,
        team_colour, type, dt,
        render_resources, details, color, rotation,
        origin, effects, layer_depth),
    diffusion_details_(diffusion_details)
{

}
const Vector2F& DiffusingProjectile::get_base_size() const
{
	return this->get_details().size;
}

Vector2F DiffusingProjectile::calculate_diffusion_size() const
{
    const float time = this->get_timer();
    const DiffusionDetails& details = this->diffusion_details_;
    const Vector2F& base_size = this->get_base_size();
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

        multiple = Vector2F::ONE +
            Vector2F(details.end_scale.x * ratio,
			details.end_scale.y * ratio);
    }
    return base_size * multiple;
}



bool Projectile::is_matching_collision_object_type(
    const ICollisionGameObject* other) const
{
    CollisionObjectType other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == CollisionObjectType::structure ||
        other_type == CollisionObjectType::structure_paintable ||
        other_type == CollisionObjectType::structure_jump_through;

    if (structure_collision)
	{
		return true;
	}

    PlayerTeam team = this->get_team();

    switch (team)
    {
    case PlayerTeam::a:
        return other_type == CollisionObjectType::player_team_b;
    case PlayerTeam::b:
        return other_type == CollisionObjectType::player_team_a;
    default:
        throw std::exception("Invalid PlayerTeam value.");
    }
}
void Projectile::on_collision(const ICollisionGameObject* other)
{
    CollisionObjectType other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == CollisionObjectType::structure ||
        other_type == CollisionObjectType::structure_paintable ||
        other_type == CollisionObjectType::structure_jump_through;

    if (structure_collision)
    {
        this->set_for_deletion(true);
    }
    else
    {
        PlayerTeam team = this->get_team();
        switch (team)
        {
        case PlayerTeam::a:
            if (other_type == CollisionObjectType::player_team_b)
            {
                this->set_for_deletion(true);
            }
            break;
        case PlayerTeam::b:
            if (other_type == CollisionObjectType::player_team_a)
            {
                this->set_for_deletion(true);
            }
            break;
        default:
            throw std::exception("Invalid PlayerTeam value.");
        }
    }
}
CollisionObjectType Projectile::get_collision_object_type() const
{
    projectile_type type = this->get_type();
    switch (this->get_team())
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
PlayerTeam Projectile::get_team() const
{
    return this->team_;
}
bool Projectile::get_for_deletion() const
{
    return this->for_deletion_;
}
void Projectile::set_for_deletion(bool for_deletion)
{
    this->for_deletion_ = for_deletion;
}
int Projectile::get_player_num() const
{
    return this->player_num_;
}
const Colour& Projectile::get_team_colour() const
{
    return this->team_colour_;
}
projectile_type Projectile::get_type() const
{
    return this->type_;
}
float Projectile::get_timer() const
{
    return this->timer_;
}
void Projectile::alter_timer(float time)
{
    this->timer_ += time;
}
float Projectile::get_dt() const
{
	return *this->dt_;
}
void Projectile::update_movement(float gravity, float wind_resistance)
{
    const float dt = this->get_dt();

    //gravity
    MovingObject::alter_velocity_y(gravity * dt);
    if (MovingObject::get_velocity_y() > projectile_consts::MAX_VELOCITY.y)
	{
        MovingObject::set_velocity_y(projectile_consts::MAX_VELOCITY.y);
	}
	else if (MovingObject::get_velocity_y() < -projectile_consts::MAX_VELOCITY.y)
	{
        MovingObject::set_velocity_y(-projectile_consts::MAX_VELOCITY.y);
	}

    //wind resistance
    if (std::fabs(MovingObject::get_velocity_x()) > wind_resistance * 4.0f)
    {
        //right
        if (MovingObject::get_velocity_x() > 0.0f)
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
    if (MovingObject::get_velocity_x() > projectile_consts::MAX_VELOCITY.x)
    {
        MovingObject::set_velocity_x(projectile_consts::MAX_VELOCITY.x);
    }
    else if (MovingObject::get_velocity_x() < -projectile_consts::MAX_VELOCITY.x)
    {
        MovingObject::set_velocity_x(-projectile_consts::MAX_VELOCITY.x);
    }

    //displacement
    MovingObject::set_dx_x(MovingObject::get_velocity_x() * dt);
    MovingObject::set_dx_y(MovingObject::get_velocity_y() * dt);

    // Age first, then test: testing before the increment let a projectile
    // survive one frame past its deadline.
    this->alter_timer(dt);

    if (this->get_timer() > this->get_delete_timer())
	{
		this->set_for_deletion(true);
	}
}

float Projectile::get_delete_timer() const
{
	return this->get_details().delete_timer;
}

Vector2F Projectile::get_col_rect_size() const
{
	return this->get_details().size;
}

float Projectile::get_player_damage() const
{
	return this->get_details().player_damage;
}

const ProjectileDetails& Projectile::get_details() const
{
	return this->details_;
}