#include "game/pch.h"

// Presentation validation shared by every menu page: the chrome (resources,
// brand, headings and prompts), Home, Credits and Title, and the status-line
// helpers Options and the settings pages share. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/config/presentation/presentation_validation.h"
#include "game/config/presentation/menu_geometry.h"
#include "game/input/input_bindings.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace labrador;
using namespace presentation_read;
using menu_consts::design_resolution;

// Phase 1: the resources every menu page draws with, the brand line, every
// heading at its page's position and every prompt in the prompt bar.
void Presentation::Validation::validate_menu_chrome() const
{
	check.frame("menu_style.fill_frame", this->presentation.menu_.sprite_sheet, this->presentation.menu_.fill_frame);
	const char* roles[] = { "a", "center", "b" };
	for (size_t index = 0; index < std::size(roles); ++index)
	{
		check.frame(std::string("main_menu.team_select.frames.") + roles[index],
			this->presentation.main_menu_.team_select_sheet, this->presentation.main_menu_.team_select_frames[index]);
		check.frame(std::string("main_menu.team_select.selected_frames.") + roles[index],
			this->presentation.main_menu_.team_select_sheet, this->presentation.main_menu_.team_select_selected_frames[index]);
	}
	check.font("menu_style.fonts.heading", this->presentation.menu_.heading_font);
	check.font("menu_style.fonts.item", this->presentation.menu_.item_font);
	check.font("menu_style.fonts.detail", this->presentation.menu_.detail_font);
	check.font("menu_style.chrome.brand_font", this->presentation.menu_.brand_font);
	check.font("main_menu.fonts.title", this->presentation.main_menu_.title_font);
	check.font("main_menu.fonts.announcement", this->presentation.main_menu_.announcement_font);
	check.font("main_menu.fonts.options", this->presentation.main_menu_.options_font);
	check.font("main_menu.fonts.weapon_description", this->presentation.main_menu_.weapon_description_font);
	check.font("main_menu.credits.font", this->presentation.main_menu_.credits_font);
	check.font("controls_settings.help_font", this->presentation.controls_settings_.help_font);
	check.font("controls_settings.item_font", this->presentation.controls_settings_.item_font);
	check.font("results_menu.contribution_font", this->presentation.results_menu_.contribution_font);
	check.font("main_menu.stage_select.record_font", this->presentation.main_menu_.stage_select.record_font);
	check.font("prompt_bar.font", this->presentation.prompt_bar_.font);
	check.font("debug_overlay.font", this->presentation.debug_overlay_.font);
	check.font("debug_overlay.station_label.font", this->presentation.debug_overlay_.station_label.font);
	check.font("hud.timer.font", this->presentation.hud_.timer_font);
	check_menu_line("menu_text.brand", this->presentation.menu_text_.brand, this->presentation.menu_.brand_font,
		this->presentation.menu_.brand_position, this->presentation.menu_.footer_y, Vector2F::zero);
	for (size_t index = 0; index < this->presentation.menu_text_.headings.size(); ++index)
	{
		Vector2F position = this->presentation.menu_.layout.position;
		switch (static_cast<MenuHeading>(index))
		{
		case MenuHeading::home: position = this->presentation.main_menu_.home_position; break;
		case MenuHeading::options: position = this->presentation.main_menu_.options_heading_position; break;
		case MenuHeading::credits: position = this->presentation.main_menu_.credits_heading_position; break;
		case MenuHeading::controls_audio:
		case MenuHeading::pad_sticks: position = this->presentation.controls_settings_.heading_position; break;
		default: break;
		}
		check_menu_line(std::string("menu_text.headings.") + menu_heading_keys[index],
			this->presentation.menu_text_.headings[index], this->presentation.menu_.heading_font, position,
			this->presentation.menu_.footer_y, this->presentation.menu_.heading_shadow_offset);
	}
	for (size_t index = 0; index < this->presentation.menu_text_.prompts.size(); ++index)
	{
		check_menu_line(std::string("menu_text.prompts.") + menu_prompt_keys[index],
			this->presentation.menu_text_.prompts[index], this->presentation.prompt_bar_.font, this->presentation.prompt_bar_.position,
			design_resolution.y, this->presentation.prompt_bar_.shadow_offset);
	}
}

