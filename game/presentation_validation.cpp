#include "game/pch.h"
#include "game/presentation_read.h"
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

// Presentation::validate_resources and validate_stage_counts, with the
// label measuring helpers only they use.

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;

namespace
{
	struct LabelPen
	{
		float x = 0.0f;
		float y = 0.0f;
		Vector2F extent = Vector2F::ZERO;
	};

	void append_label_glyph(LabelPen& pen, const Glyph& glyph, wchar_t character,
		float line_spacing, const std::string& where)
	{
		if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
			!std::isfinite(glyph.x_advance))
		{
			throw std::runtime_error(where + " requires finite font glyph metrics.");
		}
		// Keep Font::for_each_glyph/measure's FLOAT operation order, including
		// carried bearings, the zero clamp, whitespace suppression and overhang.
		pen.x += glyph.x_offset;
		if (pen.x < 0.0f) { pen.x = 0.0f; }
		const float width = static_cast<float>(glyph.subrect.width);
		if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
		{
			const float height = character == L' ' ? line_spacing :
				std::max(line_spacing, static_cast<float>(glyph.subrect.height) + glyph.y_offset);
			pen.extent.x = std::max(pen.extent.x, pen.x + width);
			pen.extent.y = std::max(pen.extent.y, pen.y + height);
		}
		pen.x += width + glyph.x_advance;
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y))
		{
			throw std::runtime_error(where + " requires finite composed text metrics.");
		}
	}

	void append_label_text(LabelPen& pen, const Font& font, std::wstring_view text,
		const std::string& where)
	{
		if (!std::isfinite(font.line_spacing()) || font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		for (const wchar_t character : text)
		{
			if (character == L'\r') { continue; }
			if (character == L'\n')
			{
				pen.x = 0.0f;
				pen.y += font.line_spacing();
				continue;
			}
			append_label_glyph(pen, font.drawn(static_cast<char32_t>(character)),
				character, font.line_spacing(), where);
		}
	}

	Vector2F round_heading_size(const Font& font, std::wstring_view text, const std::string& where)
	{
		if (!std::isfinite(font.line_spacing()))
		{
			throw std::runtime_error(where + " requires finite font line spacing.");
		}
		// The caller has checked the original, single-line template and every
		// glyph. Reuse the existing finite-metric glyph walk, but do not impose
		// append_label_text's positive line-spacing rule on this heading role.
		struct State
		{
			bool present = false;
			LabelPen maximum;
			float minimum_height = 0.0f;
			bool zero_width = false;
			float zero_width_pen = 0.0f;
		};
		const auto append = [&](State& state, wchar_t character)
		{
			const Glyph& glyph = font.drawn(static_cast<char32_t>(character));
			LabelPen maximum = state.maximum;
			LabelPen minimum_height = state.maximum;
			minimum_height.extent.y = state.minimum_height;
			append_label_glyph(maximum, glyph, character, font.line_spacing(), where);
			append_label_glyph(minimum_height, glyph, character, font.line_spacing(), where);
			if (state.zero_width)
			{
				LabelPen zero_width = state.maximum;
				zero_width.x = state.zero_width_pen;
				zero_width.extent.x = 0.0f;
				append_label_glyph(zero_width, glyph, character, font.line_spacing(), where);
				state.zero_width = zero_width.extent.x == 0.0f;
				state.zero_width_pen = zero_width.x;
			}
			state.maximum = maximum;
			state.minimum_height = minimum_height.extent.y;
		};
		const auto walk = [&](State& state, std::wstring_view words)
		{
			for (const wchar_t character : words) { append(state, character); }
		};
		const auto merge = [](State& destination, const State& candidate)
		{
			if (!candidate.present) { return; }
			if (!destination.present)
			{
				destination = candidate;
				return;
			}
			// The pen transform is monotone even with negative bearings or
			// advances. Greatest pen and ink need not come from the same value.
			destination.maximum.x = std::max(destination.maximum.x, candidate.maximum.x);
			destination.maximum.extent.x = std::max(destination.maximum.extent.x, candidate.maximum.extent.x);
			destination.maximum.extent.y = std::max(destination.maximum.extent.y, candidate.maximum.extent.y);
			destination.minimum_height = std::min(destination.minimum_height, candidate.minimum_height);
			// Among still-zero-width paths, the least pen suffices to decide
			// whether later digits or a visible suffix can keep or rescue one.
			if (candidate.zero_width &&
				(!destination.zero_width || candidate.zero_width_pen < destination.zero_width_pen))
			{
				destination.zero_width = true;
				destination.zero_width_pen = candidate.zero_width_pen;
			}
		};
		constexpr std::wstring_view token = L"{round}";
		const size_t at = text.find(token);
		State prefix;
		prefix.present = true;
		prefix.zero_width = true;
		walk(prefix, text.substr(0, at));
		const std::wstring_view suffix = text.substr(at + token.size());
		const std::wstring largest = std::to_wstring(std::numeric_limits<int>::max());
		State complete;
		for (size_t length = 1; length <= largest.size(); ++length)
		{
			const std::wstring limit = length == largest.size() ? largest : std::wstring(length, L'9');
			std::array<State, 2> states{};
			states[1] = prefix;
			for (size_t position = 0; position < length; ++position)
			{
				std::array<State, 2> next{};
				for (size_t tight = 0; tight < states.size(); ++tight)
				{
					if (!states[tight].present) { continue; }
					const wchar_t last = tight != 0 ? limit[position] : L'9';
					for (wchar_t digit = position == 0 ? L'1' : L'0'; digit <= last; ++digit)
					{
						State candidate = states[tight];
						append(candidate, digit);
						const size_t next_tight = tight != 0 && digit == last ? 1 : 0;
						merge(next[next_tight], candidate);
					}
				}
				states = next;
			}
			for (State candidate : states)
			{
				if (!candidate.present) { continue; }
				walk(candidate, suffix);
				merge(complete, candidate);
			}
		}
		if (!complete.present || complete.zero_width || complete.minimum_height <= 0.0f ||
			complete.maximum.extent.x <= 0.0f || complete.maximum.extent.y <= 0.0f)
		{
			throw std::runtime_error(where +
				" must retain finite positive measured heading bounds for every positive int round.");
		}
		return complete.maximum.extent;
	}
}

