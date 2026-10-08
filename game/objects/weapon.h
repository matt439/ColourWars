#pragma once

#include "game/play_settings.h"

#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/objects/weapon_details.h"
#include "game/tuning.h"
#include "game/objects/player_input_data.h"
#include "engine/render/colour.h"
#include "engine/render/texture_object.h"
#include "game/objects/projectile_builder.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"

// A concrete weapon with a shared firing workflow and overridable behaviour.
// Player owns it through this interface; tuning selects assets and balance.
class Weapon : public labrador::TextureObject
{
public:
	// Borrowed tuning must outlive the weapon; type selects its tuning record.
	// sound_index is the player's 0..3 match position, independent of input slot.
	// Each player needs a separate loop voice.
	Weapon(const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		int sound_index,
		const labrador::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	// Called through Player::draw and potentially shared by render workers.
	// Keep this const even though the owning unique_ptr permits mutable access.
	virtual void draw(labrador::DrawList& draw_list,
		bool debug = false) const;

	// Aim, update ammunition/sound, then shoot if allowed. Every step dispatches
	// through the hooks below, including for subclasses of RelativeVelocityWeapon.
	// Transfers ownership of live, non-null projectiles to the caller.
	virtual std::vector<std::unique_ptr<Projectile>>
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

	// Pause loops while gameplay updates are suspended, preserving playback position.
	virtual void pause_sounds() const;
	virtual void resume_sounds() const;

	// Visual bounds for any rotation about an origin inside the weapon sprite.
	// A sprite diagonal around draw_pos() covers every orientation. Player::bounds
	// unions this with the body so scene culling includes the held weapon.
	mattmath::RectangleF draw_bounds() const;

protected:
	labrador::SoundBank* sound_bank_ = nullptr;
	const PlaySettings* preferences_ = nullptr;

	// Borrowed, never null: the tuning table and this weapon's resolved record.
	const GameTuning* tuning_ = nullptr;
	const WeaponTuning* weapon_tuning_ = nullptr;

	// Loop handle resolved at construction; invalid for one-shot weapons.
	labrador::SoundBank::EffectHandle loop_sound_;

	// Every weapon definition names a fire wave; resolve it at construction
	// so a missing name is reported as a content error.
	labrador::SoundBank::WaveHandle shoot_sound_;

	bool shooting_this_update_ = false;

	virtual void handle_shoot_sound(bool shooting_this_update, bool holding_shoot);

	// Compute tint without mutating shared state during draw.
	virtual labrador::Colour draw_colour() const;

	const mattmath::Vector2F& player_center() const;


	PlayerTeam team() const;
	int player_num() const;
	const labrador::Colour& team_colour() const;
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
	bool invert_y() const;
	void set_invert_y(bool invert_y);

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

	virtual std::vector<std::unique_ptr<Projectile>> shoot(
		const mattmath::Vector2F& shoot_direction,
		const mattmath::Vector2F& player_velocity) const;

	virtual mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity,
		const mattmath::Vector2F& player_velocity) const;

	static mattmath::Vector2F calculate_sprite_origin(
		const mattmath::Vector2F& size, labrador::RotationOrigin origin);

	virtual bool check_if_shooting_and_ammo_update(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		float dt);

	virtual void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* projectile_builder() const;
	labrador::RenderResources* render_resources() const override;

private:
	// Permit only rounding at the full-shot boundary, relative to that shot's cost.
	bool can_afford_shot() const;

	static labrador::SoundBank::EffectHandle resolve_loop_sound(
		const labrador::SoundBank& sound_bank,
		const WeaponLoopSounds& loop_sounds, PlayerTeam team, int sound_index);

	std::unique_ptr<ProjectileBuilder> proj_builder_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;

	// Debug frame handle, resolved at construction.
	labrador::SpriteSheet::frame_handle nozzle_frame_;

	// Initialized from tuning in the constructor.
	float ammo_ = 0.0f;
	float shoot_timer_ = 0.0f;
	float rotation_ = 0.0f;
	bool invert_y_ = false;

	float ammo_timer_ = 0.0f;

	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	labrador::Colour team_colour_ = labrador::Colour::gray;
	WeaponType type_ = WeaponType::none;
	mattmath::Vector2F player_center_ = { 0.0f, 0.0f };

	mattmath::Vector2F wep_rotation_origin_offset(
		bool facing_left) const;
};

// Reuses the firing workflow, adding the player's velocity to each launch
// according to tuning. Further subclasses retain every Weapon hook.
class RelativeVelocityWeapon : public Weapon
{
public:
	RelativeVelocityWeapon(const GameTuning* tuning,
		PlayerTeam team,
		int player_num,
		int sound_index,
		const labrador::Colour& team_colour,
		WeaponType type,
		const mattmath::Vector2F& player_center,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

protected:
	mattmath::Vector2F calculate_projectile_launch_velocity(
		const mattmath::Vector2F& shoot_direction,
		float starting_velocity,
		const mattmath::Vector2F& player_velocity) const override;

private:
	// Velocity settings from the same borrowed record as the base weapon.
	const RelativeWeaponDetails& rel_details() const;
};
