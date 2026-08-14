#pragma once

#include "engine/math/matt_math.h"
#include "engine/render/colour.h"
#include "game/objects/weapon_type.h"

#include <array>
#include <string>

// What the shell looks like: every font, colour, offset and box size the menus,
// the HUD and the debug overlay are laid out from.
//
// This is the other half of the defect GameTuning closed, in different files.
// E2 moved the numbers a match is *balanced* on out of C++ headers and left a
// note saying the remaining ~200 SCREAMING constants were "the same defect,
// different files, and no dependency on anything". They were 152 live ones
// across seven headers, and they are ./presentation.json now.
//
// WHY IT IS A SECOND FILE AND NOT A SECTION OF tuning.json. GameTuning's own
// comment says what it is: "How much, for every number a paint match is
// balanced on." A menu's font is not that. The two files have different
// audiences - a balance pass and an art pass - and nothing in one is read by
// the other. Splitting them also keeps each readable: tuning.json is 225 lines
// already.
//
// WHAT IS NOT HERE. The same line GameTuning draws: C++ says *how* and data
// says *which* and *how much*. So which widget a menu builds, and in what
// order, is code; the font it is drawn in and where it sits are here. Paint
// tile geometry stayed in tuning.json, because how much surface a splash
// covers is a balance number that happens to be measured in pixels.
//
// THE REAL FIND WAS THE DUPLICATION. Four menu headers declared the same
// fifteen values - three font names, a shadow colour, four shadow offsets, a
// highlight pair, a sound bank and three UI sounds - and they agreed, which is
// the only reason nobody noticed. They are MenuStyle below, once. Nine of the
// 161 constants were dead, five of them copies in a namespace whose own .cpp
// never read them.

// A named sound in the shared bank, and how loud.
struct PresentationSound
{
	std::string name;
	float volume = 1.0f;
};

// The style every menu family shares.
//
// It is one struct rather than a repeated block in each menu's section
// because the four copies it replaces were identical. A menu that wants to
// differ says so in its own section - results_menu's heading is goldenrod
// where the other three are orange, and that override is one key rather than
// a fourth copy of the whole style.
struct MenuStyle
{
	std::string heading_font;
	std::string item_font;
	std::string detail_font;

	artattack::Colour shadow_colour = artattack::Colour::black;
	mattmath::Vector2F title_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F heading_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F item_shadow_offset = { 0.0f, 0.0f };
	mattmath::Vector2F detail_shadow_offset = { 0.0f, 0.0f };

	// A focused widget and an unfocused one. Named for what they do rather
	// than what colour they happen to be.
	artattack::Colour highlight = artattack::Colour::white;
	artattack::Colour unhighlight = artattack::Colour::blue;

	// A row that is on the screen and cannot be chosen. It is not a third
	// focus state: FocusGroup repaints every widget it holds from the focus of
	// every slot on every mutation, so a disabled row is one the group was
	// never given, and this is the colour it keeps for its whole life.
	artattack::Colour disabled = artattack::Colour::dim_gray;

	// A line the menu shows to say why it just refused something. Distinct
	// from `disabled`, which describes a row rather than an answer.
	artattack::Colour warning = artattack::Colour::orange;

	// The heading colour three of the four menus use. It was written as
	// colour_consts::TEAM_ORANGE in each of them - a menu heading is not a
	// team, and it only ever shared the number.
	artattack::Colour heading_colour = artattack::Colour::white;

	// The box a modal menu is drawn in. Pause, end and results all use it.
	mattmath::Vector2F box_size = { 0.0f, 0.0f };

	std::string sound_bank;
	std::string direction_sound;
	std::string confirm_sound;
	std::string cancel_sound;
};

// One row of pickable widgets: where the first sits, how big it is, and how
// far apart the rest are.
//
// Three menus wrote these as an INITIAL_WIDGET_SIZE / _POSITION / _SPACING
// triple and two more wrote only the spacing. It is one shape.
struct WidgetLayout
{
	mattmath::Vector2F position = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
};

