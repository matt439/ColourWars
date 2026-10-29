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

Presentation::Validation::SetupGeometry Presentation::Validation::validate_menu_actions()
{
	check.frame("menu_style.fill_frame", this->presentation_.menu_.sprite_sheet, this->presentation_.menu_.fill_frame);
	const char* roles[] = { "a", "center", "b" };
	for (size_t index = 0; index < std::size(roles); ++index)
	{
		check.frame(std::string("main_menu.team_select.frames.") + roles[index],
			this->presentation_.main_menu_.team_select_sheet, this->presentation_.main_menu_.team_select_frames[index]);
		check.frame(std::string("main_menu.team_select.selected_frames.") + roles[index],
			this->presentation_.main_menu_.team_select_sheet, this->presentation_.main_menu_.team_select_selected_frames[index]);
	}
	check.font("menu_style.fonts.heading", this->presentation_.menu_.heading_font);
	check.font("menu_style.fonts.item", this->presentation_.menu_.item_font);
	check.font("menu_style.fonts.detail", this->presentation_.menu_.detail_font);
	check.font("menu_style.chrome.brand_font", this->presentation_.menu_.brand_font);
	check.frame("main_menu.backdrop.frame", this->presentation_.main_menu_.backdrop_sheet,
		this->presentation_.main_menu_.backdrop_frame);
	check.font("main_menu.fonts.title", this->presentation_.main_menu_.title_font);
	check.font("main_menu.fonts.announcement", this->presentation_.main_menu_.announcement_font);
	check.font("main_menu.fonts.weapon_description", this->presentation_.main_menu_.weapon_description_font);
	check.font("main_menu.credits.font", this->presentation_.main_menu_.credits_font);
	check.font("controls_settings.help_font", this->presentation_.controls_settings_.help_font);
	check.font("controls_settings.item_font", this->presentation_.controls_settings_.item_font);
	check.font("results_menu.contribution_font", this->presentation_.results_menu_.contribution_font);
	check.font("main_menu.stage_select.record_font", this->presentation_.main_menu_.stage_select.record_font);
	check.font("prompt_bar.font", this->presentation_.prompt_bar_.font);
	check.font("debug_overlay.font", this->presentation_.debug_overlay_.font);
	check.font("debug_overlay.station_label.font", this->presentation_.debug_overlay_.station_label.font);
	check.font("hud.timer.font", this->presentation_.hud_.timer_font);
	check_menu_line("menu_text.brand", this->presentation_.menu_text_.brand, this->presentation_.menu_.brand_font,
		this->presentation_.menu_.brand_position, this->presentation_.menu_.footer_y, Vector2F::ZERO);
	for (size_t index = 0; index < this->presentation_.menu_text_.headings.size(); ++index)
	{
		Vector2F position = this->presentation_.menu_.layout.position;
		switch (static_cast<MenuHeading>(index))
		{
		case MenuHeading::home: position = this->presentation_.main_menu_.home_position; break;
		case MenuHeading::options: position = this->presentation_.main_menu_.options_heading_position; break;
		case MenuHeading::credits: position = this->presentation_.main_menu_.credits_heading_position; break;
		case MenuHeading::controls_audio:
		case MenuHeading::pad_sticks: position = this->presentation_.controls_settings_.heading_position; break;
		default: break;
		}
		check_menu_line(std::string("menu_text.headings.") + menu_heading_keys[index],
			this->presentation_.menu_text_.headings[index], this->presentation_.menu_.heading_font, position,
			this->presentation_.menu_.footer_y, this->presentation_.menu_.heading_shadow_offset);
	}
	for (size_t index = 0; index < this->presentation_.menu_text_.prompts.size(); ++index)
	{
		check_menu_line(std::string("menu_text.prompts.") + menu_prompt_keys[index],
			this->presentation_.menu_text_.prompts[index], this->presentation_.prompt_bar_.font, this->presentation_.prompt_bar_.position,
			1080.0f, this->presentation_.prompt_bar_.shadow_offset);
	}
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	MenuPaperArtPresentation options_art = main.art;
	options_art.focus_padding = main.setup_focus_padding;
	options_art.shadow_offset = main.setup_shadow_offset;
	const MenuItem home_items[]{ MenuItem::play, MenuItem::options, MenuItem::credits, MenuItem::exit };
	for (size_t index = 0; index < std::size(home_items); ++index)
	{
		check_menu_item(home_items[index], this->presentation_.menu_.item_font, main.home_position +
			Vector2F(0.0f, main.home_spacing.y * static_cast<float>(index + 2)),
			main.home_box_size, main.home_spacing.y, std::nullopt, main.art, this->presentation_.menu_.footer_y);
	}
	const MenuItem options_items[]{ MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette,
		MenuItem::controls_audio, MenuItem::restore_defaults, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(options_items); ++index)
	{
		const Vector2F position = main.options_position +
			Vector2F(0.0f, main.options_spacing.y * static_cast<float>(index));
		const std::optional<float> label_width = index < 3 ?
			std::optional<float>(main.options_value_x - position.x - options_art.focus_padding.x) : std::nullopt;
		check_menu_item(options_items[index], this->presentation_.menu_.item_font, position,
			main.options_box_size, main.options_spacing.y, label_width, options_art, this->presentation_.menu_.footer_y);
	}
	check_menu_item(MenuItem::back, this->presentation_.menu_.item_font, main.credits_back_position, main.credits_box_size, 0.0f, std::nullopt,
		main.art, this->presentation_.menu_.footer_y);
	const PauseMenuPresentation& pause = this->presentation_.pause_menu_;
	MenuPaperArtPresentation display_art = pause.art;
	display_art.focus_padding = main.setup_focus_padding;
	display_art.shadow_offset = main.setup_shadow_offset;
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
			pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	}
	check_menu_item(MenuItem::yes, this->presentation_.menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, pause.art, 1080.0f);
	check_menu_item(MenuItem::no, this->presentation_.menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	check_menu_item(MenuItem::keep, this->presentation_.menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, display_art, 1080.0f);
	check_menu_item(MenuItem::revert, this->presentation_.menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, display_art, 1080.0f);
	const EndMenuPresentation& end = this->presentation_.end_menu_;
	if (!std::isfinite(end.layout.spacing.y) || end.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.items.change_teams requires positive finite end_menu.layout.spacing.y.");
	}
	// Watch Replay is row 1 when the round has one, and the setup actions
	// follow it; without one they move up a row.
	check_menu_item(MenuItem::watch_replay, this->presentation_.menu_.item_font, end.layout.position +
		Vector2F(0.0f, end.layout.spacing.y), end.box_size, end.layout.spacing.y, std::nullopt,
		end.art, 1080.0f);
	const MenuItem end_items[]{ MenuItem::change_teams, MenuItem::change_weapons, MenuItem::change_level };
	for (size_t index = 0; index < std::size(end_items); ++index)
	{
		for (const size_t row : { index + 1, index + 2 })
		{
			check_menu_item(end_items[index], this->presentation_.menu_.item_font, end.layout.position +
				Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
				end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
		}
	}
	// Exit is row 3 between a series' rounds and row 4 otherwise, a row lower
	// with Watch Replay. Rows 1 and 5 bound every row between them, which
	// also covers the setup actions moving up a row between rounds.
	for (const int row : { 1, 5 })
	{
		check_menu_item(MenuItem::exit_to_main_menu, this->presentation_.menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
			end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
	}
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const auto check_controls_item = [&](MenuItem item, int row, bool fitted_label)
	{
		const Vector2F position = controls.layout.position +
			Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row));
		const bool fitted_action = item == MenuItem::keyboard_jump || item == MenuItem::pad_buttons ||
			item == MenuItem::profiles || item == MenuItem::apply;
		const std::optional<float> label_width = fitted_action ?
			std::optional<float>(controls.controls_position.x - controls.art.focus_padding.x * 2.0f - controls.art.shadow_offset - position.x) :
			(fitted_label ? std::optional<float>(controls.layout.spacing.x - controls.art.focus_padding.x) : std::nullopt);
		const Vector2F size = check_menu_item(item, controls.item_font, position,
			controls.box_size, controls.layout.spacing.y, label_width, controls.art, this->presentation_.menu_.footer_y, fitted_action);
		if (position.x + size.x + controls.art.focus_padding.x > controls.controls_position.x)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.items." +
				menu_item_keys[static_cast<size_t>(item)] + " must fit before the controls help column including focus padding.");
		}
		if (position.y + size.y + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.items." +
				menu_item_keys[static_cast<size_t>(item)] + " must fit above the controls status line including focus padding.");
		}
	};
	const MenuItem controls_items[]{ MenuItem::pad_sticks, MenuItem::keyboard_jump, MenuItem::pad_buttons, MenuItem::profiles, MenuItem::music_volume,
		MenuItem::sfx_volume, MenuItem::view_shake, MenuItem::aim_marker, MenuItem::reduce_effects, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(controls_items); ++index)
	{
		check_controls_item(controls_items[index], static_cast<int>(index), index >= 4 && index <= 8);
	}
	const MenuItem sticks_items[]{ MenuItem::pad, MenuItem::move_deadzone, MenuItem::aim_deadzone,
		MenuItem::rumble, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(sticks_items); ++index)
	{
		check_controls_item(sticks_items[index], static_cast<int>(index), index < 4);
	}
	check_controls_item(MenuItem::apply, 3, false);
	check_controls_item(MenuItem::back, 4, false);
	const RectangleF setup_paper((Vector2F(1920.0f, this->presentation_.menu_.footer_y) - main.setup_box_size) / 2.0f,
		main.setup_box_size);
	const float setup_edge = main.art.keyline_width * 3.0f;
	const RectangleF setup_inside(setup_paper.position() + Vector2F(setup_edge),
		setup_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	float setup_heading_bottom = 0.0f;
	for (const MenuHeading role : { MenuHeading::mode, MenuHeading::players })
	{
		const RectangleF heading = check_setup_paper(setup_inside,
			std::string("menu_text.headings.") + menu_heading_keys[static_cast<size_t>(role)],
			this->presentation_.menu_text_.heading(role), this->presentation_.menu_.heading_font, this->presentation_.menu_.layout.position,
			this->presentation_.menu_.heading_shadow_offset, false, 0.0f);
		setup_heading_bottom = std::max(setup_heading_bottom, heading.bottom());
	}
	check_setup_paper(setup_inside, "menu_text.headings.join_players", this->presentation_.menu_text_.heading(MenuHeading::join_players),
		this->presentation_.menu_.heading_font, this->presentation_.menu_.layout.position, this->presentation_.menu_.heading_shadow_offset, false, 0.0f);
	const auto check_setup_item = [&](MenuItem item, const Vector2F& spacing, int row, bool duration_label)
	{
		const std::string key = std::string("menu_text.items.") + menu_item_keys[static_cast<size_t>(item)];
		const std::string where = this->presentation_.source_path_ + ": " + key;
		const Vector2F shadow = this->presentation_.menu_.item_shadow_offset;
		if (!std::isfinite(spacing.y) || spacing.y <= 0.0f ||
			(duration_label && (!std::isfinite(spacing.x) || spacing.x <= 0.0f)))
		{
			throw std::runtime_error(where + " requires positive finite setup row/column spacing.");
		}
		const Vector2F position = this->presentation_.menu_.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
		const std::wstring& text = this->presentation_.menu_text_.item(item);
		check_menu_line(key, text, this->presentation_.menu_.item_font, position, this->presentation_.menu_.footer_y, shadow);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(this->presentation_.menu_.item_font), text);
		if (size.y + std::abs(shadow.y) > spacing.y)
		{
			throw std::runtime_error(where + " must fit its setup row including its item shadow.");
		}
		if (duration_label && size.x + std::abs(shadow.x) > spacing.x * 2.0f)
		{
			// The existing, unfitted OptionRow puts its value two columns later.
			throw std::runtime_error(where + " must end before its duration value column including its item shadow.");
		}
		const float value_x = position.x + spacing.x * 2.0f;
		if (duration_label && (!std::isfinite(value_x) ||
			value_x + main.setup_focus_padding.x + main.setup_shadow_offset >= setup_inside.right()))
		{
			throw std::runtime_error(where +
				" must leave positive duration value space inside main_menu.setup_box_size.");
		}
		const RectangleF card = check_setup_paper(setup_inside, key, text, this->presentation_.menu_.item_font, position, shadow, true, spacing.y);
		if (row == 2 && card.y < setup_heading_bottom)
		{
			throw std::runtime_error(where + " must leave its setup heading clear of the first focus card.");
		}
		return card;
	};
	// Mode rows 2-7; Match length and Series stage carry a value column.
	const MenuItem mode_items[]{ MenuItem::single_match, MenuItem::best_of_three, MenuItem::best_of_five,
		MenuItem::match_length, MenuItem::series_stage, MenuItem::back };
	float single_match_right = 0.0f;
	std::vector<RectangleF> mode_cards;
	for (size_t index = 0; index < std::size(mode_items); ++index)
	{
		const RectangleF card = check_setup_item(mode_items[index], main.mode_select_spacing, static_cast<int>(index) + 2,
			mode_items[index] == MenuItem::match_length || mode_items[index] == MenuItem::series_stage);
		if (mode_items[index] == MenuItem::single_match) { single_match_right = card.right(); }
		else { mode_cards.push_back(card); }
	}
	const MenuItem player_items[]{ MenuItem::one_player_practice, MenuItem::two_players,
		MenuItem::three_players, MenuItem::four_players, MenuItem::back };
	for (size_t index = 0; index < std::size(player_items); ++index)
	{
		check_setup_item(player_items[index], main.player_count_spacing, static_cast<int>(index) + 2, false);
	}
	// The nonfocusable hint retains the detail font and the item-shadow offset
	// that its existing widget uses; it has no action-row fit or focus padding.
	// Beside Single Match on row 2, one card padding after its complete card.
	const Vector2F hint_position(single_match_right + main.setup_focus_padding.x,
		this->presentation_.menu_.layout.position.y + main.mode_select_spacing.y * 2.0f);
	check_menu_line("menu_text.solo_practice_hint", this->presentation_.menu_text_.solo_practice_hint, this->presentation_.menu_.detail_font,
		hint_position, this->presentation_.menu_.footer_y, this->presentation_.menu_.item_shadow_offset);
	const RectangleF practice_hint = check_setup_paper(setup_inside,
		"menu_text.solo_practice_hint", this->presentation_.menu_text_.solo_practice_hint, this->presentation_.menu_.detail_font,
		hint_position, this->presentation_.menu_.item_shadow_offset, false, 0.0f);
	// Beside Single Match it must still keep clear of every lower row's card,
	// as it once had to follow Back's.
	if (std::any_of(mode_cards.begin(), mode_cards.end(),
		[&practice_hint](const RectangleF& card) { return card.intersects(practice_hint); }))
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.solo_practice_hint must stay clear of the Mode rows' complete focus cards and shade.");
	}
	const RectangleF confirmation_paper((Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f, pause.box_size);
	RectangleF first_confirmation_action(pause.layout.position,
		fixed_menu_size("menu_text.items.yes", this->presentation_.menu_text_.item(MenuItem::yes), this->presentation_.menu_.item_font));
	first_confirmation_action.inflate(pause.art.focus_padding);
	const auto check_confirmation = [&](const char* role, const std::wstring& text, float action_top)
	{
		const std::string key = std::string("menu_text.confirmations.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.detail_font);
		// Confirmation questions remain plain, unscaled and centered on the
		// full-design Pause paper rather than the footer-centered menu paper.
		const RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
			bounds.bottom() > action_top)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper before the first action's focus padding.");
		}
	};
	check_confirmation("restart", this->presentation_.menu_text_.confirmations.restart, first_confirmation_action.y);
	check_confirmation("quit", this->presentation_.menu_text_.confirmations.quit, first_confirmation_action.y);
	check_confirmation("exit", this->presentation_.menu_text_.confirmations.exit, first_confirmation_action.y);
	check_confirmation("display", this->presentation_.menu_text_.confirmations.display, pause.layout.position.y - display_art.focus_padding.y);
	const RectangleF end_paper((Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f, end.box_size);
	// The first continuation row starts here regardless of its Session label.
	const float first_end_action_top = end.layout.position.y - end.art.focus_padding.y;
	const auto check_end_heading = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.end.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->presentation_.menu_.heading_font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, end.heading_y, size.x, size.y);
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

	return { options_art, display_art, setup_edge, setup_inside, confirmation_paper };
}

