#pragma once

#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/team_colour.h"
#include "game/objects/player_input.h"
#include "game/objects/player.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"
#include "game/objects/debug_text.h"
#include "engine/render/camera_tools.h"
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

enum class LevelState
{
	start_countdown,
	active,
	zoom_out,
	overview,
	finished,
};

namespace level_consts
{
	constexpr float TIMER = 240.0f;
	constexpr float START_TIMER = 3.0f;
	const static std::wstring COUNTDOWN_TEXT = L"3";
	const static std::string COUNTDOWN_FONT_NAME = "gill_sans_mt_bold_144";
	const static artattack::Colour COUNTDOWN_COLOUR = artattack::Colour::dark_gray;
	const static artattack::Colour COUNTDOWN_SHADOW_COLOUR = artattack::Colour::black;
	constexpr float COUNTDOWN_SCALE = 2.0f;
	const static mattmath::Vector2F COUNTDOWN_SHADOW_OFFSET = { 5.0f, 5.0f };
	constexpr float COUNTDOWN_TEXT_WIDTH = 400.0f;
	constexpr float COUNTDOWN_TEXT_HEIGHT = 600.0f;

	constexpr float ZOOM_OUT_TIMER = 2.0f;
	constexpr float OVERVIEW_TIMER = 2.0f;

	const std::string COUNTDOWN_SOUND = "smash_countdown";
	constexpr float COUNTDOWN_SOUND_VOLUME = 1.0f;

	const std::string ZOOM_OUT_SOUND = "slide_whistle_up";
	constexpr float ZOOM_OUT_SOUND_VOLUME = 0.5f;
}

// One paint match: a countdown, a clock, a track, four HUDs and the two
// cameras a match is watched through.
//
// What is no longer here is everything that was not about a paint match. The
// object lists, the loops that stepped them, the collision sweep, the
// out-of-bounds retirement, the swap-and-pop compaction and the per-view
// render fan-out are an artattack::Scene now (engine/scene/scene.h) - eleven
// members and nine methods, none of which mentioned paint. This class keeps
// what only this game knows: how a camera follows a player, when a countdown
// beats, what a HUD says, which track plays and when it stops.
//
// It talks to the scene through pointers to the players it kept and the view
// list it fills. The scene owns the objects, including the players; `players_`
// below is a typed, non-owning way back to the ones this class has questions
// for.
class Level
{
public:
	// No default constructor. One never had a sound bank or a scene, and the
	// destructor below speaks to both.
	//
	// The 21 positional parameters this used to take are twelve: eleven members
	// went to the scene, two spawn lists were written and never read, and the
	// thread pool and the partitioner are the scene's fan-out rather than
	// anything a match knows about.
	Level(std::unique_ptr<artattack::Scene> scene,
		std::vector<Player*> players,
		std::vector<std::unique_ptr<artattack::GameObject>> viewport_dividers,
		const TeamColour& team_colours,
		const mattmath::RectangleF& camera_bounds,
		const mattmath::RectangleF& zoom_out_start_bounds,
		const mattmath::RectangleF& zoom_out_finish_bounds,
		const std::string& sound_bank_name,
		const std::string& music_name,
		float music_volume,
		const artattack::ResolutionManager* resolution_manager,
		const artattack::ViewportManager* viewport_manager,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources);

	// Silences everything this level started.
	//
	// It replaces stop_music(), which was public and called by hand at seven
	// GameLevel exits - so the level's music was as long-lived as whoever
	// remembered, and a restart path that forgot left two tracks over each
	// other. The track belongs to the match; this is where a match ends.
	~Level();

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;

	// A menu went up over the match, or came back down.
	//
	// Not the same as stopping: a paused weapon loop resumes mid-voice where a
	// stopped one has to be restarted from the top. Both walk the players
	// because a looping fire sound belongs to a weapon, and Level::update is
	// the only thing that ever ends one - which is why "pause" that only
	// skipped update() left a sustained tone playing under the menu.
	void suspend() const;
	void resume() const;

	void update(const std::vector<PlayerInputData>& player_inputs, float dt);

	// A pure read, all the way down. Three caller obligations went with the
	// three parameters this used to take - pre-size a vector, pre-fill it with
	// null, and Release every non-null entry - and all three are the
	// renderer's now.
	void draw(artattack::Renderer& renderer) const;

	LevelState state() const;

	LevelEndInfo level_end_info() const;

private:
	// What is in the world, where it is seen from, and the tick that steps it.
	std::unique_ptr<artattack::Scene> scene_ = nullptr;

	// The players, borrowed from the scene that owns them.
	//
	// They outlive every tick of the match, and the match ends with the scene,
	// because a Player is never retired: for_deletion() returns false and
	// set_for_deletion() is not implemented, so neither the bounds sweep nor a
	// contact response can take one out from under this vector. That is what
	// these pointers rest on, and it is stated at Player::for_deletion too.
	//
	// The index into this vector is the index of that player's view in the
	// scene's view list, which is what lets an overlay find the player whose
	// pane it is drawing. It is *not* the player's pad slot: player_num() is,
	// and the two stop agreeing the moment a pad is unplugged.
	std::vector<Player*> players_;