// A select screen where each player moves a cursor over a row of choices.
//
// No position: a select screen's widgets are placed by walking the spacing
// from a row and column index, so the spacing and the widget size are the
// whole of the layout. That is why this is not a WidgetLayout.
struct SelectPresentation
{
	mattmath::Vector2F spacing = { 0.0f, 0.0f };
	mattmath::Vector2F size = { 0.0f, 0.0f };
	artattack::Colour selected = artattack::Colour::white;
	artattack::Colour unselected = artattack::Colour::white;
};

struct MainMenuPresentation
{
	std::string title_font;
	std::string announcement_font;
	std::string weapon_description_font;

	artattack::Colour title_text_colour = artattack::Colour::white;
	artattack::Colour title_start_text_colour = artattack::Colour::white;

	// The line under Start that appears only when no pad is plugged in. The
	// game is gamepad-only and the title screen is the one place that has to
	// say so, because with no pad it is the only screen anyone can reach.
	artattack::Colour title_no_pad_text_colour = artattack::Colour::white;

	// Four screens, four keys, and they hold the same colour today. Unlike the
	// fonts and shadows MenuStyle absorbed, these are not four copies of one
	// idea - they are four screens somebody might theme apart, and collapsing
	// them would take that away to save three lines of a data file.
	artattack::Colour title_background_colour = artattack::Colour::black;
	artattack::Colour home_background_colour = artattack::Colour::black;
	artattack::Colour options_background_colour = artattack::Colour::black;
	artattack::Colour play_background_colour = artattack::Colour::black;

	artattack::Colour options_value_colour = artattack::Colour::white;

	artattack::Colour weapon_description_colour = artattack::Colour::white;
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

	// What the weapon-select screen says about each weapon.
	//
	// Wide, because the render module holds text as std::wstring and the
	// conversion belongs off the draw path. The file is UTF-8 and these come
	// across through widen() while it is being read, which is once.
	//
	// Indexed by WeaponType, and sized by the last weapon a player can pick:
	// the enum carries four more names for weapons that do not exist, and a
	// slot each would be four empty strings. A weapon with no description
	// reads as empty rather than as the literal "ERROR" the switch it
	// replaces returned - which was a string a player could actually see.
	static constexpr size_t DESCRIBED_WEAPONS =
		static_cast<size_t>(WeaponType::random_primary) + 1;
	std::array<std::wstring, DESCRIBED_WEAPONS> weapon_descriptions;
};

struct PauseMenuPresentation
{
	artattack::Colour box_colour = artattack::Colour::black;
	WidgetLayout layout;
	std::string window_open_sound;
};

struct EndMenuPresentation
{
	artattack::Colour box_colour = artattack::Colour::black;
	WidgetLayout layout;
};

struct ResultsMenuPresentation
{
	artattack::Colour box_colour = artattack::Colour::black;

	// The heading that differs from MenuStyle's, and the reason MenuStyle has
	// the field at all rather than every menu carrying its own.
	artattack::Colour heading_colour = artattack::Colour::white;

	mattmath::Vector2F fill_box_size = { 0.0f, 0.0f };
	float fill_box_y_offset = 0.0f;

	// The border is the input and the fill size is derived from it, which is
	// what the header did too - except it did it with a namespace-qualified
	// reference to a constant three lines above, at namespace scope.
	mattmath::Vector2F team_fill_border = { 0.0f, 0.0f };
	mattmath::Vector2F team_fill_size = { 0.0f, 0.0f };

	float team_fill_delay = 0.0f;
	float team_fill_time = 0.0f;
	float show_results_delay = 0.0f;

	artattack::Colour percentage_colour = artattack::Colour::white;
	float team_b_percentage_x_offset = 0.0f;

	// The A and B letters that sit outside each end of the fill box. There is
	// no colour beside them: the glyphs are black in the sheet and a tint
	// multiplies, so anything other than white can only darken black.
	mattmath::Vector2F team_mark_size = { 0.0f, 0.0f };
	float team_mark_gap = 0.0f;

