#pragma once

#include "engine/math/shape.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "engine/render/colour.h"
#include "game/objects/weapon_type.h"
#include "game/objects/team_colour.h"
#include "game/presentation_menu.h"
#include "game/presentation_menu_text.h"

#include <array>
#include <string>
#include <vector>

namespace labrador
{
	class AudioResources;
	class RenderResources;
}

// Fonts, colours and layout loaded from presentation.json for menus and HUDs.
// Keep visual choices separate from gameplay balance in tuning.json. Widget
// structure and behavior remain in C++.

// Music gain ramps use the active application's clock, separate from simulation.
struct MusicMixPresentation
{
	float fade_in = 0.0f;
	float fade_out = 0.0f;
	float pause_gain = 0.0f;
	float duck_time = 0.0f;
	float restore_time = 0.0f;
};

// Countdown drawing dimensions use the match reference pixels; timing is seconds.
struct CountdownArtPresentation
{
	labrador::Colour ink;
	labrador::Colour paper;
	labrador::Colour shade;
	labrador::Colour number;
	labrador::Colour go;
	float keyline_width = 0.0f;
	float shadow_offset = 0.0f;
	float beat_duration = 0.0f;
	float pulse = 0.0f;
};

// Screen-space layout for the start signal and device-specific reminders.
struct MatchPresentation
{
	mattmath::Vector2F reference_size;
	mattmath::Vector2F countdown_size;
	float countdown_center_y = 0.0f;
	mattmath::Vector2F stage_entry_size;
	float stage_entry_top = 0.0f;
	mattmath::Vector2F controls_margin;
	mattmath::Vector2F controls_padding;
	float controls_gap = 0.0f;
	CountdownArtPresentation countdown_art;
};

// World-camera accent sizes in world units, and their timing in seconds.
struct ViewShakePresentation
{
	float hurt_amplitude = 0.0f;
	float elimination_amplitude = 0.0f;
	float landing_amplitude = 0.0f;
	float landing_min_speed = 0.0f;
	float landing_full_speed = 0.0f;
	float duration = 0.0f;
	float cycles = 0.0f;
	float repeat_delay = 0.0f;
};

// World framing uses the full display height; split panes inherit their fraction.
// This is independent of menu and HUD coordinates and of the physical resolution.
struct CameraPresentation
{
	float reference_height = 0.0f;
	// Optional global framing controls. Zero keeps the original height-based
	// scale and follow focus; lead is also bounded by a quarter of the pane.
	float minimum_world_width = 0.0f;
	float horizontal_lead_seconds = 0.0f;
	float horizontal_lead_max_distance = 0.0f;
	ViewShakePresentation shake;
};

// Shared monochrome roster badge. Geometry is cosmetic; device slots never
// choose identity. UI sizes are design units, world sizes are world units.
struct PlayerIdentityPresentation
{
	bool enabled = true;
	mattmath::Vector2F world_size;
	// Bottom-centre offset above the player's collision rectangle.
	mattmath::Vector2F world_offset;
	float emphasis_duration = 0.0f;
	float emphasis_scale = 1.0f;
	mattmath::Vector2F ui_size;
	float ui_gap = 0.0f;
	labrador::Colour background_colour = labrador::Colour::white;
	labrador::Colour ink_colour = labrador::Colour::black;
	float border = 0.0f;
};

// Closing-clock drawing dimensions use HUD reference pixels; timing is seconds.
struct ClosingClockPresentation
{
	labrador::Colour ink;
	labrador::Colour paper;
	labrador::Colour shade;
	labrador::Colour text;
	labrador::Colour accent;
	float keyline_width = 0.0f;
	float shadow_offset = 0.0f;
	float tick_reach = 0.0f;
	float beat_duration = 0.0f;
};

// Static paper backing for the live HUD, in HUD reference pixels.
struct HudPaperPresentation
{
	labrador::Colour ink;
	labrador::Colour paper;
	labrador::Colour shade;
	float keyline_width = 0.0f;
	float shadow_offset = 0.0f;
};

// Gameplay HUD: health, ammo, match timer and player identity. The respawn
// wait has no number: the figure being redrawn at its spawn is the timer.
// Margins, bar dimensions and text positions use viewport fractions. Shadow
// offsets and identity padding use pixel distances scaled with the text.
struct HudPresentation
{
	HudPaperPresentation paper;
	std::string meter_label_font;
	labrador::Colour meter_track_colour;
	float meter_label_gap = 0.0f;
	float reference_height = 1080.0f;
	float right_margin = 0.0f;
	float ammo_top_margin = 0.0f;
	float health_top_margin = 0.0f;
	mattmath::Vector2F box_size = { 0.0f, 0.0f };
	float box_fill_border = 0.0f;
	labrador::Colour health_colour = labrador::Colour::green;

