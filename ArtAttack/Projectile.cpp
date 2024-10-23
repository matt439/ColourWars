#include "pch.h"
#include "Projectile.h"

using namespace DirectX;
using namespace MattMath;

Projectile::Projectile(const Vector2F& velocity,
    player_team team,
    int player_num,
    const Colour& team_colour,
    projectile_type type,
    const float* dt,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const ProjectileDetails& details,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    MovingObject(velocity, rotation),
    AnimationObject(dt, details.sheet_name, details.animation_strip_name, sprite_batch,
                    resource_manager, color, rotation, origin, effects, layer_depth),
    _details(details),
    _player_num(player_num),
    _team_colour(team_colour),
    _type(type),
    _team(team),
    _dt(dt)
{

}

DiffusingProjectile::DiffusingProjectile(
    const Vector2F& velocity,
    player_team team,
    int player_num,
    const Colour& team_colour,
    projectile_type type,
    const float* dt,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const ProjectileDetails& details,
    const DiffusionDetails& diffusion_details,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    Projectile(velocity, team, player_num,
        team_colour, type, dt, sprite_batch,
        resource_manager, details, color, rotation,
        origin, effects, layer_depth),
    _diffusion_details(diffusion_details)
{

}
const Vector2F& DiffusingProjectile::get_base_size() const
{
	return this->get_details().size;
}

Vector2F DiffusingProjectile::calculate_diffusion_size() const
{
    const float time = this->get_timer();
    const DiffusionDetails& details = this->_diffusion_details;
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
    collision_object_type other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == collision_object_type::STRUCTURE ||
        other_type == collision_object_type::STRUCTURE_PAINTABLE ||
        other_type == collision_object_type::STRUCTURE_JUMP_THROUGH;

    if (structure_collision)
	{
		return true;
	}

    player_team team = this->get_team();

    switch (team)
    {
    case player_team::A:
        return other_type == collision_object_type::PLAYER_TEAM_B;
    case player_team::B:
        return other_type == collision_object_type::PLAYER_TEAM_A;
    default:
        throw std::exception("Invalid player_team value.");
    }
}
void Projectile::on_collision(const ICollisionGameObject* other)
{
    collision_object_type other_type = other->get_collision_object_type();

    bool structure_collision =
        other_type == collision_object_type::STRUCTURE ||
        other_type == collision_object_type::STRUCTURE_PAINTABLE ||
        other_type == collision_object_type::STRUCTURE_JUMP_THROUGH;

    if (structure_collision)
    {
        this->set_for_deletion(true);
    }
    else
    {
        player_team team = this->get_team();
        switch (team)
        {
        case player_team::A:
            if (other_type == collision_object_type::PLAYER_TEAM_B)
            {
                this->set_for_deletion(true);
            }
            break;
        case player_team::B:
            if (other_type == collision_object_type::PLAYER_TEAM_A)
            {
                this->set_for_deletion(true);
            }
            break;
        default:
            throw std::exception("Invalid player_team value.");
        }
    }
}
collision_object_type Projectile::get_collision_object_type() const
{
    projectile_type type = this->get_type();
    switch (this->get_team())
    {
    case player_team::A:
        switch (type)
        {
        case SPRAY:
			return collision_object_type::PROJECTILE_SPRAY_TEAM_A;
        case MIST:
            return collision_object_type::PROJECTILE_MIST_TEAM_A;
        case JET:
            return collision_object_type::PROJECTILE_JET_TEAM_A;
        case ROLLING:
            return collision_object_type::PROJECTILE_ROLLING_TEAM_A;
        case BALL:
            return collision_object_type::PROJECTILE_BALL_TEAM_A;
        default:
            throw std::exception("Invalid projectile_type value.");
        }
    case player_team::B:
        switch (type)
        {
        case SPRAY:
            return collision_object_type::PROJECTILE_SPRAY_TEAM_B;
        case MIST:
            return collision_object_type::PROJECTILE_MIST_TEAM_B;
        case JET:
            return collision_object_type::PROJECTILE_JET_TEAM_B;
        case ROLLING:
            return collision_object_type::PROJECTILE_ROLLING_TEAM_B;
        case BALL:
            return collision_object_type::PROJECTILE_BALL_TEAM_B;
        default:
            throw std::exception("Invalid projectile_type value.");
        }
    default:
        throw std::exception("Invalid player_team value.");
    }
}
player_team Projectile::get_team() const
{
    return this->_team;
}
bool Projectile::get_for_deletion() const
{
    return this->_for_deletion;
}
void Projectile::set_for_deletion(bool for_deletion)
{
    this->_for_deletion = for_deletion;
}
int Projectile::get_player_num() const
{
    return this->_player_num;
}
const Colour& Projectile::get_team_colour() const
{
    return this->_team_colour;
}
projectile_type Projectile::get_type() const
{
    return this->_type;
}
float Projectile::get_timer() const
{
    return this->_timer;
}
void Projectile::alter_timer(float time)
{
    this->_timer += time;
}
float Projectile::get_dt() const
{
	return *this->_dt;
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

    if (this->get_timer() > this->get_delete_timer())
	{
		this->set_for_deletion(true);
	}
    this->alter_timer(dt);
}

float Projectile::get_delete_timer() const
{
	return this->_timer;
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
	return this->_details;
}