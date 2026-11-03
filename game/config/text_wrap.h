#pragma once

#include "engine/render/renderer.h"

#include <string>
#include <string_view>

namespace labrador
{
	class RenderResources;
}

// The wrap itself is the engine's: RenderResources::wrap_text (Font::wrap, engine
// v0.3.0) turns a space into a line break wherever the line would pass the
// width, keeps authored line breaks, and lets a word wider than the width by
// itself through whole. What stays here is the load-time half the engine does
// not do: refusing such a word with a diagnostic that names the content key.
namespace text_wrap
{
	// Throws std::runtime_error naming `where` and the word when a single word of
	// `text` is wider than max_width at `scale`, which no wrap can fix. Call while
	// content loads, beside the font check, so the diagnostic names the key.
	void check_fits(const labrador::RenderResources& render,
		labrador::FontHandle font,
		std::wstring_view text,
		float max_width,
		float scale,
		const std::string& where);
}
