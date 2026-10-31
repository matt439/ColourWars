#include "game/pch.h"

// Presentation validation for Pause and practice Pause, and for the Exit and
// Display confirmations drawn on the same paper. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/presentation_validation.h"
#include "game/menu_geometry.h"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

using namespace labrador;
using namespace mattmath;

// Phase 1: Pause's actions, practice Pause's five rows in the same paper and
// the Tool row's name for every tool.
void Presentation::Validation::validate_pause_actions() const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	if (!std::isfinite(pause.layout.spacing.y) || pause.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.items.resume requires positive finite pause_menu.layout.spacing.y.");
	}
	const MenuItem pause_items[]{ MenuItem::resume, MenuItem::restart, MenuItem::options, MenuItem::quit };
	for (size_t index = 0; index < std::size(pause_items); ++index)
	{
		check_menu_item(pause_items[index], this->presentation_.menu_.item_font, pause.layout.position +
			Vector2F(0.0f, pause.layout.spacing.y * static_cast<float>(index)),
			menu_geometry::screen_paper(pause.box_size), pause.layout.spacing.y, std::nullopt, pause.art);
	}
	// Practice fits a fifth row, Tool, into the same paper at 0.8 spacing:
	// Resume, Tool, Restart, Options and Quit. Only the paper is checked: the
	// shipped rows are closer than a focus card is tall (HYG#37), so the row
	// rule would refuse them.
	const float practice_spacing = pause.layout.spacing.y * 0.8f;
	for (size_t index = 0; index < std::size(pause_items); ++index)
	{
		const int row = index == 0 ? 0 : static_cast<int>(index) + 1;
		check_menu_item(pause_items[index], this->presentation_.menu_.item_font, pause.layout.position +
			Vector2F(0.0f, practice_spacing * static_cast<float>(row)),
			menu_geometry::screen_paper(pause.box_size), 0.0f, std::nullopt, pause.art);
	}
	{
		// The Tool row names each real tool, unscaled. Glyph coverage stays
		// with the content check that names the same token.
		const RectangleF paper = menu_geometry::screen_paper(pause.box_size);
		const Vector2F face_bottom = paper.position() + paper.size() - Vector2F(pause.art.shadow_offset);
		const Vector2F position = pause.layout.position + Vector2F(0.0f, practice_spacing);
		const Vector2F padding = pause.art.focus_padding;
		const FontHandle font = render.resolve_sprite_font(this->presentation_.menu_.item_font);
		for (int tool = 0; tool < static_cast<int>(WeaponType::max_prim_wep); ++tool)
		{
			std::wstring label;
			try
			{
				label = this->presentation_.menu_text_.practice_tool_label(
					main.weapon_select_art.choice(static_cast<WeaponType>(tool)).name);
			}
			catch (const std::invalid_argument& error)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.practice_tool: " + error.what());
			}
			if (!render.can_render(font, label)) { continue; }
			const Vector2F size = render.measure_text(font, label);
			if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f ||
				position.x - padding.x < paper.x || position.y - padding.y < paper.y ||
				position.x + size.x + padding.x > face_bottom.x ||
				position.y + size.y + padding.y > face_bottom.y)
			{
				throw std::runtime_error(this->presentation_.source_path_ +
					": menu_text.practice_tool with main_menu.weapon_select.choices." +
					weapon_name(static_cast<WeaponType>(tool)) +
					".name must fit the practice Pause paper including focus padding.");
			}
		}
	}
}

// Phase 1: the Exit confirmation's Yes and No on the Pause paper.
void Presentation::Validation::validate_exit_actions() const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	check_menu_item(MenuItem::yes, this->presentation_.menu_.item_font, pause.layout.position,
		menu_geometry::screen_paper(pause.box_size), pause.layout.spacing.y, std::nullopt, pause.art);
	check_menu_item(MenuItem::no, this->presentation_.menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		menu_geometry::screen_paper(pause.box_size), pause.layout.spacing.y, std::nullopt, pause.art);
}

// Phase 1: the Display confirmation's Keep and Revert, in the setup shade.
void Presentation::Validation::validate_display_actions(const SetupGeometry& setup) const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& display_art = setup.display_art;
	check_menu_item(MenuItem::keep, this->presentation_.menu_.item_font, pause.layout.position,
		menu_geometry::screen_paper(pause.box_size), pause.layout.spacing.y, std::nullopt, display_art);
	check_menu_item(MenuItem::revert, this->presentation_.menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		menu_geometry::screen_paper(pause.box_size), pause.layout.spacing.y, std::nullopt, display_art);
}

