#pragma once

// Menu-page layout records read from presentation.json by
// presentation_read_menu.cpp. Layout, not text: the copy is in
// presentation_menu_text.h. presentation.h includes this file.

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "engine/render/colour.h"
#include "game/objects/weapon_type.h"

#include <array>
#include <cstddef>
#include <string>

// A named sound in the shared bank, and how loud.
struct PresentationSound
{
	std::string name;
	float volume = 1.0f;
};

// Widget position, size and spacing in menu design coordinates.
struct WidgetLayout
{
	mattmath::Vector2F position = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
};

// Optional focus ornaments, captured when a menu page is constructed.
struct MenuFocusMotionPresentation
{
	float focus_duration = 0.0f;
	float press_duration = 0.0f;
	float arrow_travel = 0.0f;
	float press_fraction = 0.0f;
	float selection_inset_fraction = 0.0f;
};

// Shared menu style, with screen-specific overrides in the records below.
struct MenuStyle
{
	WidgetLayout layout;
	MenuFocusMotionPresentation motion;
	// Solid fills shared by menu backgrounds, chrome, paper and previews.
	std::string sprite_sheet;
	std::string fill_frame;
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
	PresentationSound direction_sound;
	PresentationSound confirm_sound;
	PresentationSound cancel_sound;
};

// Per-player choice layout, positioned by row and column indices.
struct SelectPresentation
{
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	labrador::Colour selected = labrador::Colour::white;
	labrador::Colour unselected = labrador::Colour::white;
};

struct StageSelectPresentation
{
	mattmath::Vector2F preview_position;
	mattmath::Vector2F preview_size;
	mattmath::Vector2F name_position;
	mattmath::Vector2F navigation_position;
	// The stage's record, on the navigation row, right-aligned to the preview.
	float record_y = 0.0f;
	std::string record_font;
	WidgetLayout briefing;
	mattmath::Vector2F briefing_padding;
	labrador::Colour selected;
	labrador::Colour unselected;
	labrador::Colour selected_ink;
};

// Stepped paper and action cards, in menu design coordinates.
struct MenuPaperArtPresentation
{
	labrador::Colour ink;
	labrador::Colour paper;
	labrador::Colour shade;
	labrador::Colour focus_fill;
	labrador::Colour focus_ink;
	// Pause uses a separate readable ink for actions awaiting controllers.
	labrador::Colour disabled_ink;
	float keyline_width = 0.0f;
	float shadow_offset = 0.0f;
	mattmath::Vector2F focus_padding;
};

struct StickPreviewPresentation
{
	mattmath::RectangleF raw_movement;
	mattmath::RectangleF movement_response;
	mattmath::RectangleF aim;
	mattmath::Vector2F aim_readout_position;
	mattmath::Vector2F association_position;
	float marker_size = 0.0f;
	float line_width = 0.0f;
};

struct ControlsSettingsPresentation
{
	MenuPaperArtPresentation art;
	mattmath::Vector2F box_size;
	WidgetLayout layout;
	mattmath::Vector2F heading_position;
	mattmath::Vector2F controls_position;
	mattmath::Vector2F controls_size;
	mattmath::Vector2F status_position;
	std::string item_font;
	std::string help_font;
	PresentationSound volume_preview_sound;
	StickPreviewPresentation stick_preview;
};

// Display metadata only; Random is a menu choice, not another tuned weapon.
struct WeaponChoicePresentation
{
	std::wstring name;
	std::string frame;
	std::string paint_frame;
	std::string supply_strip;
};

struct WeaponSelectArtworkPresentation
{
	static constexpr size_t CHOICES = static_cast<size_t>(WeaponType::random_primary) + 1;
	std::string sheet;
	std::string fill_frame;
	std::array<WeaponChoicePresentation, CHOICES> choices;
	// Reject non-selection enum values rather than displaying an error picture.
	const WeaponChoicePresentation& choice(WeaponType type) const;
};

struct MainMenuPresentation
{
	std::string backdrop_sheet;
	std::string backdrop_frame;
	// Entry, settings and setup pages share the paper treatment.
	MenuPaperArtPresentation art;
	mattmath::Vector2F title_box_size;
	mattmath::Vector2F home_box_size;
	mattmath::Vector2F options_box_size;
	mattmath::Vector2F credits_box_size;
	mattmath::Vector2F setup_box_size;
	mattmath::Vector2F team_box_size;
	mattmath::Vector2F weapon_box_size;
	mattmath::Vector2F stage_box_size;
	mattmath::Vector2F setup_focus_padding;
	float setup_shadow_offset = 0.0f;
	float display_trial_seconds = 0.0f;
	// Title text is centered using its measured width; y positions are authored.
	float title_y = 0.0f;
	float title_subtitle_y = 0.0f;
	float title_start_y = 0.0f;
	float title_no_pad_y = 0.0f;
	mattmath::Vector2F home_position;
	mattmath::Vector2F home_subtitle_position;
	mattmath::Vector2F home_spacing;
	std::string title_font;
	std::string announcement_font;
	std::string weapon_description_font;
	float build_id_y = 0.0f;