Presentation::Validation::ControlsGeometry Presentation::Validation::validate_controls_text(const SetupGeometry& setup)
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const MenuValueText& values = this->presentation_.menu_text_.values;
	const auto& options_art = setup.options_art;
	const Vector2F options_value_limit((1920.0f + main.options_box_size.x) / 2.0f -
		options_art.shadow_offset - options_art.focus_padding.x - main.options_value_x,
		main.options_spacing.y - options_art.focus_padding.y * 2.0f);
	check_value("on", values.on, this->presentation_.menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("off", values.off, this->presentation_.menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("random", values.random, this->presentation_.menu_.item_font, options_value_position(2), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	const Vector2F controls_value_limit(controls.controls_position.x - controls.art.focus_padding.x * 2.0f -
		controls.art.shadow_offset - controls.layout.position.x - controls.layout.spacing.x,
		controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
	// Rumble occupies Stick row 3; Controls toggles occupy rows 6 through 8.
	for (const int row : { 3, 6, 7, 8 })
	{
		const Vector2F position = controls.layout.position +
			Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
		check_value("on", values.on, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
		check_value("off", values.off, controls.item_font, position, controls_value_limit,
			controls.box_size, controls.art, controls.layout.spacing.y, true);
	}
	const auto centered_title = [&](const std::string& key, const std::wstring& text,
		const std::string& font, float y, const Vector2F& padding)
	{
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, y, size.x, size.y);
		check_fixed_paper(key, bounds, main.title_box_size, padding);
		return bounds;
	};
	const MenuTitleText& title = this->presentation_.menu_text_.title;
	const RectangleF wordmark = centered_title("menu_text.title.wordmark", title.wordmark,
		main.title_font, main.title_y, Vector2F::ZERO);
	const RectangleF subtitle = centered_title("menu_text.title.subtitle", title.subtitle,
		this->presentation_.menu_.detail_font, main.title_subtitle_y, Vector2F::ZERO);
	RectangleF start = centered_title("menu_text.title.start", title.start,
		this->presentation_.menu_.item_font, main.title_start_y, main.art.focus_padding);
	start.inflate(main.art.focus_padding);
	const RectangleF no_controller = centered_title("menu_text.title.no_controller", title.no_controller,
		this->presentation_.menu_.item_font, main.title_no_pad_y, Vector2F::ZERO);
	// Build identity remains generated, but it still shares the title's paper.
	const RectangleF build = centered_title("main_menu.build_id_y", build_info::label,
		this->presentation_.prompt_bar_.font, main.build_id_y, Vector2F::ZERO);
	const auto precedes = [&](const std::string& key, const RectangleF& before, const RectangleF& after)
	{
		if (before.bottom() > after.y)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must end before the next menu text or focus card.");
		}
	};
	precedes("menu_text.title.wordmark", wordmark, subtitle);
	precedes("menu_text.title.subtitle", subtitle, start);
	precedes("menu_text.title.start", start, no_controller);
	precedes("menu_text.title.no_controller", no_controller, build);
	const RectangleF home_subtitle(main.home_subtitle_position,
		fixed_menu_size("menu_text.home_subtitle", this->presentation_.menu_text_.home_subtitle, this->presentation_.menu_.detail_font));
	check_fixed_paper("menu_text.home_subtitle", home_subtitle, main.home_box_size, Vector2F::ZERO);
	const RectangleF home_heading(main.home_position,
		fixed_menu_size("menu_text.headings.home", this->presentation_.menu_text_.heading(MenuHeading::home), this->presentation_.menu_.heading_font));
	check_fixed_paper("menu_text.headings.home", home_heading, main.home_box_size, Vector2F::ZERO);
	precedes("menu_text.home_subtitle", home_heading, home_subtitle);
	RectangleF play(main.home_position + Vector2F(0.0f, main.home_spacing.y * 2.0f),
		fixed_menu_size("menu_text.items.play", this->presentation_.menu_text_.item(MenuItem::play), this->presentation_.menu_.item_font));
	play.inflate(main.art.focus_padding);
	precedes("menu_text.home_subtitle", home_subtitle, play);
	const RectangleF help_area(controls.controls_position, controls.controls_size);
	check_fixed_paper("menu_text.help.controls", help_area, controls.box_size, Vector2F::ZERO);
	const auto check_help_fit = [&](const std::string& key, const std::wstring& text,
		const Vector2F& position, const Vector2F& available)
	{
		const Vector2F raw = fixed_menu_size(key, text, controls.help_font);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			!std::isfinite(available.x) || !std::isfinite(available.y) || available.x <= 0.0f || available.y <= 0.0f ||
			!help_area.contains(RectangleF(position, available)))
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must have a finite positive fitting area inside the controls help region.");
		}
		// Preserve the two pages' existing uniform fit. The available region
		// bounds it by construction; supported long blocks need no length cap.
		const float scale = std::min({ 1.0f, available.x / raw.x, available.y / raw.y });
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must retain finite positive text bounds after fitting.");
		}
		return RectangleF(position, fitted);
	};
	RectangleF controls_help;
	bool first_help = true;
	const auto controls_variant = [&](const std::wstring& words)
	{
		const RectangleF fitted = check_help_fit("menu_text.help.controls", words,
			controls.controls_position, controls.controls_size);
		controls_help = first_help ? fitted : RectangleF::union_of(controls_help, fitted);
		first_help = false;
	};
	for (const Key key : controls_text::jump_keys())
	{
		controls_variant(controls_text::controls_help(this->presentation_.menu_text_.help.controls, key));
	}
	// Each keyboard action's every label, the others at their defaults where
	// valid, so every glyph the Controls help can show is measured.
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const KeyboardAction action = static_cast<KeyboardAction>(index);
		const std::array<MouseButton, 5> buttons = shoot_mouse_buttons();
		std::vector<DesktopControl> choices;
		if (action == KeyboardAction::pause) { choices.push_back({}); }
		for (const Key key : controls_text::jump_keys()) { choices.push_back({ key, MouseButton::none }); }
		if (action == KeyboardAction::shoot) { for (const MouseButton button : buttons) { choices.push_back({ Key::none, button }); } }
		for (const DesktopControl& choice : choices)
		{
			InputBindings bindings;
			set_keyboard_control(bindings, action, choice);
			try { bindings.validate_keyboard(); }
			catch (const std::invalid_argument&) { continue; }
			controls_variant(controls_text::controls_help(this->presentation_.menu_text_.help.controls, bindings));
		}
	}
	const KeyboardJumpMenuText& jump = this->presentation_.menu_text_.keyboard_jump;
	const auto jump_text = [&](const char* key, const std::wstring& words, const std::string& font, const Vector2F& position, bool row)
	{
		const std::string where = std::string("menu_text.keyboard_jump.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			// KeyboardJumpMenu fits its action labels uniformly in both axes.
			const Vector2F available(controls.controls_position.x - controls.art.focus_padding.x * 2.0f -
				controls.art.shadow_offset - position.x, controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted action bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF jump_heading = jump_text("heading", jump.heading, this->presentation_.menu_.heading_font, controls.heading_position, false);
	if (jump_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.keyboard_jump.heading must end before its first focus card."); }
	// Rows: six capture rows (an OptionRow label and its control value), then
	// Reset, Apply and Back.
	const char* keyboard_rows[]{ "move_left", "move_right", "jump", "drop", "shoot", "pause" };
	for (size_t index = 0; index < keyboard_action_count; ++index)
	{
		const Vector2F position = controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(index));
		Vector2F label = fixed_menu_size(std::string("menu_text.keyboard_jump.") + keyboard_rows[index], jump.row(index), controls.item_font);
		const Vector2F label_available(controls.layout.spacing.x - controls.art.focus_padding.x,
			controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
		label *= std::min({ 1.0f, label_available.x / label.x, label_available.y / label.y });
		check_fixed_paper(std::string("menu_text.keyboard_jump.") + keyboard_rows[index], RectangleF(position, label),
			controls.box_size, controls.art.focus_padding);
		for (const std::wstring& value : controls_text::keyboard_control_labels(static_cast<KeyboardAction>(index)))
		{
			jump_text(keyboard_rows[index], value, controls.item_font, position + Vector2F(controls.layout.spacing.x, 0.0f), true);
		}
	}
	jump_text("reset", jump.reset, controls.item_font,
		controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(keyboard_action_count)), true);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.help", jump.help, controls.help_font);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.debug_collision", jump.debug_collision, controls.help_font);
	for (const bool collision : { false, true })
	{
		std::ignore = check_help_fit("menu_text.keyboard_jump.help", collision ? jump.help + L'\n' + jump.debug_collision : jump.help,
			controls.controls_position, controls.controls_size);
	}
	const PadButtonsMenuText& pad_buttons = this->presentation_.menu_text_.pad_buttons;
	const auto pad_buttons_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, bool row, bool option_label = false)
	{
		const std::string where = std::string("menu_text.pad_buttons.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			const Vector2F available(option_label ? controls.layout.spacing.x - controls.art.focus_padding.x :
				controls.controls_position.x - controls.art.focus_padding.x * 2.0f - controls.art.shadow_offset - position.x,
				controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF pad_buttons_heading = pad_buttons_text("heading", pad_buttons.heading, this->presentation_.menu_.heading_font,
		controls.heading_position, false);
	if (pad_buttons_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.pad_buttons.heading must end before its first focus card."); }
	// Rows: Pad, six action entries, Reset, Apply, Back.
	const char* pad_button_rows[]{ "jump", "jump_alternate", "shoot", "shoot_alternate", "pause", "pause_alternate" };
	for (size_t index = 0; index < std::size(pad_button_rows); ++index)
	{
		const Vector2F position = controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(index + 1));
		pad_buttons_text(pad_button_rows[index], pad_buttons.row(index), controls.item_font, position, true, true);
		for (const PadControl control : pad_controls())
		{
			pad_buttons_text(pad_button_rows[index], std::wstring(pad_control_label(control)), controls.item_font,
				position + Vector2F(controls.layout.spacing.x, 0.0f), true);
		}
		pad_buttons_text(pad_button_rows[index], std::wstring(pad_control_label(PadControl::none)), controls.item_font,
			position + Vector2F(controls.layout.spacing.x, 0.0f), true);
	}
	pad_buttons_text("reset", pad_buttons.reset, controls.item_font,
		controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 7.0f), true);
	const RectangleF pad_buttons_help = check_help_fit("menu_text.pad_buttons.help", pad_buttons.help,
		controls.controls_position, controls.controls_size);
	// Profiles (ROAD#21): four option rows, then New, Delete and Back.
	const ProfilesMenuText& profiles = this->presentation_.menu_text_.profiles;
	const auto profiles_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, bool row, bool option_label = false)
	{
		const std::string where = std::string("menu_text.profiles.") + key;
		Vector2F size = fixed_menu_size(where, words, font);
		if (row)
		{
			const Vector2F available(option_label ? controls.layout.spacing.x - controls.art.focus_padding.x :
				controls.controls_position.x - controls.art.focus_padding.x * 2.0f - controls.art.shadow_offset - position.x,
				controls.layout.spacing.y - controls.art.focus_padding.y * 2.0f);
			const float scale = std::min({ 1.0f, available.x / size.x, available.y / size.y });
			if (!std::isfinite(scale) || scale <= 0.0f)
			{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->presentation_.source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF profiles_heading = profiles_text("heading", profiles.heading, this->presentation_.menu_.heading_font,
		controls.heading_position, false);
	if (profiles_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->presentation_.source_path_ + ": menu_text.profiles.heading must end before its first focus card."); }
	const auto profile_row = [&](int row) { return controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * static_cast<float>(row)); };
	const auto profile_value = [&](const char* key, const std::wstring& words, int row)
	{ std::ignore = profiles_text(key, words, controls.item_font, profile_row(row) + Vector2F(controls.layout.spacing.x, 0.0f), true); };
	profiles_text("profile", profiles.profile, controls.item_font, profile_row(0), true, true);
	profiles_text("name", profiles.name, controls.item_font, profile_row(1), true, true);
	profiles_text("boost", profiles.boost, controls.item_font, profile_row(2), true, true);
	profiles_text("worn_by", profiles.worn_by, controls.item_font, profile_row(3), true, true);
	// The widest name, as shown and while its first letter is being edited.
	const std::wstring widest_name(player_profiles::max_name_length, L'W');
	profile_value("none", profiles.none, 0);
	profile_value("none", widest_name, 0);
	profile_value("none", L"[W]" + widest_name.substr(1), 1);
	profile_value("none", L"[_]", 1);
	// Boost and pad labels are checked where they first appear; OptionRow
	// fits every value to its column, so here only this page's own words.
	profile_value("nobody", profiles.nobody, 3);
	profiles_text("new_profile", profiles.new_profile, controls.item_font, profile_row(4), true);
	profiles_text("delete_profile", profiles.delete_profile, controls.item_font, profile_row(5), true);
	const RectangleF profiles_help = check_help_fit("menu_text.profiles.help", profiles.help,
		controls.controls_position, controls.controls_size);
	const StickPreviewPresentation& preview = controls.stick_preview;
	const RectangleF pad_association = check_help_fit("menu_text.help.pad_association", this->presentation_.menu_text_.help.pad_association,
		preview.association_position, help_area.bottom_right() - preview.association_position);
	if (preview.association_position.y < preview.aim.bottom())
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.help.pad_association must begin below the aim preview.");
	}
	const Vector2F caption_size = fixed_menu_size("menu_text.help.aim_cutoff",
		this->presentation_.menu_text_.help.aim_cutoff, controls.help_font);
	const float probe_height = render.measure_text(render.resolve_sprite_font(controls.help_font), L"Move").y;
	const RectangleF caption(preview.aim.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y), caption_size);
	if (!std::isfinite(probe_height) || probe_height <= 0.0f || !help_area.contains(caption) ||
		caption.y < preview.movement_response.bottom() || caption.bottom() > preview.aim.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": menu_text.help.aim_cutoff must fit unscaled inside the controls help region between the movement and aim previews.");
	}
	const auto padded_back = [&](const Vector2F& position, const std::string& font,
		const MenuPaperArtPresentation& art)
	{
		RectangleF bounds(position, fixed_menu_size("menu_text.items.back", this->presentation_.menu_text_.item(MenuItem::back), font));
		bounds.inflate(art.focus_padding);
		return bounds;
	};
	const RectangleF options_back = padded_back(main.options_position + Vector2F(0.0f, main.options_spacing.y * 6.0f),
		this->presentation_.menu_.item_font, options_art);
	const RectangleF controls_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 10.0f),
		controls.item_font, controls.art);
	const RectangleF pad_buttons_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 9.0f),
		controls.item_font, controls.art);
	const RectangleF sticks_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 5.0f),
		controls.item_font, controls.art);
	struct StatusMessage { const char* key; const std::wstring& text; };
	const auto check_status = [&](const StatusMessage& message, const std::string& font,
		const Vector2F& position, const Vector2F& paper_size, const MenuPaperArtPresentation& art,
		std::initializer_list<RectangleF> obstacles)
	{
		const std::string_view message_key(message.key);
		const std::string key = std::string(message_key.starts_with("keyboard_jump.") || message_key.starts_with("pad_buttons.") ||
			message_key.starts_with("profiles.") ?
			"menu_text." : "menu_text.status.") + message.key;
		const Vector2F raw = fixed_menu_size(key, message.text, font);
		const float available = (1920.0f + paper_size.x) / 2.0f - art.shadow_offset -
			art.focus_padding.x - position.x;
		if (!std::isfinite(available) || available <= 0.0f)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key + " must leave finite positive status width.");
		}
		// All three pages reset to the authored font size, then fit width only.
		const float scale = std::min(1.0f, available / raw.x);
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key + " must retain finite positive fitted status bounds.");
		}
		const RectangleF bounds(position, fitted);
		check_fixed_paper(key, bounds, paper_size, Vector2F::ZERO);
		for (const RectangleF& obstacle : obstacles)
		{
			// Touching edges are allowed. Options' status and Back deliberately
			// share vertical space while remaining horizontally separate.
			if (bounds.x < obstacle.right() && bounds.right() > obstacle.x &&
				bounds.y < obstacle.bottom() && bounds.bottom() > obstacle.y)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
					" must not overlap the Back card, help text or stick previews.");
			}
		}
	};
	const MenuStatusText& status_text = this->presentation_.menu_text_.status;
	const StatusMessage options_statuses[]{ { "options_pending", status_text.options_pending },
		{ "options_saved", status_text.options_saved }, { "options_save_failed", status_text.options_save_failed },
		{ "options_reverted", status_text.options_reverted }, { "options_display_failed", status_text.options_display_failed },
		{ "options_rollback_failed", status_text.options_rollback_failed } };
	for (const StatusMessage& message : options_statuses)
	{
		check_status(message, this->presentation_.prompt_bar_.font, main.options_status_position, main.options_box_size,
			options_art, { options_back });
	}
	const StatusMessage controls_statuses[]{ { "controls_pending", status_text.controls_pending },
		{ "controls_saved", status_text.controls_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : controls_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { controls_back, controls_help });
	}
	const RectangleF jump_back = padded_back(controls.layout.position + Vector2F(0.0f,
		controls.layout.spacing.y * static_cast<float>(keyboard_action_count + 2)), controls.item_font, controls.art);
	const StatusMessage jump_statuses[]{ { "keyboard_jump.pending_status", jump.pending_status }, { "keyboard_jump.saved_status", jump.saved_status },
		{ "keyboard_jump.capture_wait_neutral", jump.capture_wait_neutral }, { "keyboard_jump.capture_ready", jump.capture_ready },
		{ "keyboard_jump.capture_ready_mouse", jump.capture_ready_mouse },
		{ "keyboard_jump.capture_rejected", jump.capture_rejected }, { "keyboard_jump.capture_conflict", jump.capture_conflict },
		{ "keyboard_jump.capture_cancelled", jump.capture_cancelled }, { "keyboard_jump.debug_collision", jump.debug_collision } };
	for (const StatusMessage& message : jump_statuses)
	{ check_status(message, controls.help_font, controls.status_position, controls.box_size, controls.art, { jump_back, help_area }); }
	for (const std::wstring& label : controls_text::keyboard_control_labels(KeyboardAction::shoot))
	{
		const std::wstring words = jump.release_key(label);
		check_status({ "keyboard_jump.capture_release", words }, controls.help_font, controls.status_position,
			controls.box_size, controls.art, { jump_back, help_area });
	}
	const StatusMessage pad_buttons_statuses[]{ { "pad_buttons.pending_status", pad_buttons.pending_status },
		{ "pad_buttons.saved_status", pad_buttons.saved_status }, { "pad_buttons.conflict_status", pad_buttons.conflict_status },
		{ "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : pad_buttons_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { pad_buttons_back, pad_buttons_help });
	}
	const RectangleF profiles_back = padded_back(profile_row(6), controls.item_font, controls.art);
	const StatusMessage profiles_statuses[]{ { "profiles.idle_status", profiles.idle_status },
		{ "profiles.saved_status", profiles.saved_status }, { "profiles.editing_status", profiles.editing_status },
		{ "profiles.name_taken_status", profiles.name_taken_status }, { "profiles.full_status", profiles.full_status },
		{ "profiles.delete_confirm_status", profiles.delete_confirm_status },
		{ "profiles.deleted_status", profiles.deleted_status }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : profiles_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { profiles_back, profiles_help });
	}
	const StatusMessage sticks_statuses[]{ { "sticks_pending", status_text.sticks_pending },
		{ "sticks_saved", status_text.sticks_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : sticks_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { sticks_back, pad_association, preview.raw_movement, preview.movement_response, preview.aim, caption });
	}

	return { options_value_limit, controls_value_limit, help_area, probe_height };
}
