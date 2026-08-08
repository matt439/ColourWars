#pragma once

#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/objects/weapon_details.h"
#include "game/objects/weapon_consts.h"
#include "game/objects/player_input_data.h"
#include "engine/math/colour.h"
#include "engine/render/texture_object.h"
#include "game/objects/projectile_builder.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"

class Weapon : public artattack::TextureObject
{
public:
	Weapon(const WeaponDetails& details,
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	// const for the same reason GameObject::draw is, even though a Weapon is
	// not one: Player::draw reaches its weapon, so it sits under the same
	// per-view fan-out and every worker enters this on the same Weapon at once.
	// The const does not come for free through Player - primary_ is a
	// unique_ptr, and a const unique_ptr still hands out a non-const pointee -
	// so it has to be declared here to be enforced at all.
	virtual void draw(artattack::DrawList& draw_list,
		bool debug = false) const;

	virtual std::vector<std::unique_ptr<CollisionObject>>
		update_and_get_projectiles(PlayerInputData input,
			const mattmath::Vector2F& player_center,
			const mattmath::Vector2F& player_velocity,
			bool player_facing_right,
		float dt);

	float ammo() const;
	void reset_ammo();
	void set_player_center(const mattmath::Vector2F& player_center);

	// Never throws: weapons with no looping fire sound stop nothing.
	virtual void stop_sounds() const;

protected:
	artattack::SoundBank* sound_bank_ = nullptr;
	WeaponDetails details_ = weapon_consts::DETAILS_DEFAULT;

	// This weapon's looping SoundEffectInstance, resolved once at construction.
	// Left unresolved for weapons that fire a one-shot wave instead - which is
	// what handle_shoot_sound and stop_sounds read as "nothing to do". The
	// default Handle is inert precisely so an absent thing can be spelt this
	// way instead of with a sentinel string.
	artattack::SoundBank::EffectHandle loop_sound_;

	// The one-shot fire wave, for the weapons that use one. Unconditionally
	// resolved, unlike the loop above: every weapon definition names one, so a
	// name the bank does not have is a content bug and should say so here.
	artattack::SoundBank::WaveHandle shoot_sound_;

	bool shooting_this_update_ = false;

	virtual void handle_shoot_sound(bool shooting_this_update, bool holding_shoot);

	// Tint for this frame's draw. A hook rather than a member assignment so
	// draw() stays a pure read - the render workers all enter it at once.
	virtual mattmath::Colour draw_colour() const;

	const mattmath::Vector2F& player_center() const;


	PlayerTeam team() const;
	int player_num() const;
	const mattmath::Colour& team_colour() const;
	WeaponType type() const;
	mattmath::RectangleF nozzle_rectangle() const;


	const WeaponDetails& details() const;
	const mattmath::Vector2F& offset() const;
	const mattmath::Vector2F& nozzle_offset() const;
	float shoot_interval() const;
	float starting_vel_length() const;
	float ammo_usage() const;

	float rotation() const;
	void set_rotation(float rotation);
	bool invert_x() const;
	void set_invert_x(bool invert_x);
	bool invert_y() const;
	void set_invert_y(bool invert_y);

	void set_gun_player_aligned(bool gun_player_aligned);
	bool gun_player_aligned() const;
	float shoot_timer() const;
	void set_shoot_timer(float shoot_timer);
	void alter_shoot_timer(float dt);

	void set_ammo(float ammo);
	void alter_ammo(float ammo);
	float ammo_timer() const;
	void set_ammo_timer(float ammo_timer);
	void alter_ammo_timer(float dt);


	mattmath::Vector2F draw_pos() const;
	mattmath::Vector2F nozzle_pos() const;
	static mattmath::Vector2F nozzle_size();
	static bool facing_left(float rotation);

	virtual std::vector<std::unique_ptr<CollisionObject>> shoot(
		const mattmath::Vector2F& shoot_direction) const;

	virtual mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity) const;

	static mattmath::Vector2F calculate_sprite_origin(
		const mattmath::Vector2F& size, artattack::RotationOrigin origin);

	virtual bool check_if_shooting_and_ammo_update(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		float dt);

	virtual void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* projectile_builder() const;
	artattack::RenderResources* render_resources() const override;

private:
	static artattack::SoundBank::EffectHandle resolve_loop_sound(
		const artattack::SoundBank& sound_bank, WeaponType type, PlayerTeam team,
		int player_num);

	std::unique_ptr<ProjectileBuilder> proj_builder_ = nullptr;
	artattack::RenderResources* render_resources_ = nullptr;

	// Resolved once at construction like every other element name, even though
	// only the debug draw reads it - a name resolved on the draw path is a
	// name resolved on the draw path, however rarely that path is taken.
	artattack::SpriteSheet::frame_handle nozzle_frame_;

	float ammo_ = weapon_consts::STARTING_AMMO;
	float shoot_timer_ = 0.0f;
	float rotation_ = 0.0f;
	bool invert_x_ = false;
	bool invert_y_ = false;
	bool gun_player_aligned_ = true;

	float ammo_timer_ = 0.0f;

	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	mattmath::Colour team_colour_ = colour_consts::GRAY;
	WeaponType type_ = WeaponType::none;
	mattmath::Vector2F player_center_ = { 0.0f, 0.0f };

	mattmath::Vector2F wep_rotation_origin_offset(
		bool facing_left) const;
};

class RelativeVelocityWeapon : public Weapon
{
public:
	RelativeVelocityWeapon(const WeaponDetails& details,
		RelativeWeaponDetails rel_details,
		PlayerTeam team,
		int player_num,
		const mattmath::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		const mattmath::Colour& color = colour_consts::WHITE,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	std::vector<std::unique_ptr<CollisionObject>>
		update_and_get_projectiles(PlayerInputData input,
			const mattmath::Vector2F& player_center,
			const mattmath::Vector2F& player_velocity,
			bool player_facing_right,
			float dt) override;

protected:
	virtual mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity,
		const mattmath::Vector2F& player_velocity,
		AddPlayerVelocity add_player_vel,
		float player_vel_amount) const;

	virtual std::vector<std::unique_ptr<CollisionObject>> shoot(
		const mattmath::Vector2F& shoot_direction,
		const mattmath::Vector2F& player_velocity) const;

private:
	RelativeWeaponDetails rel_details_ = weapon_consts::DETAILS_RELATIVE_DEFAULT;
};
