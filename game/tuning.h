#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_details.h"
#include "game/objects/weapon_details.h"
#include "game/objects/weapon_type.h"
#include "engine/math/shape.h"
#include "engine/math/vector2f.h"
#include "engine/render/colour.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace labrador
{
	class AudioResources;
	class RenderResources;
}

// Gameplay balance and asset choices loaded from tuning.json at startup.
// Mechanics and animation states remain in C++; changing these values needs
// staging the updated content and restarting.

// A named sound and how loud it is played.
struct SoundTuning
{
	std::string name;
	float volume = 1.0f;
};

// Match phase durations and countdown presentation.
// The opening's beats before the countdown (docs/backstory.md, "Opening"), in
// seconds. They move the match's state machine, so they are tuning.
struct OpeningTuning
{
	// The monitor and the person, still. A quick rematch starts after it.
	float settle = 0.0f;
	// Each of the two previews: the cursor reaches a swatch, then half the
	// picture shows in that colour.
	float preview = 0.0f;
	float undo = 0.0f;
	// The person sighs, stands and walks out.
	float leave = 0.0f;
	// The last part of the countdown, in which the view pushes through the
	// glass into split screen.
	float push = 0.0f;
};

// The reveal after the clock (docs/backstory.md, "The reveal"), in seconds.
// The pull back into the monitor is match.zoom_out; these follow it, and
// replace match.overview when the match had an intro.
struct RevealTuning
{
	// The person comes back and sits down.
	float walk_in = 0.0f;
	// Each colour hovered: the whole picture shows in it.
	float hover = 0.0f;
	// The cursor wavers between the two, then clicks the winner at once.
	float decide = 0.0f;
	// The slot is filled and the coverage shows.
	float show = 0.0f;
	// The person leans back and leaves.
	float leave = 0.0f;
	// Alone again: the winners celebrate and the losers slump.
	float react = 0.0f;
};

struct MatchTuning
{
	float duration = 0.0f;
	std::vector<float> duration_choices;
	float countdown = 0.0f;
	float zoom_out = 0.0f;
	float overview = 0.0f;

	std::string countdown_font;
	labrador::Colour countdown_colour = labrador::Colour::white;
	labrador::Colour countdown_shadow_colour = labrador::Colour::black;
	mattmath::Vector2F countdown_shadow_offset = { 0.0f, 0.0f };
	float countdown_scale = 1.0f;

	// Controls reminder shown during the countdown. Keep its presentation with
	// the countdown, and keep its text in sync with the input mapping.
	std::string controls_font;
	labrador::Colour controls_colour = labrador::Colour::white;
	labrador::Colour controls_shadow_colour = labrador::Colour::black;
	mattmath::Vector2F controls_shadow_offset = { 0.0f, 0.0f };

	// Down the screen, as a fraction of its height, so the legend clears the
	// countdown at any resolution.
	float controls_y = 0.8f;

	SoundTuning countdown_sound;
	SoundTuning zoom_out_sound;

	OpeningTuning opening;
	RevealTuning reveal;
	SoundTuning hover_sound;
	SoundTuning click_sound;
};

// Animation clip selection and optional timing override.
struct PlayerAnimationTuning
{
	std::string sheet;
	std::string strip;
	std::string uniform_texture;

	// Absent means use the sprite strip's default frame time.
	std::optional<float> frame_time = std::nullopt;
};

// Paint coverage granularity, thickness, visuals and sound.
// Tile lengths affect scoring precision, so geometry belongs with balance data.
struct PaintTuning
{
	// Target tile lengths along horizontal and vertical painted faces.
	mattmath::Vector2F size = { 0.0f, 0.0f };

	// How far paint reaches in from a painted face.
	float thickness = 0.0f;

	std::string sheet;
	std::string frame;

	std::string splash_sheet;
	std::string splash_animation;

	std::string sound_bank;
	SoundTuning splash_sound;
};

