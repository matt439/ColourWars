#ifndef WEAPON_H
#define WEAPON_H

#include "player_team.h"
#include "wep_type.h"
#include "weapon_details.h"
#include "weapon_consts.h"
#include "player_input.h"
#include "Colour.h"
#include <cmath>
#include "TextureObject.h"
#include "ProjectileBuilder.h"
#include "SoundBank.h"

struct weapon_update
{
	player_input input = player_input();
	MattMath::Vector2F player_center = { 0.0f, 0.0f };
	MattMath::Vector2F player_velocity = { 0.0f, 0.0f };
	bool player_facing_right = false;
};

class Weapon : public TextureObject
{
public:
	Weapon(const weapon_details& details,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	virtual void draw(const MattMath::Camera& camera, bool debug = false);
	virtual void draw(bool debug = false);

	virtual std::vector<std::unique_ptr<ICollisionGameObject>>
		update_and_get_projectiles(player_input input,
			const MattMath::Vector2F& player_center,
			const MattMath::Vector2F& player_velocity,
			bool player_facing_right);

	float get_ammo() const { return this->_ammo; }
	void reset_ammo() { this->_ammo = weapon_consts::STARTING_AMMO; }
	void set_player_center(const MattMath::Vector2F& player_center) { this->_player_center = player_center; }
	void stop_sounds();

protected:
	SoundBank* _sound_bank = nullptr;
	weapon_details _details = weapon_consts::DETAILS_DEFAULT;

	bool _shooting_this_update = false;

	virtual void handle_shoot_sound(bool shooting_this_update, bool holding_shoot);

	const MattMath::Vector2F& get_player_center() const { return this->_player_center; }


	player_team get_team() const { return this->_team; }
	int get_player_num() const { return this->_player_num; }
	const MattMath::Colour& get_team_colour() const { return this->_team_colour; }
	wep_type get_type() const { return this->_type; }
	MattMath::RectangleF get_nozzle_rectangle() const;


	const weapon_details& get_details() const { return this->_details; }
	const MattMath::Vector2F& get_offset() const { return this->_details.offset; }
	const MattMath::Vector2F& get_nozzle_offset() const { return this->_details.nozzle_offset; }
	float get_shoot_interval() const { return this->_details.shoot_interval; }
	float get_starting_vel_length() const { return this->_details.starting_vel_length; }
	float get_ammo_usage() const { return this->_details.ammo_usage; }

	float get_rotation() const { return this->_rotation; }
	void set_rotation(float rotation) { this->_rotation = rotation; }
	bool get_invert_x() const { return this->_invert_x; }
	void set_invert_x(bool invert_x) { this->_invert_x = invert_x; }
	bool get_invert_y() const { return this->_invert_y; }
	void set_invert_y(bool invert_y) { this->_invert_y = invert_y; }

	void set_gun_player_aligned(bool gun_player_aligned) { this->_gun_player_aligned = gun_player_aligned; }
	bool get_gun_player_aligned() const { return this->_gun_player_aligned; }
	float get_shoot_timer() const { return this->_shoot_timer; }
	void set_shoot_timer(float shoot_timer) { this->_shoot_timer = shoot_timer; }
	void alter_shoot_timer(float dt) { this->_shoot_timer += dt; }

	void set_ammo(float ammo) { this->_ammo = ammo; }
	void alter_ammo(float ammo) { this->_ammo += ammo; }
	float get_ammo_timer() const { return this->_ammo_timer; }
	void set_ammo_timer(float ammo_timer) { this->_ammo_timer = ammo_timer; }
	void alter_ammo_timer(float dt) { this->_ammo_timer += dt; }


	MattMath::Vector2F get_draw_pos() const;
	MattMath::Vector2F get_nozzle_pos() const;
	MattMath::Vector2F get_nozzle_size() const;
	bool facing_left(float rotation) const;

	virtual std::vector<std::unique_ptr<ICollisionGameObject>> shoot(
		const MattMath::Vector2F& shoot_direction) const;

	virtual MattMath::Vector2F calculate_projectile_launch_velocity(
		const MattMath::Vector2F& shoot_direction,
		float starting_velocity) const;

	MattMath::Vector2F calculate_sprite_origin(
		const MattMath::Vector2F& size, rotation_origin origin) const;

	virtual bool check_if_shooting_and_ammo_update(player_input input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity,
		bool player_facing_right);

	virtual void update_movement_and_rotation(player_input input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* get_projectile_builder() const { return this->_proj_builder.get(); }
	const float* get_dt_ptr() const { return this->_dt; }
	float get_dt() const { return *this->_dt; }
	DirectX::SpriteBatch* get_sprite_batch() const { return this->_sprite_batch; }
	ResourceManager* get_resource_manager() const { return this->_resource_manager; }
	const std::string& get_sound_effect_instance_name() const;

private:
	std::unique_ptr<ProjectileBuilder> _proj_builder = nullptr;
	const float* _dt = nullptr;
	DirectX::SpriteBatch* _sprite_batch = nullptr;
	ResourceManager* _resource_manager = nullptr;

	float _ammo = weapon_consts::STARTING_AMMO;
	float _shoot_timer = 0.0f;
	float _rotation = 0.0f;
	bool _invert_x = false;
	bool _invert_y = false;
	bool _gun_player_aligned = true;

	float _ammo_timer = 0.0f;

	player_team _team = player_team::NONE;
	int _player_num = -1;
	MattMath::Colour _team_colour = colour_consts::GRAY;
	wep_type _type = wep_type::NONE;
	MattMath::Vector2F _player_center = { 0.0f, 0.0f };

	MattMath::Vector2F get_wep_rotation_origin_offset(
		bool facing_left) const;
};




class RelativeVelocityWeapon : public Weapon
{
public:
	RelativeVelocityWeapon(const weapon_details& details,
		relative_weapon_details rel_details,
		player_team team,
		int player_num,
		const MattMath::Colour& team_colour,
		wep_type type,
		const MattMath::Vector2F& player_center,
		DirectX::SpriteBatch* sprite_batch,
		ResourceManager* resource_manager,
		const float* dt,
		const MattMath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const MattMath::Vector2F& origin = MattMath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	virtual std::vector<std::unique_ptr<ICollisionGameObject>>
		update_and_get_projectiles(player_input input,
			const MattMath::Vector2F& player_center,
			const MattMath::Vector2F& player_velocity,
			bool player_facing_right) override;

protected:
	virtual MattMath::Vector2F calculate_projectile_launch_velocity(
		const MattMath::Vector2F& shoot_direction,
		float starting_velocity,
		const MattMath::Vector2F& player_velocity,
		add_player_velocity add_player_vel,
		float player_vel_amount) const;

	virtual std::vector<std::unique_ptr<ICollisionGameObject>> shoot(
		const MattMath::Vector2F& shoot_direction,
		const MattMath::Vector2F& player_velocity) const;

private:
	relative_weapon_details _rel_details = weapon_consts::DETAILS_RELATIVE_DEFAULT;
};

#endif // !WEAPON_H