// Phase 1: the Restart, Quit, Exit and Display questions, before their first
// action's focus card.
void Presentation::Validation::validate_confirmations(const SetupGeometry& setup) const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& display_art = setup.display_art;
	const auto& confirmation_paper = setup.confirmation_paper;
	RectangleF first_confirmation_action(pause.layout.position,
		fixed_menu_size("menu_text.items.yes", this->presentation_.menu_text_.item(MenuItem::yes), this->presentation_.menu_.item_font));
	first_confirmation_action.inflate(pause.art.focus_padding);
	// Display draws the same paper with the setup shade, so its rows stay on
	// the face that shade leaves.
	const RectangleF display_face(confirmation_paper.position(),
		confirmation_paper.size() - Vector2F(display_art.shadow_offset));
	const auto check_confirmation = [&](const char* role, const std::wstring& text, float action_top,
		const RectangleF& paper)
	{
		const std::string key = std::string("menu_text.confirmations.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.detail_font);
		// Confirmation questions remain plain, unscaled and centered on the
		// full-design Pause paper rather than the footer-centered menu paper.
		const RectangleF bounds(menu_geometry::centred_x(size.x), pause.status_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !paper.contains(bounds) ||
			bounds.bottom() > action_top)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper before the first action's focus padding.");
		}
	};
	check_confirmation("restart", this->presentation_.menu_text_.confirmations.restart, first_confirmation_action.y,
		confirmation_paper);
	check_confirmation("quit", this->presentation_.menu_text_.confirmations.quit, first_confirmation_action.y,
		confirmation_paper);
	check_confirmation("exit", this->presentation_.menu_text_.confirmations.exit, first_confirmation_action.y,
		confirmation_paper);
	check_confirmation("display", this->presentation_.menu_text_.confirmations.display,
		pause.layout.position.y - display_art.focus_padding.y, display_face);
}

// Phase 5: the Exit confirmation's heading.
void Presentation::Validation::validate_exit_heading(const SetupGeometry& setup) const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& confirmation_paper = setup.confirmation_paper;
	// Home's Exit confirmation reuses Pause paper/layout, with the existing
	// Exit label rendered once more in the actual heading font.
	const Vector2F exit_heading_size = fixed_menu_size("menu_text.items.exit",
		this->presentation_.menu_text_.item(MenuItem::exit), this->presentation_.menu_.heading_font);
	const RectangleF exit_heading(menu_geometry::centred_x(exit_heading_size.x), pause.heading_y,
		exit_heading_size.x, exit_heading_size.y);
	if (!std::isfinite(exit_heading.x) || !std::isfinite(exit_heading.y) ||
		!confirmation_paper.contains(exit_heading) || exit_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.items.exit must fit unscaled inside the Pause paper before the Exit confirmation question.");
	}
}

// Phase 5: the Display confirmation's heading and countdown, and the paper
// it shares with Pause.
void Presentation::Validation::validate_display_text(const SetupGeometry& setup) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& confirmation_paper = setup.confirmation_paper;
	const auto& display_art = setup.display_art;
	const Vector2F display_heading_size = fixed_menu_size("menu_text.headings.options",
		this->presentation_.menu_text_.heading(MenuHeading::options), this->presentation_.menu_.heading_font);
	const RectangleF display_heading(menu_geometry::centred_x(display_heading_size.x), pause.heading_y,
		display_heading_size.x, display_heading_size.y);
	// Display draws the Pause paper with the setup shade; its rows keep to
	// the face that shade leaves.
	const RectangleF display_face(confirmation_paper.position(),
		confirmation_paper.size() - Vector2F(display_art.shadow_offset));
	if (!std::isfinite(display_heading.x) || !std::isfinite(display_heading.y) ||
		!display_face.contains(display_heading) || display_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.headings.options must fit unscaled inside the Pause paper before the display confirmation question.");
	}
	RectangleF revert_card(pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		fixed_menu_size("menu_text.items.revert", this->presentation_.menu_text_.item(MenuItem::revert), this->presentation_.menu_.item_font));
	revert_card.inflate(display_art.focus_padding);
	const float countdown_y = pause.layout.position.y + pause.layout.spacing.y * 2.0f;
	for (int seconds = 0; seconds <= static_cast<int>(std::ceil(main.display_trial_seconds)); ++seconds)
	{
		const std::string key = "menu_text.display_trial_countdown";
		const Vector2F size = fixed_menu_size(key, this->presentation_.menu_text_.display_countdown(seconds), this->presentation_.menu_.detail_font);
		const RectangleF bounds(menu_geometry::centred_x(size.x), countdown_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !display_face.contains(bounds) ||
			bounds.y < revert_card.bottom())
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper after the Revert focus card for every countdown value.");
		}
	}
	// Display combines compact Main geometry with Pause's retained keyline.
	// Keep the same text-to-face clearance that each authored paper requires.
	if (display_art.focus_padding.x <= display_art.shadow_offset + display_art.keyline_width ||
		display_art.focus_padding.y <= display_art.shadow_offset + display_art.keyline_width)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": main_menu.setup_focus_padding must exceed main_menu.setup_shadow_offset + "
			"pause_menu.art.keyline_width on both axes for the Display confirmation paper.");
	}
	// Pause's own box check sees only Pause's shade, not the setup shade
	// Display draws the same paper with.
	if (pause.box_size.x <= display_art.shadow_offset + display_art.keyline_width * 6.0f ||
		pause.box_size.y <= display_art.shadow_offset + display_art.keyline_width * 6.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": pause_menu.box_size must exceed main_menu.setup_shadow_offset + "
			"pause_menu.art.keyline_width * 6 on both axes for the Display confirmation paper.");
	}
}

