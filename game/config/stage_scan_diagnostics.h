#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "engine/math/rectanglef.h"

namespace labrador { class Font; }
struct StageScanPresentation;
struct MenuPaperArtPresentation;
struct StageScanMenuText;

// User-stage errors are external diagnostics, rather than authored menu copy.
// Keep their content readable with the game's printable-ASCII font contract.
namespace stage_scan_diagnostics
{
	struct Layout
	{
		mattmath::RectangleF paper, heading, unicode_hint, body, progress;
	};
	Layout layout(const StageScanPresentation& look, const MenuPaperArtPresentation& art,
		float content_height, const labrador::Font& font, const StageScanMenuText& copy);
	// Printable ASCII stays verbatim. Other UTF-16 units are shown as \uXXXX;
	// malformed UTF-8 bytes use \xXX, so a refused filename is not lost to '?'.
	std::wstring display_text(std::string_view diagnostic);

	// Wrap even a single long filename/token and page by actual font extents.
	// The caller displays one returned page, with a bounded widget count.
	// Joining pages/lines without their inserted linefeeds recovers display_text.
	std::vector<std::wstring> pages(const labrador::Font& font,
		std::string_view diagnostic, float width, float height);
}
