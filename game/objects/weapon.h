#pragma once

#include "game/play_settings.h"
#include "game/rumble_events.h"

#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/objects/weapon_details.h"
#include "game/tuning.h"
#include "game/presentation.h"
#include "game/objects/player_input_data.h"
#include "engine/render/colour.h"
#include "engine/render/texture_object.h"
#include "game/objects/projectile_builder.h"
#include "game/objects/weapon_feedback.h"
#include "engine/audio/sound_bank.h"
#include "engine/audio/audio_resources.h"

enum class ToolMotionPose
{
	idle,
	motion_1,
	motion_2,
	lift,
};

// Updated with simulation events, then read unchanged by every render view.
// Pressure is a paid-shot pulse, not ammunition; travel is turns, modulo one.
struct ToolMotionState
{
	ToolMotionPose pose = ToolMotionPose::idle;
	float pressure = 0.0f;
	float valve = 0.0f;
	float travel_phase = 0.0f;
	float lift = 0.0f;
	float compression = 0.0f;
	float bucket_swing = 0.0f;
	float hose_sway = 0.0f;
	float mist_tail_remaining = 0.0f;
	bool operator==(const ToolMotionState&) const = default;
};

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
	// Draws the paint overlay in this colour instead, while it is set: the
	// opening's grey figures.
	void set_paint_override(std::optional<labrador::Colour> colour);

	// Aim, update ammunition/sound, then shoot if allowed. Every step dispatches
	// through the hooks below, including for subclasses of RelativeVelocityWeapon.
	// Transfers ownership of live, non-null projectiles to the caller.
	virtual std::vector<std::unique_ptr<Projectile>>
		update_and_get_projectiles(PlayerInputData input,
			const mattmath::Vector2F& player_center,
			const mattmath::Vector2F& player_velocity,
			bool player_facing_right,
		float dt, float ammo_regen_multiplier = 1.0f);

	float ammo() const;
	// POL#38: below the larger of the tank share and the shot reserve in
	// tuning, never above a full tank. Feedback only; firing ignores it.
	float low_paint_level() const;
	// The warning rearms at or above this: the low line plus the larger of
	// the tuned band and one shot, so regeneration between a slow tool's
	// held shots cannot rearm it mid-burst. Never above a full tank.
	float low_paint_rearm_level() const;
	bool low_paint() const;
	// Simulated seconds continuously low; zero otherwise. Drives the blink.
	float low_paint_elapsed() const;
	// Set by the one warning a paid shot raises on entering low paint, and
	// cleared only by recovery past the rearm line or a refill.
	bool low_paint_warned() const;
	// What the tank mark draws from; an inactive cue when not low.
	LowPaintCue low_paint_cue() const;
	RumbleEvents take_rumble_events() noexcept;
	void clear_rumble_events() noexcept;
	// The same strict cooldown and full-shot paint gate used by firing.
	// Player invokes it only while the player can act.
	bool ready_to_shoot() const;
	WeaponReadiness readiness() const;
	// Direction only, never a predicted hit point, for a tool that aims.
	// The roller has no marker.
	virtual bool has_aim_marker() const { return true; }
	void draw_aim_marker(labrador::DrawList& draw_list) const;
	mattmath::RectangleF aim_marker_bounds() const;
	// Infinite-ammo refills preserve the paid shot's cooldown notification.
	// Respawn uses the default to clear all feedback silently.
	void reset_ammo(bool reset_feedback = true);
	void set_player_center(const mattmath::Vector2F& player_center);

	// Optional for direct CPU clients; the game builder supplies presentation.
	// Switching the borrowed record resets visual history, never gameplay.
	void set_motion_presentation(const ToolMotionPresentation* presentation);
	void set_feedback_presentation(const WeaponFeedbackPresentation* presentation);
	const WeaponFeedback* feedback() const;
	void reset_motion();
	const ToolMotionState& motion_state() const;
	int visual_pose() const;
	// Resolved, signed horizontal travel after every contact, with spawn settling
	// excluded. A blocked wall and airborne movement cannot turn the drum.
	void update_resolved_motion(bool supported, bool established_support,
		float signed_distance, float dt);

	// Called once after all player contacts, with spawn settling excluded.
	// Most tools do not respond to ground support; a roller does.
	virtual void update_ground_contact(bool supported, bool established_support,
		float dt);

	// Never throws: weapons with no looping fire sound stop nothing.
	virtual void stop_sounds() const;

	// Pause loops while gameplay updates are suspended, preserving playback position.
	virtual void pause_sounds() const;
	virtual void resume_sounds() const;

	// Visual bounds cover every rotated pose and any detached release haze.
	// Player::bounds unions this with the body for scene culling.
	mattmath::RectangleF draw_bounds() const;

	// Read-only attachment points for game-owned character artwork. These remain
	// the same continuous simulation aim and muzzle used to launch projectiles.
	WeaponType type() const;
	float rotation() const;
	mattmath::Vector2F draw_pos() const;
	mattmath::Vector2F nozzle_pos() const;
	bool shooting_this_update() const;
	void draw_nozzle_debug(labrador::DrawList& draw_list) const;

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
	void play_feedback_sound(labrador::SoundBank::WaveHandle sound,
		const SoundTuning& tuning) const;

	// Tint only the paint overlay without mutating shared state during draw.
	virtual labrador::Colour paint_colour() const;

	// Resolved ground travel's share of the decorative tool motion, called
	// only while motion presentation is enabled. The default swings a bucket's
	// handle or a sprayer's hose; the roller turns its sleeve instead.
	virtual void advance_travel_motion(bool supported, bool established_support,
		float signed_distance, float dt);
	// The pose the motion state shows now. The roller reads its lift and turn.
	virtual ToolMotionPose motion_pose() const;
	ToolMotionState& tool_motion();
	const ToolMotionState& tool_motion() const;
	// The borrowed record; only while motion presentation is set.
	const ToolMotionPresentation& motion_look() const;
	// Whether the last resolved travel was supported; cleared with the motion.
	bool motion_supported_ = false;

	const mattmath::Vector2F& player_center() const;


	PlayerTeam team() const;
	int player_num() const;
	const labrador::Colour& team_colour() const;
	mattmath::RectangleF nozzle_rectangle() const;


	const WeaponDetails& details() const;
	const mattmath::Vector2F& offset() const;
	const mattmath::Vector2F& nozzle_offset() const;
	float shoot_interval() const;
	float starting_vel_length() const;
	float ammo_usage() const;

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
		const mattmath::Vector2F& size, colourwars::RotationOrigin origin);

	virtual bool check_if_shooting_and_ammo_update(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		float dt, float ammo_regen_multiplier);

	virtual void update_movement_and_rotation(PlayerInputData input,
		const mattmath::Vector2F& player_center,
		const mattmath::Vector2F& player_velocity,
		bool player_facing_right);

	ProjectileBuilder* projectile_builder() const;
	labrador::RenderResources* render_resources() const override;

