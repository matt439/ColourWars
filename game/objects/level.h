#pragma once

#include "game/play_settings.h"
#include "game/presentation.h"

#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/team_colour.h"
#include "game/objects/player_input.h"
#include "game/objects/player.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"
#include "game/objects/debug_text.h"
#include "game/objects/player_camera.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/level_end_info.h"
#include "game/carried/text_drop_shadow.h"
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
#include "game/objects/match_voices.h"
#include "game/carried/sound_bank_object.h"
#include "game/objects/idle_drip.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/player_badge.h"
#include "game/objects/teammate_marker.h"
#include "game/objects/countdown_art.h"
#include "game/objects/reveal_art.h"
#include "game/objects/music_envelope.h"
#include "game/match_setup.h"
#include <cstdint>
#include <optional>
#include <span>
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

// POL#39: one of the cursor marquee's three nested frames, in window pixels.
// Drawn as four edges `width` thick just inside `box`.
struct CursorMarkerFrame
{
	mattmath::RectangleF box;
	float width = 0.0f;
	labrador::Colour colour;
};


// One paint match: countdown, clock, cameras, HUDs and music.
// Scene owns and steps the objects; Level keeps borrowed player pointers
// and supplies the views and game-specific overlays. It owns the match's
// voices (voices_): its own and every player's, paused, resumed and stopped
// together.
struct InputBindings;
class TranslatingStructurePaintable;
class TeamGateStructurePaintable;

