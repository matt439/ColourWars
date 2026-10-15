#pragma once

#include "engine/math/shape.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "engine/render/colour.h"
#include "game/objects/weapon_type.h"
#include "game/objects/team_colour.h"

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

// A named sound in the shared bank, and how loud.
struct PresentationSound
{
	std::string name;
	float volume = 1.0f;
};

// Shared menu style, with screen-specific overrides in the records below.
struct MenuStyle
{
	std::string heading_font;
	std::string item_font;
	std::string detail_font;

	labrador::Colour shadow_colour = labrador::Colour::black;
	mattmath::Vector2F title_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F heading_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F item_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F detail_shadow_offset = { 0.0f, 0.0f };

	labrador::Colour highlight = labrador::Colour::white;
	labrador::Colour unhighlight = labrador::Colour::blue;

	// FocusGroup uses this colour for disabled rows.
	labrador::Colour disabled = labrador::Colour::dim_gray;

	// Feedback for a refused action, independent of disabled-row styling.
	labrador::Colour warning = labrador::Colour::orange;

	labrador::Colour heading_colour = labrador::Colour::white;

	// Shared arcade chrome. Focus geometry follows the actual widget bounds.
	labrador::Colour focus_panel_colour;
	mattmath::Vector2F focus_padding;
	float accent_width = 0.0f;
	labrador::Colour panel_edge_colour;
	mattmath::Vector2F panel_shadow_offset;
	labrador::Colour scrim_colour;
	mattmath::Vector2F brand_position;
	std::string brand_font;
	float footer_y = 0.0f;
	float rule_height = 0.0f;
	mattmath::Vector2F icon_padding;

	// The box a modal menu is drawn in. Pause, end and results all use it.
	mattmath::Vector2F box_size = { 0.0f, 0.0f };

	std::string sound_bank;
	float repeat_delay = 0.0f;
	float repeat_interval = 0.0f;
	std::string direction_sound;
	std::string confirm_sound;
	std::string cancel_sound;
};

// Widget position, size and spacing in menu design coordinates.
struct WidgetLayout
{
	mattmath::Vector2F position = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
};

// Per-player choice layout, positioned by row and column indices.
struct SelectPresentation
{
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	labrador::Colour selected = labrador::Colour::white;
	labrador::Colour unselected = labrador::Colour::white;
};

struct ControlsSettingsPresentation
{
	WidgetLayout layout;
	mattmath::Vector2F heading_position;
	mattmath::Vector2F controls_position;
	std::string help_font;
	PresentationSound volume_preview_sound;
};

struct StageSelectPresentation
{
	mattmath::Vector2F preview_position;
	mattmath::Vector2F preview_size;
	mattmath::Vector2F name_position;
	mattmath::Vector2F navigation_position;
	WidgetLayout briefing;
	mattmath::Vector2F briefing_padding;
	labrador::Colour selected;
	labrador::Colour unselected;
};

// Screen-space layout for the start signal and device-specific reminders.
struct MatchPresentation
{
	mattmath::Vector2F reference_size;
	mattmath::Vector2F countdown_size;
	float countdown_center_y = 0.0f;
	mattmath::Vector2F controls_margin;
	mattmath::Vector2F controls_padding;
	float controls_gap = 0.0f;
};

// World framing uses the full display height; split panes inherit their fraction.
// This is independent of menu and HUD coordinates and of the physical resolution.
struct CameraPresentation
{
	float reference_height = 0.0f;
};

struct MainMenuPresentation
{
	std::string backdrop_sheet;
	std::string backdrop_frame;
	mattmath::Vector2F title_position;
	mattmath::Vector2F title_subtitle_position;
	mattmath::Vector2F title_start_position;
	mattmath::Vector2F home_subtitle_position;
	mattmath::Vector2F home_spacing;
	std::string title_font;
	std::string announcement_font;
	std::string weapon_description_font;
	mattmath::Vector2F build_id_position = { 0.0f, 0.0f };

	labrador::Colour title_text_colour = labrador::Colour::white;
	labrador::Colour title_start_text_colour = labrador::Colour::white;