// Phase 1: Home's four actions.
void Presentation::Validation::validate_home_actions() const
{
	const MainMenuPresentation& main = this->presentation.main_menu_;
	const MenuItem home_items[]{ MenuItem::play, MenuItem::options, MenuItem::credits, MenuItem::exit };
	for (size_t index = 0; index < std::size(home_items); ++index)
	{
		check_menu_item(home_items[index], this->presentation.menu_.item_font, main.home_position +
			Vector2F(0.0f, main.home_spacing.y * static_cast<float>(index + 2)),
			menu_geometry::footer_paper(main.home_box_size, this->presentation.menu_.footer_y), main.home_spacing.y, std::nullopt, main.art);
	}
}

// Phase 1: Credits' Back and its fitted body.
void Presentation::Validation::validate_credits_text() const
{
	const MainMenuPresentation& main = this->presentation.main_menu_;
	check_menu_item(MenuItem::back, this->presentation.menu_.item_font, main.credits_back_position,
		menu_geometry::footer_paper(main.credits_box_size, this->presentation.menu_.footer_y), 0.0f, std::nullopt,
		main.art);
	{
		// Credits fits its body uniformly into the paper face beside it and the
		// room above Back's focus card, and skips the fit when either is empty.
		const std::string key = "main_menu.credits.text";
		const Vector2F raw = fixed_menu_size(key, main.credits_text, main.credits_font);
		const Vector2F available(menu_geometry::paper_text_right(
			menu_geometry::footer_paper(main.credits_box_size, this->presentation.menu_.footer_y), main.art) -
			main.credits_position.x,
			main.credits_back_position.y - main.art.focus_padding.y - main.credits_position.y);
		const float scale = std::min({ 1.0f, available.x / raw.x, available.y / raw.y });
		if (!std::isfinite(available.x) || !std::isfinite(available.y) || available.x <= 0.0f ||
			available.y <= 0.0f || !std::isfinite(scale) || scale <= 0.0f)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				" must have a finite positive fitting area inside the Credits paper above Back's focus padding.");
		}
	}
}

// Phase 3: the Title page's centred lines in order, then Home's subtitle
// between its heading and Play's focus card.
void Presentation::Validation::validate_title_and_home_text() const
{
	const MainMenuPresentation& main = this->presentation.main_menu_;
	const auto centered_title = [&](const std::string& key, const std::wstring& text,
		const std::string& font, float y, const Vector2F& padding)
	{
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds(menu_geometry::centred_x(size.x), y, size.x, size.y);
		check_fixed_paper(key, bounds, main.title_box_size, padding);
		return bounds;
	};
	const MenuTitleText& title = this->presentation.menu_text_.title;
	const RectangleF wordmark = centered_title("menu_text.title.wordmark", title.wordmark,
		main.title_font, main.title_y, Vector2F::zero);
	const RectangleF subtitle = centered_title("menu_text.title.subtitle", title.subtitle,
		this->presentation.menu_.detail_font, main.title_subtitle_y, Vector2F::zero);
	RectangleF start = centered_title("menu_text.title.start", title.start,
		this->presentation.menu_.item_font, main.title_start_y, main.art.focus_padding);
	start.inflate(main.art.focus_padding);
	const RectangleF no_controller = centered_title("menu_text.title.no_controller", title.no_controller,
		this->presentation.menu_.item_font, main.title_no_pad_y, Vector2F::zero);
	// The build line's fields are generated, but it still shares the title's paper.
	const RectangleF build = centered_title("menu_text.title.build", title.build_label(),
		this->presentation.prompt_bar_.font, main.build_id_y, Vector2F::zero);
	const auto precedes = [&](const std::string& key, const RectangleF& before, const RectangleF& after)
	{
		if (before.bottom() > after.y)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				" must end before the next menu text or focus card.");
		}
	};
	precedes("menu_text.title.wordmark", wordmark, subtitle);
	precedes("menu_text.title.subtitle", subtitle, start);
	precedes("menu_text.title.start", start, no_controller);
	precedes("menu_text.title.no_controller", no_controller, build);
	const RectangleF home_subtitle(main.home_subtitle_position,
		fixed_menu_size("menu_text.home_subtitle", this->presentation.menu_text_.home_subtitle, this->presentation.menu_.detail_font));
	check_fixed_paper("menu_text.home_subtitle", home_subtitle, main.home_box_size, Vector2F::zero);
	const RectangleF home_heading(main.home_position,
		fixed_menu_size("menu_text.headings.home", this->presentation.menu_text_.heading(MenuHeading::home), this->presentation.menu_.heading_font));
	check_fixed_paper("menu_text.headings.home", home_heading, main.home_box_size, Vector2F::zero);
	precedes("menu_text.home_subtitle", home_heading, home_subtitle);
	RectangleF play(main.home_position + Vector2F(0.0f, main.home_spacing.y * 2.0f),
		fixed_menu_size("menu_text.items.play", this->presentation.menu_text_.item(MenuItem::play), this->presentation.menu_.item_font));
	play.inflate(main.art.focus_padding);
	precedes("menu_text.home_subtitle", home_subtitle, play);
}