private:
	// Permit only rounding at the full-shot boundary, relative to that shot's cost.
	bool can_afford_shot() const;
	void update_feedback_sounds(bool holding_shoot, bool shot,
		bool affordable_before, float dt);
	void update_motion(bool shot, float dt);
	void reset_mechanism_motion();
	void select_motion_pose();
	void draw_mist_tail(labrador::DrawList& draw_list) const;

	static labrador::SoundBank::EffectHandle resolve_loop_sound(
		const labrador::SoundBank& sound_bank,
		const WeaponLoopSounds& loop_sounds, PlayerTeam team, int sound_index);

	std::unique_ptr<ProjectileBuilder> proj_builder_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;

	// Paint overlay and debug frame handles, resolved at construction.
	labrador::SpriteSheet::frame_handle paint_frame_;
	// Full-canvas row levels, sharing the body's transform in every view.
	// The nozzle's wet paint stays in paint_frame_ as the reservoir empties.
	std::optional<labrador::SpriteSheet::strip_handle> supply_strip_;
	labrador::SpriteSheet::frame_handle nozzle_frame_;
	std::array<labrador::SpriteSheet::frame_handle, 4> motion_frames_;
	std::array<labrador::SpriteSheet::frame_handle, 4> motion_paint_frames_;
	const ToolMotionPresentation* motion_presentation_ = nullptr;
	ToolMotionState motion_;
	std::unique_ptr<WeaponFeedback> feedback_;
	labrador::RenderResources::SpriteSheetHandle mist_sheet_;
	labrador::SpriteSheet::frame_handle mist_frame_;
	bool motion_firing_ = false;
	mattmath::Vector2F mist_release_position_;
	float mist_release_rotation_ = 0.0f;
	labrador::SoundBank::WaveHandle empty_sound_;
	labrador::SoundBank::WaveHandle recovered_sound_;
	labrador::SoundBank::WaveHandle stop_sound_;
	labrador::SoundBank::WaveHandle ready_sound_;
	labrador::SoundBank::WaveHandle low_sound_;

	// Initialized from tuning in the constructor.
	float ammo_ = 0.0f;
	float shoot_timer_ = 0.0f;
	float rotation_ = 0.0f;
	bool invert_y_ = false;
	// Until the first explicit aim, follow facing. Thereafter a released stick
	// retains the last aim through movement, pauses and respawns in this match.
	bool aim_requested_ = false;

	float ammo_timer_ = 0.0f;
	// stop_sounds() is const because it changes presentation, not gameplay.
	mutable bool loop_playing_ = false;
	bool empty_warned_ = false;
	RumbleEvents pending_rumble_;
	float empty_repeat_remaining_ = 0.0f;
	bool awaiting_recovery_ = false;
	bool cooldown_feedback_pending_ = false;
	// Set by the warning, cleared only by recovery past the rearm line.
	bool low_warned_ = false;
	float low_elapsed_ = 0.0f;

	PlayerTeam team_ = PlayerTeam::none;
	int player_num_ = -1;
	labrador::Colour team_colour_ = labrador::Colour::gray;
	WeaponType type_ = WeaponType::none;
	mattmath::Vector2F player_center_ = { 0.0f, 0.0f };

	mattmath::Vector2F wep_rotation_origin_offset(
		bool facing_left) const;
	// Set while the opening draws the figures grey.
	std::optional<labrador::Colour> paint_override_;
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