	// Colour for the title's no-controller hint.
	labrador::Colour title_no_pad_text_colour = labrador::Colour::white;

	// Separate colours allow each main-menu screen to be themed independently.
	labrador::Colour title_background_colour = labrador::Colour::black;
	labrador::Colour home_background_colour = labrador::Colour::black;
	labrador::Colour options_background_colour = labrador::Colour::black;
	labrador::Colour play_background_colour = labrador::Colour::black;

	labrador::Colour options_value_colour = labrador::Colour::white;
	mattmath::Vector2F options_spacing = { 0.0f, 0.0f };

	labrador::Colour weapon_description_colour = labrador::Colour::white;
	mattmath::Vector2F weapon_description_shadow_offset = { 0.0f, 0.0f };
	float weapon_description_x_offset = 0.0f;

	mattmath::Vector2F mode_select_spacing = { 0.0f, 0.0f };
	mattmath::Vector2F player_count_spacing = { 0.0f, 0.0f };

	SelectPresentation team_select;
	SelectPresentation weapon_select;
	StageSelectPresentation stage_select;
	std::string credits_font;
	mattmath::Vector2F credits_position;
	mattmath::Vector2F credits_back_position;
	std::wstring credits_text;

	PresentationSound music;
	std::string error_sound;
	std::string ready_sound;
	PresentationSound unready_sound;

	// Weapon descriptions converted from UTF-8 to normalized wide text at load.
	// Indexed by WeaponType, including the random-primary choice.
	static constexpr size_t DESCRIBED_WEAPONS =
		static_cast<size_t>(WeaponType::random_primary) + 1;
	std::array<std::wstring, DESCRIBED_WEAPONS> weapon_descriptions;
};

struct PauseMenuPresentation
{
	mattmath::Vector2F box_size;
	labrador::Colour box_colour = labrador::Colour::black;
	WidgetLayout layout;
	std::string window_open_sound;
	PresentationSound pad_lost_sound;
	PresentationSound pad_back_sound;
};

struct EndMenuPresentation
{
	labrador::Colour box_colour = labrador::Colour::black;
	WidgetLayout layout;
	PresentationSound series_sound;
};

struct ResultsMenuPresentation
{
	mattmath::Vector2F box_size = { 0.0f, 0.0f };
	mattmath::Vector2F session_score_position = { 0.0f, 0.0f };
	mattmath::Vector2F session_result_position = { 0.0f, 0.0f };
	mattmath::Vector2F contribution_position = { 0.0f, 0.0f };
	float contribution_spacing = 0.0f;
	std::string contribution_font;
	labrador::Colour box_colour = labrador::Colour::black;

	// Overrides the shared menu heading colour.
	labrador::Colour heading_colour = labrador::Colour::white;

	mattmath::Vector2F fill_box_size = { 0.0f, 0.0f };
	float fill_box_y_offset = 0.0f;

	// Fill size is derived from the box size and border.
	mattmath::Vector2F team_fill_border = { 0.0f, 0.0f };
	mattmath::Vector2F team_fill_size = { 0.0f, 0.0f };

	float team_fill_delay = 0.0f;
	float team_fill_time = 0.0f;
	float show_results_delay = 0.0f;

	labrador::Colour percentage_colour = labrador::Colour::white;

	// Placement boxes for native team-letter labels beside the results bar.
	mattmath::Vector2F team_mark_size = { 0.0f, 0.0f };
	float team_mark_gap = 0.0f;

	mattmath::Vector2F proceed_position = { 0.0f, 0.0f };
	mattmath::Vector2F heading_position = { 0.0f, 0.0f };
	mattmath::Vector2F winner_position = { 0.0f, 0.0f };

	PresentationSound winner_sound;
	PresentationSound draw_sound;
	PresentationSound fill_sound;
};

