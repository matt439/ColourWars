#pragma once

#include "engine/math/shape.h"
#include "engine/math/vector2f.h"
#include "engine/render/colour.h"
#include "game/objects/weapon_type.h"

#include <array>
#include <string>

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

	// The box a modal menu is drawn in. Pause, end and results all use it.
	mattmath::Vector2F box_size = { 0.0f, 0.0f };

	std::string sound_bank;
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
};

struct MainMenuPresentation
{
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
	SelectPresentation stage_select;

	PresentationSound music;
	std::string error_sound;
	std::string ready_sound;

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
};

struct EndMenuPresentation
{
	labrador::Colour box_colour = labrador::Colour::black;
	WidgetLayout layout;
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
	float team_b_percentage_x_offset = 0.0f;

	// Team marks use black glyphs from the sprite sheet, drawn without a tint.
	mattmath::Vector2F team_mark_size = { 0.0f, 0.0f };
	float team_mark_gap = 0.0f;

	mattmath::Vector2F proceed_position = { 0.0f, 0.0f };
	mattmath::Vector2F heading_position = { 0.0f, 0.0f };
	mattmath::Vector2F winner_position = { 0.0f, 0.0f };

	PresentationSound winner_sound;
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

// Gameplay HUD: health, ammo, match timer, player identity and respawn count.
// Margins, bar dimensions and text positions use viewport fractions. Shadow
// offsets and identity padding use pixel distances scaled with the text.
struct HudPresentation
{
	float right_margin = 0.0f;
	float ammo_top_margin = 0.0f;
	float health_top_margin = 0.0f;
	mattmath::Vector2F box_size = { 0.0f, 0.0f };
	float box_fill_border = 0.0f;
	labrador::Colour health_colour = labrador::Colour::green;

	std::string sprite_sheet;
	std::string empty_box_frame;
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

	std::string respawn_font;
	mattmath::Vector2F respawn_offset = { 0.0f, 0.0f };
	mattmath::Vector2F respawn_shadow_offset = { 0.0f, 0.0f };
	labrador::Colour respawn_colour = labrador::Colour::white;
	labrador::Colour respawn_shadow_colour = labrador::Colour::black;
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

// Presentation settings parsed once at startup.
class Presentation
{
public:
	// Loads and validates the file, reporting invalid content with source context.
	explicit Presentation(const char* path);

	void validate_resources(const labrador::RenderResources& render,
		const labrador::AudioResources& audio) const;

	const MenuStyle& menu() const;
	const MainMenuPresentation& main_menu() const;
	const ControlsSettingsPresentation& controls_settings() const;
	const PauseMenuPresentation& pause_menu() const;
	const EndMenuPresentation& end_menu() const;
	const ResultsMenuPresentation& results_menu() const;
	const PromptBarPresentation& prompt_bar() const;
	const HudPresentation& hud() const;
	const DebugOverlayPresentation& debug_overlay() const;

private:
	std::string source_path_;
	MenuStyle menu_;
	MainMenuPresentation main_menu_;
	ControlsSettingsPresentation controls_settings_;
	PauseMenuPresentation pause_menu_;
	EndMenuPresentation end_menu_;
	ResultsMenuPresentation results_menu_;
	PromptBarPresentation prompt_bar_;
	HudPresentation hud_;
	DebugOverlayPresentation debug_overlay_;
};
