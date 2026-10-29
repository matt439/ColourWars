#pragma once

#include "game/play_settings.h"

#include "game/objects/weapon.h"
#include "engine/render/colour.h"

class WeaponRoller final : public Weapon
{
public:
	WeaponRoller(const GameTuning* tuning,
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

	void update_ground_contact(bool supported, bool established_support,
		float dt) override;
	// The roller lays paint where it rolls; it has no aim to mark.
	bool has_aim_marker() const override { return false; }

protected:
	// Ground contact, rather than input, turns the sleeve, lifts it in the
	// air and compresses it on landing.
	void advance_travel_motion(bool supported, bool established_support,
		float signed_distance, float dt) override;
	ToolMotionPose motion_pose() const override;

private:
	labrador::SoundBank::WaveHandle lift_sound_;
	labrador::SoundBank::WaveHandle settle_sound_;
	bool had_ground_support_ = false;

	void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right) override;

	// The sleeve takes the team colour only while it is laying paint.
	labrador::Colour paint_colour() const override;
};
