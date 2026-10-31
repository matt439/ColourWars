#include "game/pch.h"

// Presentation validation for the End page. The phase functions in
// presentation_validation.cpp call these in diagnostic order.
#include "game/presentation_validation.h"
#include "game/menu_geometry.h"

#include <cmath>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

using namespace mattmath;

// Phase 1: End's actions in every row they can take.
void Presentation::Validation::validate_end_actions() const
{
	const EndMenuPresentation& end = this->presentation_.end_menu_;
	if (!std::isfinite(end.layout.spacing.y) || end.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.items.change_teams requires positive finite end_menu.layout.spacing.y.");
	}
	// Watch Replay is row 1 when the round has one, and the setup actions
	// follow it; without one they move up a row.
	check_menu_item(MenuItem::watch_replay, this->presentation_.menu_.item_font, end.layout.position +
		Vector2F(0.0f, end.layout.spacing.y), menu_geometry::screen_paper(end.box_size), end.layout.spacing.y, std::nullopt,
		end.art);
	const MenuItem end_items[]{ MenuItem::change_teams, MenuItem::change_weapons, MenuItem::change_level };
	for (size_t index = 0; index < std::size(end_items); ++index)
	{
		for (const size_t row : { index + 1, index + 2 })
		{
			check_menu_item(end_items[index], this->presentation_.menu_.item_font, end.layout.position +
				Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
				menu_geometry::screen_paper(end.box_size), end.layout.spacing.y, std::nullopt, end.art);
		}
	}
	// Exit is row 3 between a series' rounds and row 4 otherwise, a row lower
	// with Watch Replay. Rows 1 and 5 bound every row between them, which
	// also covers the setup actions moving up a row between rounds.
	for (const int row : { 1, 5 })
	{
		check_menu_item(MenuItem::exit_to_main_menu, this->presentation_.menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
			menu_geometry::screen_paper(end.box_size), end.layout.spacing.y, std::nullopt, end.art);
	}
}

// Phase 1: End's headings, before its first action's focus card.
void Presentation::Validation::validate_end_headings() const
{
	const EndMenuPresentation& end = this->presentation_.end_menu_;
	const RectangleF end_paper = menu_geometry::screen_paper(end.box_size);
	// The first continuation row starts here regardless of its Session label.
	const float first_end_action_top = end.layout.position.y - end.art.focus_padding.y;
	const auto check_end_heading = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.end.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.heading_font);
		const RectangleF bounds(menu_geometry::centred_x(size.x), end.heading_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(first_end_action_top) ||
			!end_paper.contains(bounds) || bounds.bottom() > first_end_action_top)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the End paper before the first action's focus padding.");
		}
	};
	check_end_heading("match_complete", this->presentation_.menu_text_.end.match_complete);
	check_end_heading("round_complete", this->presentation_.menu_text_.end.round_complete);
	check_end_heading("series_complete", this->presentation_.menu_text_.end.series_complete);
}

// Phase 9: the Session labels End's first action can take.
void Presentation::Validation::validate_end_session_labels() const
{
	const EndMenuPresentation& end = this->presentation_.end_menu_;
	// Session selects one of these plain, unscaled labels for End's first
	// action. Retain the same paper face, padding and row rule as its other
	// actions: the face stops short of the shade at its right and bottom.
	const RectangleF end_paper = menu_geometry::screen_paper(end.box_size);
	const Vector2F end_top = end_paper.position();
	const Vector2F end_bottom = end_top + end_paper.size() - Vector2F(end.art.shadow_offset);
	for (const auto& field : { std::pair("rematch", &this->presentation_.menu_text_.end.rematch),
		std::pair("play_series_again", &this->presentation_.menu_text_.end.play_series_again),
		std::pair("replay_draw", &this->presentation_.menu_text_.end.replay_draw),
		std::pair("next_round", &this->presentation_.menu_text_.end.next_round) })
	{
		const std::string key = std::string("menu_text.end.") + field.first;
		const Vector2F size = fixed_menu_size(key, *field.second, this->presentation_.menu_.item_font);
		const Vector2F position = end.layout.position;
		const Vector2F padding = end.art.focus_padding;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < end_top.x || position.y - padding.y < end_top.y ||
			position.x + size.x + padding.x > end_bottom.x ||
			position.y + size.y + padding.y > end_bottom.y ||
			size.y + padding.y * 2.0f > end.layout.spacing.y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit its paper and row including focus padding.");
		}
	}
}
