#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/menu_geometry.h"

#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;
using namespace presentation_validation;
using menu_consts::DESIGN_RESOLUTION;

void Presentation::Validation::check_menu_line(const std::string& key, const std::wstring& text,
	const std::string& font, const Vector2F& position, float bottom, const Vector2F& shadow) const
{
	const std::string where = this->presentation_.source_path_ + ": " + key;
	drawable_text::check_font_can_draw(&render, font, text, where);
	const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
	const float width = DESIGN_RESOLUTION.x - position.x - this->presentation_.menu_.brand_position.x;
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
}

Vector2F Presentation::Validation::check_menu_item(MenuItem item, const std::string& font, const Vector2F& position,
	const RectangleF& paper, float row_spacing, const std::optional<float>& label_width,
	const MenuPaperArtPresentation& art, bool uniform_fit) const
{
	const std::string where = this->presentation_.source_path_ + ": menu_text.items." +
		menu_item_keys[static_cast<size_t>(item)];
	const std::wstring& text = this->presentation_.menu_text_.item(item);
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
	const Vector2F top = paper.position();
	const Vector2F bottom = top + paper.size() - Vector2F(art.shadow_offset);
	if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
		position.x - padding.x < top.x || position.y - padding.y < top.y ||
		position.x + size.x + padding.x > bottom.x ||
		position.y + size.y + padding.y > bottom.y ||
		(row_spacing > 0.0f && size.y + padding.y * 2.0f > row_spacing))
	{
		throw std::runtime_error(where + " must fit its paper and row including focus padding.");
	}
	return size;
}

RectangleF Presentation::Validation::check_setup_paper(const RectangleF& setup_inside, const std::string& key, const std::wstring& words,
	const std::string& font, const Vector2F& position, const Vector2F& shadow,
	bool action, float row_spacing) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
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
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
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
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit main_menu.setup_box_size including its retained shadow bounds and paper ornaments.");
	}
	return bounds;
}

Vector2F Presentation::Validation::fixed_menu_size(const std::string& key, const std::wstring& text, const std::string& font) const
{
	const std::string where = this->presentation_.source_path_ + ": " + key;
	drawable_text::check_font_can_draw(&render, font, text, where);
	const Vector2F size = render.measure_text(render.resolve_sprite_font(font), text);
	if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
	{
		throw std::runtime_error(where + " must have finite positive measured text bounds.");
	}
	return size;
}

void Presentation::Validation::check_fixed_paper(const std::string& key, RectangleF bounds,
	const Vector2F& paper_size, const Vector2F& padding) const
{
	bounds.inflate(padding);
	const RectangleF paper = menu_geometry::footer_paper(paper_size, this->presentation_.menu_.footer_y);
	if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y) || !paper.contains(bounds))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit its menu paper including any focus padding.");
	}
}

void Presentation::Validation::check_value(const char* role, const std::wstring& text, const std::string& font,
	const Vector2F& position, const Vector2F& limit, const Vector2F& paper_size,
	const MenuPaperArtPresentation& art, float row_spacing, bool controls_value) const
{
	const ControlsSettingsPresentation& controls = this->presentation_.controls_settings_;
	const std::string key = std::string("menu_text.values.") + role;
	const Vector2F raw = fixed_menu_size(key, text, font);
	if (!std::isfinite(limit.x) || !std::isfinite(limit.y) || limit.x <= 0.0f || limit.y <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
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
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must retain finite positive fitted value bounds.");
	}
	const RectangleF bounds(position, fitted);
	check_fixed_paper(key, bounds, paper_size, art.focus_padding);
	if (fitted.y + art.focus_padding.y * 2.0f > row_spacing)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit its row including focus padding.");
	}
	if (controls_value && (bounds.right() + art.focus_padding.x > controls.controls_position.x ||
		bounds.bottom() + art.focus_padding.y > controls.status_position.y))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit before the controls help and status including focus padding.");
	}
}

RectangleF Presentation::Validation::check_stage_briefing(const RectangleF& stage_briefing, const Vector2F& stage_info, const char* role, const std::wstring& text, const std::string& font,
	float line, float next_line) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const StageSelectPresentation& stage = main.stage_select;
	const std::string key = std::string("menu_text.stage.") + role;
	const Vector2F size = fixed_menu_size(key, text, font);
	const Vector2F position = stage_info + stage.briefing.spacing * line;
	const Vector2F shadow = this->presentation_.menu_.detail_shadow_offset;
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
		!menu_geometry::design_area().contains(bounds) || bounds.bottom() > next_top)
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit unscaled inside the design area and briefing vertical span before the next authored row origin, including its shadow.");
	}
	return bounds;
}