void Presentation::validate_resources(const RenderResources& render,
	const AudioResources& audio) const
{
	const ResourceValidation check(this->source_path_, render, audio);
	check.frame("menu_style.fill_frame", this->menu_.sprite_sheet, this->menu_.fill_frame);
	const char* roles[] = { "a", "center", "b" };
	for (size_t index = 0; index < std::size(roles); ++index)
	{
		check.frame(std::string("main_menu.team_select.frames.") + roles[index],
			this->main_menu_.team_select_sheet, this->main_menu_.team_select_frames[index]);
		check.frame(std::string("main_menu.team_select.selected_frames.") + roles[index],
			this->main_menu_.team_select_sheet, this->main_menu_.team_select_selected_frames[index]);
	}
	check.font("menu_style.fonts.heading", this->menu_.heading_font);
	check.font("menu_style.fonts.item", this->menu_.item_font);
	check.font("menu_style.fonts.detail", this->menu_.detail_font);
	check.font("menu_style.chrome.brand_font", this->menu_.brand_font);
	check.frame("main_menu.backdrop.frame", this->main_menu_.backdrop_sheet,
		this->main_menu_.backdrop_frame);
	check.font("main_menu.fonts.title", this->main_menu_.title_font);
	check.font("main_menu.fonts.announcement", this->main_menu_.announcement_font);
	check.font("main_menu.fonts.weapon_description", this->main_menu_.weapon_description_font);
	check.font("main_menu.credits.font", this->main_menu_.credits_font);
	check.font("controls_settings.help_font", this->controls_settings_.help_font);
	check.font("controls_settings.item_font", this->controls_settings_.item_font);
	check.font("results_menu.contribution_font", this->results_menu_.contribution_font);
	check.font("main_menu.stage_select.record_font", this->main_menu_.stage_select.record_font);
	check.font("prompt_bar.font", this->prompt_bar_.font);
	check.font("debug_overlay.font", this->debug_overlay_.font);
	check.font("debug_overlay.station_label.font", this->debug_overlay_.station_label.font);
	check.font("hud.timer.font", this->hud_.timer_font);
	const auto check_menu_line = [&](const std::string& key, const std::wstring& text,
		const std::string& font, const Vector2F& position, float bottom, const Vector2F& shadow)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font, text, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		const float width = 1920.0f - position.x - this->menu_.brand_position.x;
		const float height = bottom - position.y;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x < 0.0f || position.y < 0.0f || !std::isfinite(width) || !std::isfinite(height) ||
			!std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			position.x + std::min(0.0f, shadow.x) < 0.0f ||
			position.y + std::min(0.0f, shadow.y) < 0.0f ||
			!std::isfinite(size.x) || !std::isfinite(size.y) ||
			size.x + std::abs(shadow.x) > width || size.y + std::abs(shadow.y) > height)
		{
			throw std::runtime_error(where + " must fit its menu design area including its shadow and right margin.");
		}
	};
	check_menu_line("menu_text.brand", this->menu_text_.brand, this->menu_.brand_font,
		this->menu_.brand_position, this->menu_.footer_y, Vector2F::ZERO);
	for (size_t index = 0; index < this->menu_text_.headings.size(); ++index)
	{
		Vector2F position = this->menu_.layout.position;
		switch (static_cast<MenuHeading>(index))
		{
		case MenuHeading::home: position = this->main_menu_.home_position; break;
		case MenuHeading::options: position = this->main_menu_.options_heading_position; break;
		case MenuHeading::credits: position = this->main_menu_.credits_heading_position; break;
		case MenuHeading::controls_audio:
		case MenuHeading::pad_sticks: position = this->controls_settings_.heading_position; break;
		default: break;
		}
		check_menu_line(std::string("menu_text.headings.") + menu_heading_keys[index],
			this->menu_text_.headings[index], this->menu_.heading_font, position,
			this->menu_.footer_y, this->menu_.heading_shadow_offset);
	}
	for (size_t index = 0; index < this->menu_text_.prompts.size(); ++index)
	{
		check_menu_line(std::string("menu_text.prompts.") + menu_prompt_keys[index],
			this->menu_text_.prompts[index], this->prompt_bar_.font, this->prompt_bar_.position,
			1080.0f, this->prompt_bar_.shadow_offset);
	}
	const auto check_menu_item = [&](MenuItem item, const std::string& font, const Vector2F& position,
		const Vector2F& paper_size, float row_spacing, const std::optional<float>& label_width,
		const MenuPaperArtPresentation& art, float content_height, bool uniform_fit = false)
	{
		const std::string where = this->source_path_ + ": menu_text.items." +
			menu_item_keys[static_cast<size_t>(item)];
		const std::wstring& text = this->menu_text_.item(item);
		drawable_text::check_font_can_draw(&render, font, text, where);
		Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{
			throw std::runtime_error(where + " must have finite positive measured text bounds.");
		}
		const Vector2F padding = art.focus_padding;
		if (label_width.has_value())
		{
			// Match the action's uniform fit or OptionRow's width-then-height
			// fit; both retain all text within the available column and row.
			const float width = *label_width;
			const float height = row_spacing - padding.y * 2.0f;
			if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
			{
				throw std::runtime_error(where + " must leave a positive finite fitted label column and row height.");
			}
			float scale = uniform_fit ? std::min({ 1.0f, width / size.x, height / size.y }) : 1.0f;
			if (!uniform_fit)
			{
				if (size.x > width) { scale = width / size.x; }
				if (size.y * scale > height) { scale = height / size.y; }
			}
			if (!std::isfinite(scale) || scale <= 0.0f)
			{
				throw std::runtime_error(where + " must leave a positive fitted label column and row height.");
			}
			size *= scale;
		}
		const Vector2F top = (Vector2F(1920.0f, content_height) - paper_size) / 2.0f;
		const Vector2F bottom = top + paper_size;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < top.x || position.y - padding.y < top.y ||
			position.x + size.x + padding.x > bottom.x ||
			position.y + size.y + padding.y > bottom.y ||
			(row_spacing > 0.0f && size.y + padding.y * 2.0f > row_spacing))
		{
			throw std::runtime_error(where + " must fit its paper and row including focus padding.");
		}
		return size;
	};
	const MainMenuPresentation& main = this->main_menu_;
	MenuPaperArtPresentation options_art = main.art;
	options_art.focus_padding = main.setup_focus_padding;
	options_art.shadow_offset = main.setup_shadow_offset;
	const MenuItem home_items[]{ MenuItem::play, MenuItem::options, MenuItem::credits, MenuItem::exit };
	for (size_t index = 0; index < std::size(home_items); ++index)
	{
		check_menu_item(home_items[index], this->menu_.item_font, main.home_position +
			Vector2F(0.0f, main.home_spacing.y * static_cast<float>(index + 2)),
			main.home_box_size, main.home_spacing.y, std::nullopt, main.art, this->menu_.footer_y);
	}
	const MenuItem options_items[]{ MenuItem::resolution, MenuItem::fullscreen, MenuItem::team_palette,
		MenuItem::controls_audio, MenuItem::restore_defaults, MenuItem::apply, MenuItem::back };
	for (size_t index = 0; index < std::size(options_items); ++index)
	{
		const Vector2F position = main.options_position +
			Vector2F(0.0f, main.options_spacing.y * static_cast<float>(index));
		const std::optional<float> label_width = index < 3 ?
			std::optional<float>(main.options_value_x - position.x - options_art.focus_padding.x) : std::nullopt;
		check_menu_item(options_items[index], this->menu_.item_font, position,
			main.options_box_size, main.options_spacing.y, label_width, options_art, this->menu_.footer_y);
	}
	check_menu_item(MenuItem::back, this->menu_.item_font, main.credits_back_position, main.credits_box_size, 0.0f, std::nullopt,
		main.art, this->menu_.footer_y);
	const PauseMenuPresentation& pause = this->pause_menu_;
	MenuPaperArtPresentation display_art = pause.art;
	display_art.focus_padding = main.setup_focus_padding;
	display_art.shadow_offset = main.setup_shadow_offset;
	if (!std::isfinite(pause.layout.spacing.y) || pause.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.resume requires positive finite pause_menu.layout.spacing.y.");
	}
	const MenuItem pause_items[]{ MenuItem::resume, MenuItem::restart, MenuItem::options, MenuItem::quit };
	for (size_t index = 0; index < std::size(pause_items); ++index)
	{
		check_menu_item(pause_items[index], this->menu_.item_font, pause.layout.position +
			Vector2F(0.0f, pause.layout.spacing.y * static_cast<float>(index)),
			pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	}
	check_menu_item(MenuItem::yes, this->menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, pause.art, 1080.0f);
	check_menu_item(MenuItem::no, this->menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, pause.art, 1080.0f);
	check_menu_item(MenuItem::keep, this->menu_.item_font, pause.layout.position, pause.box_size, pause.layout.spacing.y,
		std::nullopt, display_art, 1080.0f);
	check_menu_item(MenuItem::revert, this->menu_.item_font, pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		pause.box_size, pause.layout.spacing.y, std::nullopt, display_art, 1080.0f);
	const EndMenuPresentation& end = this->end_menu_;
	if (!std::isfinite(end.layout.spacing.y) || end.layout.spacing.y <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.change_teams requires positive finite end_menu.layout.spacing.y.");
	}
	const MenuItem end_items[]{ MenuItem::change_teams, MenuItem::change_weapons, MenuItem::change_level };
	for (size_t index = 0; index < std::size(end_items); ++index)
	{
		check_menu_item(end_items[index], this->menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(index + 1)),
			end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
	}
	// Exit is row 3 between a series' rounds and row 4 otherwise. Rows 1 and
	// 4 bound every row between them, which also covers the setup actions
	// moving up a row between rounds.
	for (const int row : { 1, 4 })
	{
		check_menu_item(MenuItem::exit_to_main_menu, this->menu_.item_font, end.layout.position +
			Vector2F(0.0f, end.layout.spacing.y * static_cast<float>(row)),
			end.box_size, end.layout.spacing.y, std::nullopt, end.art, 1080.0f);
	}
	const ControlsSettingsPresentation& controls = this->controls_settings_;
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
			controls.box_size, controls.layout.spacing.y, label_width, controls.art, this->menu_.footer_y, fitted_action);
		if (position.x + size.x + controls.art.focus_padding.x > controls.controls_position.x)
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.items." +
				menu_item_keys[static_cast<size_t>(item)] + " must fit before the controls help column including focus padding.");
		}
		if (position.y + size.y + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.items." +
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
	const RectangleF setup_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.setup_box_size) / 2.0f,
		main.setup_box_size);
	const float setup_edge = main.art.keyline_width * 3.0f;
	const RectangleF setup_inside(setup_paper.position() + Vector2F(setup_edge),
		setup_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const auto check_setup_paper = [&](const std::string& key, const std::wstring& words,
		const std::string& font, const Vector2F& position, const Vector2F& shadow,
		bool action, float row_spacing)
	{
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (action)
		{
			// Transparent text shadows retain their original navigation extents.
			// Include the entire paper card, its shade and the detached arrow.
			bounds.inflate(main.setup_focus_padding);
			if (std::max(bounds.height + main.setup_shadow_offset,
				main.art.keyline_width * 6.0f) > row_spacing)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" must fit its setup row including paper padding and shade.");
			}
			const RectangleF arrow(bounds.x - main.art.keyline_width * 7.0f,
				bounds.center().y - main.art.keyline_width * 3.0f,
				main.art.keyline_width * 5.0f, main.art.keyline_width * 6.0f);
			bounds = RectangleF::union_of(arrow, RectangleF::union_of(bounds,
				RectangleF(bounds.position() + Vector2F(main.setup_shadow_offset), bounds.size())));
		}
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !setup_inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit main_menu.setup_box_size including its retained shadow bounds and paper ornaments.");
		}
		return bounds;
	};
	float setup_heading_bottom = 0.0f;
	for (const MenuHeading role : { MenuHeading::mode, MenuHeading::players })
	{
		const RectangleF heading = check_setup_paper(
			std::string("menu_text.headings.") + menu_heading_keys[static_cast<size_t>(role)],
			this->menu_text_.heading(role), this->menu_.heading_font, this->menu_.layout.position,
			this->menu_.heading_shadow_offset, false, 0.0f);
		setup_heading_bottom = std::max(setup_heading_bottom, heading.bottom());
	}
	check_setup_paper("menu_text.headings.join_players", this->menu_text_.heading(MenuHeading::join_players),
		this->menu_.heading_font, this->menu_.layout.position, this->menu_.heading_shadow_offset, false, 0.0f);
	const auto check_setup_item = [&](MenuItem item, const Vector2F& spacing, int row, bool duration_label)
	{
		const std::string key = std::string("menu_text.items.") + menu_item_keys[static_cast<size_t>(item)];
		const std::string where = this->source_path_ + ": " + key;
		const Vector2F shadow = this->menu_.item_shadow_offset;
		if (!std::isfinite(spacing.y) || spacing.y <= 0.0f ||
			(duration_label && (!std::isfinite(spacing.x) || spacing.x <= 0.0f)))
		{
			throw std::runtime_error(where + " requires positive finite setup row/column spacing.");
		}
		const Vector2F position = this->menu_.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
		const std::wstring& text = this->menu_text_.item(item);
		check_menu_line(key, text, this->menu_.item_font, position, this->menu_.footer_y, shadow);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(this->menu_.item_font), text);
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
		const RectangleF card = check_setup_paper(key, text, this->menu_.item_font, position, shadow, true, spacing.y);
		if (row == 2 && card.y < setup_heading_bottom)
		{
			throw std::runtime_error(where + " must leave its setup heading clear of the first focus card.");
		}
		return card;
	};
	const MenuItem mode_items[]{ MenuItem::single_match, MenuItem::best_of_three, MenuItem::best_of_five,
		MenuItem::match_length, MenuItem::back };
	float mode_back_bottom = 0.0f;
	for (size_t index = 0; index < std::size(mode_items); ++index)
	{
		const RectangleF card = check_setup_item(mode_items[index], main.mode_select_spacing, static_cast<int>(index) + 2,
			mode_items[index] == MenuItem::match_length);
		if (mode_items[index] == MenuItem::back) { mode_back_bottom = card.bottom(); }
	}
	const MenuItem player_items[]{ MenuItem::one_player_practice, MenuItem::two_players,
		MenuItem::three_players, MenuItem::four_players, MenuItem::back };
	for (size_t index = 0; index < std::size(player_items); ++index)
	{
		check_setup_item(player_items[index], main.player_count_spacing, static_cast<int>(index) + 2, false);
	}
	// The nonfocusable hint retains the detail font and the item-shadow offset
	// that its existing widget uses; it has no action-row fit or focus padding.
	check_menu_line("menu_text.solo_practice_hint", this->menu_text_.solo_practice_hint, this->menu_.detail_font,
		this->menu_.layout.position + Vector2F(0.0f, main.mode_select_spacing.y * 7.0f),
		this->menu_.footer_y, this->menu_.item_shadow_offset);
	const RectangleF practice_hint = check_setup_paper(
		"menu_text.solo_practice_hint", this->menu_text_.solo_practice_hint, this->menu_.detail_font,
		this->menu_.layout.position + Vector2F(0.0f, main.mode_select_spacing.y * 7.0f),
		this->menu_.item_shadow_offset, false, 0.0f);
	if (practice_hint.y < mode_back_bottom)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.solo_practice_hint must follow the complete Back focus card and shade.");
	}
	const auto fixed_menu_size = [&](const std::string& key, const std::wstring& text, const std::string& font)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font, text, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{
			throw std::runtime_error(where + " must have finite positive measured text bounds.");
		}
		return size;
	};
	const RectangleF confirmation_paper((Vector2F(1920.0f, 1080.0f) - pause.box_size) / 2.0f, pause.box_size);
	RectangleF first_confirmation_action(pause.layout.position,
		fixed_menu_size("menu_text.items.yes", this->menu_text_.item(MenuItem::yes), this->menu_.item_font));
	first_confirmation_action.inflate(pause.art.focus_padding);
	const auto check_confirmation = [&](const char* role, const std::wstring& text, float action_top)
	{
		const std::string key = std::string("menu_text.confirmations.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.detail_font);
		// Confirmation questions remain plain, unscaled and centered on the
		// full-design Pause paper rather than the footer-centered menu paper.
		const RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
			bounds.bottom() > action_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper before the first action's focus padding.");
		}
	};
	check_confirmation("restart", this->menu_text_.confirmations.restart, first_confirmation_action.y);
	check_confirmation("quit", this->menu_text_.confirmations.quit, first_confirmation_action.y);
	check_confirmation("exit", this->menu_text_.confirmations.exit, first_confirmation_action.y);
	check_confirmation("display", this->menu_text_.confirmations.display, pause.layout.position.y - display_art.focus_padding.y);
	const RectangleF end_paper((Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f, end.box_size);
	// The first continuation row starts here regardless of its Session label.
	const float first_end_action_top = end.layout.position.y - end.art.focus_padding.y;
	const auto check_end_heading = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.end.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.heading_font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, end.heading_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(first_end_action_top) ||
			!end_paper.contains(bounds) || bounds.bottom() > first_end_action_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the End paper before the first action's focus padding.");
		}
	};
	check_end_heading("match_complete", this->menu_text_.end.match_complete);
	check_end_heading("round_complete", this->menu_text_.end.round_complete);
	check_end_heading("series_complete", this->menu_text_.end.series_complete);
	const auto check_fixed_paper = [&](const std::string& key, RectangleF bounds,
		const Vector2F& paper_size, const Vector2F& padding)
	{
		bounds.inflate(padding);
		const RectangleF paper((Vector2F(1920.0f, this->menu_.footer_y) - paper_size) / 2.0f, paper_size);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !paper.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its menu paper including any focus padding.");
		}
	};
	const auto check_value = [&](const char* role, const std::wstring& text, const std::string& font,
		const Vector2F& position, const Vector2F& limit, const Vector2F& paper_size,
		const MenuPaperArtPresentation& art, float row_spacing, bool controls_value)
	{
		const std::string key = std::string("menu_text.values.") + role;
		const Vector2F raw = fixed_menu_size(key, text, font);
		if (!std::isfinite(limit.x) || !std::isfinite(limit.y) || limit.x <= 0.0f || limit.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must leave finite positive value width and row height.");
		}
		// Match OptionRow::set_value: reset to the authored font size, fit
		// width first, then height. Supported long values remain valid.
		float scale = 1.0f;
		if (raw.x > limit.x) { scale = limit.x / raw.x; }
		if (raw.y * scale > limit.y) { scale = limit.y / raw.y; }
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must retain finite positive fitted value bounds.");
		}
		const RectangleF bounds(position, fitted);
		check_fixed_paper(key, bounds, paper_size, art.focus_padding);
		if (fitted.y + art.focus_padding.y * 2.0f > row_spacing)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its row including focus padding.");
		}
		if (controls_value && (bounds.right() + art.focus_padding.x > controls.controls_position.x ||
			bounds.bottom() + art.focus_padding.y > controls.status_position.y))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit before the controls help and status including focus padding.");
		}
	};
	const MenuValueText& values = this->menu_text_.values;
	const std::string random_name_key = "menu_text.values.random";
	const Vector2F random_name_size = fixed_menu_size(random_name_key, values.random, this->menu_.item_font);
	const StageSelectPresentation& stage = main.stage_select;
	const Vector2F name_shadow = this->menu_.item_shadow_offset;
	const float name_bottom = stage.preview_position.y - this->menu_.rule_height;
	if (!std::isfinite(name_shadow.x) || !std::isfinite(name_shadow.y) ||
		!std::isfinite(stage.preview_size.x) || stage.preview_size.x <= 0.0f ||
		!std::isfinite(name_bottom) || name_bottom <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must have a finite shadow, positive preview width and positive name area.");
	}
	const RectangleF random_name_union = RectangleF::union_of(RectangleF(Vector2F::ZERO, random_name_size),
		RectangleF(name_shadow, random_name_size));
	if (!std::isfinite(random_name_union.x) || !std::isfinite(random_name_union.y) ||
		!std::isfinite(random_name_union.width) || !std::isfinite(random_name_union.height) ||
		random_name_union.width <= 0.0f || random_name_union.height <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must have finite positive stage-name bounds including its shadow.");
	}
	// Match the stage page's width-only fit, including signed shadow offsets.
	const float random_name_scale = std::min(1.0f, stage.preview_size.x / random_name_union.width);
	const Vector2F fitted_name_size = random_name_size * random_name_scale;
	const RectangleF fitted_random_name = RectangleF::union_of(RectangleF(stage.name_position, fitted_name_size),
		RectangleF(stage.name_position + name_shadow * random_name_scale, fitted_name_size));
	if (!std::isfinite(random_name_scale) || random_name_scale <= 0.0f ||
		!std::isfinite(fitted_random_name.x) || !std::isfinite(fitted_random_name.y) ||
		!std::isfinite(fitted_random_name.width) || !std::isfinite(fitted_random_name.height) ||
		fitted_random_name.width <= 0.0f || fitted_random_name.height <= 0.0f ||
		!RectangleF(0.0f, 0.0f, 1920.0f, name_bottom).contains(fitted_random_name))
	{
		throw std::runtime_error(this->source_path_ + ": " + random_name_key +
			" must fit its stage-name design area above the preview edge including its scaled shadow.");
	}
	const RectangleF stage_briefing(stage.briefing.position, stage.briefing.size);
	const Vector2F stage_info = stage.briefing.position + stage.briefing_padding;
	const Vector2F stage_status_position = stage_info + stage.briefing.spacing * 6.0f;
	const float stage_hint_top = (stage_info + stage.briefing.spacing * 7.0f).y;
	const auto check_stage_status = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.detail_font);
		const Vector2F shadow = this->menu_.detail_shadow_offset;
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage_status_position, size),
			RectangleF(stage_status_position + shadow, size));
		// Status text keeps its unscaled font. Padding locates its origin;
		// unlike the following hint, it has no inset-width fitting rule.
		if (!std::isfinite(stage_briefing.x) || !std::isfinite(stage_briefing.y) ||
			!std::isfinite(stage_briefing.width) || !std::isfinite(stage_briefing.height) ||
			stage_briefing.width <= 0.0f || stage_briefing.height <= 0.0f ||
			!std::isfinite(stage_hint_top) || !std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !stage_briefing.contains(bounds) ||
			!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) || bounds.bottom() > stage_hint_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the briefing panel and design area before the authored hint row origin, including its shadow.");
		}
	};
	check_stage_status("browse_status", this->menu_text_.stage.browse_status);
	check_stage_status("locked_status", this->menu_text_.stage.locked_status);
	const auto check_stage_briefing = [&](const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, font);
		const Vector2F position = stage_info + stage.briefing.spacing * line;
		const Vector2F shadow = this->menu_.detail_shadow_offset;
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		const float next_top = (stage_info + stage.briefing.spacing * next_line).y;
		// These labels and authored blocks retain the briefing's unscaled font.
		// Their text is not clipped to the panel's width, including the shipped
		// solo objective, which extends beyond its right edge.
		if (!std::isfinite(next_top) || !std::isfinite(shadow.x) || !std::isfinite(shadow.y) ||
			!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || bounds.y < stage_briefing.y ||
			bounds.bottom() > stage_briefing.bottom() ||
			!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) || bounds.bottom() > next_top)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the design area and briefing vertical span before the next authored row origin, including its shadow.");
		}
		return bounds;
	};
	check_stage_briefing("briefing_label", this->menu_text_.stage.briefing_label, this->menu_.brand_font, 0.0f, 1.0f);
	check_stage_briefing("goal", this->menu_text_.stage.goal, this->menu_.brand_font, 3.0f, 4.0f);
	check_stage_briefing("solo_objective", this->menu_text_.stage.solo_objective, this->menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing("versus_objective", this->menu_text_.stage.versus_objective, this->menu_.brand_font, 4.0f, 6.0f);
	check_stage_briefing("solo_mode", this->menu_text_.stage.solo_mode, this->menu_.detail_font, 1.0f, 2.0f);
	check_stage_briefing("single_match_mode", this->menu_text_.stage.single_match_mode, this->menu_.detail_font, 1.0f, 2.0f);
	const auto check_stage_summary = [&](const char* role, const std::wstring& text, const std::string& font,
		float line, float next_line)
	{
		RectangleF bounds = check_stage_briefing(role, text, font, line, next_line);
		// The page expands the briefing artwork around unscaled copy rather
		// than fitting copy to its authored width. That expanded card must
		// still fit the outer paper and remain separate from the preview.
		const float edge = main.art.keyline_width * 3.0f;
		const float inset = edge + main.setup_shadow_offset;
		bounds.inflate(inset);
		const RectangleF card = RectangleF::union_of(stage_briefing, bounds);
		const RectangleF outer((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
			main.stage_box_size);
		const RectangleF inside(outer.position() + Vector2F(edge),
			outer.size() - Vector2F(edge * 2.0f + main.setup_shadow_offset));
		RectangleF preview(stage.preview_position, stage.preview_size);
		preview.inflate(this->menu_.rule_height);
		const auto finite_rectangle = [](const RectangleF& rectangle)
		{
			return std::isfinite(rectangle.x) && std::isfinite(rectangle.y) &&
				std::isfinite(rectangle.width) && std::isfinite(rectangle.height) &&
				std::isfinite(rectangle.right()) && std::isfinite(rectangle.bottom()) &&
				rectangle.width > 0.0f && rectangle.height > 0.0f;
		};
		if (!std::isfinite(edge) || !std::isfinite(inset) || !finite_rectangle(card) ||
			!finite_rectangle(inside) || !finite_rectangle(preview) || !inside.contains(card) ||
			(card.x < preview.right() && card.right() > preview.x &&
				card.y < preview.bottom() && card.bottom() > preview.y))
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.stage." + role +
				" must keep the expanded briefing card inside the Stage paper and separate from the preview edge, including its retained artwork and shadow.");
		}
	};
	// Stage records share the navigation row, right-aligned to the preview,
	// and the page shrinks a line to the room beside the arena counter. The
	// copy must draw in its font and the row must sit below the preview edge
	// and above the footer.
	const RectangleF record_preview(stage.preview_position, stage.preview_size);
	for (const std::wstring* label : { &this->menu_text_.records.best_win, &this->menu_text_.records.best_coverage })
	{
		const std::wstring line = *label + L" 100.0% / 99 " + this->menu_text_.records.played;
		const Vector2F size = fixed_menu_size("menu_text.records", line, stage.record_font) +
			this->menu_.detail_shadow_offset;
		if (stage.record_y < record_preview.bottom() + this->menu_.rule_height ||
			stage.record_y + size.y > this->menu_.footer_y)
		{
			throw std::runtime_error(this->source_path_ +
				": main_menu.stage_select.record_y and record_font must place menu_text.records "
				"below the preview edge and above the footer.");
		}
	}
	const ResultsMenuPresentation& results = this->results_menu_;
	const RectangleF results_panel((Vector2F(1920.0f, 1080.0f) - results.box_size) / 2.0f, results.box_size);
	const float results_fill_right = (1920.0f + results.fill_box_size.x) / 2.0f;
	const float results_fill_bottom = results_panel.y + results.fill_box_y_offset + results.fill_box_size.y;
	const RectangleF contribution_heading(results.contribution_position, fixed_menu_size(
		"menu_text.results.contribution_heading", this->menu_text_.results.contribution_heading,
		results.contribution_font));
	for (const std::wstring* label : { &this->menu_text_.records.new_best, &this->menu_text_.records.stage_best })
	{
		const Vector2F size = fixed_menu_size("menu_text.records", *label + L" 100.0%", results.contribution_font);
		const RectangleF bounds(Vector2F(results_fill_right - size.x, results.record_y), size);
		if (!results_panel.contains(bounds) || bounds.y < results_fill_bottom ||
			bounds.bottom() > results.proceed_y || bounds.x <= contribution_heading.right())
		{
			throw std::runtime_error(this->source_path_ +
				": results_menu.record_y and menu_text.records must fit the panel below the score bar, "
				"above the prompt and to the right of the contribution heading.");
		}
	}
	const auto finite_stage_rectangle = [](const RectangleF& bounds)
	{
		return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
			std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
			std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
			bounds.width > 0.0f && bounds.height > 0.0f;
	};
	const auto stage_rectangles_overlap = [](const RectangleF& first, const RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	};
	const std::string navigation_key = "menu_text.stage.random_navigation";
	const Vector2F navigation_size = fixed_menu_size(navigation_key, this->menu_text_.stage.random_navigation,
		this->menu_.detail_font);
	const Vector2F stage_detail_shadow = this->menu_.detail_shadow_offset;
	const RectangleF navigation_bounds = RectangleF::union_of(RectangleF(stage.navigation_position, navigation_size),
		RectangleF(stage.navigation_position + stage_detail_shadow, navigation_size));
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->menu_.rule_height);
	if (!std::isfinite(stage_detail_shadow.x) || !std::isfinite(stage_detail_shadow.y) ||
		!std::isfinite(this->menu_.footer_y) || !finite_stage_rectangle(navigation_bounds) ||
		!finite_stage_rectangle(preview_edge) || !finite_stage_rectangle(stage_briefing) ||
		!RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(navigation_bounds) ||
		navigation_bounds.bottom() > this->menu_.footer_y ||
		stage_rectangles_overlap(navigation_bounds, preview_edge) ||
		stage_rectangles_overlap(navigation_bounds, stage_briefing))
	{
		throw std::runtime_error(this->source_path_ + ": " + navigation_key +
			" must fit unscaled inside the design area before the footer without overlapping the preview edge or briefing panel, including its shadow.");
	}
	const Vector2F stage_hint_position = stage_info + stage.briefing.spacing * 7.0f;
	const float stage_hint_width = stage.briefing.size.x - 2.0f * stage.briefing_padding.x;
	const auto check_stage_hint = [&](const char* role, const std::wstring& text)
	{
		const std::string key = std::string("menu_text.stage.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.brand_font);
		const RectangleF raw = RectangleF::union_of(RectangleF(Vector2F::ZERO, size),
			RectangleF(stage_detail_shadow, size));
		if (!std::isfinite(stage_hint_width) || stage_hint_width <= 0.0f || !finite_stage_rectangle(raw))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must have a finite positive hint width and text bounds including its shadow.");
		}
		// Match refresh: measure at scale one and fit only the full union's width.
		const float scale = std::min(1.0f, stage_hint_width / raw.width);
		const Vector2F fitted_size = size * scale;
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage_hint_position, fitted_size),
			RectangleF(stage_hint_position + stage_detail_shadow * scale, fitted_size));
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds) ||
			!stage_briefing.contains(bounds) || !RectangleF(0.0f, 0.0f, 1920.0f, 1080.0f).contains(bounds) ||
			bounds.bottom() > this->menu_.footer_y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its briefing panel and design area before the footer using width-only fitting, including its scaled shadow.");
		}
	};
	check_stage_hint("locked_hint", this->menu_text_.stage.locked_hint);
	check_stage_hint("browse_hint", this->menu_text_.stage.browse_hint);
	const Vector2F options_value_limit((1920.0f + main.options_box_size.x) / 2.0f -
		options_art.shadow_offset - options_art.focus_padding.x - main.options_value_x,
		main.options_spacing.y - options_art.focus_padding.y * 2.0f);
	const auto options_value_position = [&](int row)
	{
		return Vector2F(main.options_value_x, main.options_position.y + main.options_spacing.y * static_cast<float>(row));
	};
	check_value("on", values.on, this->menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("off", values.off, this->menu_.item_font, options_value_position(1), options_value_limit,
		main.options_box_size, options_art, main.options_spacing.y, false);
	check_value("random", values.random, this->menu_.item_font, options_value_position(2), options_value_limit,
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
	const MenuTitleText& title = this->menu_text_.title;
	const RectangleF wordmark = centered_title("menu_text.title.wordmark", title.wordmark,
		main.title_font, main.title_y, Vector2F::ZERO);
	const RectangleF subtitle = centered_title("menu_text.title.subtitle", title.subtitle,
		this->menu_.detail_font, main.title_subtitle_y, Vector2F::ZERO);
	RectangleF start = centered_title("menu_text.title.start", title.start,
		this->menu_.item_font, main.title_start_y, main.art.focus_padding);
	start.inflate(main.art.focus_padding);
	const RectangleF no_controller = centered_title("menu_text.title.no_controller", title.no_controller,
		this->menu_.item_font, main.title_no_pad_y, Vector2F::ZERO);
	// Build identity remains generated, but it still shares the title's paper.
	const RectangleF build = centered_title("main_menu.build_id_y", build_info::label,
		this->prompt_bar_.font, main.build_id_y, Vector2F::ZERO);
	const auto precedes = [&](const std::string& key, const RectangleF& before, const RectangleF& after)
	{
		if (before.bottom() > after.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must end before the next menu text or focus card.");
		}
	};
	precedes("menu_text.title.wordmark", wordmark, subtitle);
	precedes("menu_text.title.subtitle", subtitle, start);
	precedes("menu_text.title.start", start, no_controller);
	precedes("menu_text.title.no_controller", no_controller, build);
	const RectangleF home_subtitle(main.home_subtitle_position,
		fixed_menu_size("menu_text.home_subtitle", this->menu_text_.home_subtitle, this->menu_.detail_font));
	check_fixed_paper("menu_text.home_subtitle", home_subtitle, main.home_box_size, Vector2F::ZERO);
	const RectangleF home_heading(main.home_position,
		fixed_menu_size("menu_text.headings.home", this->menu_text_.heading(MenuHeading::home), this->menu_.heading_font));
	check_fixed_paper("menu_text.headings.home", home_heading, main.home_box_size, Vector2F::ZERO);
	precedes("menu_text.home_subtitle", home_heading, home_subtitle);
	RectangleF play(main.home_position + Vector2F(0.0f, main.home_spacing.y * 2.0f),
		fixed_menu_size("menu_text.items.play", this->menu_text_.item(MenuItem::play), this->menu_.item_font));
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
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must have a finite positive fitting area inside the controls help region.");
		}
		// Preserve the two pages' existing uniform fit. The available region
		// bounds it by construction; supported long blocks need no length cap.
		const float scale = std::min({ 1.0f, available.x / raw.x, available.y / raw.y });
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must retain finite positive text bounds after fitting.");
		}
		return RectangleF(position, fitted);
	};
	RectangleF controls_help;
	bool first_help = true;
	for (const Key key : controls_text::jump_keys())
	{
		const RectangleF fitted = check_help_fit("menu_text.help.controls",
			controls_text::controls_help(this->menu_text_.help.controls, key), controls.controls_position, controls.controls_size);
		controls_help = first_help ? fitted : RectangleF::union_of(controls_help, fitted);
		first_help = false;
	}
	const KeyboardJumpMenuText& jump = this->menu_text_.keyboard_jump;
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
			{ throw std::runtime_error(this->source_path_ + ": " + where + " must retain finite positive fitted action bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF jump_heading = jump_text("heading", jump.heading, this->menu_.heading_font, controls.heading_position, false);
	if (jump_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->source_path_ + ": menu_text.keyboard_jump.heading must end before its first focus card."); }
	jump_text("capture", jump.capture, controls.item_font, controls.layout.position, true);
	jump_text("reset", jump.reset, controls.item_font, controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y), true);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.help", jump.help, controls.help_font);
	std::ignore = fixed_menu_size("menu_text.keyboard_jump.debug_collision", jump.debug_collision, controls.help_font);
	for (const Key current : controls_text::jump_keys())
	{
		std::ignore = fixed_menu_size("menu_text.keyboard_jump.current", jump.current_key(input_key_label(current)), controls.help_font);
		std::ignore = fixed_menu_size("menu_text.keyboard_jump.pending", jump.pending_key(input_key_label(current)), controls.help_font);
	}
	for (const Key current : controls_text::jump_keys())
	{
		for (const Key pending : controls_text::jump_keys())
		{
			const std::wstring block = jump.help_block(input_key_label(current), input_key_label(pending));
			for (const bool collision : { false, true })
			{
				std::ignore = check_help_fit("menu_text.keyboard_jump.help", collision ? block + L'\n' + jump.debug_collision : block,
					controls.controls_position, controls.controls_size);
			}
		}
	}
	const PadButtonsMenuText& pad_buttons = this->menu_text_.pad_buttons;
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
			{ throw std::runtime_error(this->source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF pad_buttons_heading = pad_buttons_text("heading", pad_buttons.heading, this->menu_.heading_font,
		controls.heading_position, false);
	if (pad_buttons_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->source_path_ + ": menu_text.pad_buttons.heading must end before its first focus card."); }
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
	const ProfilesMenuText& profiles = this->menu_text_.profiles;
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
			{ throw std::runtime_error(this->source_path_ + ": " + where + " must retain finite positive fitted bounds."); }
			size *= scale;
		}
		check_fixed_paper(where, RectangleF(position, size), controls.box_size, row ? controls.art.focus_padding : Vector2F::ZERO);
		if (position.x + size.x + (row ? controls.art.focus_padding.x : 0.0f) > controls.controls_position.x ||
			(row && size.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y))
		{ throw std::runtime_error(this->source_path_ + ": " + where + " must fit its controls column and row."); }
		return RectangleF(position, size);
	};
	const RectangleF profiles_heading = profiles_text("heading", profiles.heading, this->menu_.heading_font,
		controls.heading_position, false);
	if (profiles_heading.bottom() > controls.layout.position.y - controls.art.focus_padding.y)
	{ throw std::runtime_error(this->source_path_ + ": menu_text.profiles.heading must end before its first focus card."); }
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
	const RectangleF pad_association = check_help_fit("menu_text.help.pad_association", this->menu_text_.help.pad_association,
		preview.association_position, help_area.bottom_right() - preview.association_position);
	if (preview.association_position.y < preview.aim.bottom())
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.help.pad_association must begin below the aim preview.");
	}
	const Vector2F caption_size = fixed_menu_size("menu_text.help.aim_cutoff",
		this->menu_text_.help.aim_cutoff, controls.help_font);
	const float probe_height = render.measure_text(render.resolve_sprite_font(controls.help_font), L"Move").y;
	const RectangleF caption(preview.aim.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y), caption_size);
	if (!std::isfinite(probe_height) || probe_height <= 0.0f || !help_area.contains(caption) ||
		caption.y < preview.movement_response.bottom() || caption.bottom() > preview.aim.y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.help.aim_cutoff must fit unscaled inside the controls help region between the movement and aim previews.");
	}
	const auto padded_back = [&](const Vector2F& position, const std::string& font,
		const MenuPaperArtPresentation& art)
	{
		RectangleF bounds(position, fixed_menu_size("menu_text.items.back", this->menu_text_.item(MenuItem::back), font));
		bounds.inflate(art.focus_padding);
		return bounds;
	};
	const RectangleF options_back = padded_back(main.options_position + Vector2F(0.0f, main.options_spacing.y * 6.0f),
		this->menu_.item_font, options_art);
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
			throw std::runtime_error(this->source_path_ + ": " + key + " must leave finite positive status width.");
		}
		// All three pages reset to the authored font size, then fit width only.
		const float scale = std::min(1.0f, available / raw.x);
		const Vector2F fitted = raw * scale;
		if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(fitted.x) || !std::isfinite(fitted.y) ||
			fitted.x <= 0.0f || fitted.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " must retain finite positive fitted status bounds.");
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
				throw std::runtime_error(this->source_path_ + ": " + key +
					" must not overlap the Back card, help text or stick previews.");
			}
		}
	};
	const MenuStatusText& status_text = this->menu_text_.status;
	const StatusMessage options_statuses[]{ { "options_pending", status_text.options_pending },
		{ "options_saved", status_text.options_saved }, { "options_save_failed", status_text.options_save_failed },
		{ "options_reverted", status_text.options_reverted }, { "options_display_failed", status_text.options_display_failed },
		{ "options_rollback_failed", status_text.options_rollback_failed } };
	for (const StatusMessage& message : options_statuses)
	{
		check_status(message, this->prompt_bar_.font, main.options_status_position, main.options_box_size,
			options_art, { options_back });
	}
	const StatusMessage controls_statuses[]{ { "controls_pending", status_text.controls_pending },
		{ "controls_saved", status_text.controls_saved }, { "active_save_failed", status_text.active_save_failed } };
	for (const StatusMessage& message : controls_statuses)
	{
		check_status(message, controls.help_font, controls.status_position, controls.box_size,
			controls.art, { controls_back, controls_help });
	}
	const RectangleF jump_back = padded_back(controls.layout.position + Vector2F(0.0f, controls.layout.spacing.y * 3.0f),
		controls.item_font, controls.art);
	const StatusMessage jump_statuses[]{ { "keyboard_jump.pending_status", jump.pending_status }, { "keyboard_jump.saved_status", jump.saved_status },
		{ "keyboard_jump.capture_wait_neutral", jump.capture_wait_neutral }, { "keyboard_jump.capture_ready", jump.capture_ready },
		{ "keyboard_jump.capture_rejected", jump.capture_rejected }, { "keyboard_jump.capture_conflict", jump.capture_conflict },
		{ "keyboard_jump.capture_cancelled", jump.capture_cancelled }, { "keyboard_jump.debug_collision", jump.debug_collision } };
	for (const StatusMessage& message : jump_statuses)
	{ check_status(message, controls.help_font, controls.status_position, controls.box_size, controls.art, { jump_back, help_area }); }
	for (const Key key : controls_text::jump_keys())
	{
		const std::wstring words = jump.release_key(std::wstring(input_key_label(key)));
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
	check.font("hud.meter_label_font", this->hud_.meter_label_font);
	check.font("hud.player_identity.font", this->hud_.player_identity_font);
	check.frame("hud.fill_frame", this->hud_.sprite_sheet, this->hud_.fill_frame);
	check.frame("movement_feedback.fill_frame", this->movement_feedback_.sprite_sheet,
		this->movement_feedback_.fill_frame);
	check.frame("combat_feedback.fill_frame", this->combat_feedback_.sprite_sheet,
		this->combat_feedback_.fill_frame);
	check.frame("weapon_feedback.fill_frame", this->weapon_feedback_.sprite_sheet,
		this->weapon_feedback_.fill_frame);
	check.frame("blocked_shot.fill_frame", this->blocked_shot_.sprite_sheet,
		this->blocked_shot_.fill_frame);
	check.frame("idle_drip.fill_frame", this->idle_drip().sprite_sheet,
		this->idle_drip().fill_frame);
	check.frame("figure.rest_arms_frame", this->figure_.sprite_sheet, this->figure_.rest_arms_frame);
	check.frame("figure.rest_arms_paint_frame", this->figure_.sprite_sheet,
		this->figure_.rest_arms_paint_frame);
	check.frame("figure.sleeves_frame", this->figure_.sprite_sheet, this->figure_.sleeves_frame);
	check.frame("figure.sleeves_paint_frame", this->figure_.sprite_sheet,
		this->figure_.sleeves_paint_frame);
	check.frame("tool_motion.mist_tail.fill_frame", this->tool_motion_.mist_sheet,
		this->tool_motion_.mist_frame);
	check.frame("respawn_redraw.pen_frame", this->respawn_redraw_.sprite_sheet,
		this->respawn_redraw_.pen_frame);
	check.frame("opening.fill_frame", this->opening_.sprite_sheet,
		this->opening_.fill_frame);
	check.sheet("opening.palette_sheet", this->opening_.palette_sheet);
	const SpriteSheet* palette_sheet = render.sprite_sheet(this->opening_.palette_sheet);
	for (size_t index = 0; index < this->opening_.palette_frames.size(); ++index)
	{
		const std::string key = "opening.palette_frames[" + std::to_string(index) + "]";
		const std::string& name = this->opening_.palette_frames[index];
		check.frame(key, this->opening_.palette_sheet, name);
		const SpriteFrame& swatch = palette_sheet->sprite_frame(palette_sheet->resolve_sprite_frame(name));
		const RectangleI& source = swatch.source_rectangle();
		if (source.width <= 0 || source.height <= 0 || source.width != source.height ||
			swatch.origin() != Vector2F::ZERO)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must name a positive square swatch with zero authored origin.");
		}
	}
	check.font("opening.percent_font", this->opening_.percent_font);
	const Vector2F heading_size = render.measure_text(
		render.resolve_sprite_font(this->menu_.detail_font), L"GET READY");
	const float heading_scale = std::min(1.0f,
		(this->match_.countdown_size.x - this->match_.controls_padding.x * 2.0f) /
			(heading_size.x + std::abs(this->menu_.detail_shadow_offset.x)));
	if ((heading_size.y + std::abs(this->menu_.detail_shadow_offset.y)) * heading_scale +
		this->match_.controls_padding.y * 3.0f >= this->match_.countdown_size.y)
	{
		throw std::runtime_error(this->source_path_ +
			": match.countdown_size must leave room below the GET READY heading.");
	}

	for (const SoundReference& sound : this->sound_references())
	{
		check.sound(sound);
	}

	// Non-menu primitive drawing and developer overlays still use this texel.
	check.frame("built-in primitive frame 'pixel'", "sprite_sheet_1", "pixel");
	const WeaponSelectArtworkPresentation& artwork = this->main_menu_.weapon_select_art;
	check.frame("main_menu.weapon_select.fill_frame", artwork.sheet, artwork.fill_frame);
	const SpriteSheet* weapon_sheet = render.sprite_sheet(artwork.sheet);
	const float name_width = this->main_menu_.weapon_description_x_offset;
	if (!std::isfinite(name_width) || name_width <= 0.0f)
	{
		throw std::runtime_error(this->source_path_ +
			": main_menu.weapon_description.x_offset must be finite and positive for the weapon name column.");
	}
	const FontHandle name_font = render.resolve_sprite_font(this->main_menu_.weapon_description_font);
	for (size_t index = 0; index < artwork.choices.size(); ++index)
	{
		const WeaponChoicePresentation& choice = artwork.choices[index];
		const std::string key = std::string("main_menu.weapon_select.choices.") + weapon_choice_keys[index];
		drawable_text::check_font_can_draw(&render, this->main_menu_.weapon_description_font,
			choice.name, this->source_path_ + ": " + key + ".name");
		const Vector2F name_size = render.measure_text(name_font, choice.name);
		if (!std::isfinite(name_size.x) ||
			name_size.x + std::abs(this->menu_.detail_shadow_offset.x) > name_width)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				".name must fit the name column before main_menu.weapon_description.x_offset.");
		}
		check.frame(key + ".frame", artwork.sheet, choice.frame);
		const SpriteFrame& body = weapon_sheet->sprite_frame(weapon_sheet->resolve_sprite_frame(choice.frame));
		if (body.source_rectangle().width <= 0 || body.source_rectangle().height <= 0)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				".frame must have positive source dimensions.");
		}
		if (!choice.paint_frame.empty())
		{
			check.frame(key + ".paint_frame", artwork.sheet, choice.paint_frame);
			const SpriteFrame& paint = weapon_sheet->sprite_frame(
				weapon_sheet->resolve_sprite_frame(choice.paint_frame));
			if (body.source_rectangle().size() != paint.source_rectangle().size() || body.origin() != paint.origin())
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					".paint_frame must match its body dimensions and origin.");
			}
		}
		if (!choice.supply_strip.empty())
		{
			check.animation(key + ".supply_strip", artwork.sheet, choice.supply_strip);
			const AnimationStrip& supply = weapon_sheet->animation_strip(
				weapon_sheet->resolve_animation_strip(choice.supply_strip));
			if (supply.frame_count() <= 0 || body.origin() != Vector2F::ZERO)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					".supply_strip needs at least one frame and a body with zero authored origin.");
			}
			for (int frame = 0; frame < supply.frame_count(); ++frame)
			{
				if (supply.frame_rect(frame).size() != body.source_rectangle().size())
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						".supply_strip must retain its body dimensions for every frame.");
				}
			}
		}
	}
	// Preserve the existing diagnostics when a shared font or shadow also
	// invalidates an earlier page, then check Team's additional paper bounds.
	const RectangleF team_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.team_box_size) / 2.0f,
		main.team_box_size);
	const RectangleF team_inside(team_paper.position() + Vector2F(setup_edge),
		team_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const auto check_team_text = [&](const char* key, const std::wstring& words, const std::string& font,
		const Vector2F& position, const Vector2F& shadow)
	{
		drawable_text::check_font_can_draw(&render, font, words, this->source_path_ + ": " + key);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), words);
		const RectangleF bounds = RectangleF::union_of(RectangleF(position, size), RectangleF(position + shadow, size));
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			bounds.width <= 0.0f || bounds.height <= 0.0f || !team_inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit main_menu.team_box_size including its retained shadow bounds.");
		}
		return bounds;
	};
	const RectangleF team_heading = check_team_text("menu_text.headings.teams",
		this->menu_text_.heading(MenuHeading::teams), this->menu_.heading_font,
		this->menu_.layout.position, this->menu_.heading_shadow_offset);
	const RectangleF team_warning = check_team_text(
		"menu_text.connection.teams_required at main_menu.team_select.warning_position",
		this->menu_text_.connection.teams_required, this->menu_.detail_font,
		main.team_warning_position, this->menu_.detail_shadow_offset);
	if (team_warning.y < team_heading.bottom())
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.connection.teams_required at main_menu.team_select.warning_position must follow the Team Select heading.");
	}
	const RectangleF weapon_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.weapon_box_size) / 2.0f,
		main.weapon_box_size);
	const RectangleF weapon_inside(weapon_paper.position() + Vector2F(setup_edge),
		weapon_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const Vector2F weapon_heading_size = render.measure_text(render.resolve_sprite_font(this->menu_.heading_font),
		this->menu_text_.heading(MenuHeading::weapons));
	const RectangleF weapon_heading = RectangleF::union_of(
		RectangleF(this->menu_.layout.position, weapon_heading_size),
		RectangleF(this->menu_.layout.position + this->menu_.heading_shadow_offset, weapon_heading_size));
	if (!std::isfinite(weapon_heading.x) || !std::isfinite(weapon_heading.y) ||
		!std::isfinite(weapon_heading.width) || !std::isfinite(weapon_heading.height) ||
		weapon_heading.width <= 0.0f || weapon_heading.height <= 0.0f || !weapon_inside.contains(weapon_heading))
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.weapons must fit main_menu.weapon_box_size including its retained shadow bounds.");
	}
	const RectangleF stage_paper((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
		main.stage_box_size);
	const RectangleF stage_inside(stage_paper.position() + Vector2F(setup_edge),
		stage_paper.size() - Vector2F(setup_edge * 2.0f + main.setup_shadow_offset));
	const Vector2F stage_heading_size = render.measure_text(render.resolve_sprite_font(this->menu_.heading_font),
		this->menu_text_.heading(MenuHeading::arena));
	const RectangleF stage_heading = RectangleF::union_of(
		RectangleF(this->menu_.layout.position, stage_heading_size),
		RectangleF(this->menu_.layout.position + this->menu_.heading_shadow_offset, stage_heading_size));
	if (!std::isfinite(stage_heading.x) || !std::isfinite(stage_heading.y) ||
		!std::isfinite(stage_heading.width) || !std::isfinite(stage_heading.height) ||
		stage_heading.width <= 0.0f || stage_heading.height <= 0.0f || !stage_inside.contains(stage_heading))
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.arena must fit main_menu.stage_box_size including its retained shadow bounds.");
	}
	// Home's Exit confirmation reuses Pause paper/layout, with the existing
	// Exit label rendered once more in the actual heading font.
	const Vector2F exit_heading_size = fixed_menu_size("menu_text.items.exit",
		this->menu_text_.item(MenuItem::exit), this->menu_.heading_font);
	const RectangleF exit_heading((1920.0f - exit_heading_size.x) / 2.0f, pause.heading_y,
		exit_heading_size.x, exit_heading_size.y);
	if (!std::isfinite(exit_heading.x) || !std::isfinite(exit_heading.y) ||
		!confirmation_paper.contains(exit_heading) || exit_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.items.exit must fit unscaled inside the Pause paper before the Exit confirmation question.");
	}
	const Vector2F display_heading_size = fixed_menu_size("menu_text.headings.options",
		this->menu_text_.heading(MenuHeading::options), this->menu_.heading_font);
	const RectangleF display_heading((1920.0f - display_heading_size.x) / 2.0f, pause.heading_y,
		display_heading_size.x, display_heading_size.y);
	if (!std::isfinite(display_heading.x) || !std::isfinite(display_heading.y) ||
		!confirmation_paper.contains(display_heading) || display_heading.bottom() > pause.status_y)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.headings.options must fit unscaled inside the Pause paper before the display confirmation question.");
	}
	RectangleF revert_card(pause.layout.position + Vector2F(0.0f, pause.layout.spacing.y),
		fixed_menu_size("menu_text.items.revert", this->menu_text_.item(MenuItem::revert), this->menu_.item_font));
	revert_card.inflate(display_art.focus_padding);
	const float countdown_y = pause.layout.position.y + pause.layout.spacing.y * 2.0f;
	for (int seconds = 0; seconds <= static_cast<int>(std::ceil(main.display_trial_seconds)); ++seconds)
	{
		const std::string key = "menu_text.display_trial_countdown";
		const Vector2F size = fixed_menu_size(key, this->menu_text_.display_countdown(seconds), this->menu_.detail_font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, countdown_y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !confirmation_paper.contains(bounds) ||
			bounds.y < revert_card.bottom())
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper after the Revert focus card for every countdown value.");
		}
	}
	// Display combines compact Main geometry with Pause's retained keyline.
	// Keep the same text-to-face clearance that each authored paper requires.
	if (display_art.focus_padding.x <= display_art.shadow_offset + display_art.keyline_width ||
		display_art.focus_padding.y <= display_art.shadow_offset + display_art.keyline_width)
	{
		throw std::runtime_error(this->source_path_ +
			": main_menu.setup_focus_padding must exceed main_menu.setup_shadow_offset + "
			"pause_menu.art.keyline_width on both axes for the Display confirmation paper.");
	}
	const std::string palette_timing_key = "menu_text.options_palette_timing";
	const Vector2F palette_timing_size = fixed_menu_size(palette_timing_key,
		this->menu_text_.options_palette_timing, this->menu_.detail_font);
	const RectangleF palette_timing(Vector2F(main.options_value_x, main.options_heading_position.y), palette_timing_size);
	check_fixed_paper(palette_timing_key, palette_timing, main.options_box_size, Vector2F::ZERO);
	const RectangleF options_heading(main.options_heading_position,
		fixed_menu_size("menu_text.headings.options", this->menu_text_.heading(MenuHeading::options), this->menu_.heading_font));
	if (palette_timing.x < options_heading.right() ||
		palette_timing.bottom() > main.options_position.y - options_art.focus_padding.y)
	{
		throw std::runtime_error(this->source_path_ + ": " + palette_timing_key +
			" must fit unscaled beside the Options heading and above the first action's focus padding.");
	}
	// Check these additional roles after the established pages so a shared font
	// or geometry error retains its earlier diagnostic wherever possible.
	constexpr std::array<BotDifficulty, 3> label_difficulties{
		BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
	constexpr int label_choices = input_slots::count + static_cast<int>(label_difficulties.size());
	const auto label_words = [&](int player, int choice)
	{
		return choice < input_slots::count ? this->menu_text_.roster_label(player, choice) :
			this->menu_text_.bot_roster_label(player, label_difficulties[static_cast<size_t>(choice - input_slots::count)]);
	};
	const auto label_key = [](int slot)
	{
		if (slot >= input_slots::count)
		{
			constexpr std::array<const char*, 3> roles{ "bot_easy", "bot_normal", "bot_hard" };
			return std::string("menu_text.labels.roster_row with menu_text.labels.bot and menu_text.labels.") +
				roles[static_cast<size_t>(slot - input_slots::count)];
		}
		return std::string("menu_text.labels.roster_row with menu_text.labels.") +
			(input_slots::is_pad(slot) ? "pad" : input_slots::is_keyboard_and_mouse(slot) ? "keyboard_mouse" : "waiting");
	};
	const auto label_size = [&](const std::string& key, const std::wstring& words, const std::string& font_name)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font_name, words, where);
		const Font& font = *render.font(render.resolve_sprite_font(font_name));
		LabelPen pen;
		append_label_text(pen, font, words, where);
		if (pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
		{
			throw std::runtime_error(where + " must retain finite positive composed label bounds.");
		}
		return pen.extent;
	};
	for (int player = 1; player < 4; ++player)
	{
		for (const BotDifficulty difficulty : label_difficulties)
		{
			std::ignore = label_size("menu_text.labels.bot at world player identity",
				this->menu_text_.bot_roster_label(player, difficulty), this->hud_.player_identity_font);
		}
	}
	const MenuConnectionText& connection = this->menu_text_.connection;
	const std::string reconnect_key = "menu_text.connection.join_reconnect_suffix";
	const RectangleF join_heading = check_setup_paper("menu_text.headings.join_players",
		this->menu_text_.heading(MenuHeading::join_players), this->menu_.heading_font,
		this->menu_.layout.position, this->menu_.heading_shadow_offset, false, 0.0f);
	float join_previous_bottom = join_heading.bottom();
	std::vector<std::string> join_previous_keys{ "menu_text.headings.join_players" };
	const auto previous_join_context = [&]
	{
		std::string context;
		for (const std::string& key : join_previous_keys)
		{
			if (!context.empty()) { context += " or "; }
			context += key;
		}
		return context;
	};
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = this->menu_.layout.position +
			Vector2F(0.0f, main.player_count_spacing.y * static_cast<float>(player + 2));
		float row_bottom = join_previous_bottom;
		std::vector<std::string> row_bottom_keys = join_previous_keys;
		for (int slot = -1; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::wstring label = label_words(player, slot);
			const auto check_join_row = [&](const std::wstring& words, bool reconnect)
			{
				const std::string key = label_key(slot) + " at Join" + (reconnect ? " with " + reconnect_key : "");
				std::ignore = label_size(key, words, this->menu_.item_font);
				RectangleF bounds = check_setup_paper(key, words, this->menu_.item_font,
					position, this->menu_.item_shadow_offset, false, 0.0f);
				// Match Join's retained acknowledgement strip beside the original
				// text bounds, without adding focus-card padding to these rows.
				const RectangleF accent(bounds.x - this->menu_.focus_padding.x, bounds.y,
					std::max(0.0f, this->menu_.focus_padding.x), bounds.height);
				if (player > 0 && (slot < 0 || slot >= input_slots::count))
				{
					const RectangleF cursor(bounds.x, bounds.bottom() + this->menu_.rule_height,
						bounds.width, this->menu_.rule_height);
					bounds = RectangleF::union_of(bounds, cursor);
				}
				bounds = RectangleF::union_of(bounds, accent);
				if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
					!std::isfinite(bounds.width) || !std::isfinite(bounds.height) || !setup_inside.contains(bounds))
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						" must fit every composed Join player/device row inside the setup paper including its retained shadow and acknowledgement bounds.");
				}
				if (bounds.y < join_previous_bottom)
				{
					throw std::runtime_error(this->source_path_ + ": " + key +
						" requires Join player rows to follow the heading and each other without overlap"
						" (preceding envelope contributions from " + previous_join_context() + ").");
				}
				if (bounds.bottom() > row_bottom)
				{
					row_bottom_keys = { key };
				}
				else if (bounds.bottom() == row_bottom &&
					std::find(row_bottom_keys.begin(), row_bottom_keys.end(), key) == row_bottom_keys.end())
				{
					row_bottom_keys.push_back(key);
				}
				row_bottom = std::max(row_bottom, bounds.bottom());
			};
			check_join_row(label, false);
			// The shared keyboard/mouse input is always connected. Only a
			// reserved pad can actually display the reconnect suffix, and only
			// a connected pad shows the profile it wears, whose name may be
			// the widest one.
			if (input_slots::is_pad(slot))
			{
				check_join_row(label + connection.join_reconnect_suffix, true);
				check_join_row(this->menu_text_.worn_profile_label(label,
					std::string(player_profiles::max_name_length, 'W')), false);
			}
		}
		join_previous_bottom = row_bottom;
		join_previous_keys = std::move(row_bottom_keys);
	}
	const auto check_join_status = [&](const char* role, const std::wstring& words)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		std::ignore = fixed_menu_size(key, words, this->menu_.detail_font);
		const RectangleF bounds = check_setup_paper(key, words, this->menu_.detail_font,
			this->menu_.layout.position + Vector2F(0.0f, main.player_count_spacing.y * 7.0f),
			this->menu_.detail_shadow_offset, false, 0.0f);
		if (bounds.y < join_previous_bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must follow all four Join player rows without overlap"
				" (preceding envelope contributions from " + previous_join_context() + ").");
		}
	};
	check_join_status("join_ready", connection.join_ready);
	check_join_status("join_waiting", connection.join_waiting);
	const auto check_pause_connection = [&](const char* role, const std::wstring& words, bool heading)
	{
		const std::string key = std::string("menu_text.connection.") + role;
		const Vector2F size = fixed_menu_size(key, words,
			heading ? this->menu_.heading_font : this->menu_.detail_font);
		// Pause uses plain UiText, centered on the full-design paper. It has
		// neither Main's footer centering nor retained text-shadow bounds.
		const RectangleF bounds((1920.0f - size.x) / 2.0f,
			heading ? pause.heading_y : pause.status_y, size.x, size.y);
		const float bottom = heading ? pause.status_y : pause.layout.position.y - pause.art.focus_padding.y;
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !std::isfinite(bottom) ||
			!confirmation_paper.contains(bounds) || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Pause paper in heading, status, first-action order including the action's focus padding.");
		}
	};
	check_pause_connection("pause_title", connection.pause_title, true);
	for (int number = 1; number <= 4; ++number)
	{
		check_pause_connection("pause_player_title", this->menu_text_.pause_title_for_player(number), true);
	}
	check_pause_connection("controller_lost", connection.controller_lost, true);
	check_pause_connection("all_connected", connection.all_connected, false);
	const Font& stick_font = *render.font(render.resolve_sprite_font(controls.help_font));
	const auto stick_numeric_size = [&](const char* role, const std::wstring& text)
	{
		const std::string where = this->source_path_ + ": menu_text.stick_readouts." + role;
		drawable_text::check_font_can_draw(&render, controls.help_font, L"-0123456789", where);
		struct Pen
		{
			float x = 0.0f;
			float y = 0.0f;
			Vector2F extent = Vector2F::ZERO;
		};
		const auto walk = [&](Pen& pen, std::wstring_view words)
		{
			// Match Font's pen order, including accumulated bearings, the clamp
			// at zero, whitespace suppression and glyph overhang beyond a line.
			for (const wchar_t character : words)
			{
				if (character == L'\n')
				{
					pen.x = 0.0f;
					pen.y += stick_font.line_spacing();
					continue;
				}
				const Glyph& glyph = stick_font.drawn(static_cast<char32_t>(character));
				if (!std::isfinite(glyph.x_offset) || !std::isfinite(glyph.y_offset) ||
					!std::isfinite(glyph.x_advance))
				{
					throw std::runtime_error(where + " requires finite font glyph metrics.");
				}
				const float width = static_cast<float>(glyph.subrect.width);
				pen.x += glyph.x_offset;
				if (pen.x < 0.0f) { pen.x = 0.0f; }
				if (character != L' ' || glyph.subrect.width > 1 || glyph.subrect.height > 1)
				{
					const float height = character == L' ' ? stick_font.line_spacing() :
						std::max(stick_font.line_spacing(), static_cast<float>(glyph.subrect.height) + glyph.y_offset);
					pen.extent.x = std::max(pen.extent.x, pen.x + width);
					pen.extent.y = std::max(pen.extent.y, pen.y + height);
				}
				pen.x += width + glyph.x_advance;
			}
		};
		if (!std::isfinite(stick_font.line_spacing()) || stick_font.line_spacing() <= 0.0f)
		{
			throw std::runtime_error(where + " requires finite positive font line spacing.");
		}
		Pen pen;
		size_t begin = 0;
		for (;;)
		{
			const size_t token = text.find(L'{', begin);
			const std::wstring_view literal = std::wstring_view(text).substr(begin,
				token == std::wstring::npos ? token : token - begin);
			drawable_text::check_font_can_draw(&render, controls.help_font, literal, where);
			walk(pen, literal);
			if (token == std::wstring::npos) { break; }
			// Every incoming-pen transform is monotone. The maximum reachable
			// pen suffices for the next token, but its greatest ink extent may
			// come from a different value, so retain both maxima independently.
			// Tokens contain no LF; their line position is independent of value.
			float maximum_pen = -std::numeric_limits<float>::infinity();
			Vector2F maximum_extent = pen.extent;
			for (int value = -100; value <= 100; ++value)
			{
				Pen candidate = pen;
				walk(candidate, std::to_wstring(value));
				maximum_pen = std::max(maximum_pen, candidate.x);
				maximum_extent.x = std::max(maximum_extent.x, candidate.extent.x);
				maximum_extent.y = std::max(maximum_extent.y, candidate.extent.y);
			}
			pen.x = maximum_pen;
			pen.extent = maximum_extent;
			// The required template was checked while its source key was read.
			begin = text.find(L'}', token) + 1;
		}
		if (!std::isfinite(pen.x) || !std::isfinite(pen.y) ||
			!std::isfinite(pen.extent.x) || !std::isfinite(pen.extent.y) ||
			pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
		{
			throw std::runtime_error(where + " must retain finite positive measured text bounds for every rounded percentage.");
		}
		return pen.extent;
	};
	const auto check_stick_region = [&](const char* role, const Vector2F& size,
		const Vector2F& position, float top, float bottom)
	{
		const std::string key = std::string("menu_text.stick_readouts.") + role +
			((std::string_view(role) == "connected" || std::string_view(role) == "disconnected") ?
				" with menu_text.labels.pad" : "");
		const RectangleF bounds(position, size);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
			!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
			!std::isfinite(top) || !std::isfinite(bottom) || !help_area.contains(bounds) ||
			bounds.y < top || bounds.bottom() > bottom)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside its controls preview/readout region without overlapping the adjacent preview or help text.");
		}
		return bounds;
	};
	const Vector2F raw_caption_position = preview.raw_movement.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const Vector2F response_caption_position = preview.movement_response.position() -
		Vector2F(0.0f, probe_height + controls.art.focus_padding.y);
	const MenuText& stick_text = this->menu_text_;
	for (const bool connected : { true, false })
	{
		const char* role = connected ? "connected" : "disconnected";
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			const Vector2F size = label_size(std::string("menu_text.stick_readouts.") + role + " with menu_text.labels.pad",
				stick_text.stick_connection(slot, connected), controls.help_font);
			check_stick_region(role, size, controls.controls_position, help_area.y, raw_caption_position.y);
		}
	}
	check_stick_region("raw_movement", stick_numeric_size("raw_movement", stick_text.stick_readouts.raw_movement),
		raw_caption_position, help_area.y, preview.raw_movement.y);
	check_stick_region("movement_response", stick_numeric_size("movement_response", stick_text.stick_readouts.movement_response),
		response_caption_position, preview.raw_movement.bottom(), preview.movement_response.y);
	for (const auto& readout : { std::pair("aim_inactive", &stick_text.stick_readouts.aim_inactive),
		std::pair("aim_active", &stick_text.stick_readouts.aim_active) })
	{
		check_stick_region(readout.first, stick_numeric_size(readout.first, *readout.second),
			preview.aim_readout_position, preview.aim.y, preview.association_position.y);
		if (preview.aim_readout_position.x < preview.aim.right())
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.stick_readouts." + readout.first +
				" must remain beside the aim preview.");
		}
	}

	// The remaining label consumers have different fonts, shadows and badge
	// geometry. Validate their complete finite outputs, retaining their existing
	// unfitted layouts and the Pad row's existing OptionRow fit.
	const auto check_label_bounds = [&](const std::string& key, const RectangleF& bounds, const RectangleF& inside)
	{
		if (!finite_stage_rectangle(bounds) || !inside.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its paper including its actual text, shadow, badge and row bounds.");
		}
	};
	const auto shadow_bounds = [](const Vector2F& position, const Vector2F& size, const Vector2F& offset)
	{
		return RectangleF::union_of(RectangleF(position, size), RectangleF(position + offset, size));
	};
	using LabelRows = std::array<std::array<RectangleF, label_choices>, 4>;
	LabelRows team_rows;
	LabelRows weapon_rows;
	LabelRows results_rows;
	const SpriteSheet* team_sheet = render.sprite_sheet(main.team_select_sheet);
	for (int player = 0; player < 4; ++player)
	{
		const float row = static_cast<float>(player + 2);
		const Vector2F team_position = this->menu_.layout.position + Vector2F(0.0f, main.team_select.spacing.y * row);
		const Vector2F weapon_position = this->menu_.layout.position + Vector2F(0.0f, main.weapon_select.spacing.y * row);
		RectangleF team_icons;
		float first_team_x = std::numeric_limits<float>::infinity();
		for (size_t icon = 0; icon < main.team_select_frames.size(); ++icon)
		{
			const Vector2F position = team_position + Vector2F(main.team_select.spacing.x * 2.0f +
				main.team_select.size.x * static_cast<float>(icon), 0.0f);
			const RectangleF canvas(position, main.team_select.size);
			const SpriteFrame& frame = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_frames[icon]));
			const RectangleF bounds = sprite_world_bounds(canvas, frame.source_rectangle(), 0.0f, frame.origin());
			check_label_bounds("menu_text.labels.roster_row at Team Select icons", bounds, team_inside);
			team_icons = icon == 0 ? bounds : RectangleF::union_of(team_icons, bounds);
			if (icon == 0)
			{
				const SpriteFrame& selected = team_sheet->sprite_frame(team_sheet->resolve_sprite_frame(main.team_select_selected_frames[icon]));
				const RectangleF selected_bounds = sprite_world_bounds(canvas, selected.source_rectangle(), 0.0f, selected.origin());
				check_label_bounds("menu_text.labels.roster_row at Team Select selected icon", selected_bounds, team_inside);
				first_team_x = std::min(bounds.x, selected_bounds.x);
			}
		}
		const Vector2F weapon_canvas_position = weapon_position + Vector2F(main.weapon_select.spacing.x * 2.0f, 0.0f);
		RectangleF weapon_tile(weapon_canvas_position, main.weapon_select.size);
		weapon_tile.inflate(this->menu_.icon_padding);
		weapon_tile.inflate(this->menu_.rule_height);
		RectangleF weapon_contents = weapon_tile;
		for (size_t choice = 0; choice < artwork.choices.size(); ++choice)
		{
			const Vector2F name_position = weapon_canvas_position + Vector2F(0.0f,
				main.weapon_select.size.y + this->menu_.icon_padding.y + this->menu_.rule_height);
			const Vector2F name_size = render.measure_text(name_font, artwork.choices[choice].name);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(name_position, name_size, this->menu_.detail_shadow_offset));
			const Vector2F description_position = weapon_canvas_position + Vector2F(main.weapon_description_x_offset, 0.0f);
			const std::wstring wrapped = text_wrap::wrap(render, name_font, main.weapon_descriptions[choice], main.weapon_description_max_width);
			const Vector2F description_size = render.measure_text(name_font, wrapped);
			weapon_contents = RectangleF::union_of(weapon_contents,
				shadow_bounds(description_position, description_size, main.weapon_description_shadow_offset));
		}
		for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::wstring words = label_words(player, slot);
			const std::string key = label_key(slot);
			const RectangleF team_label = shadow_bounds(team_position,
				label_size(key + " at Team Select", words, this->menu_.detail_font), this->menu_.item_shadow_offset);
			RectangleF team_card = RectangleF::union_of(team_label, UiPlayerBadge::before(team_label, this->player_identity_));
			team_card = RectangleF::union_of(team_card, team_icons);
			team_card.inflate(main.setup_focus_padding);
			check_label_bounds(key + " at Team Select", team_card, team_inside);
			if (!std::isfinite(first_team_x) || team_label.right() > first_team_x)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" at Team Select must end before the first choice icon in either captured frame.");
			}
			team_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = team_card;
			// Weapon passes no offset to UiTextDropShadow: its transparent
			// default (2,2) still contributes to bounds.
			const RectangleF weapon_label = shadow_bounds(weapon_position,
				label_size(key + " at Weapon Select", words, this->menu_.detail_font), Vector2F(2.0f, 2.0f));
			RectangleF weapon_row = RectangleF::union_of(weapon_label, UiPlayerBadge::before(weapon_label, this->player_identity_));
			weapon_row = RectangleF::union_of(weapon_row, weapon_contents);
			// The row's handicap sits under its label, in the label's column,
			// in the smaller tool-name font so the row keeps its height.
			for (int level = 0; level < handicaps::count; ++level)
			{
				constexpr std::array<const char*, handicaps::count> handicap_roles{
					"handicap_none", "handicap_plus_one", "handicap_plus_two" };
				const std::string handicap_key = std::string("menu_text.labels.") +
					handicap_roles[static_cast<size_t>(level)] + " at Weapon Select";
				const RectangleF handicap = shadow_bounds(Vector2F(weapon_position.x, weapon_label.bottom()),
					label_size(handicap_key, this->menu_text_.handicap_label(level), main.weapon_description_font),
					Vector2F(2.0f, 2.0f));
				check_label_bounds(handicap_key, handicap, weapon_inside);
				if (handicap.right() > weapon_tile.x)
				{
					throw std::runtime_error(this->source_path_ + ": " + handicap_key +
						" must end before the first weapon tile including its border.");
				}
				weapon_row = RectangleF::union_of(weapon_row, handicap);
			}
			check_label_bounds(key + " at Weapon Select", weapon_row, weapon_inside);
			if (weapon_label.right() > weapon_tile.x)
			{
				throw std::runtime_error(this->source_path_ + ": " + key +
					" at Weapon Select must end before the first weapon tile including its border.");
			}
			weapon_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = weapon_row;
		}
	}
	const auto check_roster_rows = [&](const LabelRows& rows, float heading_bottom, float bottom,
		const char* site)
	{
		// Human devices are unique; bot difficulty choices may repeat. P1
		// remains human. Visit every legal combination of participant labels.
		const auto visit = [&](const auto& self, int player, unsigned int used, float previous_bottom) -> void
		{
			for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
			{
				const unsigned int bit = slot < input_slots::count ? 1u << slot : 0u;
				if ((used & bit) != 0u) { continue; }
				const RectangleF& bounds = rows[static_cast<size_t>(player)][static_cast<size_t>(slot)];
				if (!std::isfinite(heading_bottom) || !std::isfinite(bottom) ||
					bounds.y < previous_bottom || bounds.bottom() > bottom)
				{
					throw std::runtime_error(this->source_path_ + ": " + label_key(slot) + " at " + site +
						" must follow the heading and complete preceding rows without overlap, before the following content.");
				}
				if (player < 3) { self(self, player + 1, used | bit, bounds.bottom()); }
			}
		};
		visit(visit, 0, 0u, heading_bottom);
	};
	check_roster_rows(team_rows, team_heading.bottom(), team_warning.y, "Team Select");
	check_roster_rows(weapon_rows, weapon_heading.bottom(), weapon_inside.bottom(), "Weapon Select");

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
			const Vector2F size = label_size(missing_key, words, this->menu_.detail_font);
			const RectangleF bounds((1920.0f - size.x) / 2.0f, pause.status_y, size.x, size.y);
			check_label_bounds(missing_key, bounds, confirmation_paper);
			if (!std::isfinite(pause.layout.position.y) || !std::isfinite(pause.art.focus_padding.y) ||
				bounds.bottom() > pause.layout.position.y - pause.art.focus_padding.y)
			{
				throw std::runtime_error(this->source_path_ + ": " + missing_key +
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
			next += this->menu_text_.roster_label(player, slot);
			self(self, player + 1, used | bit, count + 1, next);
		}
	};
	missing_lists(missing_lists, 0, 0u, 0, std::wstring());

	const std::string results_key = "menu_text.labels.roster_row at Results";
	const Font& contribution_font = *render.font(render.resolve_sprite_font(results.contribution_font));
	std::ignore = label_size(results_key, L"0123456789.%", results.contribution_font);
	struct PercentageSuffix
	{
		std::wstring words;
		std::array<const Glyph*, 6> glyphs = {};
	};
	std::array<PercentageSuffix, 1001> percentages;
	for (size_t tenth = 0; tenth < percentages.size(); ++tenth)
	{
		PercentageSuffix& suffix = percentages[tenth];
		// These are the legal one-decimal outputs, not rounded float samples:
		// LevelEndInfo's formatter truncates std::to_wstring at one decimal.
		suffix.words = std::to_wstring(tenth / 10) + L"." + std::to_wstring(tenth % 10) + L"%";
		for (size_t character = 0; character < suffix.words.size(); ++character)
		{
			suffix.glyphs[character] = &contribution_font.drawn(static_cast<char32_t>(suffix.words[character]));
		}
	}
	for (int player = 0; player < 4; ++player)
	{
		const Vector2F position = results.contribution_position + Vector2F(0.0f,
			results.contribution_spacing * static_cast<float>(player + 1));
		for (int slot = 0; slot < (player == 0 ? input_slots::count : label_choices); ++slot)
		{
			const std::string key = label_key(slot) + " at Results";
			RectangleF envelope;
			bool first = true;
			for (const std::wstring* team : { &this->menu_text_.results.contribution_team_a,
				&this->menu_text_.results.contribution_team_b })
			{
				const std::string team_key = key + " with menu_text.results." +
					(team == &this->menu_text_.results.contribution_team_a ? "contribution_team_a" : "contribution_team_b");
				const std::string where = this->source_path_ + ": " + team_key;
				const std::wstring prefix = label_words(player, slot) + *team;
				std::ignore = label_size(team_key, prefix, results.contribution_font);
				LabelPen prefix_pen;
				append_label_text(prefix_pen, contribution_font, prefix, where);
				for (const PercentageSuffix& suffix : percentages)
				{
					// Walk each fixed prefix once, then append each cached suffix in
					// Font's sequential FLOAT order. Regrouped sums would change
					// rounding for some finite bearings at an inclusive fit edge.
					LabelPen pen = prefix_pen;
					for (size_t character = 0; character < suffix.words.size(); ++character)
					{
						append_label_glyph(pen, *suffix.glyphs[character], suffix.words[character],
							contribution_font.line_spacing(), where);
					}
					// Results explicitly uses zero shadow. Include each actual badge
					// position: shorter lines can put its top above a taller line's.
					const RectangleF text(position, pen.extent);
					const RectangleF row = RectangleF::union_of(text, UiPlayerBadge::before(text, this->player_identity_));
					check_label_bounds(team_key, row, results_panel);
					envelope = first ? row : RectangleF::union_of(envelope, row);
					first = false;
				}
			}
			results_rows[static_cast<size_t>(player)][static_cast<size_t>(slot)] = envelope;
		}
	}
	check_roster_rows(results_rows, contribution_heading.bottom(), results.proceed_y,
		"Results with menu_text.results.contribution_team_a and menu_text.results.contribution_team_b");
	// An award sits at the right of a contribution row, right-aligned to the
	// score bar. The page leaves out one that would meet its own row's text,
	// so the rows above stay the only bound on how long a label may be.
	for (const auto& [award, key] : { std::pair{ MatchAward::paint_laid, "award_paint_laid" },
		std::pair{ MatchAward::recaptured, "award_recaptured" },
		std::pair{ MatchAward::airtime, "award_airtime" },
		std::pair{ MatchAward::splats, "award_splats" } })
	{
		const std::string award_key = std::string("menu_text.results.") + key;
		const Vector2F size = fixed_menu_size(award_key,
			this->menu_text_.results.award_line(award), results.contribution_font);
		for (int player = 0; player < 4; ++player)
		{
			const RectangleF bounds(Vector2F(results_fill_right - size.x,
				results.contribution_position.y +
					results.contribution_spacing * static_cast<float>(player + 1)), size);
			if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
				!results_panel.contains(bounds) || bounds.bottom() > results.proceed_y)
			{
				throw std::runtime_error(this->source_path_ + ": " + award_key +
					" must fit the Results panel at the end of every contribution row, above the prompt.");
			}
		}
	}
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		const std::string key = "menu_text.labels.pad at Pad Sticks value";
		const Vector2F raw = label_size(key, this->menu_text_.device_label(slot), controls.item_font);
		if (!std::isfinite(controls_value_limit.x) || !std::isfinite(controls_value_limit.y) ||
			controls_value_limit.x <= 0.0f || controls_value_limit.y <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " requires finite positive value limits.");
		}
		float scale = 1.0f;
		if (raw.x > controls_value_limit.x) { scale = controls_value_limit.x / raw.x; }
		if (raw.y * scale > controls_value_limit.y) { scale = controls_value_limit.y / raw.y; }
		const Vector2F fitted = raw * scale;
		const RectangleF bounds(controls.layout.position + Vector2F(controls.layout.spacing.x, 0.0f), fitted);
		if (!std::isfinite(scale) || scale <= 0.0f || !finite_stage_rectangle(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key + " must retain finite positive fitted bounds.");
		}
		check_fixed_paper(key, bounds, controls.box_size, controls.art.focus_padding);
		if (fitted.y + controls.art.focus_padding.y * 2.0f > controls.layout.spacing.y ||
			bounds.right() + controls.art.focus_padding.x > controls.controls_position.x ||
			bounds.bottom() + controls.art.focus_padding.y > controls.status_position.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its row before the controls help and status including focus padding.");
		}
	}
	for (const int best_of : { 3, 5 })
	{
		check_stage_summary("best_of_mode", this->menu_text_.stage_best_of_mode(best_of), this->menu_.detail_font, 1.0f, 2.0f);
	}
	for (int players = 1; players <= 4; ++players)
	{
		check_stage_summary(players == 1 ? "solo_roster" : "versus_roster", this->menu_text_.stage_roster(players),
			this->menu_.brand_font, 2.0f, 3.0f);
	}
	// Only glyph coverage is independent of the stage catalog. Count-aware
	// geometry is checked after the actual total and eligibility are known.
	std::ignore = fixed_menu_size("menu_text.stage.numbered_navigation",
		this->menu_text_.stage_navigation(1, 1) + L"0123456789", this->menu_.detail_font);
	std::ignore = fixed_menu_size("menu_text.stage.random_hint",
		this->menu_text_.stage_random_hint(1) + L"0123456789", this->menu_.brand_font);
	// Controls volumes occupy rows two and three; Sticks effective deadzones
	// occupy rows one and two in the same captured font/value column. Validate every
	// integer output, including saved non-step volumes and inherited 100%.
	for (int value = 0; value <= 100; ++value)
	{
		const std::wstring text = this->menu_text_.setting_percentage(value);
		for (const int row : { 1, 2, 3, 4 })
		{
			const Vector2F position = controls.layout.position +
				Vector2F(controls.layout.spacing.x, controls.layout.spacing.y * static_cast<float>(row));
			check_value("percentage", text, controls.item_font, position, controls_value_limit,
				controls.box_size, controls.art, controls.layout.spacing.y, true);
		}
	}
	// No tuning value belongs to Presentation alone. Validate each original
	// duration template and its literal glyphs here; the joint content check
	// measures actual defaults/choices, and Mode checks retained values at refresh.
	for (const auto& field : { std::pair("duration_minutes", &values.duration_minutes),
		std::pair("duration_default", &values.duration_default), std::pair("shared", &values.shared) })
	{
		const std::string where = this->source_path_ + ": menu_text.values." + field.first;
		try
		{
			const std::wstring literal = format_label_template(*field.second, { { L"{value}", L"" } });
			drawable_text::check_font_can_draw(&render, this->menu_.item_font, literal, where);
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(where + ": " + error.what());
		}
	}
	// Fullscreen sizes are positive integers, not a closed set of presets.
	// Check every generated glyph and retain the value column's complete fit
	// area: a shorter, narrower value can stay taller than a long fitted one.
	const std::string resolution_key = "menu_text.values.resolution";
	try
	{
		const std::wstring literal = format_label_template(values.resolution,
			{ { L"{width}", L"" }, { L"{height}", L"" } });
		drawable_text::check_font_can_draw(&render, this->menu_.item_font,
			literal + L"0123456789", this->source_path_ + ": " + resolution_key);
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": " + resolution_key + ": " + error.what());
	}
	const auto check_resolution = [&](const Vector2I& size)
	{
		check_value("resolution", this->menu_text_.setting_resolution(size.x, size.y), this->menu_.item_font,
			options_value_position(0), options_value_limit, main.options_box_size, options_art,
			main.options_spacing.y, false);
	};
	ResolutionManager preset;
	for (int index = 0; index <= static_cast<int>(ScreenResolution::max); ++index)
	{
		preset.set_resolution(static_cast<ScreenResolution>(index));
		check_resolution(preset.resolution_ivec());
	}
	const int largest = std::numeric_limits<int>::max();
	for (const Vector2I size : { Vector2I(1, 1), Vector2I(1600, 900),
		Vector2I(1, largest), Vector2I(largest, 1), Vector2I(largest, largest) })
	{
		check_resolution(size);
	}
	// OptionRow always resets to scale 1, then fits width and height. Each
	// actual finite positive output therefore stays within this rectangle;
	// INT_MAX is a domain endpoint, not an assertion about the widest digits.
	check_fixed_paper(resolution_key, RectangleF(options_value_position(0), options_value_limit),
		main.options_box_size, options_art.focus_padding);
	// Session selects one of these plain, unscaled labels for End's first
	// action. Retain the same paper, padding and row rule as its other actions.
	const Vector2F end_top = (Vector2F(1920.0f, 1080.0f) - end.box_size) / 2.0f;
	const Vector2F end_bottom = end_top + end.box_size;
	for (const auto& field : { std::pair("rematch", &this->menu_text_.end.rematch),
		std::pair("play_series_again", &this->menu_text_.end.play_series_again),
		std::pair("replay_draw", &this->menu_text_.end.replay_draw),
		std::pair("next_round", &this->menu_text_.end.next_round) })
	{
		const std::string key = std::string("menu_text.end.") + field.first;
		const Vector2F size = fixed_menu_size(key, *field.second, this->menu_.item_font);
		const Vector2F position = end.layout.position;
		const Vector2F padding = end.art.focus_padding;
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			position.x - padding.x < end_top.x || position.y - padding.y < end_top.y ||
			position.x + size.x + padding.x > end_bottom.x ||
			position.y + size.y + padding.y > end_bottom.y ||
			size.y + padding.y * 2.0f > end.layout.spacing.y)
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit its paper and row including focus padding.");
		}
	}
	// Results uses zero-offset transparent text shadows. Center the actual
	// unscaled text bounds on the full-height paper, as its widgets do.
	const auto check_results_centered = [&](const char* role, const std::wstring& text,
		const std::string& font, float y)
	{
		const std::string key = std::string("menu_text.results.") + role;
		const Vector2F size = fixed_menu_size(key, text, font);
		const RectangleF bounds((1920.0f - size.x) / 2.0f, y, size.x, size.y);
		if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !results_panel.contains(bounds))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Results paper.");
		}
	};
	check_results_centered("single_match_heading", this->menu_text_.results.single_match_heading,
		this->menu_.heading_font, results.heading_y);
	check_results_centered("proceed", this->menu_text_.results.proceed,
		this->menu_.detail_font, results.proceed_y);
	check_results_centered("series_team_a_wins", this->menu_text_.results.series_team_a_wins,
		this->menu_.detail_font, results.session_result_y);
	check_results_centered("series_team_b_wins", this->menu_text_.results.series_team_b_wins,
		this->menu_.detail_font, results.session_result_y);
	check_results_centered("draw_no_win", this->menu_text_.results.draw_no_win,
		this->menu_.detail_font, results.session_result_y);
	check_results_centered("series_continues", this->menu_text_.results.series_continues,
		this->menu_.detail_font, results.session_result_y);
	// Measure every reachable ledger, including single-match Sessions.
	// Session completion prevents both teams reaching the target.
	try
	{
		for (int target = 1; target <= 3; ++target)
		{
			for (int team_a_wins = 0; team_a_wins <= target; ++team_a_wins)
			{
				for (int team_b_wins = 0; team_b_wins <= target; ++team_b_wins)
				{
					if (team_a_wins == target && team_b_wins == target) { continue; }
					check_results_centered("series_score",
						this->menu_text_.results.score_line(team_a_wins, team_b_wins, target),
						this->menu_.detail_font, results.session_score_y);
				}
			}
		}
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": menu_text.results.series_score: " + error.what());
	}
	const std::string round_key = "menu_text.results.round_heading";
	std::wstring round_literals;
	try
	{
		round_literals = format_label_template(this->menu_text_.results.round_heading, { { L"{round}", L"" } });
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": " + round_key + ": " + error.what());
	}
	const std::string round_where = this->source_path_ + ": " + round_key;
	drawable_text::check_font_can_draw(&render, this->menu_.heading_font,
		round_literals + L"0123456789", round_where);
	const Font& round_font = *render.font(render.resolve_sprite_font(this->menu_.heading_font));
	const Vector2F round_size = round_heading_size(round_font, this->menu_text_.results.round_heading, round_where);
	const RectangleF round_bounds((1920.0f - round_size.x) / 2.0f, results.heading_y, round_size.x, round_size.y);
	if (!std::isfinite(round_bounds.x) || !std::isfinite(round_bounds.y) || !results_panel.contains(round_bounds))
	{
		throw std::runtime_error(round_where + " must fit unscaled inside the Results paper for every positive int round.");
	}
	check_results_centered("competitive_team_a_wins", this->menu_text_.results.competitive_team_a_wins,
		this->menu_.item_font, results.winner_y);
	check_results_centered("competitive_team_b_wins", this->menu_text_.results.competitive_team_b_wins,
		this->menu_.item_font, results.winner_y);
	check_results_centered("competitive_draw", this->menu_text_.results.competitive_draw,
		this->menu_.item_font, results.winner_y);
	// These are reference rendering outputs, not a restriction on the public
	// float arithmetic or percentage_text's spellings. Wrappers accept the
	// caller's already-rendered value without parsing or sanitizing it.
	for (const auto& field : { std::pair("solo_coverage", &this->menu_text_.results.solo_coverage),
		std::pair("team_a_percentage", &this->menu_text_.results.team_a_percentage),
		std::pair("team_b_percentage", &this->menu_text_.results.team_b_percentage) })
	{
		try
		{
			std::ignore = format_label_template(*field.second, { { L"{percentage}", L"" } });
		}
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(this->source_path_ + ": menu_text.results." + field.first + ": " + error.what());
		}
	}
	const Vector2F results_fill_position((1920.0f - results.fill_box_size.x) / 2.0f,
		results_panel.y + results.fill_box_y_offset);
	const Vector2F percentage_a_position = results_fill_position + results.team_fill_border;
	const Vector2F percentage_b_top_right = results_fill_position + Vector2F(results.fill_box_size.x, 0.0f) +
		Vector2F(-results.team_fill_border.x, results.team_fill_border.y);
	const auto check_results_percentage = [&](const char* role, const std::wstring& text, bool right_aligned)
	{
		const std::string key = std::string("menu_text.results.") + role;
		const Vector2F size = fixed_menu_size(key, text, this->menu_.detail_font);
		const Vector2F position = right_aligned ?
			Vector2F(percentage_b_top_right.x - size.x, percentage_b_top_right.y) : percentage_a_position;
		RectangleF plate(position, size);
		// MenuPaperArtwork draws its face and shade within the supplied bounds.
		// Match the widgets' single padding inflation and zero text shadow.
		plate.inflate(results.art.focus_padding);
		if (!finite_stage_rectangle(plate) || !results_panel.contains(plate))
		{
			throw std::runtime_error(this->source_path_ + ": " + key +
				" must fit unscaled inside the Results paper including its percentage plate padding.");
		}
	};
	for (int tenth = 0; tenth <= 1000; ++tenth)
	{
		const std::wstring percentage = std::to_wstring(tenth / 10) + L"." + std::to_wstring(tenth % 10) + L"%";
		check_results_centered("solo_coverage", this->menu_text_.results.solo_coverage_line(percentage),
			this->menu_.item_font, results.winner_y);
		const std::wstring team_a = this->menu_text_.results.team_a_percentage_line(percentage);
		const std::wstring team_b = this->menu_text_.results.team_b_percentage_line(percentage);
		check_results_percentage("team_a_percentage", team_a, false);
		check_results_percentage("team_b_percentage", team_b, true);
		// The monitor has its own font. Check the complete reference strings;
		// RevealArt keeps its existing runtime width/height fit and outline.
		std::ignore = fixed_menu_size("menu_text.results.team_a_percentage at monitor", team_a,
			this->opening_.percent_font);
		std::ignore = fixed_menu_size("menu_text.results.team_b_percentage at monitor", team_b,
			this->opening_.percent_font);
	}
}

