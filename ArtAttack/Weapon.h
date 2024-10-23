#ifndef WEAPON_H
#define WEAPON_H

#include "player_team.h"
#include "wep_type.h"
#include "WeaponDetails.h"
#include "weapon_consts.h"
#include "PlayerInputData.h"
#include "Colour.h"
#include "TextureObject.h"
#include "ProjectileBuilder.h"
#include "SoundBank.h"

class Weapon : public TextureObject
{
public:
	Weapon(const WeaponDetails& details,
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
		update_and_get_projectiles(PlayerInputData input,
			const MattMath::Vector2F& player_center,
			const MattMath::Vector2F& player_velocity,
			bool player_facing_right);

	float get_ammo() const;
	void reset_ammo();
	void set_player_center(const MattMath::Vector2F& player_center);
	void stop_sounds() const;

protected:
	SoundBank* _sound_bank = nullptr;
	WeaponDetails _details = weapon_consts::DETAILS_DEFAULT;

	bool _shooting_this_update = false;

	virtual void handle_shoot_sound(bool shooting_this_update, bool holding_shoot);

	const MattMath::Vector2F& get_player_center() const;


	player_team get_team() const;
	int get_player_num() const;
	const MattMath::Colour& get_team_colour() const;
	wep_type get_type() const;
	MattMath::RectangleF get_nozzle_rectangle() const;


	const WeaponDetails& get_details() const;
	const MattMath::Vector2F& get_offset() const;
	const MattMath::Vector2F& get_nozzle_offset() const;
	float get_shoot_interval() const;
	float get_starting_vel_length() const;
	float get_ammo_usage() const;

	float get_rotation() const;
	void set_rotation(float rotation);
	bool get_invert_x() const;
	void set_invert_x(bool invert_x);
	bool get_invert_y() const;
	void set_invert_y(bool invert_y);

	void set_gun_player_aligned(bool gun_player_aligned);
	bool get_gun_player_aligned() const;
	float get_shoot_timer() const;
	void set_shoot_timer(float shoot_timer);
	void alter_shoot_timer(float dt);

	void set_ammo(float ammo);
	void alter_ammo(float ammo);
	float get_ammo_timer() const;
	void set_ammo_timer(float ammo_timer);
	void alter_ammo_timer(float dt);


	MattMath::Vector2F get_draw_pos() const;
	MattMath::Vector2F get_nozzle_pos() const;
	static MattMath::Vector2F get_nozzle_size();
	static bool facing_left(float rotation);

	virtual std::vector<std::unique_ptr<ICollisionGameObject>> shoot(
		const MattMath::Vector2F& shoot_direction) const;

	virtual MattMath::Vector2F calculate_projectile_launch_velocity(
		const MattMath::Vector2F& shoot_direction,
		float starting_velocity) const;

	static MattMath::Vector2F calculate_sprite_origin(
		const MattMath::Vector2F& size, rotation_origin origin);

	virtual bool check_if_shooting_and_ammo_update(PlayerInputData input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity);

	virtual void update_movement_and_rotation(PlayerInputData input,
		const MattMath::Vector2F& player_center,
		const MattMath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* get_projectile_builder() const;
	const float* get_dt_ptr() const;
	float get_dt() const;
	DirectX::SpriteBatch* get_sprite_batch() const override;
	ResourceManager* get_resource_manager() const override;
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
	RelativeVelocityWeapon(const WeaponDetails& details,
		RelativeWeaponDetails rel_details,
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

	std::vector<std::unique_ptr<ICollisionGameObject>>
		update_and_get_projectiles(PlayerInputData input,
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
	RelativeWeaponDetails _rel_details = weapon_consts::DETAILS_RELATIVE_DEFAULT;
};

#endif // !WEAPON_H
