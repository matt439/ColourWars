#include "game/pch.h"
#include "game/objects/weapon.h"

using namespace DirectX;
using namespace mattmath;
using namespace weapon_consts;
using namespace artattack;

Weapon::Weapon(const WeaponDetails& details,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    TextureObject(details.sheet_name, details.frame_name, render_resources,
                  color, rotation, origin, effects, layer_depth),
        details_(details),
        dt_(dt),
        render_resources_(render_resources),
        team_(team),
        player_num_(player_num),
        team_colour_(team_colour),
        type_(type),
        player_center_(player_center)
{
    this->proj_builder_ = std::make_unique<ProjectileBuilder>();
    this->sound_bank_ = audio_resources->sound_bank(details.sound_bank_name);
    this->loop_sound_ = resolve_loop_sound(*this->sound_bank_, type, team,
        player_num);
    this->shoot_sound_ = this->sound_bank_->resolve_wave(
        details.shoot_sound_name);
    this->nozzle_frame_ = this->sprite_sheet()->
        resolve_sprite_frame(NOZZLE_FRAME);
}

void Weapon::draw(SpriteBatch* sprite_batch, const Camera& camera, bool debug) const
{
    //draw weapon
    Vector2F draw_pos = this->draw_pos();
    auto draw_rectangle = RectangleF(draw_pos, this->details().size);

    Vector2F origin = this->calculate_sprite_origin(
        this->details().size, RotationOrigin::left_center);

    SpriteEffects effects = SpriteEffects_None;
    bool invert_y = this->invert_y();
    bool invert_x = this->invert_x();
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

    // Pure read: every per-draw value is a local passed to draw_with, not a
    // member assigned first. draw() is entered by every render worker on the
    // same Weapon at once, and back when the element was a std::string,
    // assigning it from several threads was heap corruption, not just a torn
    // frame.
    TextureObject::draw_with(sprite_batch, draw_rectangle, camera,
        this->frame(), this->draw_colour(),
        origin, effects, this->rotation());

    if (debug)
    {
        // draw nozzle
        RectangleF draw_rectangle_noz = this->nozzle_rectangle();
        Vector2F origin_noz = this->calculate_sprite_origin(
            this->nozzle_size(), RotationOrigin::center);

        TextureObject::draw_with(sprite_batch, draw_rectangle_noz, camera,
            this->nozzle_frame_, this->draw_colour(),
            origin_noz, SpriteEffects_None, 0.0f);
    }
}

mattmath::Colour Weapon::draw_colour() const
{
    return this->colour();
}
void Weapon::draw(SpriteBatch* sprite_batch, bool debug) const
{
    this->draw(sprite_batch, Camera::DEFAULT_CAMERA, debug);
}

Vector2F Weapon::calculate_sprite_origin(const Vector2F& size, RotationOrigin origin)
{
    switch (origin)
    {
    case RotationOrigin::center:
        return Vector2F(size) / 2.0f;
    case RotationOrigin::left_center:
        return {0.0f, size.y / 2.0f};
    case RotationOrigin::top_left:
        return Vector2F::ZERO;
    default:
        return Vector2F::ZERO;
    }
}

RectangleF Weapon::nozzle_rectangle() const
{
	Vector2F nozzle_pos = this->nozzle_pos();

    return { nozzle_pos, NOZZLE_SIZE };
}

Vector2F Weapon::nozzle_size()
{
	return NOZZLE_SIZE;
}




Vector2F Weapon::draw_pos() const
{
	Vector2F result = this->player_center();
	if (this->facing_left(this->rotation_))
	{
		result.x -= this->details_.offset.x;
		result.y += this->details_.offset.y;
	}
	//gun facing right
	else
	{
		result += this->details_.offset;
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

Vector2F Weapon::nozzle_pos() const
{
	Vector2F result = this->details_.nozzle_offset;
	result.x += this->details_.size.x;
	result = Vector2F::rotate_vector(result, this->rotation_);
	result += this->wep_rotation_origin_offset(
		facing_left(this->rotation_));
	result += this->player_center();
	return result;
}

Vector2F Weapon::wep_rotation_origin_offset(bool facing_left) const
{
	if (facing_left)
	{
		return Vector2F(-this->details_.offset.x,
			this->details_.offset.y);
	}
	// gun facing right
	return Vector2F(this->details_.offset.x, this->details_.offset.y);
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
        return this->shoot(Vector2F::unit_vec_from_angle(this->rotation()));
	}
	else
	{
		return std::vector<std::unique_ptr<ICollisionGameObject>>();
    }
}

bool Weapon::check_if_shooting_and_ammo_update(
    PlayerInputData input,
    const Vector2F& /*player_center*/,
    const Vector2F& /*player_velocity*/)
{
    const float dt = *this->dt_;
    bool normal_gun_ok_to_shoot = input.primary_shoot &&
        this->gun_player_aligned();

    bool ok_to_shoot = normal_gun_ok_to_shoot;


    bool result = false;

    if (this->shoot_timer() > this->shoot_interval() &&
        this->ammo() > 0.0f &&
        ok_to_shoot)
    {
        result = true;

        this->set_shoot_timer(0.0f);
        this->alter_ammo(-this->ammo_usage());
        if (this->ammo() < 0.0f)
        {
            this->set_ammo(0.0f);
        }
        this->set_ammo_timer(0.0f);
    }

    //ammo regen
    if (this->ammo_timer() >= AMMO_REGEN_DELAY)
    {
        this->alter_ammo(weapon_consts::AMMO_REGEN_RATE * dt);
        if (this->ammo() > 1.0f)
        {
            this->set_ammo(1.0f);
        }
    }
    this->alter_ammo_timer(dt);
    this->alter_shoot_timer(dt);

    this->shooting_this_update_ = ok_to_shoot && this->ammo() > 0.0f;

    this->handle_shoot_sound(result, this->shooting_this_update_);

    return result;

}
void Weapon::handle_shoot_sound(bool shooting_this_update, bool holding_shoot)
{
    if (!this->loop_sound_.valid())
    {
        return;
    }

    if (holding_shoot || shooting_this_update)
    {
        this->sound_bank_->play_effect(
            this->loop_sound_,
            true,
            this->details_.shoot_sound_volume);
    }
    else
    {
        this->sound_bank_->stop_effect(this->loop_sound_, true);
    }
}
void Weapon::stop_sounds() const
{
    if (!this->loop_sound_.valid())
    {
        return;
    }
    this->sound_bank_->stop_effect(this->loop_sound_, true);
}
SoundBank::EffectHandle Weapon::resolve_loop_sound(const SoundBank& sound_bank,
    WeaponType type, PlayerTeam team, int player_num)
{
    // Sniper and Bucket fire a one-shot wave (see their handle_shoot_sound
    // overrides) and have no looping instance at all. They get an unresolved
    // handle, which both handle_shoot_sound and stop_sounds read as "nothing
    // to do".
    //
    // Resolved once here rather than looked up per call so that stop_sounds -
    // a teardown path with no catch anywhere above it - cannot throw. That
    // mattered more when this searched a map by name; it is now also the
    // difference between an index and a string compare per frame of firing.
    if (team != PlayerTeam::a && team != PlayerTeam::b)
    {
        return {};
    }
    if (player_num < 0 || player_num > 3)
    {
        return {};
    }

    switch (type)
    {
    case WeaponType::sprayer:
        return sound_bank.resolve_effect(
            SPRAYER_SOUND_DETAILS.sound_name(team, player_num));
    case WeaponType::roller:
        return sound_bank.resolve_effect(
            ROLLER_SOUND_DETAILS.sound_name(team, player_num));
    case WeaponType::mister:
        return sound_bank.resolve_effect(
            MISTER_SOUND_DETAILS.sound_name(team, player_num));
    default:
        return {};
    }
}
void Weapon::update_movement_and_rotation(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& /*player_velocity*/,
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
    if (facing_left(this->rotation()))
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
        shoot_direction, this->starting_vel_length());

    return this->proj_builder_->build_projectiles(
        this->nozzle_pos(),
        launch_velocity,
        this->team(),
        this->player_num(),
        this->team_colour(),
        this->details().proj_type,
        this->dt_,
        this->render_resources_);
}

Vector2F Weapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity) const
{
    Vector2F result = shoot_direction * starting_velocity;
    return result;
}