// Shared layout and style for page-specific button prompts.
struct PromptBarPresentation
{
	std::string font;
	mattmath::Vector2F position = { 0.0f, 0.0f };
	labrador::Colour colour = labrador::Colour::white;
	labrador::Colour shadow_colour = labrador::Colour::black;
	mattmath::Vector2F shadow_offset = { 0.0f, 0.0f };
};

// Gameplay HUD: health, ammo, match timer and player identity. The respawn
// wait has no number: the figure being redrawn at its spawn is the timer.
// Margins, bar dimensions and text positions use viewport fractions. Shadow
// offsets and identity padding use pixel distances scaled with the text.
struct HudPresentation
{
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

	std::string player_identity_font;
	mattmath::Vector2F player_identity_offset = { 0.0f, 0.0f };
	mattmath::Vector2F player_identity_shadow_offset = { 0.0f, 0.0f };
	labrador::Colour player_identity_colour = labrador::Colour::white;
	labrador::Colour player_identity_shadow_colour = labrador::Colour::black;
	labrador::Colour player_identity_background_colour = labrador::Colour::black;
	mattmath::Vector2F player_identity_padding = { 0.0f, 0.0f };
	float player_identity_reference_width = 1920.0f;
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

// Figure details in the painter's authored canvas. Grips belong to the tool,
// while shoulders and the head accent follow the posed figure rectangle.
struct FigurePresentation
{
	std::string sprite_sheet;
	// Team-local roster indices, so all four manual teammates remain distinct.
	std::array<std::string, 4> accent_frames;
	std::string rest_arms_frame;
	mattmath::Vector2F canvas_size;
	mattmath::Vector2F accent_offset;
	mattmath::Vector2F accent_size;
	mattmath::Vector2F rear_shoulder;
	mattmath::Vector2F front_shoulder;
	float arm_width = 0.0f;
	float elbow_drop = 0.0f;
	float hand_size = 0.0f;
	float hand_edge = 0.0f;
	labrador::Colour arm_colour;
	labrador::Colour hand_colour;
	std::array<FigureGripPresentation, static_cast<size_t>(WeaponType::bucket) + 1> grips;
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
	mattmath::RectangleF glass;
	labrador::Colour glass_colour;
	// Behind the picture, where it does not fill the glass.
	labrador::Colour program_colour;
	// What the glass shows once the program is closed without saving.
	labrador::Colour desktop_colour;
	// The palette along the bottom of the glass: full of greys, but for one
	// empty slot. Its height is a fraction of the glass's.
	float palette_height = 0.0f;
	std::vector<labrador::Colour> palette_greys;
	labrador::Colour slot_outline;
	std::vector<labrador::Colour> sticky_notes;
	std::vector<labrador::Colour> stickers;
	labrador::Colour scribble;
	labrador::Colour coffee;
	labrador::Colour crack;
	labrador::Colour silhouette;
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
	labrador::Colour percent_shadow;
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

	const TeamPalettePresentation& team_palette() const;
	const MenuStyle& menu() const;
	const MainMenuPresentation& main_menu() const;
	const ControlsSettingsPresentation& controls_settings() const;
	const PauseMenuPresentation& pause_menu() const;
	const EndMenuPresentation& end_menu() const;
	const ResultsMenuPresentation& results_menu() const;
	const PromptBarPresentation& prompt_bar() const;
	const HudPresentation& hud() const;
	const MatchPresentation& match() const;
	const CameraPresentation& camera() const;
	const MovementFeedbackPresentation& movement_feedback() const;
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
	MainMenuPresentation main_menu_;
	ControlsSettingsPresentation controls_settings_;
	PauseMenuPresentation pause_menu_;
	EndMenuPresentation end_menu_;
	ResultsMenuPresentation results_menu_;
	PromptBarPresentation prompt_bar_;
	HudPresentation hud_;
	MatchPresentation match_;
	CameraPresentation camera_;
	MovementFeedbackPresentation movement_feedback_;
	FigurePresentation figure_;
	ToolMotionPresentation tool_motion_;
	RespawnRedrawPresentation respawn_redraw_;
	OpeningPresentation opening_;
	DebugOverlayPresentation debug_overlay_;
};