struct PlayerTuning
{
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F max_velocity = { 0.0f, 0.0f };

	float x_initial_velocity = 0.0f;
	float x_acceleration = 0.0f;
	float x_deacceleration = 0.0f;

	float gravity = 0.0f;
	float ramp_gravity_multiplier = 1.0f;

	float jump_launch_velocity = 0.0f;
	float jump_acceleration = 0.0f;
	float jump_max_time = 0.0f;
	// Seconds; zero disables the corresponding input forgiveness window.
	float jump_buffer_time = 0.0f;
	float ledge_grace_time = 0.0f;

	float drop_down_velocity = 0.0f;
	float drop_down_analog_threshold = 0.0f;

	float starting_health = 1.0f;
	float health_regen_delay = 0.0f;
	float health_regen_rate = 0.0f;
	float respawn_delay = 0.0f;
	float damage_sound_delay = 0.0f;

	std::string sound_bank;
	SoundTuning damage_sound;
	SoundTuning death_sound;
	SoundTuning jump_sound;
	SoundTuning respawn_sound;

	PlayerAnimationTuning idle;
	PlayerAnimationTuning walk;
	PlayerAnimationTuning running;
	PlayerAnimationTuning jumping;
	PlayerAnimationTuning falling;
};

struct WeaponTuning
{
	WeaponDetails details;

	// Defaults apply when a weapon does not inherit the player's velocity.
	RelativeWeaponDetails relative;

	// Empty for weapons using one-shot waves instead of looping effects.
	WeaponLoopSounds loop_sounds;
};

struct ProjectileTuning
{
	ProjectileDetails details;

	// `type` is no_diffusion unless the file gave a "diffusion" block.
	DiffusionDetails diffusion;

	// Common integrator velocity cap, copied into each projectile record.
	mattmath::Vector2F max_velocity = { 0.0f, 0.0f };
};

// Parsed once at startup and borrowed by gameplay objects, including during
// updates. Named records are indexed by enum after loading.
class GameTuning
{
public:
	// Loads and validates the file, reporting invalid content with source context.
	explicit GameTuning(const char* path);

	// Call after the manifest loads, before any menu or match can use these.
	void validate_resources(const labrador::RenderResources& render,
		const labrador::AudioResources& audio) const;
	// Match sounds are played through the bank chosen by each level.
	void validate_match_sounds(const labrador::RenderResources& render,
		const labrador::AudioResources& audio, const std::string& bank) const;

	const MatchTuning& match() const;
	const PaintTuning& paint() const;
	const PlayerTuning& player() const;

	// Shared across every weapon: how fast ammo comes back, and how much of it
	// a weapon starts with.
	float ammo_regen_delay() const;
	float ammo_regen_rate() const;
	float starting_ammo() const;

	// Throws std::out_of_range if the requested record was not supplied.
	const WeaponTuning& weapon(WeaponType type) const;
	const ProjectileTuning& projectile(projectile_type type) const;

private:
	std::string source_path_;

	// Store only concrete primary weapons; random choices and unsupported types
	// must not resolve to empty tuning records.
	static constexpr size_t WEAPON_COUNT =
		static_cast<size_t>(WeaponType::bucket) + 1;
	static constexpr size_t PROJECTILE_COUNT =
		static_cast<size_t>(BALL) + 1;

	MatchTuning match_;
	PaintTuning paint_;
	PlayerTuning player_;

	float ammo_regen_delay_ = 0.0f;
	float ammo_regen_rate_ = 0.0f;
	float starting_ammo_ = 1.0f;

	std::array<WeaponTuning, WEAPON_COUNT> weapons_{};
	std::array<ProjectileTuning, PROJECTILE_COUNT> projectiles_{};

	// Distinguish absent records from valid records containing zero values.
	std::array<bool, WEAPON_COUNT> weapon_tuned_{};
	std::array<bool, PROJECTILE_COUNT> projectile_tuned_{};
};
