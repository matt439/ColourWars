#include "game/pch.h"
#include "game/presentation.h"
#include "game/stage_scan_diagnostics.h"
#include "engine/render/font.h"
#include "engine/render/render_resources.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

void Presentation::validate_stage_scan_resources(const labrador::RenderResources& render) const
{
	try
	{
		const StageScanPresentation& look = this->main_menu_.stage_scan;
		const StageScanMenuText& copy = this->menu_text_.stage_scan;
		const labrador::Font& font = *render.font(render.resolve_sprite_font(look.font));
		// External filenames/reasons can need every printable ASCII glyph after escaping.
		std::wstring alphabet;
		for (wchar_t character = L' '; character <= L'~'; ++character) { alphabet.push_back(character); }
		if (font.first_unrenderable(alphabet) != std::wstring::npos)
		{ throw std::invalid_argument("font must contain every printable ASCII diagnostic character."); }
		const auto layout = stage_scan_diagnostics::layout(look, this->main_menu_.art,
			this->menu_.footer_y, font, copy);
		for (const std::wstring* text : { &copy.heading, &copy.unicode_hint, &copy.browse_prompt,
			&copy.details_prompt, &copy.no_refusals, &copy.status, &copy.counter })
		{
			if (text->find_first_not_of(L' ') == std::wstring::npos ||
				text->find_first_of(L"\r\n") != std::wstring::npos ||
				text->find_first_not_of(alphabet) != std::wstring::npos)
			{ throw std::invalid_argument("copy must be nonblank single-line printable ASCII."); }
		}
		std::ignore = copy.summary(1);
		std::ignore = copy.progress(1, 1, 1, 1);
		const mattmath::Vector2F empty_size = font.measure(copy.no_refusals) * look.text_scale;
		if (empty_size.x > layout.body.width || empty_size.y > layout.body.height)
		{ throw std::invalid_argument("no_refusals must fit the diagnostic body."); }
		for (const wchar_t character : alphabet)
		{
			std::ignore = stage_scan_diagnostics::pages(font, std::string(1, static_cast<char>(character)),
				layout.body.width / look.text_scale, layout.body.height / look.text_scale);
		}
		const mattmath::RectangleF status(look.status.position + mattmath::Vector2F(0.0f, this->menu_.footer_y),
			look.status.size);
		const auto prompt_fits = [&](const std::wstring& text)
		{
			const mattmath::Vector2F size = font.measure(text) * look.text_scale;
			const mattmath::Vector2F shadow = look.prompt_shadow_offset * look.text_scale;
			const mattmath::Vector2F origin = look.prompt_position + mattmath::Vector2F(0.0f, this->menu_.footer_y);
			return origin.x + std::min(0.0f, shadow.x) >= 0.0f &&
				origin.y + std::min(0.0f, shadow.y) >= status.bottom() &&
				origin.x + size.x + std::max(0.0f, shadow.x) <= 1920.0f &&
				origin.y + size.y + std::max(0.0f, shadow.y) <= 1080.0f;
		};
		if (!std::isfinite(status.x) || !std::isfinite(status.y) || !std::isfinite(status.width) ||
			!std::isfinite(status.height) || status.x < 0 || status.width <= 0 || status.height <= 0 ||
			status.right() > 1920.0f || status.y < this->menu_.footer_y + this->menu_.rule_height ||
			status.bottom() > this->menu_.footer_y + look.prompt_position.y ||
			!std::isfinite(look.status_scale) || look.status_scale <= 0 ||
			font.measure(copy.summary(1)).y * look.status_scale > status.height ||
			!prompt_fits(copy.browse_prompt) || !prompt_fits(copy.details_prompt))
		{ throw std::invalid_argument("status and input legends must fit below the chrome without overlapping."); }
	}
	catch (const std::exception& error)
	{ throw std::runtime_error(this->source_path_ + ": main_menu.stage_scan: " + error.what()); }
}