	std::string sprite_sheet;
	std::string fill_frame;

	std::string timer_font;
	mattmath::Vector2F timer_offset = { 0.0f, 0.0f };
	mattmath::Vector2F timer_shadow_offset = { 0.0f, 0.0f };
	labrador::Colour timer_colour = labrador::Colour::gray;
	labrador::Colour timer_shadow_colour = labrador::Colour::black;

	// Reference viewport width for scaling the timer font.
	float timer_reference_width = 1920.0f;
	ClosingClockPresentation closing_clock;

	std::string player_identity_font;
	mattmath::Vector2F player_identity_offset = { 0.0f, 0.0f };
	mattmath::Vector2F player_identity_shadow_offset = { 0.0f, 0.0f };
	labrador::Colour player_identity_colour = labrador::Colour::white;
	labrador::Colour player_identity_shadow_colour = labrador::Colour::black;
	labrador::Colour player_identity_background_colour = labrador::Colour::black;
	mattmath::Vector2F player_identity_padding = { 0.0f, 0.0f };
	float player_identity_reference_width = 1920.0f;
};

// overlay.stations: each station's name in world units, its top-left at the
// station's position (a character's top-left) plus offset. The font is taken
// when a level first shows the names; scale, offset and colour on every toggle.
struct StationLabelPresentation
{
	std::string font;
	float scale = 1.0f;
	mattmath::Vector2F offset = { 0.0f, 0.0f };
	labrador::Colour colour = labrador::Colour::black;
};

struct DebugOverlayPresentation
{
	std::string font;
	float font_scale = 1.0f;
	float line_spacing = 0.0f;
	mattmath::Vector2F position = { 0.0f, 0.0f };
	labrador::Colour colour = labrador::Colour::light_gray;
	labrador::Colour shadow_colour = labrador::Colour::black;
	mattmath::Vector2F shadow_offset = { 0.0f, 0.0f };
	float shadow_scale = 1.0f;
	StationLabelPresentation station_label;
};

struct MovementBurstPresentation
{
	float duration = 0.0f;
	float spread = 0.0f;
	float lift = 0.0f;
	float particle_size = 0.0f;
};

// Cosmetic world-space movement cues; none of these values affect physics.
struct MovementFeedbackPresentation
{
	bool enabled = true;
	std::string sprite_sheet;
	std::string fill_frame;
	labrador::Colour colour;
	labrador::Colour highlight;
	float pixel_size = 0.0f;
	float landing_min_speed = 0.0f;
	float full_impact_speed = 0.0f;
	float minimum_strength = 0.0f;
	MovementBurstPresentation takeoff;
	MovementBurstPresentation landing;
	MovementBurstPresentation step;
	MovementBurstPresentation skid;
	labrador::Colour keyline;
	float keyline_width = 0.0f;
};

// Local, cosmetic contact accents and the fragments left where a figure pops.
// These never change the player's collision, health, paint or respawn timing.
struct CombatFeedbackPresentation
{
	bool enabled = true;
	std::string sprite_sheet;
	std::string fill_frame;
	labrador::Colour highlight;
	labrador::Colour keyline;
	float hurt_duration = 0.0f;
	float hurt_repeat_delay = 0.0f;
	float hurt_width = 0.0f;
	float hurt_reach = 0.0f;
	float elimination_duration = 0.0f;
	float elimination_spread = 0.0f;
	float elimination_lift = 0.0f;
	float particle_size = 0.0f;
	float keyline_width = 0.0f;
};

// Cosmetic shot/supply responses and the optional local direction/readiness cue.
// Every size is in world units; these never change the firing workflow.
struct WeaponFeedbackPresentation
{
	bool enabled = true;
	std::string sprite_sheet;
	std::string fill_frame;
	labrador::Colour keyline;
	labrador::Colour highlight;
	float muzzle_duration = 0.0f;
	float empty_duration = 0.0f;
	float recovered_duration = 0.0f;
	float keyline_width = 0.0f;
	float status_size = 0.0f;
	float status_offset = 0.0f;
	float aim_marker_distance = 0.0f;
	float aim_marker_size = 0.0f;
	float aim_marker_stroke = 0.0f;
	std::array<mattmath::Vector2F, static_cast<size_t>(WeaponType::bucket) + 1> muzzle_sizes;
};