void Weapon::set_gun_player_aligned(bool gun_player_aligned)
{
	this->gun_player_aligned_ = gun_player_aligned;
}

bool Weapon::gun_player_aligned() const
{
	return this->gun_player_aligned_;
}

void Weapon::set_shoot_timer(float shoot_timer)
{
	this->shoot_timer_ = shoot_timer;
}

void Weapon::alter_shoot_timer(float dt)
{
	this->shoot_timer_ += dt;
}

float Weapon::shoot_timer() const
{
	return this->shoot_timer_;
}

void Weapon::set_ammo(float ammo)
{
	this->ammo_ = ammo;
}

void Weapon::alter_ammo(float ammo)
{
	this->ammo_ += ammo;
}

float Weapon::ammo_timer() const
{
	return this->ammo_timer_;
}

void Weapon::set_ammo_timer(float ammo_timer)
{
	this->ammo_timer_ = ammo_timer;
}

void Weapon::alter_ammo_timer(float dt)
{
	this->ammo_timer_ += dt;
}

float Weapon::ammo() const
{
	return this->ammo_;
}

const WeaponDetails& Weapon::details() const
{
	return this->details_;
}

const Vector2F& Weapon::offset() const
{
	return this->details_.offset;
}

const Vector2F& Weapon::nozzle_offset() const
{
	return this->details_.nozzle_offset;
}