RectangleF Presentation::Validation::check_stage_summary(const RectangleF& stage_briefing, const Vector2F& stage_info, const char* role, const std::wstring& text, const std::string& font,
	float line, float next_line) const
{
	const RectangleF bounds = check_stage_briefing(stage_briefing, stage_info, role, text, font, line, next_line);
	check_briefing_card(stage_briefing, std::string("menu_text.stage.") + role, bounds);
	return bounds;
}

void Presentation::Validation::check_briefing_card(const RectangleF& stage_briefing, const std::string& key, RectangleF bounds) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	const StageSelectPresentation& stage = main.stage_select;
	// The page expands the briefing artwork around unscaled copy rather
	// than fitting copy to its authored width. That expanded card must
	// still fit the outer paper and remain separate from the preview.
	const float edge = main.art.keyline_width * 3.0f;
	const float inset = edge + main.setup_shadow_offset;
	bounds.inflate(inset);
	const RectangleF card = RectangleF::union_of(stage_briefing, bounds);
	const RectangleF inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.stage_box_size, this->presentation_.menu_.footer_y),
		menu_geometry::setup_art(main));
	RectangleF preview(stage.preview_position, stage.preview_size);
	preview.inflate(this->presentation_.menu_.rule_height);
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
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must keep the expanded briefing card inside the Stage paper and separate from the preview edge, including its retained artwork and shadow.");
	}
}

bool Presentation::Validation::finite_stage_rectangle(const RectangleF& bounds)
{
	return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
		std::isfinite(bounds.width) && std::isfinite(bounds.height) &&
		std::isfinite(bounds.right()) && std::isfinite(bounds.bottom()) &&
		bounds.width > 0.0f && bounds.height > 0.0f;
}

Vector2F Presentation::Validation::options_value_position(int row) const
{
	const MainMenuPresentation& main = this->presentation_.main_menu_;
	return Vector2F(main.options_value_x, main.options_position.y + main.options_spacing.y * static_cast<float>(row));
}

std::wstring Presentation::Validation::label_words(int player, int choice) const
{
	return choice < input_slots::count ? this->presentation_.menu_text_.roster_label(player, choice) :
		this->presentation_.menu_text_.bot_roster_label(player, label_difficulties[static_cast<size_t>(choice - input_slots::count)]);
}

std::string Presentation::Validation::label_key(int slot)
{
	if (slot >= input_slots::count)
	{
		constexpr std::array<const char*, 3> roles{ "bot_easy", "bot_normal", "bot_hard" };
		return std::string("menu_text.labels.roster_row with menu_text.labels.bot and menu_text.labels.") +
			roles[static_cast<size_t>(slot - input_slots::count)];
	}
	return std::string("menu_text.labels.roster_row with menu_text.labels.") +
		(input_slots::is_pad(slot) ? "pad" : input_slots::is_keyboard_and_mouse(slot) ? "keyboard_mouse" : "waiting");
}

Vector2F Presentation::Validation::label_size(const std::string& key, const std::wstring& words, const std::string& font_name) const
{
	const std::string where = this->presentation_.source_path_ + ": " + key;
	drawable_text::check_font_can_draw(&render, font_name, words, where);
	const Font& font = *render.font(render.resolve_sprite_font(font_name));
	LabelPen pen;
	append_label_text(pen, font, words, where);
	if (pen.extent.x <= 0.0f || pen.extent.y <= 0.0f)
	{
		throw std::runtime_error(where + " must retain finite positive composed label bounds.");
	}
	return pen.extent;
}

void Presentation::Validation::check_label_bounds(const std::string& key, const RectangleF& bounds, const RectangleF& inside) const
{
	if (!finite_stage_rectangle(bounds) || !inside.contains(bounds))
	{
		throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
			" must fit its paper including its actual text, shadow, badge and row bounds.");
	}
}

RectangleF Presentation::Validation::shadow_bounds(const Vector2F& position, const Vector2F& size, const Vector2F& offset)
{
	return RectangleF::union_of(RectangleF(position, size), RectangleF(position + offset, size));
}

void Presentation::Validation::check_roster_rows(const LabelRows& rows, float heading_bottom, float bottom,
	const char* site) const
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
				throw std::runtime_error(this->presentation_.source_path_ + ": " + label_key(slot) + " at " + site +
					" must follow the heading and complete preceding rows without overlap, before the following content.");
			}
			if (player < 3) { self(self, player + 1, used | bit, bounds.bottom()); }
		}
	};
	visit(visit, 0, 0u, heading_bottom);
}