// Detached, short-lived contacts for flying paint stopped by unpainted cover.
// These values author a cosmetic cue, never collision or surface ownership.
struct BlockedShotPresentation
{
	bool enabled = true;
	std::string sprite_sheet;
	std::string fill_frame;
	labrador::Colour highlight;
	labrador::Colour keyline;
	float duration = 0.0f;
	float repeat_delay = 0.0f;
	float width = 0.0f;
	float reach = 0.0f;
	float bead_size = 0.0f;
	float stroke_width = 0.0f;
	float keyline_width = 0.0f;
};

// Residual tool paint, entirely separate from projectiles and scoring.
struct IdleDripPresentation
{
	bool enabled = true;
	std::string sprite_sheet;
	std::string fill_frame;
	labrador::Colour keyline;
	float interval_min = 0.0f;
	float interval_max = 0.0f;
	float grow_time = 0.0f;
	float stretch_time = 0.0f;
	float lifetime = 0.0f;
	float gravity = 0.0f;
	float size = 0.0f;
	float stretch = 0.0f;
	float nozzle_inset = 0.0f;
	float nozzle_underside = 0.0f;
	float keyline_width = 0.0f;
};

struct FigureGripPresentation
{
	// World-space offsets from the weapon's actual draw pivot before rotation.
	mattmath::Vector2F rear;
	mattmath::Vector2F front;
};

// Seconds and distances for the held tools' mechanisms. They select authored
// poses without moving the simulation pivot, muzzle, aim or collision shapes.
struct ToolMotionStyle
{
	float shot_recovery = 0.0f;
	float valve_time = 0.0f;
};

struct ToolMotionPresentation
{
	bool enabled = true;
	std::array<ToolMotionStyle, static_cast<size_t>(WeaponType::bucket) + 1> tools;
	float shot_pose_split = 0.0f;
	float roller_travel_per_turn = 0.0f;
	float roller_lift_time = 0.0f;
	float roller_settle_time = 0.0f;
	float hose_travel_per_sway = 0.0f;
	float hose_settle_time = 0.0f;
	float bucket_travel_per_swing = 0.0f;
	float bucket_settle_time = 0.0f;
	std::string mist_sheet;
	std::string mist_frame;
	float mist_tail_duration = 0.0f;
	float mist_tail_distance = 0.0f;
	mattmath::Vector2F mist_tail_size;
	float mist_tail_opacity = 0.0f;
};

// Optional, cosmetic character motion. Distances are authored source texels;
// its foot baseline and the tool's simulation attachments never move.
struct QuietCharacterPresentation
{
	bool enabled = false;
	float settle_time = 0.0f;
	float breath_period = 0.0f;
	float breath_height = 0.0f;
	float shift_period = 0.0f;
	float shift_distance = 0.0f;
	float blink_min_interval = 0.0f;
	float blink_max_interval = 0.0f;
	float blink_duration = 0.0f;
	float lean_distance = 0.0f;
	float lean_acceleration = 0.0f;
	float lean_settle_time = 0.0f;
	std::array<mattmath::RectangleF, 2> eyes;
	float eyelid_height = 0.0f;
	labrador::Colour face_colour;
};

// Figure details in the painter's authored canvas. Grips belong to the tool,
// while shoulders follow the posed figure canvas.
struct FigurePresentation
{
	std::string sprite_sheet;
	// Arms drawn over the body, as neutral and team-paint pairs: rest_arms
	// while no tool is held, sleeves (the same shoulders) while one is.
	std::string rest_arms_frame;
	std::string rest_arms_paint_frame;
	std::string sleeves_frame;
	std::string sleeves_paint_frame;
	mattmath::Vector2F canvas_size;
	// Canvas texels either side of the hitbox: the brim and hands overhang it.
	float body_inset = 0.0f;
	mattmath::Vector2F rear_shoulder;
	mattmath::Vector2F front_shoulder;
	float arm_width = 0.0f;
	float elbow_drop = 0.0f;
	float hand_size = 0.0f;
	float hand_edge = 0.0f;
	labrador::Colour arm_colour;
	labrador::Colour hand_colour;
	std::array<FigureGripPresentation, static_cast<size_t>(WeaponType::bucket) + 1> grips;
	QuietCharacterPresentation quiet;

	// Canvas texels the hitbox spans.
	float body_width() const { return canvas_size.x - body_inset * 2.0f; }

