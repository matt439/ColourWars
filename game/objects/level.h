#pragma once

#include "game/play_settings.h"

#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/team_colour.h"
#include "game/objects/player_input.h"
#include "game/objects/player.h"
#include "engine/render/resolution_manager.h"
#include "engine/input/mouse.h"
#include "engine/render/viewport_manager.h"
#include "game/objects/debug_text.h"
#include "game/objects/player_camera.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/level_end_info.h"
#include "engine/render/text_drop_shadow.h"
#include "engine/audio/sound_bank.h"
#include "game/objects/paintable_object.h"
#include "engine/render/render_resources.h"
#include "engine/audio/audio_resources.h"
#include "engine/scene/scene.h"
#include "engine/render/colour.h"
#include "engine/render/camera.h"
#include "game/tuning.h"

enum class LevelState
{
	start_countdown,
	active,
	zoom_out,
	overview,
	finished,
};

// One paint match: countdown, clock, cameras, HUDs and music.
// Scene owns and steps the objects; Level keeps borrowed player pointers
// and supplies the views and game-specific overlays.
class Level
{
public:
	Level(std::unique_ptr<labrador::Scene> scene,
		std::vector<Player*> players,
		std::vector<std::unique_ptr<labrador::GameObject>> viewport_dividers,
		const TeamColour& team_colours,
		const mattmath::RectangleF& camera_bounds,
		const mattmath::RectangleF& zoom_out_start_bounds,
		const mattmath::RectangleF& zoom_out_finish_bounds,
		const GameTuning* tuning,
		const Presentation* presentation,
		const std::string& sound_bank_name,
		const std::string& music_name,
		float music_volume,
		const labrador::ResolutionManager* resolution_manager,
		const labrador::ViewportManager* viewport_manager,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::Mouse* mouse);

	// Stop the level's music, transition sound and player weapon loops.
	~Level();

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;

	// Pause and resume level audio when a menu covers the match. Weapon loops
	// retain their playback position while gameplay updates are suspended.
	void suspend() const;
	void resume() const;

	void update(const std::vector<PlayerInputData>& player_inputs, float dt);

	// Refresh size-dependent presentation before rendering, even when a menu
	// covers the match. Called on the main thread, before render workers start.
	void refresh_layout();

	// Read-only rendering; scene objects and overlays may be drawn concurrently.
	void draw(labrador::Renderer& renderer) const;

	LevelState state() const;

	LevelEndInfo level_end_info() const;

private:
	std::unique_ptr<labrador::Scene> scene_ = nullptr;

	// Borrowed from scene_. Players are never retired: death leads to respawn,
	// so these pointers remain valid for the match.
	// The vector index is the player's view index, not their input slot (player_num()).
	std::vector<Player*> players_;

	// Scene-owned geometry and players, which live for the whole match. Flying
	// paint sweeps this list so its cost does not grow with other projectiles.
	std::vector<labrador::CollisionObject*> projectile_targets_;

	// Screen-space dividers owned outside Scene and drawn by each pane's overlay.
	std::vector<std::unique_ptr<labrador::GameObject>> viewport_dividers_;

	labrador::SoundBank* sound_bank_ = nullptr;
	const PlaySettings* preferences_ = nullptr;

	// Resolve the level's music and fixed match sounds once at construction.
	labrador::SoundBank::EffectHandle music_;
	labrador::SoundBank::EffectHandle zoom_out_sound_;
	labrador::SoundBank::WaveHandle countdown_sound_;
	float music_volume_ = 0.0f;

	// Written during the main-thread update and read by render workers for dt/fps.
	float frame_dt_ = 0.0f;

	// Cached once per tick for all debug overlays.
	int projectile_count_ = 0;

	std::unique_ptr<labrador::TextDropShadow> countdown_text_ = nullptr;
	std::unique_ptr<labrador::TextDropShadow> countdown_heading_ = nullptr;
	mattmath::RectangleF countdown_card_;