// Back's focus card at a position, for the status lines that keep clear of it.
RectangleF Presentation::Validation::padded_back(const Vector2F& position, const std::string& font,
	const MenuPaperArtPresentation& art) const
{
	RectangleF bounds(position, fixed_menu_size("menu_text.items.back", this->presentation.menu_text_.item(MenuItem::back), font));
	bounds.inflate(art.focus_padding);
	return bounds;
}

// Phase 3: the Back cards of Options, Controls, Pad Buttons and Sticks, measured
// together before any status line. Keyboard Keys and Profiles measure theirs
// with their own status lines.
Presentation::Validation::StatusBacks Presentation::Validation::status_back_cards(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation.main_menu_;
	const ControlsSettingsPresentation& controls = this->presentation.controls_settings_;
	const auto& options_art = setup.options_art;
	const RectangleF options_back = padded_back(main.options_position + Vector2F(0.0f, main.options_spacing.y * 7.0f),
		main.options_font, options_art);
	const RectangleF controls_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 10.0f),
		controls.item_font, controls.art);
	// Pad Buttons: the Pad row, its entries, Reset and Apply, then Back.
	const RectangleF pad_buttons_back = padded_back(controls.layout.position + Vector2F(0.0f,
		controls.layout.spacing.y * static_cast<float>(pad_entry_count + 3)),
		controls.item_font, controls.art);
	const RectangleF sticks_back = padded_back(controls.layout.position + Vector2F(0.0f,
		controls.layout.spacing.y * static_cast<float>(settings_rows::sticks_back)),
		controls.item_font, controls.art);

	return { options_back, controls_back, pad_buttons_back, sticks_back };
}

// A status line, fitted to its paper by width and clear of each obstacle.
// Options and every settings page share it.
void Presentation::Validation::check_status(const StatusMessage& message, const std::string& font,
	const Vector2F& position, const Vector2F& paper_size, const MenuPaperArtPresentation& art,
	std::initializer_list<RectangleF> obstacles) const
{
	const std::string_view message_key(message.key);
	const std::string key = std::string(message_key.starts_with("keyboard_jump.") || message_key.starts_with("pad_buttons.") ||
		message_key.starts_with("profiles.") ?
		"menu_text." : "menu_text.status.") + message.key;
	const Vector2F raw = fixed_menu_size(key, message.text, font);
	const float available = menu_geometry::paper_text_right(
		menu_geometry::footer_paper(paper_size, this->presentation.menu_.footer_y), art) - position.x;
	if (!std::isfinite(available) || available <= 0.0f)
	{
		throw std::runtime_error(this->presentation.source_path_ + ": " + key + " must leave finite positive status width.");
	}
	// Every page that draws a status line resets it to the authored font
	// size, then fits its width only.
	const float scale = std::min(1.0f, available / raw.x);
	const Vector2F fitted = raw * scale;
	if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
		fitted.x <= 0.0f || fitted.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation.source_path_ + ": " + key + " must retain finite positive fitted status bounds.");
	}
	const RectangleF bounds(position, fitted);
	check_fixed_paper(key, bounds, paper_size, Vector2F::zero);
	for (const RectangleF& obstacle : obstacles)
	{
		// Touching edges are allowed. Options' status and Back deliberately
		// share vertical space while remaining horizontally separate.
		if (overlaps(bounds, obstacle))
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				" must not overlap the Back card, help text or stick previews.");
		}
	}
}