	// The whole canvas around a figure's hitbox rectangle, overhang included.
	mattmath::RectangleF canvas_rectangle(const mattmath::RectangleF& body) const
	{
		const float overhang = body_inset * body.width / this->body_width();
		return { body.x - overhang, body.y, body.width + overhang * 2.0f, body.height };
	}
};

// One stroke of a figure as the person drew it: the path of the pen's
// top-left corner, in texels of the right-facing idle frame. The pen draws
// only horizontal and vertical lines, as the original art does.
struct RespawnStroke
{
	std::string part;
	std::vector<mattmath::Vector2F> points;
};

// The undo history redrawing a splatted figure at its spawn, fast-forward,
// over the respawn wait (docs/backstory.md, "Respawn"). The wait itself is
// tuning.player.respawn_delay; these only divide it up, and nothing here
// affects the simulation.
struct RespawnRedrawPresentation
{
	std::string sprite_sheet;
	std::string pen_frame;
	labrador::Colour pen_colour;
	// The texels of the frame the strokes trace, stretched over the player.
	mattmath::Vector2F canvas_size;
	float pen_width = 0.0f;
	// Fractions of the wait: empty after the pop until draw_from, the outline
	// drawn by draw_from..fill_at, then the filled figure until it respawns.
	float draw_from = 0.0f;
	float fill_at = 1.0f;
	// Seconds the dead player's camera stays where they fell before it cuts
	// to the spawn to watch the redraw.
	float camera_hold = 0.0f;
	std::vector<RespawnStroke> strokes;
};

// Static physical monitor artwork. Rectangles are fractions of the screen;
// pen, corner and shadow widths are fractions of its height.
struct MonitorArtPresentation
{
	labrador::Colour ink;
	labrador::Colour highlight;
	labrador::Colour power_light;
	float pen_width = 0.0f;
	float corner_step = 0.0f;
	float shadow_width = 0.0f;
	mattmath::RectangleF vent_area;
	int vent_count = 0;
	mattmath::RectangleF power_button;
	mattmath::RectangleF power_indicator;
};

// Low-value, anonymous rear silhouette. Widths use the person's seated height.
struct SilhouetteArtPresentation
{
	labrador::Colour ink;
	labrador::Colour rim;
	labrador::Colour shade;
	float pen_width = 0.0f;
	float shade_width = 0.0f;
};

// Coarse-cell pointer; offsets and click reach use its authored height.
struct CursorArtPresentation
{
	labrador::Colour ink;
	labrador::Colour fill;
	labrador::Colour shade;
	labrador::Colour shadow;
	float shadow_offset = 0.0f;
	float click_duration = 0.0f;
	float click_reach = 0.0f;
};

// Coverage cards shown only after the person's reveal click. Measurements are
// fractions of each card's height, so the complete drawing fits its bounds.
struct RevealArtPresentation
{
	labrador::Colour ink;
	labrador::Colour paper;
	labrador::Colour shade;
	float keyline_width = 0.0f;
	float padding = 0.0f;
	float shadow_offset = 0.0f;
	float tick_length = 0.0f;
	float beat_duration = 0.0f;
};

// The monitor the picture is on, the person at it and the cursor they use:
// the opening's look (docs/backstory.md, "Opening"). Rectangles are fractions
// of the screen, sizes fractions of its height, colours are drawn as is.
struct OpeningPresentation
{
	std::string sprite_sheet;
	std::string fill_frame;
	// Each match picks one; each is a different person's computer.
	std::vector<labrador::Colour> bezel_shades;
	labrador::Colour bezel_shadow;
	MonitorArtPresentation monitor_art;
	mattmath::RectangleF glass;
	labrador::Colour glass_colour;
	// Behind the picture, where it does not fill the glass.
	labrador::Colour program_colour;
	// What the glass shows once the program is closed without saving.
	labrador::Colour desktop_colour;
	// The level kit's swatches along the bottom of the glass, followed by one
	// empty slot. Hues come from atlas texels, shared with the terrain.
	// Its height is a fraction of the glass's.
	float palette_height = 0.0f;
	std::string palette_sheet;
	std::vector<std::string> palette_frames;
	labrador::Colour slot_outline;
	std::vector<labrador::Colour> sticky_notes;
	std::vector<labrador::Colour> stickers;
	labrador::Colour scribble;
	labrador::Colour coffee;
	labrador::Colour crack;
	labrador::Colour silhouette;
	SilhouetteArtPresentation silhouette_art;
	CursorArtPresentation cursor_art;
	RevealArtPresentation reveal_art;
	// From the bottom of the screen to the top of the head, seated.
	float adult_height = 0.0f;
	float child_height = 0.0f;
	float cursor_size = 0.0f;
	// How strongly a preview shows its colour over the picture, 0..1.
	float preview_strength = 0.0f;
	// The figures before they are coloured.
	labrador::Colour figure_grey;
	// The coverage the reveal shows, each half's number in its colour.
	std::string percent_font;
	float percent_height = 0.0f;
	// The figures' acting: what an eye is painted out with, the sweat, and how
	// far a winner hops, a cheat shakes and a loser slumps, as fractions of a
	// figure's height (the shake of its width).
	labrador::Colour face;
	labrador::Colour sweat;
	float hop = 0.0f;
	float shake = 0.0f;
	float slump = 0.0f;
};