	// Separate colours allow each main-menu screen to be themed independently.
	labrador::Colour title_background_colour = labrador::Colour::black;
	labrador::Colour home_background_colour = labrador::Colour::black;
	labrador::Colour options_background_colour = labrador::Colour::black;
	labrador::Colour play_background_colour = labrador::Colour::black;

	labrador::Colour options_value_colour = labrador::Colour::white;
	labrador::Colour options_warning_colour;
	mattmath::Vector2F options_heading_position;
	mattmath::Vector2F options_position;
	mattmath::Vector2F options_status_position;
	float options_value_x = 0.0f;
	mattmath::Vector2F options_spacing = { 0.0f, 0.0f };

	labrador::Colour weapon_description_colour = labrador::Colour::white;
	mattmath::Vector2F weapon_description_shadow_offset = { 0.0f, 0.0f };
	float weapon_description_x_offset = 0.0f;
	// The width each description wraps to, from its column (game/text_wrap.h).
	// Authored breaks are kept, so text that already fits draws as authored.
	float weapon_description_max_width = 0.0f;

	mattmath::Vector2F mode_select_spacing = { 0.0f, 0.0f };
	mattmath::Vector2F player_count_spacing = { 0.0f, 0.0f };

	SelectPresentation team_select;
	mattmath::Vector2F team_warning_position;
	// Three fixed roles: Team A, unassigned, Team B. The current choice selects a frame.
	std::string team_select_sheet;
	std::array<std::string, 3> team_select_frames;
	std::array<std::string, 3> team_select_selected_frames;
	SelectPresentation weapon_select;
	WeaponSelectArtworkPresentation weapon_select_art;
	StageSelectPresentation stage_select;
	std::string credits_font;
	mattmath::Vector2F credits_heading_position;
	mattmath::Vector2F credits_position;
	mattmath::Vector2F credits_back_position;
	std::wstring credits_text;

	PresentationSound music;
	PresentationSound error_sound;
	PresentationSound ready_sound;
	PresentationSound unready_sound;

	// Weapon descriptions converted from UTF-8 to normalized wide text at load.
	// Indexed by WeaponType, including the random-primary choice.
	static constexpr size_t DESCRIBED_WEAPONS =
		static_cast<size_t>(WeaponType::random_primary) + 1;
	std::array<std::wstring, DESCRIBED_WEAPONS> weapon_descriptions;
};

struct PauseMenuPresentation
{
	// A shared wall-clock count-in; the match itself stays frozen throughout.
	float resume_countdown = 0.0f;
	PresentationSound resume_sound;
	MenuPaperArtPresentation art;
	mattmath::Vector2F box_size;
	float heading_y = 0.0f;
	float status_y = 0.0f;
	WidgetLayout layout;
	PresentationSound window_open_sound;
	PresentationSound pad_lost_sound;
	PresentationSound pad_back_sound;
};

struct EndMenuPresentation
{
	MenuPaperArtPresentation art;
	mattmath::Vector2F box_size;
	float heading_y = 0.0f;
	// The first action, independent of the centered heading above it.
	WidgetLayout layout;
	PresentationSound series_sound;
};

struct ResultsMenuPresentation
{
	MenuPaperArtPresentation art;
	mattmath::Vector2F box_size = { 0.0f, 0.0f };
	float session_score_y = 0.0f;
	float session_result_y = 0.0f;
	// The stage record line, right-aligned to the score bar, below it.
	float record_y = 0.0f;
	mattmath::Vector2F contribution_position = { 0.0f, 0.0f };
	float contribution_spacing = 0.0f;
	std::string contribution_font;
	mattmath::Vector2F fill_box_size = { 0.0f, 0.0f };
	float fill_box_y_offset = 0.0f;
	labrador::Colour track_colour;

	// Fill size is derived from the box size and border.
	mattmath::Vector2F team_fill_border = { 0.0f, 0.0f };
	mattmath::Vector2F team_fill_size = { 0.0f, 0.0f };

	float team_fill_delay = 0.0f;
	float team_fill_time = 0.0f;
	float show_results_delay = 0.0f;

	// Placement boxes for native team-letter labels beside the results bar.
	mattmath::Vector2F team_mark_size = { 0.0f, 0.0f };
	float team_mark_gap = 0.0f;

	// Shared centered lettering, placed from actual font measurements.
	float proceed_y = 0.0f;
	float heading_y = 0.0f;
	float winner_y = 0.0f;

	PresentationSound winner_sound;
	PresentationSound draw_sound;
	PresentationSound fill_sound;
	PresentationSound music;
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
