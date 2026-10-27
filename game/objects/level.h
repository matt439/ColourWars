#pragma once

#include "game/play_settings.h"
#include "game/presentation.h"

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
#include "game/objects/level_overlays.h"
#include "game/objects/level_station.h"
#include "engine/ui/widget.h"
#include "game/tuning.h"
#include "game/objects/seat_input_filter.h"
#include "game/objects/monitor.h"
#include "game/objects/opening_mode.h"
#include "game/objects/silhouette.h"
#include "game/objects/screen_cursor.h"
#include "game/objects/figure_acting.h"
#include "game/objects/paint_audio.h"
#include "game/objects/idle_drip.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/player_badge.h"
#include "game/objects/countdown_art.h"
#include "game/objects/reveal_art.h"
#include "game/objects/music_envelope.h"
#include "game/states/menu_level_settings.h"
#include <cstdint>
#include <string_view>

class MatchBotDriver;
struct MatchBotSupport;
struct MatchBotRamp;

enum class LevelState
{
	start_countdown,
	active,
	zoom_out,
	overview,
	finished,
	// Before the countdown. Last, so the values above keep their numbers.
	opening,
};


// One paint match: countdown, clock, cameras, HUDs and music.
// Scene owns and steps the objects; Level keeps borrowed player pointers
// and supplies the views and game-specific overlays.
struct InputBindings;
class TranslatingStructurePaintable;

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
		const std::wstring& stage_name,
		const std::string& sound_bank_name,
		const std::string& music_name,
		float music_volume,
		const std::string& end_sting_name,
		float end_sting_volume,
		const labrador::ResolutionManager* resolution_manager,
		const labrador::ViewportManager* viewport_manager,
		labrador::RenderResources* render_resources,
		const labrador::AudioResources* audio_resources,
		const PlaySettings* preferences,
		const labrador::Mouse* mouse,
		std::vector<LevelStation> stations,
		OpeningMode opening, std::optional<float> duration = std::nullopt,
		const InputBindings* input_bindings = nullptr,
		const MenuLevelSettings* roster = nullptr, std::uint64_t bot_seed = 0);

	// Stop the level's voices and player weapon/redraw loops.
	~Level();

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;

	// Hard-pause level audio. GameLevel may resume just the ducked arena music
	// under normal pause menus. Weapon loops retain their playback position.
	// A suspend also resets every player's jump input, so a jump held across
	// the menu is not taken as a new press.
	void suspend() const;
	void resume();
	// The top state supplies one active real-time tick, even under pause pages.
	// Simulation, replay steps and drawing never advance these music ramps.
	void update_music(float dt);
	void set_music_ducked(bool ducked);
	void resume_music() const;
	// Complete the count-in without a simulation tick. Neutral jump input
	// rearms the next press; old fire loops must not return before a new shot.
	void prepare_resume(const std::vector<PlayerInputData>& inputs);
	// Pauses the same voices without touching any player: for a frozen
	// simulation that no menu covers (the Debug clock's pause).
	void pause_audio() const;

	// `seats`, when given, sees and may replace every participant's final input
	// after the shipping bots (Debug bots/record/play); null in Release.
	void update(const std::vector<PlayerInputData>& player_inputs, float dt,
		SeatInputFilter* seats = nullptr);
	// Feedback intents, retaining each Player's physical slot. Drain once after
	// a completed tick, including blocked bot/replay/keyboard events; this never
	// changes simulation/RNG state. A lifecycle stop may explicitly discard them.
	std::vector<SlotRumbleEvents> take_rumble_events();
	void clear_rumble_events() noexcept;

	// Refresh size-dependent presentation before rendering, even when a menu
	// covers the match. Called on the main thread, before render workers start.
	void refresh_layout();
	// Snapshot the shell's measured interval on the main thread, before view
	// workers start. Drawing reads this value and never advances its clock.
	void set_debug_frame_time(double frame_ms);

	// Read-only rendering; scene objects and overlays may be drawn concurrently.
	void draw(labrador::Renderer& renderer) const;
	// Shared paper signal over the frozen panes; this never advances the world.
	void draw_resume_countdown(labrador::Renderer& renderer, float remaining) const;

	LevelState state() const;
	// The countdown is over and the players move.
	bool match_started() const;
	// The intro is on screen: A, B or Start skips it rather than pausing.
	bool intro_playing() const;
	// The match ends on the person's monitor, which shows the winner and the
	// coverage; the results page then keeps only the percentages.
	bool revealed() const;

	// Read-only participants for developer status; the index is the stable
	// roster seat, independent of physical slot and human pane.
	int player_count() const;
	const Player& player(int view) const;
	// Participant indices are stable roster seats; views contain only humans.
	int view_count() const;
	int participant_for_view(int view) const;
	int view_for_participant(int participant) const;
	bool is_bot(int participant) const;
	BotDifficulty bot_difficulty(int participant) const;
	std::wstring_view bot_identity(int participant) const;
	// Conservative current structural bounds for bot planning and sight. No
	// projectile/player state is exposed by these geometry queries.
	std::vector<mattmath::RectangleF> bot_obstacles() const;
	std::vector<MatchBotSupport> bot_supports() const;
	std::vector<MatchBotRamp> bot_ramps() const;
	bool bot_line_of_sight(mattmath::Vector2F from, mattmath::Vector2F to) const;
	// Cosmetic camera by participant; player(index).camera() remains the stable aim/follow camera.
	labrador::Camera view_camera(int view) const;
	// Null for tools without an idle drip. Index follows player(view).
	const IdleDrip* idle_drip(int view) const;
	// A local cue only during live play; frozen pause draws preserve its state.
	bool aim_marker_visible(int view) const;
	bool player_identity_visible(int view) const;
	float player_identity_emphasis(int view) const;
	mattmath::RectangleF player_identity_bounds(int view) const;
	// Reacquire one's own figure after a covering menu or focus loss.
	void emphasise_players();
	const BlockedShotFeedback& blocked_shot_feedback() const;

	LevelEndInfo level_end_info() const;
	// Exact scoring tiles in Scene paintable-role order, then per-source tile
	// order. Faces overlap independently at corners; do not union their areas.
	// Source indices are snapshot-local, not authored object names or replay IDs.
	std::vector<PaintSurfaceSample> paint_surfaces() const;
    bool has_horizontal_motion() const;
    std::vector<HorizontalStructureState> horizontal_structure_states() const;

	// Developer tools. Present in every build; only Debug builds call them,
	// between world updates. `view` indexes players_ (see below).
	void set_overlays(const LevelOverlays& overlays);
	void set_players_showing_debug(bool showing);
	void set_player_cheats(bool damage_immune, bool infinite_ammo);
	// At rest at `top_left`, respawning first if dead; the weapon and the
	// cameras follow at once, so the move shows even while the clock is
	// paused. False: no such view.
	bool place_player(int view, const mattmath::Vector2F& top_left);
	bool respawn_player(int view);
	// Dies on the next world update, at the spawn's redraw. False: no such view.
	bool kill_player(int view);
	// What each view's HUD names instead of its device ("Bot: paint");
	// empty entries, or no list, keep the device name.
	void set_seat_labels(std::vector<std::wstring> labels);
	// The countdown's opening cards in window pixels while they are drawn,
	// else none: what a developer panel must not cover.
	std::vector<mattmath::RectangleF> opening_cards() const;
	std::wstring_view stage_entry_name() const;
	mattmath::RectangleF stage_entry_bounds() const;
	mattmath::RectangleF stage_entry_text_bounds() const;
	std::wstring_view keyboard_controls_text() const;
	mattmath::RectangleF keyboard_controls_text_bounds() const;
	mattmath::RectangleF keyboard_controls_card_bounds() const;
	std::wstring_view gamepad_controls_text() const;
	mattmath::RectangleF gamepad_controls_text_bounds() const;
	mattmath::RectangleF gamepad_controls_card_bounds() const;
	// overlay.stations as the scene holds it: one UiText per station, in
	// LevelLoadedInfo::stations() order, made the first time the overlay is
	// turned on and only hidden after that. None until then, so none in Release.
	int station_label_count() const;
	const labrador::UiText& station_label(int index) const;
	// Counts for the perf overlay, as of the last update.
	int projectile_count() const;
	int collision_object_count() const;