float Weapon::shoot_interval() const
{
	return this->details_.shoot_interval;
}

float Weapon::starting_vel_length() const
{
	return this->details_.starting_vel_length;
}

float Weapon::ammo_usage() const
{
	return this->details_.ammo_usage;
}

float Weapon::rotation() const
{
	return this->rotation_;
}

void Weapon::set_rotation(float rotation)
{
	this->rotation_ = rotation;
}

bool Weapon::invert_x() const
{
	return this->invert_x_;
}

void Weapon::set_invert_x(bool invert_x)
{
	this->invert_x_ = invert_x;
}

bool Weapon::invert_y() const
{
	return this->invert_y_;
}

void Weapon::set_invert_y(bool invert_y)
{
	this->invert_y_ = invert_y;
}

const Vector2F& Weapon::player_center() const
{
	return this->player_center_;
}

void Weapon::set_player_center(const Vector2F& player_center)
{
	this->player_center_ = player_center;
}

PlayerTeam Weapon::team() const
{
	return this->team_;
}

int Weapon::player_num() const
{
	return this->player_num_;
}

const Colour& Weapon::team_colour() const
{
	return this->team_colour_;
}

WeaponType Weapon::type() const
{
	return this->type_;
}

void Weapon::reset_ammo()
{
	this->ammo_ = 1.0f;
}

ProjectileBuilder* Weapon::projectile_builder() const
{
	return this->proj_builder_.get();
}
const float* Weapon::dt_ptr() const
{
	return this->dt_;
}
float Weapon::dt() const
{
	return *this->dt_;
}
RenderResources* Weapon::render_resources() const
{
	return this->render_resources_;
}

RelativeVelocityWeapon::RelativeVelocityWeapon(
    const WeaponDetails& details,
    RelativeWeaponDetails rel_details,
    PlayerTeam team,
    int player_num,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    DirectX::SpriteEffects effects,
    float layer_depth) :
    Weapon(details, team, player_num, team_colour, type,
        player_center, render_resources, audio_resources, dt,
        color, rotation, origin, effects, layer_depth),
    rel_details_(rel_details)
{

}

Vector2F RelativeVelocityWeapon::calculate_projectile_launch_velocity(
    const Vector2F& shoot_direction,
    float starting_velocity,
    const Vector2F& player_velocity,
    AddPlayerVelocity add_player_vel,
    float player_vel_amount) const
{
    Vector2F result = shoot_direction * starting_velocity;

    if (add_player_vel == AddPlayerVelocity::x_and_y)
    {
        result += player_velocity * player_vel_amount;
    }
    else if (add_player_vel == AddPlayerVelocity::x_only)
    {
        result.x += player_velocity.x * player_vel_amount;
    }
    else //if (add_player_vel == AddPlayerVelocity::y_only)
    {
        result.y += player_velocity.y * player_vel_amount;
    }
    return result;
}

std::vector<std::unique_ptr<ICollisionGameObject>> RelativeVelocityWeapon::shoot(
    const mattmath::Vector2F& shoot_direction,
    const mattmath::Vector2F& player_velocity) const
{
    RelativeWeaponDetails rel_details = this->rel_details_;
    
    AddPlayerVelocity add_player_vel = rel_details.add_vel;

    Vector2F launch_velocity = this->calculate_projectile_launch_velocity(
        shoot_direction, this->starting_vel_length(),
        player_velocity, add_player_vel,
        rel_details.player_vel_amount);
    
    return this->projectile_builder()->build_projectiles(
        this->nozzle_pos(),
        launch_velocity,
        this->team(),
        this->player_num(),
        this->team_colour(),
        this->details().proj_type,
        this->dt_ptr(),
        this->render_resources());
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
            Vector2F::unit_vec_from_angle(this->rotation()), player_velocity);
	}
    else
    {
        return std::vector<std::unique_ptr<ICollisionGameObject>>();
    }
}