class Level : public colourwars::SoundBankObject
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
		std::vector<LevelStation> stations,
		OpeningMode opening, std::optional<float> duration = std::nullopt,
		const InputBindings* input_bindings = nullptr,
		const MatchSetup* roster = nullptr, std::uint64_t bot_seed = 0,
		std::vector<LevelLesson> lessons = {});

	// Stop the match's voices: the level's and the player weapon/redraw loops.
	~Level();

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;

	// Hard-pause level audio. GameLevel may resume just the ducked arena music
	// under normal pause menus. Weapon loops retain their playback position.
	// A suspend also resets every player's jump input, so a jump held across
	// the menu is not taken as a new press.
	void suspend();
	// What suspend does to the players, without pausing a voice: an instant
	// replay repeats a round's suspends where they happened (game/round_tape.h).
	void interrupt_players();
	void resume();
	// The top state supplies one active real-time tick, even under pause pages.
	// Simulation, replay steps and drawing never advance these music ramps.
	void update_music(float dt);
	void set_music_ducked(bool ducked);
	void resume_music();
	// Complete the count-in without a simulation tick. Neutral jump input
	// rearms the next press; old fire loops must not return before a new shot.
	// `seats` is one command per participant, as update takes them.
	void prepare_resume(const std::vector<PlayerInputData>& seats);
	// Pauses the same voices without touching any player: for a frozen
	// simulation that no menu covers (the Debug clock's pause).
	void pause_audio();

	// `commands` holds one command per participant, in roster order: device
	// slots already mapped and keyboard/mouse aim resolved (SeatCommandBuilder).
	// A participant past its end gets neutral input. `seats`, when given, sees
	// and may replace every participant's final input after the shipping bots,
	// unless it replaces their inputs. GameLevel passes the shipping recorder
	// in both builds, optionally wrapping a Debug filter; instant replay
	// supplies a tape player instead.
	void update(const std::vector<PlayerInputData>& commands, float dt,
		SeatInputFilter* seats = nullptr);
	// Skip the intro on the next update, as A, B or Start does: honoured only
	// by an update that steps (dt > 0) while the opening or its countdown plays,
	// and dropped at the end of that update either way.
	void request_intro_skip();
	// Feedback intents, retaining each Player's stable input slot. Drain once after
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
	// The card that says the panes are an instant replay, not the live round.
	void draw_replay_banner(labrador::Renderer& renderer) const;

	LevelState state() const;
	// Skip the countdown to GO!, for a practice rebuild that keeps the
	// player's place. Call before the first update.
	void start_without_countdown();
	// Untimed solo practice (POL#19): the level was built with lessons, so its
	// clock never runs out and the HUD shows no clock. The player leaves
	// through the pause menu.
	bool practice() const;
	// The lesson whose area holds the human's centre during live play, as of
	// the last update, in the order the builder supplied them; -1 for none.
	int active_lesson() const;
	int lesson_count() const;
	const LevelLesson& lesson(int index) const;
	// The shown lesson's card in window pixels, else none.
	mattmath::RectangleF lesson_card_bounds() const;
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
	const Player& player(int participant) const;
	// Participant indices are stable roster seats; views contain only humans.
	int view_count() const;
	int participant_for_view(int view) const;
	int view_for_participant(int participant) const;
	bool is_bot(int participant) const;
	BotDifficulty bot_difficulty(int participant) const;
	std::wstring_view bot_identity(int participant) const;
	// Conservative traversal bounds. A permitted team's open gates are neither
	// obstacles nor standing supports; none retains all structural bounds.
	std::vector<mattmath::RectangleF> bot_obstacles(PlayerTeam team = PlayerTeam::none) const;
	std::vector<MatchBotSupport> bot_supports(PlayerTeam team = PlayerTeam::none) const;
	std::vector<MatchBotRamp> bot_ramps() const;
	// Projectile cover includes every gate, even one the bot can walk through.
	bool bot_line_of_sight(mattmath::Vector2F from, mattmath::Vector2F to) const;
	// Candidate route visibility only; the policy separately sweeps its body.
	bool bot_traversal_line_of_sight(mattmath::Vector2F from, mattmath::Vector2F to, PlayerTeam team) const;
	// Cosmetic camera by participant; player(index).camera() remains the stable aim/follow camera.
	labrador::Camera view_camera(int participant) const;
	// Null for tools without an idle drip. Index follows player(participant).
	const IdleDrip* idle_drip(int participant) const;
	// A local cue only during live play; frozen pause draws preserve its state.
	bool aim_marker_visible(int participant) const;
	bool player_identity_visible(int participant) const;
	float player_identity_emphasis(int participant) const;
	mattmath::RectangleF player_identity_bounds(int participant) const;
	// The same badge as seen by the pad in viewer_slot: a bot's badge in a
	// human pane follows that pane's Reduce effects, not the bot's.
	float player_identity_emphasis_for(int participant, int viewer_slot) const;
	mattmath::RectangleF player_identity_bounds_for(int participant, int viewer_slot) const;
	// ROAD#18: one marker per living teammate (human or bot) whose body is
	// outside human pane `view`'s camera, during live play only, in that pane's
	// HUD pixels and roster order. Empty for an invalid pane. Opacity follows
	// the time each has been out of view and the pane pad's Reduce effects.
	std::vector<TeammateMarkerPlacement> teammate_markers(int view) const;
	// Reacquire one's own figure after a covering menu or focus loss.
	void emphasise_players();
	const BlockedShotFeedback& blocked_shot_feedback() const;
	// POL#39: the marquee around the mouse pointer. Shown while enabled, the
	// mouse is focused inside the window and a human seat uses keyboard/mouse.
	bool cursor_marker_visible() const;
	// Where the focused mouse pointer is, in window pixels, or none while the
	// mouse is unfocused. The shell sets it before each draw; none until then.
	void set_pointer(std::optional<mattmath::Vector2F> window_point);
	// Its frames (outer ink, paper rim, inner ink) centred on window pixel
	// `at` in a window `window_height` lines tall, rounded to whole pixels.
	static std::array<CursorMarkerFrame, 3> cursor_marker_frames(
		const CursorMarkerPresentation& look, const mattmath::Vector2F& at, float window_height);

	LevelEndInfo level_end_info() const;
	// Exact scoring tiles in Scene paintable-role order, then per-source tile
	// order. Faces overlap independently at corners; do not union their areas.
	// Source indices are snapshot-local, not authored object names or replay IDs.
	std::vector<PaintSurfaceSample> paint_surfaces() const;
    bool has_horizontal_motion() const;
    std::vector<HorizontalStructureState> horizontal_structure_states() const;
    // The stage's team gates in Scene order, settled once a step.
    std::span<const TeamGateStructurePaintable* const> team_gates() const;

	// Developer tools and shipping setup/replay adapters, called between world
	// updates. Shipping code sets pad mappings and replay also restores cheats.
	void set_overlays(const LevelOverlays& overlays);
	void set_players_showing_debug(bool showing);
	void set_player_cheats(bool damage_immune, bool infinite_ammo);
	// At rest at `top_left`, respawning first if dead; the weapon and the
	// cameras follow at once, so the move shows even while the clock is
	// paused. False: no such participant.
	bool place_player(int participant, const mattmath::Vector2F& top_left);
	bool respawn_player(int participant);
	// Dies on the next world update, at the spawn's redraw. False: no such participant.
	bool kill_player(int participant);
	// By participant: an optional identity-label override ("Bot: paint"). Empty
	// entries keep the shipping bot difficulty label or human device name.
	void set_seat_labels(std::vector<std::wstring> labels);
	// The pad each pad input slot reads (MatchSetup::pad_devices), so
	// a HUD names the pad a reclaimed seat now uses. Rebuilds only on change.
	void set_pad_devices(const std::array<int, input_slots::pad_count>& devices);
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
	std::unique_ptr<TeammateMarker> teammate_marker_;
	// By human pane, then participant: seconds that teammate's body has been
	// outside the pane's view. Presentation only, advanced by live ticks.
	std::vector<float> teammate_marker_age_;
    std::unique_ptr<labrador::Scene> scene_ = nullptr;
    // Admitted fixed movers explicitly refuse retirement. These borrows live
    // for this Level; no terrain add/remove API is exposed after admission.
    std::vector<TranslatingStructurePaintable*> horizontal_structures_;
    std::vector<HorizontalSupportRef> horizontal_supports_;
    // Borrowed from scene_ at admission, like the movers.
    std::vector<const TeamGateStructurePaintable*> team_gates_view_;
    std::vector<TeamGateStructurePaintable*> team_gates_;
    void settle_team_gates();
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
	std::vector<SeatSetup> roster_;
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

	const PlaySettings* preferences_ = nullptr;

	// The arena loop resumes only from a pause, and at the gain the fade and
	// the duck have reached, not where it was left.
	class MusicVoice final : public MatchVoice
	{
	public:
		MusicVoice() = default;
		MusicVoice(const Level* level, const std::string& sound_bank_name,
			const labrador::AudioResources* audio_resources,
			const std::string& effect_name);
		void resume() override;
	private:
		const Level* level_ = nullptr;
	};

	// Every voice below, then each player's, in the order the constructor
	// registers them. Pause, resume and teardown go through this alone.
	MatchVoices voices_;

	// Resolve the level's music and fixed match sounds once at construction.
	MusicVoice music_;
	MatchVoice end_sting_;
	MatchVoice zoom_out_sound_;
	MatchVoice countdown_sound_;
	MatchVoice go_sound_;
	MatchVoice time_up_sound_;
	MatchVoice last_ten_sound_;
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
	std::unique_ptr<colourwars::TextDropShadow> countdown_heading_ = nullptr;
	std::unique_ptr<colourwars::TextDropShadow> resume_heading_;
	std::unique_ptr<colourwars::TextDropShadow> replay_heading_;
	mattmath::RectangleF replay_card_;
	mattmath::RectangleF countdown_card_;
	std::unique_ptr<colourwars::TextDropShadow> stage_entry_;
	mattmath::RectangleF stage_entry_card_;
	mattmath::RectangleF stage_entry_text_bounds_;

	// One shared reminder per input device type present in the match.
	struct ControlsReminder
	{
		std::unique_ptr<colourwars::TextDropShadow> heading;
		std::unique_ptr<colourwars::TextDropShadow> text;
		mattmath::RectangleF card;
		mattmath::RectangleF text_bounds;
	};
	std::vector<ControlsReminder> controls_reminders_;
	// Save owns the effective binding; a paused draw refreshes only its cached
	// words and fitting, retaining the text widget's original font and copy.
	const InputBindings* input_bindings_ = nullptr;
	colourwars::TextDropShadow* keyboard_controls_ = nullptr;
	std::wstring keyboard_controls_words_;
	colourwars::TextDropShadow* gamepad_controls_ = nullptr;
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
	// See set_pad_devices; -1 until set, which is the slot's own pad.
	std::array<int, input_slots::pad_count> pad_devices_ = { -1, -1, -1, -1 };
	// By participant: each HUD identity box's whole text. Built from the seat, the
	// player's team, pad (fixed unless a seat is reclaimed) and seat_labels_,
	// so only construction, set_seat_labels and set_pad_devices rebuild it;
	// draws only read it.
	std::vector<std::wstring> identity_texts_;

	// Borrowed shell services; both outlive every level.
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const labrador::ViewportManager* viewport_manager_ = nullptr;

	// See set_pointer; read only by the cursor marker.
	std::optional<mattmath::Vector2F> pointer_;
	// See request_intro_skip.
	bool intro_skip_requested_ = false;

	// Practice lessons and one prepared card per lesson, so a draw only
	// chooses among them. Laid out in window pixels by centre_lesson_cards.
	struct LessonCard
	{
		std::unique_ptr<colourwars::TextDropShadow> title;
		std::unique_ptr<colourwars::TextDropShadow> text;
		mattmath::RectangleF card;
	};
	std::vector<LevelLesson> lessons_;
	std::vector<LessonCard> lesson_cards_;
	int active_lesson_ = -1;
	void build_lesson_cards(labrador::RenderResources* render_resources);
	void centre_lesson_cards();
	void update_active_lesson();
	void draw_lesson_card(labrador::DrawList& draw_list) const;

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
	MatchVoice sigh_sound_;
	MatchVoice chair_sound_;
	MatchVoice room_tone_sound_;
	bool room_tone_playing_ = false;
	MatchVoice glass_in_sound_;
	MatchVoice celebrate_sound_;
	MatchVoice slump_sound_;
	MatchVoice colour_in_sound_;
	MatchVoice skip_sound_;
	MatchVoice undo_sound_;
	// The countdown plays in the picture and ends by pushing through the glass.
	bool intro_countdown_ = false;
	bool glass_in_started_ = false;
	std::unique_ptr<Monitor> monitor_;
	std::unique_ptr<Silhouette> silhouette_;
	std::unique_ptr<ScreenCursor> cursor_;
	std::unique_ptr<FigureActing> acting_;
	// By roster participant: what each figure is doing, and for how long.
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
	bool skip_requested() const;
	void colour_players(bool team_a, bool team_b);
	void skip_intro();
	void set_room_tone(bool playing);
	void update_opening(float dt);
	void add_intro_views();
	void draw_intro(labrador::Renderer& renderer) const;

	// Advance the match state; update() refreshes views after every path returns.
	void update_state(const std::vector<PlayerInputData>& commands,
		float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal = nullptr);

	void update_level_logic(const std::vector<PlayerInputData>& commands,
		float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal = nullptr);

	// Give each participant its command, then let the bot driver and `seats`
	// (if any) replace it per participant.
	void apply_player_inputs(
		const std::vector<PlayerInputData>& commands, SeatInputFilter* seats);

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
	void update_teammate_markers(float dt);
	// The HUD's scale for human pane `view`, as InterfaceGameplay computes it.
	float hud_scale(int view) const;
	// Rebuilds identity_texts_. Main thread, never from a draw.
	void refresh_identity_texts();
	int pad_device(int slot) const;
	// Makes the labels on first use, then applies overlays_.stations and the
	// current presentation (so tune is live). Main thread, between updates.
	void refresh_station_labels();

	void draw_countdown_text(labrador::DrawList& draw_list) const;
	// POL#39: draws cursor_marker_frames at the mouse pointer.
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

	bool restart_lost_music();
	// The arena loop's gain: the stage's volume through the fade and the duck.
	float music_gain() const;
	void stop_player_sounds();

	int count_projectiles() const;

	// Build countdown text and cameras before the first frame is drawn.
	void build_opening_frame(labrador::RenderResources* render_resources);
};
