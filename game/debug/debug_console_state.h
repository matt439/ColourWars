#pragma once

#include "engine/core/state.h"
#include "engine/math/rectanglef.h"
#include "engine/render/render_resources.h"
#include "game/debug/debug_text_field.h"
#include <array>
#include <string>
#include <vector>

class DebugDirectorImpl;
class GameData;

// The developer console: a non-covering state pushed above whatever is on top
// (a menu page or the match). Only the top state updates, so while it is open
// the match is frozen and no game reader sees the keyboard. It draws last, over
// every pane, in window pixels.
class DebugConsoleState final : public labrador::State
{
public:
	DebugConsoleState(GameData* data, DebugDirectorImpl* director);
	~DebugConsoleState() override;

	void init() override;
	void update(float dt) override;
	void draw(labrador::Renderer& renderer) const override;
	bool covers_screen() const override { return false; }

	enum class Anchor
	{
		bottom_left,
		bottom_right,
	};

	// A small panel of overlay-font lines in a bottom corner of the window,
	// clear of every pane's HUD (top) and the opening cards (bottom centre):
	// the non-default strip (x0.25, PAUSED, GOD ...) and the paint coverage.
	// Draws nothing for no lines. A panel that would overlap `avoid` (the
	// rectangle an earlier call returned) is lifted to sit above it instead.
	// Returns the panel's rectangle, empty when nothing was drawn.
	static mattmath::RectangleF draw_panel(labrador::Renderer& renderer, const GameData& data,
		const std::vector<std::string>& lines, Anchor anchor,
		const mattmath::RectangleF& avoid = mattmath::RectangleF());

private:
	GameData* data_ = nullptr;
	DebugDirectorImpl* director_ = nullptr;

	// Text typed on the frame the console opened belongs to the toggle key
	// (on any layout), so the first update reads no text.
	bool first_update_ = true;
	bool closing_ = false;
	float caret_time_ = 0.0f;

	enum RepeatKey { repeat_left, repeat_right, repeat_backspace, repeat_delete,
		repeat_up, repeat_down, repeat_count };
	std::array<DebugKeyRepeat, repeat_count> repeats_{};

	labrador::RenderResources::SpriteSheetHandle sheet_;
	labrador::SpriteSheet::frame_handle pixel_;
	labrador::FontHandle font_;
	float char_width_ = 8.0f;

	void close();
};