std::vector<SoundReference> Presentation::sound_references() const
{
	const std::string& bank = this->menu_.sound_bank;
	const auto wave = [&](const char* key, const PresentationSound& sound,
		SoundGroupRole group = SoundGroupRole::sfx)
	{
		return SoundReference{ key, bank, sound.name, false, group };
	};
	const auto effect = [&](const char* key, const PresentationSound& sound,
		SoundGroupRole group = SoundGroupRole::sfx)
	{
		return SoundReference{ key, bank, sound.name, true, group };
	};
	return {
		wave("menu_style.sounds.direction.name", this->menu_.direction_sound),
		wave("menu_style.sounds.confirm.name", this->menu_.confirm_sound),
		wave("menu_style.sounds.cancel.name", this->menu_.cancel_sound),
		wave("main_menu.sounds.error.name", this->main_menu_.error_sound),
		wave("main_menu.sounds.ready.name", this->main_menu_.ready_sound),
		wave("main_menu.sounds.unready.name", this->main_menu_.unready_sound),
		// Auditions the pending SFX value, which the group does not carry yet.
		wave("controls_settings.sounds.volume_preview.name",
			this->controls_settings_.volume_preview_sound, SoundGroupRole::none),
		effect("main_menu.music.name", this->main_menu_.music, SoundGroupRole::music),
		wave("pause_menu.window_open_sound.name", this->pause_menu_.window_open_sound),
		wave("pause_menu.sounds.pad_lost.name", this->pause_menu_.pad_lost_sound),
		wave("pause_menu.sounds.pad_back.name", this->pause_menu_.pad_back_sound),
		effect("pause_menu.sounds.resume.name", this->pause_menu_.resume_sound),
		wave("end_menu.sounds.series.name", this->end_menu_.series_sound),
		wave("results_menu.sounds.winner.name", this->results_menu_.winner_sound),
		wave("results_menu.sounds.draw.name", this->results_menu_.draw_sound),
		effect("results_menu.sounds.fill.name", this->results_menu_.fill_sound),
		effect("results_menu.music.name", this->results_menu_.music, SoundGroupRole::music),
	};
}

