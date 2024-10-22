#include "pch.h"
#include "Weapon.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;

Weapon::Weapon(const WeaponDetails& details,
    player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    TextureObject(details.sheet_name, details.frame_name, sprite_batch, resource_manager,
                  color, rotation, origin, effects, layer_depth),
        _details(details),
        _dt(dt),
        _sprite_batch(sprite_batch),
        _resource_manager(resource_manager),
        _team(team),
        _player_num(player_num),
        _team_colour(team_colour),
        _type(type),
        _player_center(player_center)
{
    this->_proj_builder = std::make_unique<ProjectileBuilder>();
    this->_sound_bank = resource_manager->get_sound_bank(details.sound_bank_name);
}

void Weapon::draw(const Camera& camera, bool debug)
{
    //draw weapon
    Vector2F draw_pos = this->get_draw_pos();
    auto draw_rectangle = RectangleF(draw_pos, this->get_details().size);

    Vector2F origin = this->calculate_sprite_origin(
        this->get_details().size, rotation_origin::LEFT_CENTER);

    SpriteEffects effects = SpriteEffects_None;
    bool invert_y = this->get_invert_y();
    bool invert_x = this->get_invert_x();
    if (invert_y && invert_x)
    {
        effects = SpriteEffects_FlipBoth;
    }
    else if (invert_y)
    {
        effects = SpriteEffects_FlipVertically;
    }
    else if (invert_x)
    {
        effects = SpriteEffects_FlipHorizontally;
    }

    TextureObject::set_element_name(this->get_details().frame_name);
    DrawObject::set_origin(origin);
    DrawObject::set_effects(effects);
    DrawObject::set_draw_rotation(this->get_rotation());

    TextureObject::draw(draw_rectangle, camera);

    if (debug)
    {
        // draw nozzle
        TextureObject::set_element_name("nozzle");
        RectangleF draw_rectangle_noz = this->get_nozzle_rectangle();
        Vector2F origin_noz = this->calculate_sprite_origin(
            this->get_nozzle_size(), rotation_origin::CENTER);

        TextureObject::set_origin(origin_noz);
        TextureObject::set_effects(SpriteEffects_None);
        TextureObject::set_draw_rotation(0.0f);

        TextureObject::draw(draw_rectangle_noz, camera);
    }
}
void Weapon::draw(bool debug)
{
    this->draw(Camera::DEFAULT_CAMERA, debug);
}

Vector2F Weapon::calculate_sprite_origin(const Vector2F& size, rotation_origin origin)
{
    switch (origin)
    {
    case rotation_origin::CENTER:
        return Vector2F(size) / 2.0f;
    case rotation_origin::LEFT_CENTER:
        return {0.0f, size.y / 2.0f};
    case rotation_origin::TOP_LEFT:
        return Vector2F::ZERO;
    default:
        return Vector2F::ZERO;
    }
}

RectangleF Weapon::get_nozzle_rectangle() const
{
	Vector2F nozzle_pos = this->get_nozzle_pos();

    return { nozzle_pos, NOZZLE_SIZE };
}

Vector2F Weapon::get_nozzle_size()
{
	return NOZZLE_SIZE;
}




Vector2F Weapon::get_draw_pos() const
{
	Vector2F result = this->get_player_center();
	if (this->facing_left(this->_rotation))
	{
		result.x -= this->_details.offset.x;
		result.y += this->_details.offset.y;
	}
	//gun facing right
	else
	{
		result += this->_details.offset;
	}
	return result;
}

bool Weapon::facing_left(float rotation)
{
	//gun facing left
	if (rotation > PI / 2.0f ||
		rotation < PI / -2.0f)
	{
		return true;
	}
	//gun facing right
	return false;
}

Vector2F Weapon::get_nozzle_pos() const
{
	Vector2F result = this->_details.nozzle_offset;
	result.x += this->_details.size.x;
	result = Vector2F::rotate_vector(result, this->_rotation);
	result += this->get_wep_rotation_origin_offset(
		facing_left(this->_rotation));
	result += this->get_player_center();
	return result;
}

Vector2F Weapon::get_wep_rotation_origin_offset(bool facing_left) const
{
	if (facing_left)
	{
		return Vector2F(-this->_details.offset.x,
			this->_details.offset.y);
	}
	// gun facing right
	return Vector2F(this->_details.offset.x, this->_details.offset.y);
}

std::vector<std::unique_ptr<ICollisionGameObject>>
    Weapon::update_and_get_projectiles(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right)
{
    this->update_movement_and_rotation(input, player_center,
		player_velocity, player_facing_right);

    if (this->check_if_shooting_and_ammo_update(input, player_center,
        player_velocity))
    {
        return this->shoot(Vector2F::unit_vec_from_angle(this->get_rotation()));
	}
	else
	{
		return std::vector<std::unique_ptr<ICollisionGameObject>>();
    }
}

bool Weapon::check_if_shooting_and_ammo_update(
    PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity)
{
    const float dt = *this->_dt;
    bool normal_gun_ok_to_shoot = input.primary_shoot &&
        this->get_gun_player_aligned();

    bool ok_to_shoot = normal_gun_ok_to_shoot;


    bool result = false;

    if (this->get_shoot_timer() > this->get_shoot_interval() &&
        this->get_ammo() > 0.0f &&
        ok_to_shoot)
    {
        result = true;

        this->set_shoot_timer(0.0f);
        this->alter_ammo(-this->get_ammo_usage());
        if (this->get_ammo() < 0.0f)
        {
            this->set_ammo(0.0f);
        }
        this->set_ammo_timer(0.0f);
    }

    //ammo regen
    if (this->get_ammo_timer() >= AMMO_REGEN_DELAY)
    {
        this->alter_ammo(weapon_consts::AMMO_REGEN_RATE * dt);
        if (this->get_ammo() > 1.0f)
        {
            this->set_ammo(1.0f);
        }
    }
    this->alter_ammo_timer(dt);
    this->alter_shoot_timer(dt);

    this->_shooting_this_update = ok_to_shoot && this->get_ammo() > 0.0f;

    this->handle_shoot_sound(result, this->_shooting_this_update);

    return result;

}
void Weapon::handle_shoot_sound(bool shooting_this_update, bool holding_shoot)
{
    const std::string& sound_name = this->get_sound_effect_instance_name();
    
    if (holding_shoot || shooting_this_update)
    {
        this->_sound_bank->play_effect(
            sound_name,
            true,
            this->_details.shoot_sound_volume);
    }
    else
    {
        this->_sound_bank->stop_effect(sound_name, true);
    }
}
void Weapon::stop_sounds() const
{
    const std::string& sound_name = this->get_sound_effect_instance_name();
    
    this->_sound_bank->stop_effect(sound_name, true);
}
const std::string& Weapon::get_sound_effect_instance_name() const
{
    player_team team = this->get_team();
    int player_num = this->get_player_num();

    wep_type type = this->get_type();
    switch (type)
    {
    case wep_type::SPRAYER:
        return SPRAYER_SOUND_DETAILS.get_sound_name(team, player_num);
    case wep_type::ROLLER:
        return ROLLER_SOUND_DETAILS.get_sound_name(team, player_num);
    case wep_type::MISTER:
        return MISTER_SOUND_DETAILS.get_sound_name(team, player_num);
    default:
       throw std::exception("Weapon::get_sound_effect_instance_name() - "
		   "invalid weapon type");
    }

}
void Weapon::update_movement_and_rotation(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right)
{
    if (input.shoot_direction_requested)
    {
        this->set_rotation(input.shoot_angle);
    }
    else
    {
		if (player_facing_right)
		{
			this->set_rotation(0.0f);
		}
		else
		{
			this->set_rotation(PI);
		}
    }

    this->set_player_center(player_center);

    //gun facing left
    if (facing_left(this->get_rotation()))
    {
        this->set_invert_y(true);
    }
    //gun facing right
    else
    {
        this->set_invert_y(false);
    }
}

std::vector<std::unique_ptr<ICollisionGameObject>>
Weapon::shoot(const Vector2F& shoot_direction) const
{
    Vector2F launch_velocity = this->calculate_projectile_launch_velocity(
        shoot_direction, this->get_starting_vel_length());

    return this->_proj_builder->build_projectiles(
        this->get_nozzle_pos(),
        launch_velocity,
        this->get_team(),
        this->get_player_num(),
        this->get_team_colour(),
        this->get_details().proj_type,
        this->_dt,
        this->_sprite_batch,
        this->_resource_manager);
}

Vector2F Weapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity) const
{
    Vector2F result = shoot_direction * starting_velocity;
    return result;
}

