#pragma once

#include "engine/render/renderer.h"

#include <string>
#include <string_view>

namespace labrador
{
	class RenderResources;
}

// Greedy word wrap against the font that will draw the text, for a client that
// opts in with a width. Measurement is the engine's own measure_text, so a
// wrapped line is exactly as wide as the widget that draws it reports.
//
// Authored line breaks are kept: wrapping only ever adds a break, at a space,
// and never joins lines or splits a word. So text whose every authored line
// already fits comes back unchanged, which is how a width can be added to an
// existing layout without moving a pixel.
namespace text_wrap
{
	// `text` with a space turned into a line break wherever the next word would
	// take the line past max_width at `scale`. The space at a break is dropped;
	// every other space, leading ones included, is kept as authored. A word wider
	// than max_width on its own stays whole on its own line - check_fits is the
	// load-time refusal for that, so a widget wrapping at draw time never throws.
	std::wstring wrap(const labrador::RenderResources& render,
		labrador::FontHandle font,
		std::wstring_view text,
		float max_width,
		float scale = 1.0f);

	// Throws std::runtime_error naming `where` and the word when a single word of
	// `text` is wider than max_width at `scale`, which wrap cannot fix. Call while
	// content loads, beside the font check, so the diagnostic names the key.
	void check_fits(const labrador::RenderResources& render,
		labrador::FontHandle font,
		std::wstring_view text,
		float max_width,
		float scale,
		const std::string& where);
}