private:
	std::unique_ptr<PaintAudio> paint_audio_;
	std::unique_ptr<BlockedShotFeedback> blocked_shot_feedback_;
	std::unique_ptr<PlayerBadge> player_badge_;
	std::vector<float> player_identity_remaining_;
    std::unique_ptr<labrador::Scene> scene_ = nullptr;
    // Admitted fixed movers explicitly refuse retirement. These borrows live
    // for this Level; no terrain add/remove API is exposed after admission.
    std::vector<TranslatingStructurePaintable*> horizontal_structures_;
    std::vector<HorizontalSupportRef> horizontal_supports_;
    struct HorizontalFrame
    {
        std::vector<HorizontalPreparedStep> motions;
        std::vector<std::optional<HorizontalCarryStep>> carriers;
    };
    void admit_horizontal_structures();
    void validate_horizontal_admission() const;
    void validate_horizontal_budget(const HorizontalFrame* prepared) const;
    HorizontalFrame prepare_horizontal_frame(float dt) const;
    void commit_horizontal_frame(const HorizontalFrame& prepared) noexcept;
    void publish_horizontal_zero() noexcept;

	// Borrowed from scene_. Players are never retired: death leads to respawn,
	// so these pointers remain valid for the match.
	// The vector index is the participant's stable roster seat, independent of
	// their human view or physical input slot (player_num()).
	std::vector<Player*> players_;
	std::vector<MenuPlayerSettings> roster_;
	std::vector<int> view_participants_;
	std::vector<int> participant_views_;
	std::unique_ptr<MatchBotDriver> bot_driver_;
	// Immutable labels and metrics shared safely by every human pane.
	std::vector<std::unique_ptr<labrador::Text>> bot_identity_texts_;
	// Scene-owned cosmetics, advanced after final player contacts and muzzle repair.
	std::vector<IdleDrip*> idle_drips_;

	// What overlay.stations names: the authored stations, then the spawns.
	std::vector<LevelStation> stations_;
	// Borrowed from scene_, one per stations_ entry once shown. The scene
	// never retires an object that is not a collision object, so these live
	// for the match; hiding them is UiText::set_hidden, which keeps their
	// words, box and colour.
	std::vector<labrador::UiText*> station_labels_;

	// Scene-owned geometry and players, which live for the whole match. Flying
	// paint sweeps this list so its cost does not grow with other projectiles.
	std::vector<labrador::CollisionObject*> projectile_targets_;

	// Screen-space dividers owned outside Scene and drawn by each pane's overlay.
	std::vector<std::unique_ptr<labrador::GameObject>> viewport_dividers_;

	labrador::SoundBank* sound_bank_ = nullptr;
	const PlaySettings* preferences_ = nullptr;

	// Resolve the level's music and fixed match sounds once at construction.
	labrador::SoundBank::EffectHandle music_;
	labrador::SoundBank::EffectHandle end_sting_;
	labrador::SoundBank::EffectHandle zoom_out_sound_;
	labrador::SoundBank::EffectHandle countdown_sound_;
	labrador::SoundBank::EffectHandle go_sound_;
	labrador::SoundBank::EffectHandle time_up_sound_;
	labrador::SoundBank::EffectHandle last_ten_sound_;
	// The displayed positive second most recently sounded; zero means none yet.
	float last_ten_second_ = 0.0f;
	float music_volume_ = 0.0f;
	float end_sting_volume_ = 0.0f;
	MusicEnvelope music_fade_;
	MusicEnvelope music_duck_{ 1.0f };
	bool music_ending_ = false;

	// Simulation and rendering have independent clocks; both snapshots have
	// identical layout in Debug and Release and are read-only during fan-out.
	float simulation_dt_ = 0.0f;
	double debug_frame_ms_ = 0.0;

	// Cached once per tick for all debug overlays.
	int projectile_count_ = 0;

	std::unique_ptr<CountdownArt> countdown_art_;
	std::wstring countdown_label_;
	mattmath::RectangleF countdown_number_box_;
	std::unique_ptr<labrador::TextDropShadow> countdown_heading_ = nullptr;
	std::unique_ptr<labrador::TextDropShadow> resume_heading_;
	mattmath::RectangleF countdown_card_;
	std::unique_ptr<labrador::TextDropShadow> stage_entry_;
	mattmath::RectangleF stage_entry_card_;
	mattmath::RectangleF stage_entry_text_bounds_;

	// One shared reminder per input device type present in the match.
	struct ControlsReminder
	{
		std::unique_ptr<labrador::TextDropShadow> heading;
		std::unique_ptr<labrador::TextDropShadow> text;
		mattmath::RectangleF card;
		mattmath::RectangleF text_bounds;
	};
	std::vector<ControlsReminder> controls_reminders_;
	// Save owns the effective binding; a paused draw refreshes only its cached
	// words and fitting, retaining the text widget's original font and copy.
	const InputBindings* input_bindings_ = nullptr;
	labrador::TextDropShadow* keyboard_controls_ = nullptr;
	std::wstring keyboard_jump_label_;
	labrador::TextDropShadow* gamepad_controls_ = nullptr;
	std::vector<int> gamepad_slots_;
	std::array<PadProfile, input_slots::pad_count> gamepad_pads_{};
	struct ControlsReminderLook
	{
		mattmath::Vector2F reference_size, margin, padding, heading_shadow, text_shadow;
		float gap = 0.0f, y = 0.0f;
	} controls_look_;
	void refresh_keyboard_controls();
	const Presentation* presentation_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SpriteSheetHandle overlay_sheet_;
	labrador::SpriteSheet::frame_handle overlay_fill_;

	PlayerCamera player_camera_;

	std::unique_ptr<InterfaceGameplay> interface_gameplay_ = nullptr;
	std::unique_ptr<DebugText> debug_text_ = nullptr;
	LevelOverlays overlays_{};

	// By participant; see set_seat_labels. Present in every build, only ever set by
	// the Debug director, so empty in Release.
	std::vector<std::wstring> seat_labels_;
	// By participant: each HUD identity box's whole text. Built from the seat, the
	// player's team and pad slot (all fixed for the match) and seat_labels_,
	// so only construction and set_seat_labels rebuild it; draws only read it.
	std::vector<std::wstring> identity_texts_;

	// Borrowed shell services; both outlive every level.
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const labrador::ViewportManager* viewport_manager_ = nullptr;

	// Borrowed mouse state for camera-dependent aiming; see resolve_mouse_aim.
	const labrador::Mouse* mouse_ = nullptr;

	// Initialized from match tuning in the constructor.
	float timer_ = 0.0f;
	float start_timer_ = 0.0f;
	bool countdown_started_ = false;
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

	// The opening (level_intro.cpp). The monitor, the person and the cursor
	// exist only when the match has an intro.
	struct OpeningBeat
	{
		bool previewing_a = false;
		bool previewing_b = false;
		bool picker = false;
		float cursor_at_a = 0.0f;
		float cursor_at_b = 0.0f;
		float cursor_back = 0.0f;
		SilhouettePose pose;
	};
	float opening_time_ = 0.0f;
	// Room cues are owned voices so a paused or skipped opening cannot leave
	// its person making a noise after the silhouette has stopped or gone.
	labrador::SoundBank::EffectHandle sigh_sound_;
	labrador::SoundBank::EffectHandle chair_sound_;
	labrador::SoundBank::EffectHandle room_tone_sound_;
	bool room_tone_playing_ = false;
	labrador::SoundBank::EffectHandle glass_in_sound_;
	labrador::SoundBank::EffectHandle celebrate_sound_;
	labrador::SoundBank::EffectHandle slump_sound_;
	labrador::SoundBank::EffectHandle colour_in_sound_;
	labrador::SoundBank::EffectHandle skip_sound_;
	labrador::SoundBank::EffectHandle undo_sound_;
	// The countdown plays in the picture and ends by pushing through the glass.
	bool intro_countdown_ = false;
	bool glass_in_started_ = false;
	std::unique_ptr<Monitor> monitor_;
	std::unique_ptr<Silhouette> silhouette_;
	std::unique_ptr<ScreenCursor> cursor_;
	std::unique_ptr<FigureActing> acting_;
	// By view: what each figure is doing, and for how long.
	std::vector<FigureAct> acts_;
	float act_time_ = 0.0f;
	// What of the world the picture shows, and where its two halves meet.
	mattmath::RectangleF picture_bounds_;
	float picture_middle_ = 0.0f;

	// The reveal (level_intro.cpp): the same monitor and person at the end.
	struct RevealBeat
	{
		float cursor_at_a = 0.0f;
		float cursor_at_b = 0.0f;
		bool showing_a = false;
		bool showing_b = false;
		bool picker = false;
		float waver = 0.0f;
		bool clicked = false;
		SilhouettePose pose;
	};
	float reveal_time_ = 0.0f;
	LevelEndInfo end_info_;
	PlayerTeam winner_ = PlayerTeam::none;
	labrador::SoundBank::WaveHandle hover_sound_;
	labrador::SoundBank::WaveHandle click_sound_;
	labrador::SoundBank::WaveHandle close_sound_;
	std::unique_ptr<RevealArt> reveal_art_;
	// Capture the wrappers alongside RevealArt's percent font, so replacement
	// presentation text never draws against the preceding font snapshot.
	MenuResultsText reveal_text_;

	RevealBeat reveal_beat() const;
	float reveal_length() const;
	bool reveal_showing() const;
	// How far the view is through the glass: 0 at the monitor, 1 in the panes.
	float glass_push() const;
	void start_reveal();
	void update_reveal(float dt);
	void draw_reveal(labrador::DrawList& front, const mattmath::RectangleF& screen) const;
	// Chooses each figure's act for this moment and poses it.
	void update_acting(float dt);
	void draw_acting(labrador::DrawList& list, int view) const;

	OpeningBeat opening_beat() const;
	float opening_length() const;
	bool intro_showing() const;
	float push_ratio() const;
	mattmath::RectangleF intro_screen() const;
	bool skip_requested(const std::vector<PlayerInputData>& player_inputs) const;
	void colour_players(bool team_a, bool team_b);
	void skip_intro();
	void set_room_tone(bool playing);
	void update_opening(const std::vector<PlayerInputData>& player_inputs, float dt);
	void add_intro_views();
	void draw_intro(labrador::Renderer& renderer) const;

	// Advance the match state; update() refreshes views after every path returns.
	void update_state(const std::vector<PlayerInputData>& player_inputs,
		float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal = nullptr);

	void update_level_logic(const std::vector<PlayerInputData>& player_inputs,
		float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal = nullptr);

	// Apply input by each player's device slot, not by their position in
	// players_, then let the bot driver and `seats` (if any) replace it per participant.
	void apply_player_inputs(
		const std::vector<PlayerInputData>& player_inputs, SeatInputFilter* seats);

	// Fill keyboard/mouse aim using this player's camera and human pane. view_index
	// is the human pane, independent of participant and physical slot.
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
	void update_idle_drips(float dt);

	// One pane per human during the match, one fullscreen view afterward.
	// Refresh after updates and after resize, before render workers start.
	void refresh_views();

	// The HUD, the dividers and the debug text, over one pane.
	// Runs on the worker that owns that view.
	void draw_player_overlay(int view_index, labrador::DrawList& list) const;
	void update_player_identity(float dt);
	// Rebuilds identity_texts_. Main thread, never from a draw.
	void refresh_identity_texts();
	// Makes the labels on first use, then applies overlays_.stations and the
	// current presentation (so tune is live). Main thread, between updates.
	void refresh_station_labels();

	void draw_countdown_text(labrador::DrawList& draw_list) const;
	// POL#39: the marquee around the mouse pointer, in window pixels.
	bool cursor_marker_visible() const;
	void draw_cursor_marker(labrador::DrawList& draw_list) const;
	void draw_opening_card(labrador::DrawList& draw_list,
		const mattmath::RectangleF& rectangle, float scale) const;
	float opening_scale() const;

	// Sets the displayed beat; drawing derives its age from the match clock.
	void set_countdown_text(const std::wstring& text);
	void centre_countdown_text();
	void centre_stage_entry();
	void centre_controls_text();

	float zoom_out_camera_ratio() const;

	void stop_player_sounds() const;
	void pause_player_sounds() const;
	void resume_player_sounds() const;

	int count_projectiles() const;

	// Build countdown text and cameras before the first frame is drawn.
	void build_opening_frame(labrador::RenderResources* render_resources);
};