RelativeVelocityWeapon::RelativeVelocityWeapon(
    const WeaponDetails& details,
    RelativeWeaponDetails rel_details,
    player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    DirectX::SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    DirectX::SpriteEffects effects,
    float layer_depth) :
    Weapon(details, team, player_num, team_colour, type,
        player_center, sprite_batch, resource_manager, dt,
        color, rotation, origin, effects, layer_depth),
    _rel_details(rel_details)
{

}

Vector2F RelativeVelocityWeapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity,
    const Vector2F& player_velocity,
    add_player_velocity add_player_vel,
    float player_vel_amount) const
{
    Vector2F result = shoot_direction * starting_velocity;

    if (add_player_vel == add_player_velocity::X_AND_Y)
    {
        result += player_velocity * player_vel_amount;
    }
    else if (add_player_vel == add_player_velocity::X_ONLY)
    {
        result.x += player_velocity.x * player_vel_amount;
    }
    else //if (add_player_vel == add_player_velocity::Y_ONLY)
    {
        result.y += player_velocity.y * player_vel_amount;
    }
    return result;
}

std::vector<std::unique_ptr<ICollisionGameObject>> RelativeVelocityWeapon::shoot(
    const MattMath::Vector2F& shoot_direction,
    const MattMath::Vector2F& player_velocity) const
{
    RelativeWeaponDetails rel_details = this->_rel_details;
    
    add_player_velocity add_player_vel = rel_details.add_vel;

    Vector2F launch_velocity = this->calculate_projectile_launch_velocity(
        shoot_direction, this->get_starting_vel_length(),
        player_velocity, add_player_vel,
        rel_details.player_vel_amount);
    
    return this->get_projectile_builder()->build_projectiles(
        this->get_nozzle_pos(),
        launch_velocity,
        this->get_team(),
        this->get_player_num(),
        this->get_team_colour(),
        this->get_details().proj_type,
        this->get_dt_ptr(),
        this->get_sprite_batch(),
        this->get_resource_manager());
}

std::vector<std::unique_ptr<ICollisionGameObject>>
    RelativeVelocityWeapon::update_and_get_projectiles(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right)
{
    Weapon::update_movement_and_rotation(input, player_center,
        player_velocity, player_facing_right);

    if (Weapon::check_if_shooting_and_ammo_update(input, player_center,
		player_velocity))
    {
        return this->shoot(
            Vector2F::unit_vec_from_angle(this->get_rotation()), player_velocity);
	}
    else
    {
        return std::vector<std::unique_ptr<ICollisionGameObject>>();
    }
}
