#pragma once

// Menu page geometry as a pure function of the presentation records. The
// pages draw with it, and the presentation reader and validators check the
// same rectangles before any page exists, so a box, an interior or a fitted
// width has one formula. It owns the design size the menus are authored in.
//
// It includes only labrador and the menu records, so the reader and the
// validators in game/config/presentation/ use it without including states/.

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/config/presentation/presentation_menu.h"

#include <cmath>

namespace menu_consts
{
	// Menu geometry stays in these design units; ui_camera() maps it to the window.
	inline const labrador::Vector2F design_resolution = { 1920.0f, 1080.0f };
}

// The value rows of the two settings pages, named once for the pages that
// place them and the validator that fit-checks them. Controls opens Sticks,
// Keyboard Keys, Pad Buttons and Profiles from rows 0-3; Apply and Back follow
// the value rows on both pages.
namespace settings_rows
{
	inline constexpr int controls_music = 4;
	inline constexpr int controls_sfx = 5;
	inline constexpr int controls_view_shake = 6;
	inline constexpr int controls_aim_marker = 7;
	inline constexpr int controls_reduce_effects = 8;

	inline constexpr int sticks_pad = 0;
	inline constexpr int sticks_movement = 1;
	inline constexpr int sticks_aim = 2;
	inline constexpr int sticks_rumble = 3;
	inline constexpr int sticks_view_shake = 4;
	inline constexpr int sticks_aim_marker = 5;
	inline constexpr int sticks_reduce_effects = 6;
}

namespace menu_geometry
{
	// The largest papers the reader accepts. Footer papers are at most this
	// wide; the setup pages' wider papers get 40 px a side, the scan page 20.
	inline constexpr float paper_max_width = 1760.0f;
	inline constexpr float setup_paper_max_width = 1840.0f;
	inline constexpr float scan_paper_max_width = 1880.0f;
	// Height a footer paper leaves free above menu_style.chrome.footer_y,
	// split evenly above and below it.
	inline constexpr float paper_footer_margin = 40.0f;
	// Overlay papers (Pause, End, Results) and the y values placed on them.
	inline constexpr float overlay_max_y = 900.0f;
	inline const labrador::Vector2F overlay_paper_max = { 1600.0f, 900.0f };

	// The whole design: backgrounds fill it and the UI camera frames it.
	labrador::RectangleF design_area();

	// The scale, never above 1, that fits size within available, per axis
	// or on both at once. A size or limit that is not positive does not
	// constrain its axis.
	float fit_scale(float size, float available);
	float fit_scale(const labrador::Vector2F& size, const labrador::Vector2F& available);

	// A rectangle whose edges are all finite and whose size is positive.
	inline bool finite_rectangle(const labrador::RectangleF& bounds)
	{
		return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
			std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
			std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
			bounds.width > 0.0f && bounds.height > 0.0f;
	}
	// A finite, non-empty rectangle that lies within container: what a page
	// checks its retained widgets against before it draws them on a paper.
	inline bool fits(const labrador::RectangleF& bounds, const labrador::RectangleF& container)
	{
		return finite_rectangle(bounds) && container.contains(bounds);
	}
	// Whether two rectangles share area; touching edges do not overlap.
	inline bool overlaps(const labrador::RectangleF& first, const labrador::RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	}

	// The x that centres a span of `size` in `extent`, and in the design width.
	float centred(float size, float extent);
	float centred_x(float width);

	// A paper centred above the footer (menu pages), or on the whole design
	// (Exit, Display and the match overlays).
	labrador::RectangleF footer_paper(const labrador::Vector2F& box, float footer_y);
	labrador::RectangleF screen_paper(const labrador::Vector2F& box);

	// The paper's face inside its keyline edge (three keylines) and shade.
	labrador::RectangleF paper_interior(const labrador::RectangleF& paper,
		const MenuPaperArtPresentation& art);
	// The rightmost x text may reach on a paper, clear of shade and focus card.
	float paper_text_right(const labrador::RectangleF& paper, const MenuPaperArtPresentation& art);

	// The chrome every page draws: the footer band below footer_y and its rule.
	labrador::RectangleF footer_band(const MenuStyle& style);
	labrador::RectangleF footer_rule(const MenuStyle& style);

	// The setup pages' paper: the main art with the setup focus card and shade.
	MenuPaperArtPresentation setup_art(const MainMenuPresentation& main);
	// Display confirmation draws the pause record with the setup card and shade.
	MenuPaperArtPresentation display_art(const PauseMenuPresentation& pause,
		const MainMenuPresentation& main);

	// Width an action label starting at x may use before the settings help column.
	float settings_action_width(const ControlsSettingsPresentation& look, float x);

	// The Results score bar, centred on the design and offset from the paper top.
	labrador::RectangleF results_fill_box(const ResultsMenuPresentation& look);
}
