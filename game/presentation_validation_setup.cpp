#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/player_profiles.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
#include "game/states/ui_player_badge.h"
#include "game/text_wrap.h"
#include "engine/render/sprite_geometry.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;
using namespace presentation_validation;

Presentation::Validation::RosterPapers Presentation::Validation::validate_additional_papers(const SetupGeometry& setup)
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	const auto& setup_edge = setup.setup_edge;
	const auto& confirmation_paper = setup.confirmation_paper;
	const auto& display_art = setup.display_art;
	const auto& options_art = setup.options_art;
	// Preserve the existing diagnostics when a shared font or shadow also
	// invalidates an earlier page, then check Team's additional paper bounds.
	const RectangleF team_paper((Vector2F(1920.0f, this->presentation_.menu_.footer_y) - main.team_box_size) / 2.0f,
		main.team_box_size);
	const RectangleF team_inside(team_paper.position() + Vector2F(setup_edge),
		team_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const auto check_team_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, const Vector2F& shadow)
	{
		drawable_text::check_font_can_draw(&render, font, words, this->presentation_.source_path_ + ": " + key);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !team_inside.contains(bounds))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit main_menu.team_box_size including its retained shadow bounds.");
		}
		return bounds;
	};
	const RectangleF team_heading = check_team_text("menu_text.headings.teams",
		this->presentation_.menu_text_.heading(MenuHeading::teams), this->presentation_.menu_.heading_font,
		this->presentation_.menu_.layout.position, this->presentation_.menu_.heading_shadow_offset);
	const RectangleF team_warning = check_team_text(
		"menu_text.connection.teams_required at main_menu.team_select.warning_position",
		this->presentation_.menu_text_.connection.teams_required, this->presentation_.menu_.detail_font,
		main.team_warning_position, this->presentation_.menu_.detail_shadow_offset);
	if (team_warning.y < team_heading.bottom())
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.connection.teams_required at main_menu.team_select.warning_position must follow the Team Select heading.");
	}
	const RectangleF weapon_paper((Vector2F(1920.0f, this->presentation_.menu_.footer_y) - main.weapon_box_size) / 2.0f,
		main.weapon_box_size);
	const RectangleF weapon_inside(weapon_paper.position() + Vector2F(setup_edge),
		weapon_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const Vector2F weapon_heading_size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.heading_font),
		this->presentation_.menu_text_.heading(MenuHeading::weapons));
	const RectangleF weapon_heading = RectangleF::union_of(
		RectangleF(this->presentation_.menu_.layout.position, weapon_heading_size),
		RectangleF(this->presentation_.menu_.layout.position + this->presentation_.menu_.heading_shadow_offset, weapon_heading_size));
	if (!std::isfinite(weapon_heading.x) || !std::isfinite(weapon_heading.y) ||
		!std::isfinite(weapon_heading.width) || !std::isfinite(weapon_heading.height) ||
		weapon_heading.width <= 0.0f || weapon_heading.height <= 0.0f || !weapon_inside.contains(weapon_heading))
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.headings.weapons must fit main_menu.weapon_box_size including its retained shadow bounds.");
	}
	const RectangleF stage_paper((Vector2F(1920.0f, this->presentation_.menu_.footer_y) - main.stage_box_size) / 2.0f,
		main.stage_box_size);
	const RectangleF stage_inside(stage_paper.position() + Vector2F(setup_edge),
		stage_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const std::wstring& stage_heading_words = this->presentation_.menu_text_.heading(MenuHeading::arena);
	// Loser's pick (ROAD#20) puts the picking team's heading in its place.
	const auto check_stage_heading = [&](const std::string& key, const std::wstring& words)
	{
		drawable_text::check_font_can_draw(&render, this->presentation_.menu_.heading_font, words, this->presentation_.source_path_ + ": " + key);
		const Vector2F stage_heading_size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.heading_font),
			words);
		const RectangleF stage_heading = RectangleF::union_of(
			RectangleF(this->presentation_.menu_.layout.position, stage_heading_size),
			RectangleF(this->presentation_.menu_.layout.position + this->presentation_.menu_.heading_shadow_offset, stage_heading_size));
		if (!std::isfinite(stage_heading.x) || !std::isfinite(stage_heading.y) ||
			!std::isfinite(stage_heading.width) || !std::isfinite(stage_heading.height) ||
			stage_heading.width <= 0.0f || stage_heading.height <= 0.0f || !stage_inside.contains(stage_heading))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit main_menu.stage_box_size including its retained shadow bounds.");
		}
	};
	check_stage_heading("menu_text.headings.arena", stage_heading_words);
	check_stage_heading("menu_text.stage.team_a_picks", this->presentation_.menu_text_.stage.team_a_picks);
	check_stage_heading("menu_text.stage.team_b_picks", this->presentation_.menu_text_.stage.team_b_picks);
	// Home's Exit confirmation reuses Pause paper/layout, with the existing
	// Exit label rendered once more in the actual heading font.
	const Vector2F exit_heading_size = fixed_menu_size("menu_text.items.exit",
		this->presentation_.menu_text_.item(MenuItem::exit), this->presentation_.menu_.heading_font);
	const RectangleF exit_heading((1920.0f - exit_heading_size.x) / 2.0f, pause.heading_y,
		exit_heading_size.x, exit_heading_size.y);
	if (!std::isfinite(exit_heading.x) || !std::isfinite(exit_heading.y) ||
		!confirmation_paper.contains(exit_heading) || exit_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.items.exit must fit unscaled inside the Pause paper before the Exit confirmation question.");
	}
	const Vector2F display_heading_size = fixed_menu_size("menu_text.headings.options",
		this->presentation_.menu_text_.heading(MenuHeading::options), this->presentation_.menu_.heading_font);
	const RectangleF display_heading((1920.0f - display_heading_size.x) / 2.0f, pause.heading_y,
		display_heading_size.x, display_heading_size.y);
	if (!std::isfinite(display_heading.x) || !std::isfinite(display_heading.y) ||
		!confirmation_paper.contains(display_heading) || display_heading.bottom() > pause.status_y)
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
		const RectangleF bounds((1920.0f - size.x) / 2.0f, countdown_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
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
	const std::string palette_timing_key = "menu_text.options_palette_timing";
	const Vector2F palette_timing_size = fixed_menu_size(palette_timing_key,
		this->presentation_.menu_text_.options_palette_timing, this->presentation_.menu_.detail_font);
	const RectangleF palette_timing(Vector2F(main.options_value_x, main.options_heading_position.y), palette_timing_size);
	check_fixed_paper(palette_timing_key, palette_timing, main.options_box_size, Vector2F::ZERO);
	const RectangleF options_heading(main.options_heading_position,
		fixed_menu_size("menu_text.headings.options", this->presentation_.menu_text_.heading(MenuHeading::options), this->presentation_.menu_.heading_font));
	if (palette_timing.x < options_heading.right() ||
		palette_timing.bottom() > main.options_position.y - options_art.focus_padding.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + palette_timing_key +
			" must fit unscaled beside the Options heading and above the first action's focus padding.");
	}

	return { team_inside, team_heading, team_warning, weapon_inside, weapon_heading };
}
