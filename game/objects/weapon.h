#pragma once

#include "game/objects/player_team.h"
#include "game/objects/wep_type.h"
#include "game/objects/weapon_details.h"
#include "game/objects/weapon_consts.h"
#include "game/objects/player_input_data.h"
#include "engine/math/colour.h"
#include "engine/render/texture_object.h"
#include "game/objects/projectile_builder.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"

class Weapon : public TextureObject
{
public:
	Weapon(const WeaponDetails& details,
		player_team team,
		int player_num,
		const mattmath::Colour& team_colour,
		wep_type type,
		const mattmath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	virtual void draw(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& camera, bool debug = false);
	virtual void draw(DirectX::SpriteBatch* sprite_batch, bool debug = false);

	virtual std::vector<std::unique_ptr<ICollisionGameObject>>
		update_and_get_projectiles(PlayerInputData input,
			const mattmath::Vector2F& player_center,
			const mattmath::Vector2F& player_velocity,
			bool player_facing_right);

	float get_ammo() const;
	void reset_ammo();
	void set_player_center(const mattmath::Vector2F& player_center);

	// Never throws: weapons with no looping fire sound stop nothing.
	virtual void stop_sounds() const;

protected:
	SoundBank* sound_bank_ = nullptr;
	WeaponDetails details_ = weapon_consts::DETAILS_DEFAULT;

	// This weapon's looping SoundEffectInstance, resolved once at construction.
	// Left unresolved for weapons that fire a one-shot wave instead - which is
	// what handle_shoot_sound and stop_sounds read as "nothing to do". The
	// default Handle is inert precisely so an absent thing can be spelt this
	// way instead of with a sentinel string.
	SoundBank::EffectHandle loop_sound_;

	// The one-shot fire wave, for the weapons that use one. Unconditionally
	// resolved, unlike the loop above: every weapon definition names one, so a
	// name the bank does not have is a content bug and should say so here.
	SoundBank::WaveHandle shoot_sound_;

	bool shooting_this_update_ = false;

	virtual void handle_shoot_sound(bool shooting_this_update, bool holding_shoot);

	// Tint for this frame's draw. A hook rather than a member assignment so
	// draw() stays a pure read - the render workers all enter it at once.
	virtual mattmath::Colour get_draw_colour() const;

	const mattmath::Vector2F& get_player_center() const;


	player_team get_team() const;
	int get_player_num() const;
	const mattmath::Colour& get_team_colour() const;
	wep_type get_type() const;
	mattmath::RectangleF get_nozzle_rectangle() const;


	const WeaponDetails& get_details() const;
	const mattmath::Vector2F& get_offset() const;
	const mattmath::Vector2F& get_nozzle_offset() const;
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


	mattmath::Vector2F get_draw_pos() const;
	mattmath::Vector2F get_nozzle_pos() const;
	static mattmath::Vector2F get_nozzle_size();
	static bool facing_left(float rotation);

	virtual std::vector<std::unique_ptr<ICollisionGameObject>> shoot(
		const mattmath::Vector2F& shoot_direction) const;

	virtual mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity) const;

	static mattmath::Vector2F calculate_sprite_origin(
		const mattmath::Vector2F& size, rotation_origin origin);

	virtual bool check_if_shooting_and_ammo_update(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity);

	virtual void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* get_projectile_builder() const;
	const float* get_dt_ptr() const;
	float get_dt() const;
	RenderResources* get_render_resources() const override;

private:
	static SoundBank::EffectHandle resolve_loop_sound(
		const SoundBank& sound_bank, wep_type type, player_team team,
		int player_num);

	std::unique_ptr<ProjectileBuilder> proj_builder_ = nullptr;
	const float* dt_ = nullptr;
	RenderResources* render_resources_ = nullptr;

	// Resolved once at construction like every other element name, even though
	// only the debug draw reads it - a name resolved on the draw path is a
	// name resolved on the draw path, however rarely that path is taken.
	SpriteSheet::frame_handle nozzle_frame_;

	float ammo_ = weapon_consts::STARTING_AMMO;
	float shoot_timer_ = 0.0f;
	float rotation_ = 0.0f;
	bool invert_x_ = false;
	bool invert_y_ = false;
	bool gun_player_aligned_ = true;

	float ammo_timer_ = 0.0f;

	player_team team_ = player_team::NONE;
	int player_num_ = -1;
	mattmath::Colour team_colour_ = colour_consts::GRAY;
	wep_type type_ = wep_type::NONE;
	mattmath::Vector2F player_center_ = { 0.0f, 0.0f };

	mattmath::Vector2F get_wep_rotation_origin_offset(
		bool facing_left) const;
};

class RelativeVelocityWeapon : public Weapon
{
public:
	RelativeVelocityWeapon(const WeaponDetails& details,
		RelativeWeaponDetails rel_details,
		player_team team,
		int player_num,
		const mattmath::Colour& team_colour,
		wep_type type,
		const mattmath::Vector2F& player_center,
		RenderResources* render_resources,
		const AudioResources* audio_resources,
		const float* dt,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layer_depth = 0.0f);

	std::vector<std::unique_ptr<ICollisionGameObject>>
		update_and_get_projectiles(PlayerInputData input,
			const mattmath::Vector2F& player_center,
			const mattmath::Vector2F& player_velocity,
			bool player_facing_right) override;

protected:
	virtual mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity,
		const mattmath::Vector2F& player_velocity,
		add_player_velocity add_player_vel,
		float player_vel_amount) const;

	virtual std::vector<std::unique_ptr<ICollisionGameObject>> shoot(
		const mattmath::Vector2F& shoot_direction,
		const mattmath::Vector2F& player_velocity) const;

private:
	RelativeWeaponDetails rel_details_ = weapon_consts::DETAILS_RELATIVE_DEFAULT;
};