// Presentation settings parsed once at startup.
struct NamedTeamColour
{
	std::string name;
	labrador::Colour colour = labrador::Colour::gray;
};

struct TeamPaletteEntry
{
	std::string key;
	std::wstring label;
	TeamColour colours;
};

// Fixed preference keys remain save-compatible. Extra authored entries join
// Random without adding an enum or a fixed Options choice.
struct TeamPalettePresentation
{
	std::vector<NamedTeamColour> colours;
	std::vector<TeamPaletteEntry> pairs;

	const TeamPaletteEntry& fixed_pair(TeamColourPair pair) const;
	const TeamPaletteEntry& random_pair(size_t draw) const;
};

class Presentation
{
public:
	// Loads and validates the file, reporting invalid content with source context.
	explicit Presentation(const char* path);

	void validate_resources(const labrador::RenderResources& render,
		const labrador::AudioResources& audio) const;
	// Validate composed Stage text against the actual loaded catalog, without
	// assuming a maximum stage count or a particular widest numeric glyph.
	void validate_stage_counts(const labrador::RenderResources& render,
		size_t total, size_t eligible) const;

	const TeamPalettePresentation& team_palette() const;
	const MenuStyle& menu() const;
	const MenuText& menu_text() const;
	const MusicMixPresentation& music_mix() const;
	const MainMenuPresentation& main_menu() const;
	const ControlsSettingsPresentation& controls_settings() const;
	const PauseMenuPresentation& pause_menu() const;
	const EndMenuPresentation& end_menu() const;
	const ResultsMenuPresentation& results_menu() const;
	const PromptBarPresentation& prompt_bar() const;
	const HudPresentation& hud() const;
	const PlayerIdentityPresentation& player_identity() const;
	const MatchPresentation& match() const;
	const CameraPresentation& camera() const;
	const MovementFeedbackPresentation& movement_feedback() const;
	const CombatFeedbackPresentation& combat_feedback() const;
	const WeaponFeedbackPresentation& weapon_feedback() const;
	const BlockedShotPresentation& blocked_shot() const;
	const IdleDripPresentation& idle_drip(WeaponType type = WeaponType::sprayer) const;
	const FigurePresentation& figure() const;
	const ToolMotionPresentation& tool_motion() const;
	const RespawnRedrawPresentation& respawn_redraw() const;
	const OpeningPresentation& opening() const;
	const DebugOverlayPresentation& debug_overlay() const;

	// The developer console's live feedback toggle. The only mutator: only
	// the Debug director holds a non-const Presentation. MovementFeedback reads
	// the flag every frame, so the change applies immediately.
	void set_movement_feedback_enabled(bool enabled);

private:
	std::string source_path_;
	TeamPalettePresentation team_palette_;
	MenuStyle menu_;
	MenuText menu_text_;
	MusicMixPresentation music_mix_;
	MainMenuPresentation main_menu_;
	ControlsSettingsPresentation controls_settings_;
	PauseMenuPresentation pause_menu_;
	EndMenuPresentation end_menu_;
	ResultsMenuPresentation results_menu_;
	PromptBarPresentation prompt_bar_;
	HudPresentation hud_;
	PlayerIdentityPresentation player_identity_;
	MatchPresentation match_;
	CameraPresentation camera_;
	MovementFeedbackPresentation movement_feedback_;
	CombatFeedbackPresentation combat_feedback_;
	WeaponFeedbackPresentation weapon_feedback_;
	BlockedShotPresentation blocked_shot_;
	std::array<IdleDripPresentation, 5> idle_drips_;
	FigurePresentation figure_;
	ToolMotionPresentation tool_motion_;
	RespawnRedrawPresentation respawn_redraw_;
	OpeningPresentation opening_;
	DebugOverlayPresentation debug_overlay_;
};