	mattmath::Vector2F proceed_position = { 0.0f, 0.0f };
	mattmath::Vector2F heading_position = { 0.0f, 0.0f };
	mattmath::Vector2F winner_position = { 0.0f, 0.0f };

	PresentationSound winner_sound;
	PresentationSound fill_sound;
};

// The line of button prompts along the bottom of a menu page. What each page
// puts in it is the page's business; where it sits and what it looks like is
// the same everywhere, which is why it is one block and not twelve.
struct PromptBarPresentation
{
	std::string font;
	mattmath::Vector2F position = { 0.0f, 0.0f };
	artattack::Colour colour = artattack::Colour::white;
	artattack::Colour shadow_colour = artattack::Colour::black;
	mattmath::Vector2F shadow_offset = { 0.0f, 0.0f };
};

// The gameplay HUD: health, ammo, the match timer and the respawn count.
//
// Most of these are fractions of the viewport rather than pixels, which is why
// they survive a resolution change and the countdown did not.
struct HudPresentation
{
	float right_margin = 0.0f;
	float ammo_top_margin = 0.0f;
	float health_top_margin = 0.0f;
	mattmath::Vector2F box_size = { 0.0f, 0.0f };
	float box_fill_border = 0.0f;
	artattack::Colour health_colour = artattack::Colour::green;

	std::string sprite_sheet;
	std::string empty_box_frame;
	std::string fill_frame;

	std::string timer_font;
	mattmath::Vector2F timer_offset = { 0.0f, 0.0f };
	mattmath::Vector2F timer_shadow_offset = { 0.0f, 0.0f };
	artattack::Colour timer_colour = artattack::Colour::gray;
	artattack::Colour timer_shadow_colour = artattack::Colour::black;

	// The width the timer's point size was chosen against. The drawn scale is
	// the real width over this, so the HUD reads the same on every resolution.
	float timer_reference_width = 1920.0f;

	std::string respawn_font;
	mattmath::Vector2F respawn_offset = { 0.0f, 0.0f };
	mattmath::Vector2F respawn_shadow_offset = { 0.0f, 0.0f };
	artattack::Colour respawn_colour = artattack::Colour::white;
	artattack::Colour respawn_shadow_colour = artattack::Colour::black;
};

struct DebugOverlayPresentation
{
	std::string font;
	float font_scale = 1.0f;
	float line_spacing = 0.0f;
	mattmath::Vector2F position = { 0.0f, 0.0f };
	artattack::Colour colour = artattack::Colour::light_gray;
	artattack::Colour shadow_colour = artattack::Colour::black;
	mattmath::Vector2F shadow_offset = { 0.0f, 0.0f };
	float shadow_scale = 1.0f;
};

// Everything ./presentation.json says, parsed once at startup.
//
// Read eagerly and whole, exactly like GameTuning: a missing key or a colour
// this engine does not know by name is a startup failure naming the file and
// the position in it, not a menu that draws wrong three screens in (T6).
class Presentation
{
public:
	// Reads `path` in full. Throws std::runtime_error naming the file, the
	// position in it and the key if anything is missing or of the wrong type.
	explicit Presentation(const char* path);

	const MenuStyle& menu() const;
	const MainMenuPresentation& main_menu() const;
	const PauseMenuPresentation& pause_menu() const;
	const EndMenuPresentation& end_menu() const;
	const ResultsMenuPresentation& results_menu() const;
	const PromptBarPresentation& prompt_bar() const;
	const HudPresentation& hud() const;
	const DebugOverlayPresentation& debug_overlay() const;

private:
	MenuStyle menu_;
	MainMenuPresentation main_menu_;
	PauseMenuPresentation pause_menu_;
	EndMenuPresentation end_menu_;
	ResultsMenuPresentation results_menu_;
	PromptBarPresentation prompt_bar_;
	HudPresentation hud_;
	DebugOverlayPresentation debug_overlay_;
};
