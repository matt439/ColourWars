#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/drawable_text.h"
#include "game/menu_geometry.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace labrador;
using namespace mattmath;
using namespace presentation_read;
using namespace presentation_validation;

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
	const RectangleF inside = menu_geometry::paper_interior(
		menu_geometry::footer_paper(main.stage_box_size, this->menu_.footer_y), menu_geometry::setup_art(main));
	const RectangleF briefing(stage.briefing.position, stage.briefing.size);
	RectangleF preview_edge(stage.preview_position, stage.preview_size);
	preview_edge.inflate(this->menu_.rule_height);
	const RectangleF design = menu_geometry::design_area();
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
