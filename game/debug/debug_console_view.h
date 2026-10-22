#pragma once

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include "engine/render/render_resources.h"
#include "game/debug/debug_text_field.h"
#include <array>
#include <string>
#include <vector>

class DebugDirectorImpl;
class DebugPerf;
class GameData;

namespace labrador
{
	class Keyboard;
	class Renderer;
}

// The console's editing and drawing, without the state: the frozen console
// (DebugConsoleState, pushed over the stack) owns one, and so does the
// director for the live console drawn over a running match. Debug-only.
class DebugConsoleView
{
public:
	enum class Outcome
	{
		stay,
		close,
	};

	enum class Anchor
	{
		bottom_left,
		bottom_right,
	};

	// Live: a shorter, more transparent panel over a match that keeps running,
	// and a close_console status does not close it.
	explicit DebugConsoleView(bool live);

	void init(const GameData& data);

	// One frame of typing. Close on grave (after the first frame) or on the
	// Escape release; Enter runs the line through the director.
	Outcome update(const labrador::Keyboard& keyboard, float dt, DebugDirectorImpl& director);

	void draw(labrador::Renderer& renderer, const GameData& data,
		const DebugDirectorImpl& director) const;

	bool live() const { return this->live_; }

	// Where a panel of `size` (its text) goes in a bottom corner of a
	// `screen` window: the padded backing rectangle, lifted above every
	// rectangle in `avoid` it would overlap (up to four times), and never above
	// the top margin. Pure.
	static mattmath::RectangleF place_panel(const mattmath::Vector2F& size, Anchor anchor,
		const mattmath::Vector2F& screen, const std::vector<mattmath::RectangleF>& avoid);

	// The panels' resources by handle: the overlay font, the sheet and its
	// "pixel" frame. Each is a search by name, so a caller drawing every frame
	// resolves them once, outside the draw, and passes them in.
	struct OverlayHandles
	{
		labrador::RenderResources::SpriteSheetHandle sheet;
		labrador::SpriteSheet::frame_handle pixel;
		labrador::FontHandle font;

		static OverlayHandles resolve(const GameData& data);
	};

	// A small panel of overlay-font lines in a bottom corner of the window,
	// clear of every pane's HUD (top) and of `avoid` (the opening cards, the
	// panels drawn before it): the non-default strip (x0.25, PAUSED, GOD ...)
	// and the paint coverage. Draws nothing for no lines. Returns the panel's
	// rectangle, empty when nothing was drawn. Without `handles`, resolves them.
	static mattmath::RectangleF draw_panel(labrador::Renderer& renderer, const GameData& data,
		const std::vector<std::string>& lines, Anchor anchor,
		const std::vector<mattmath::RectangleF>& avoid = {});
	static mattmath::RectangleF draw_panel(labrador::Renderer& renderer, const GameData& data,
		const OverlayHandles& handles, const std::vector<std::string>& lines, Anchor anchor,
		const std::vector<mattmath::RectangleF>& avoid);

	// The perf overlay, bottom left: a bar graph of the last frames' frame
	// time over its min / avg / max lines.
	static mattmath::RectangleF draw_perf(labrador::Renderer& renderer, const GameData& data,
		const DebugPerf& perf, const std::vector<mattmath::RectangleF>& avoid);
	static mattmath::RectangleF draw_perf(labrador::Renderer& renderer, const GameData& data,
		const OverlayHandles& handles, const DebugPerf& perf,
		const std::vector<mattmath::RectangleF>& avoid);

private:
	bool live_ = false;

	// Text typed on the frame the console opened belongs to the toggle key
	// (on any layout), so the first update reads no text.
	bool first_update_ = true;
	float caret_time_ = 0.0f;

	enum RepeatKey { repeat_left, repeat_right, repeat_backspace, repeat_delete,
		repeat_up, repeat_down, repeat_count };
	std::array<DebugKeyRepeat, repeat_count> repeats_{};

	labrador::RenderResources::SpriteSheetHandle sheet_;
	labrador::SpriteSheet::frame_handle pixel_;
	labrador::FontHandle font_;
	float char_width_ = 8.0f;
};