// Phase 6: Pause's connection headings and status.
void Presentation::Validation::validate_pause_connection(const SetupGeometry& setup) const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& confirmation_paper = setup.confirmation_paper;
	const MenuConnectionText& connection = this->presentation_.menu_text_.connection;
	const auto check_pause_connection = [&](const char* role, const std::wstring& words, bool heading)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		const Vector2F size = fixed_menu_size(key, words,
			heading ? this->presentation_.menu_.heading_font : this->presentation_.menu_.detail_font);
		// Pause uses plain UiText, centered on the full-design paper. It has
		// neither Main's footer centering nor retained text-shadow bounds.
		const RectangleF bounds(menu_geometry::centred_x(size.x),
			heading ? pause.heading_y : pause.status_y, size.x, size.y);
		const float bottom = heading ? pause.status_y : pause.layout.position.y - pause.art.focus_padding.y;
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(bottom) ||
			!confirmation_paper.contains(bounds) || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper in heading, status, first-action order including the action's focus padding.");
		}
	};
	check_pause_connection("pause_title", connection.pause_title, true);
	for (int number = 1; number <= 4; ++number)
	{
		check_pause_connection("pause_player_title", this->presentation_.menu_text_.pause_title_for_player(number), true);
	}
	check_pause_connection("controller_lost", connection.controller_lost, true);
	check_pause_connection("all_connected", connection.all_connected, false);
}

// Phase 6: every list of missing players Pause can show.
void Presentation::Validation::validate_pause_missing_lists(const SetupGeometry& setup) const
{
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& confirmation_paper = setup.confirmation_paper;
	// A missing list contains distinct pad slots in roster order; keyboard
	// never goes missing. Skipping a seat or choosing one remaining pad gives
	// all 208 nonempty reachable lists, with the existing two-column joins.
	const std::string missing_key = "menu_text.labels.roster_row with menu_text.labels.pad at Pause";
	const auto missing_lists = [&](const auto& self, int player, unsigned int used,
		int count, const std::wstring& words) -> void
	{
		if (player == 4)
		{
			if (count == 0) { return; }
			const Vector2F size = label_size(missing_key, words, this->presentation_.menu_.detail_font);
			const RectangleF bounds(menu_geometry::centred_x(size.x), pause.status_y, size.x, size.y);
			check_label_bounds(missing_key, bounds, confirmation_paper);
			if (!std::isfinite(pause.layout.position.y) || !std::isfinite(pause.art.focus_padding.y) ||
				bounds.bottom() > pause.layout.position.y - pause.art.focus_padding.y)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + missing_key +
					" must fit before the first Pause action including its focus padding.");
			}
			return;
		}
		self(self, player + 1, used, count, words);
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const unsigned int bit = 1u << slot;
			if ((used & bit) != 0u) { continue; }
			std::wstring next = words;
			if (count > 0) { next += count % 2 == 0 ? L"\n" : L"   "; }
			next += this->presentation_.menu_text_.roster_label(player, slot);
			self(self, player + 1, used | bit, count + 1, next);
		}
	};
	missing_lists(missing_lists, 0, 0u, 0, std::wstring());
}