	// Screen-space, per pane, drawn over the world rather than in it - so they
	// are not scene objects: nothing culls them, nothing sweeps them, and they
	// want the pane's camera rather than the player's. The overlay draws them.
	std::vector<std::unique_ptr<artattack::GameObject>> viewport_dividers_;

	artattack::SoundBank* sound_bank_ = nullptr;

	// The level's own music comes from its definition file, so its name is not
	// known until here - but it is known once, at construction, which is where
	// it stops being a name. Same for the two fixed sounds the level plays.
	artattack::SoundBank::EffectHandle music_;
	artattack::SoundBank::EffectHandle zoom_out_sound_;
	artattack::SoundBank::WaveHandle countdown_sound_;
	float music_volume_ = 0.0f;

	// This frame's dt, for the debug overlay's dt/fps readout - the only thing
	// on the draw path that wants frame timing. Written once in update() and
	// read by the render workers, which is the same shape as every other
	// member they read: written on the single-threaded tick, never during the
	// draw. It replaces a const float* handed in at construction, so what used
	// to be a pointer that device loss could invalidate is now a value.
	float frame_dt_ = 0.0f;

	// Also for the debug overlay, and also written once per tick. It used to be
	// count_projectiles(), called from inside the per-view draw - so a
	// four-player match rescanned every collision object in the level four
	// times per frame for one line of text, on four threads at once.
	int projectile_count_ = 0;

	std::unique_ptr<artattack::TextDropShadow> countdown_text_ = nullptr;

	// A value, not a unique_ptr. CameraTools holds no state at all - one const
	// method and one static - so the heap allocation it used to get was a
	// separate cache line and a null to check, bought for nothing.
	artattack::CameraTools camera_tools_;

	std::unique_ptr<InterfaceGameplay> interface_gameplay_ = nullptr;
	std::unique_ptr<DebugText> debug_text_ = nullptr;

	// The three borrowed pointers left of the seven this class held. Both are
	// owned by the shell and outlive every level: the resolution for the
	// countdown's placement and the zoom-out camera's aspect, the layout for
	// which pane a player is in. RenderResources was the fourth and is a
	// constructor parameter now - the three things that resolve a handle
	// against it are all built there.
	const artattack::ResolutionManager* resolution_manager_ = nullptr;
	const artattack::ViewportManager* viewport_manager_ = nullptr;

	float timer_ = level_consts::TIMER;
	float start_timer_ = level_consts::START_TIMER;
	float zoom_out_timer_ = level_consts::ZOOM_OUT_TIMER;
	float overview_timer_ = level_consts::OVERVIEW_TIMER;

	artattack::Camera zoom_out_camera_ = artattack::Camera::DEFAULT_CAMERA;

	TeamColour team_colours_ = TeamColour();

	mattmath::RectangleF camera_bounds_ = mattmath::RectangleF::ZERO;
	mattmath::RectangleF zoom_out_start_bounds_ = mattmath::RectangleF::ZERO;
	mattmath::RectangleF zoom_out_finish_bounds_ = mattmath::RectangleF::ZERO;

	LevelState state_ = LevelState::start_countdown;

	// The match's own state machine: the countdown, the clock, the zoom-out and
	// the overview. Separate from update() only so that refresh_views() runs on
	// every path through it, including the two that return early.
	void update_state(const std::vector<PlayerInputData>& player_inputs,
		float dt);

	// Not const, and it never should have been: it moves every object in the
	// level. The declaration used to survive only because the members it
	// wrote through were pointers, and a const method may write through a
	// pointer it owns.
	void update_level_logic(const std::vector<PlayerInputData>& player_inputs,
		float dt);

	// Each player reads the pad in its own slot, before anything steps.
	//
	// Indexed by the player's own pad slot, not by position in the loop.
	// player_inputs is one entry per slot; a running counter only happened to
	// agree while every pad stayed connected.
	void apply_player_inputs(
		const std::vector<PlayerInputData>& player_inputs);

	// What a player's tick needs that "step and collide" does not cover: the
	// camera that follows the body the scene has just moved, and the
	// projectiles the weapon just fired, which go to the scene.
	void update_players(float dt);

	// The scene's phase 3, run here rather than named on GameObject - see
	// engine/scene/scene.h for why a virtual with an empty body on five
	// thousand paint tiles is not the answer. Not const: it moves the weapon
	// the body a contact just pushed is holding.
	void update_players_after_resolve();

	// Says where this frame is watched from: one pane per player while the
	// match is running, one fullscreen view once it is over. Called at the end
	// of every update() and once at construction, because draw() is a pure
	// read and a camera that follows a player is chosen where the player moved.
	void refresh_views();

	// The HUD, the dividers, the debug text and the countdown, over one pane.
	// Runs on the worker that owns that view.
	void draw_player_overlay(int view_index, artattack::DrawList& list) const;

	void draw_countdown_text(artattack::DrawList& draw_list) const;

	float zoom_out_camera_ratio() const;

	void stop_player_sounds() const;
	void pause_player_sounds() const;
	void resume_player_sounds() const;

	int count_projectiles() const;

	// Positions the countdown text and gives every player the camera their
	// first frame is drawn through. Both used to happen on the first update(),
	// which is why GameLevel had two states whose whole job was to not draw
	// through a null pointer and a default camera.
	void build_opening_frame(artattack::RenderResources* render_resources);
};