void Presentation::validate_stage_counts(const RenderResources& render, size_t total, size_t eligible) const
{
	if (total == 0 || eligible > total)
	{
		throw std::runtime_error(this->source_path_ +
			": menu_text.stage requires a positive stage total and an eligible count no greater than that total.");
	}
	const auto size_of = [&](const std::string& key, const std::wstring& text, const std::string& font)
	{
		const std::string where = this->source_path_ + ": " + key;
		drawable_text::check_font_can_draw(&render, font, text, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{
			throw std::runtime_error(where + " must have finite positive measured text bounds.");
		}
		return size;
	};
	const auto finite_rectangle = [](const RectangleF& bounds)
	{
		return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
			std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
			std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
			bounds.width > 0.0f && bounds.height > 0.0f;
	};
	const auto overlaps = [](const RectangleF& first, const RectangleF& second)
	{
		return first.x < second.right() && first.right() > second.x &&
			first.y < second.bottom() && first.bottom() > second.y;
	};
	const MainMenuPresentation& main = this->main_menu_;
	const StageSelectPresentation& stage = main.stage_select;
	const Vector2F shadow = this->menu_.detail_shadow_offset;
	const float edge = main.art.keyline_width * 3.0f;
	const float inset = edge + main.setup_shadow_offset;
	const RectangleF outer((Vector2F(1920.0f, this->menu_.footer_y) - main.stage_box_size) / 2.0f,
		main.stage_box_size);
	const RectangleF inside(outer.position() + Vector2F(edge),
		outer.size() - Vector2F(edge * 2.0f + main.setup_shadow_offset));
	const RectangleF briefing(stage.briefing.position, stage.briefing.size);
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->menu_.rule_height);
	const RectangleF design(0.0f, 0.0f, 1920.0f, 1080.0f);
	const std::string navigation_key = "menu_text.stage.numbered_navigation";
	// Check every actual index: proportional digits and carried bearings can
	// make an earlier number wider than the final one. The zero-based offset
	// keeps the size_t domain from overflowing after its terminal index.
	for (size_t offset = 0; offset < total; ++offset)
	{
		const size_t index = offset + 1;
		std::wstring text;
		try { text = this->menu_text_.stage_navigation(index, total); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key + ": " + error.what());
		}
		const Vector2F size = size_of(navigation_key, text, this->menu_.detail_font);
		const RectangleF bounds = RectangleF::union_of(RectangleF(stage.navigation_position, size),
			RectangleF(stage.navigation_position + shadow, size));
		if (!std::isfinite(shadow.x) || !std::isfinite(shadow.y) || !std::isfinite(this->menu_.footer_y) ||
			!finite_rectangle(bounds) || !finite_rectangle(inside) || !finite_rectangle(preview_edge) ||
			!finite_rectangle(briefing) || !inside.contains(bounds) || !design.contains(bounds) ||
			bounds.bottom() > this->menu_.footer_y || overlaps(bounds, preview_edge) || overlaps(bounds, briefing))
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key +
				" must fit unscaled inside the Stage paper and design area before the footer without overlapping the preview edge or briefing card, including its shadow.");
		}
		const float record_room = stage.preview_position.x + stage.preview_size.x - bounds.right() -
			this->menu_.focus_padding.x;
		if (!std::isfinite(record_room) || record_room <= 0.0f)
		{
			throw std::runtime_error(this->source_path_ + ": " + navigation_key +
				" must leave finite positive room for the stage record after menu_style focus padding.");
		}
	}
	if (eligible == 0) { return; }
	const std::string hint_key = "menu_text.stage.random_hint";
	std::wstring hint;
	try { hint = this->menu_text_.stage_random_hint(eligible); }
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key + ": " + error.what());
	}
	const Vector2F size = size_of(hint_key, hint, this->menu_.brand_font);
	const RectangleF raw = RectangleF::union_of(RectangleF(Vector2F::ZERO, size), RectangleF(shadow, size));
	const float width = stage.briefing.size.x - 2.0f * stage.briefing_padding.x;
	if (!std::isfinite(width) || width <= 0.0f || !finite_rectangle(raw))
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key +
			" must have a finite positive hint width and text bounds including its shadow.");
	}
	// Match the page's existing width-only fit, including the signed shadow.
	const float scale = std::min(1.0f, width / raw.width);
	const Vector2F position = stage.briefing.position + stage.briefing_padding + stage.briefing.spacing * 7.0f;
	const Vector2F fitted = size * scale;
	const RectangleF bounds = RectangleF::union_of(RectangleF(position, fitted),
		RectangleF(position + shadow * scale, fitted));
	RectangleF padded = bounds;
	padded.inflate(inset);
	const RectangleF card = RectangleF::union_of(briefing, padded);
	if (!std::isfinite(scale) || scale <= 0.0f || !std::isfinite(edge) || !std::isfinite(inset) ||
		!finite_rectangle(bounds) || !finite_rectangle(card) || !briefing.contains(bounds) ||
		!design.contains(bounds) || bounds.bottom() > this->menu_.footer_y ||
		!inside.contains(card) || overlaps(card, preview_edge))
	{
		throw std::runtime_error(this->source_path_ + ": " + hint_key +
			" must fit its briefing panel, expanded Stage paper and design area before the footer using width-only fitting, including its scaled shadow.");
	}
}