	// One shared reminder per input device type present in the match.
	struct ControlsReminder
	{
		std::unique_ptr<labrador::TextDropShadow> heading;
		std::unique_ptr<labrador::TextDropShadow> text;
		mattmath::RectangleF card;
	};
	std::vector<ControlsReminder> controls_reminders_;
	const Presentation* presentation_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SpriteSheetHandle overlay_sheet_;
	labrador::SpriteSheet::frame_handle overlay_fill_;

	PlayerCamera player_camera_;

	std::unique_ptr<InterfaceGameplay> interface_gameplay_ = nullptr;
	std::unique_ptr<DebugText> debug_text_ = nullptr;

	// Borrowed shell services; both outlive every level.
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const labrador::ViewportManager* viewport_manager_ = nullptr;

	// Borrowed mouse state for camera-dependent aiming; see resolve_mouse_aim.
	const labrador::Mouse* mouse_ = nullptr;

	// Initialized from match tuning in the constructor.
	float timer_ = 0.0f;
	float start_timer_ = 0.0f;
	float zoom_out_timer_ = 0.0f;
	float overview_timer_ = 0.0f;

	labrador::Camera zoom_out_camera_ = labrador::Camera::DEFAULT_CAMERA;

	TeamColour team_colours_ = TeamColour();

	// Borrowed, never null: match durations, countdown presentation and sounds.
	const GameTuning* tuning_ = nullptr;

	mattmath::RectangleF camera_bounds_ = mattmath::RectangleF::ZERO;

	// Authored world bounds survive a resize; the cameras are refitted whenever
	// the actual screen size changes, including during the results overview.
	mattmath::RectangleF zoom_out_start_bounds_;
	mattmath::RectangleF zoom_out_finish_bounds_;
	mattmath::Vector2F layout_size_ = mattmath::Vector2F::ZERO;
	labrador::Camera zoom_out_start_camera_ = labrador::Camera::DEFAULT_CAMERA;
	labrador::Camera zoom_out_finish_camera_ = labrador::Camera::DEFAULT_CAMERA;

	LevelState state_ = LevelState::start_countdown;

	// Advance the match state; update() refreshes views after every path returns.
	void update_state(const std::vector<PlayerInputData>& player_inputs,
		float dt);

	void update_level_logic(const std::vector<PlayerInputData>& player_inputs,
		float dt);

	// Apply input by each player's device slot, not by their position in players_.
	void apply_player_inputs(
		const std::vector<PlayerInputData>& player_inputs);

	// Fill keyboard/mouse aim using this player's camera and pane. view_index
	// is the position in players_, not the device slot returned by player_num().
	// The camera uses positive uniform scale, so normalizing the screen-space
	// direction gives the same aim direction as normalizing in world space.
	void resolve_mouse_aim(Player* player, size_t view_index,
		PlayerInputData& input) const;

	// Update player cameras and submit projectiles produced by their weapons.
	void update_players(float dt);
	void resolve_projectile_movement();
	void refresh_player_cameras();

	// After collision resolution, align each weapon with its corrected player position.
	void update_players_after_resolve();

	// One pane per player during the match, one fullscreen view afterward.
	// Refresh after updates and after resize, before render workers start.
	void refresh_views();

	// The HUD, the dividers and the debug text, over one pane.
	// Runs on the worker that owns that view.
	void draw_player_overlay(int view_index, labrador::DrawList& list) const;

	void draw_countdown_text(labrador::DrawList& draw_list) const;
	void draw_opening_card(labrador::DrawList& draw_list,
		const mattmath::RectangleF& rectangle, float scale) const;
	float opening_scale() const;

	// Sets the beat, then re-centres on what it actually measures.
	void set_countdown_text(const std::wstring& text);
	void centre_countdown_text();
	void centre_controls_text();

	float zoom_out_camera_ratio() const;

	void stop_player_sounds() const;
	void pause_player_sounds() const;
	void resume_player_sounds() const;

	int count_projectiles() const;

	// Build countdown text and cameras before the first frame is drawn.
	void build_opening_frame(labrador::RenderResources* render_resources);
};
