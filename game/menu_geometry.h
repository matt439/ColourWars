#pragma once

// Menu page geometry as a pure function of the presentation records. The
// pages draw with it, and the presentation reader and validators check the
// same rectangles before any page exists, so a box, an interior or a fitted
// width has one formula. It owns the design size the menus are authored in.
//
// It includes only mattmath and the menu records, so the reader and the
// validators reach it from the game/ root without including states/.

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "game/presentation_menu.h"

namespace menu_consts
{
	// Menu geometry stays in these design units; ui_camera() maps it to the window.
	inline const mattmath::Vector2F DESIGN_RESOLUTION = { 1920.0f, 1080.0f };
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
	inline const mattmath::Vector2F overlay_paper_max = { 1600.0f, 900.0f };

	// The whole design: backgrounds fill it and the UI camera frames it.
	mattmath::RectangleF design_area();

	// The x that centres a span of `size` in `extent`, and in the design width.
	float centred(float size, float extent);
	float centred_x(float width);

	// A paper centred above the footer (menu pages), or on the whole design
	// (Exit, Display and the match overlays).
	mattmath::RectangleF footer_paper(const mattmath::Vector2F& box, float footer_y);
	mattmath::RectangleF screen_paper(const mattmath::Vector2F& box);

	// The paper's face inside its keyline edge (three keylines) and shade.
	mattmath::RectangleF paper_interior(const mattmath::RectangleF& paper,
		const MenuPaperArtPresentation& art);
	// The rightmost x text may reach on a paper, clear of shade and focus card.
	float paper_text_right(const mattmath::RectangleF& paper, const MenuPaperArtPresentation& art);

	// The chrome every page draws: the footer band below footer_y and its rule.
	mattmath::RectangleF footer_band(const MenuStyle& style);
	mattmath::RectangleF footer_rule(const MenuStyle& style);

	// The setup pages' paper: the main art with the setup focus card and shade.
	MenuPaperArtPresentation setup_art(const MainMenuPresentation& main);
	// Display confirmation draws the pause record with the setup card and shade.
	MenuPaperArtPresentation display_art(const PauseMenuPresentation& pause,
		const MainMenuPresentation& main);

	// Width an action label starting at x may use before the settings help column.
	float settings_action_width(const ControlsSettingsPresentation& look, float x);

	// The Results score bar, centred on the design and offset from the paper top.
	mattmath::RectangleF results_fill_box(const ResultsMenuPresentation& look);
}
