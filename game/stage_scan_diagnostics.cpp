#include "game/pch.h"
#include "game/stage_scan_diagnostics.h"
#include "engine/render/font.h"
#include "game/presentation_menu.h"
#include "game/presentation_menu_text.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	void append_escape(std::wstring& output, unsigned int value, int digits, wchar_t kind)
	{
		constexpr wchar_t hex[] = L"0123456789ABCDEF";
		output.push_back(L'\\');
		output.push_back(kind);
		for (int digit = digits - 1; digit >= 0; --digit)
		{
			output.push_back(hex[(value >> (digit * 4)) & 0xfu]);
		}
	}

	std::vector<std::wstring> wrap(const labrador::Font& font,
		const std::wstring& text, float width)
	{
		std::vector<std::wstring> lines;
		std::wstring line;
		for (const wchar_t character : text)
		{
			if (font.measure(std::wstring(1, character)).x > width)
			{
				throw std::invalid_argument("User-stage diagnostic width cannot hold one font glyph.");
			}
			line.push_back(character);
			while (font.measure(line).x > width)
			{
				if (line.size() == 1)
				{
					throw std::invalid_argument("User-stage diagnostic width cannot hold one font glyph.");
				}
				// Prefer a word boundary, retaining the space on the previous line.
				// A proportional-font suffix may still be too wide after that split.
				const size_t space = line.find_last_of(L' ', line.size() - 2);
				const size_t split = space != std::wstring::npos && space > 0 ? space + 1 : line.size() - 1;
				lines.push_back(line.substr(0, split));
				line.erase(0, split);
			}
		}
		if (!line.empty() || lines.empty()) { lines.push_back(std::move(line)); }
		return lines;
	}
}

stage_scan_diagnostics::Layout stage_scan_diagnostics::layout(const StageScanPresentation& look,
	const MenuPaperArtPresentation& art, float content_height, const labrador::Font& font,
	const StageScanMenuText& copy)
{
	using namespace mattmath;
	const auto positive = [](float value) { return std::isfinite(value) && value > 0.0f; };
	if (!positive(look.heading_scale) || !positive(look.text_scale) || !positive(look.gap) ||
		!positive(look.padding.x) || !positive(look.padding.y) || !positive(look.box_size.x) ||
		!positive(look.box_size.y) || look.box_size.x > 1880.0f || look.box_size.y > content_height - 40.0f)
	{ throw std::invalid_argument("stage_scan needs positive scales/geometry and a paper inside the menu chrome."); }
	Layout result;
	result.paper = RectangleF((Vector2F(1920.0f, content_height) - look.box_size) / 2.0f, look.box_size);
	const float edge = art.keyline_width * 3.0f;
	const Vector2F padding = look.padding + Vector2F(edge);
	const Vector2F top = result.paper.position() + padding;
	const Vector2F bottom = result.paper.bottom_right() - padding - Vector2F(art.shadow_offset);
	const float width = bottom.x - top.x;
	std::wstring alphabet;
	for (wchar_t character = L' '; character <= L'~'; ++character) { alphabet.push_back(character); }
	const float line_height = font.measure(alphabet).y * look.text_scale;
	result.heading = RectangleF(top, Vector2F(width, font.measure(copy.heading).y * look.heading_scale));
	result.unicode_hint = RectangleF(Vector2F(top.x, result.heading.bottom() + look.gap), Vector2F(width, line_height));
	result.progress = RectangleF(Vector2F(top.x, bottom.y - line_height), Vector2F(width, line_height));
	result.body = RectangleF(Vector2F(top.x, result.unicode_hint.bottom() + look.gap),
		Vector2F(width, result.progress.y - look.gap - result.unicode_hint.bottom() - look.gap));
	if (!positive(width) || result.body.height < line_height ||
		font.measure(copy.heading).x * look.heading_scale > width ||
		font.measure(copy.unicode_hint).x * look.text_scale > width)
	{ throw std::invalid_argument("stage_scan paper must fit its heading, Unicode hint and at least one diagnostic line."); }
	return result;
}

std::wstring stage_scan_diagnostics::display_text(std::string_view diagnostic)
{
	if (diagnostic.empty()) { return {}; }
	int units = 0;
	if (diagnostic.size() <= static_cast<size_t>((std::numeric_limits<int>::max)()))
	{
		units = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, diagnostic.data(),
			static_cast<int>(diagnostic.size()), nullptr, 0);
	}
	std::wstring output;
	if (units == 0)
	{
		for (const char character : diagnostic)
		{
			const unsigned char byte = static_cast<unsigned char>(character);
			if (byte >= 0x20 && byte <= 0x7e) { output.push_back(static_cast<wchar_t>(byte)); }
			else { append_escape(output, byte, 2, L'x'); }
		}
		return output;
	}
	std::wstring decoded(static_cast<size_t>(units), L'\0');
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, diagnostic.data(),
		static_cast<int>(diagnostic.size()), decoded.data(), units) != units)
	{
		throw std::runtime_error("Could not decode the retained user-stage diagnostic.");
	}
	for (const wchar_t character : decoded)
	{
		if (character >= L' ' && character <= L'~') { output.push_back(character); }
		else { append_escape(output, static_cast<unsigned int>(character), 4, L'u'); }
	}
	return output;
}

std::vector<std::wstring> stage_scan_diagnostics::pages(const labrador::Font& font,
	std::string_view diagnostic, float width, float height)
{
	if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
	{
		throw std::invalid_argument("User-stage diagnostic area must be finite and positive.");
	}
	std::vector<std::wstring> result;
	std::wstring page;
	for (const std::wstring& line : wrap(font, display_text(diagnostic), width))
	{
		if (font.measure(line).y > height)
		{
			throw std::invalid_argument("User-stage diagnostic height cannot hold one font line.");
		}
		std::wstring candidate = page;
		if (!page.empty()) { candidate.push_back(L'\n'); }
		candidate += line;
		if (!page.empty() && font.measure(candidate).y > height)
		{
			result.push_back(std::move(page));
			page = line;
		}
		else { page = std::move(candidate); }
	}
	if (!page.empty() || result.empty()) { result.push_back(std::move(page)); }
	return result;
}
