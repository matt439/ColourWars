#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_console_view.h"
#include "engine/input/keyboard.h"
#include "engine/render/camera.h"
#include "engine/render/renderer.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_perf.h"
#include "game/game_data.h"

using namespace labrador;
using namespace mattmath;

// Developer-only styling; presentation.json ships, this console does not.
namespace debug_console_consts
{
	constexpr float panel_fraction = 0.45f;
	// The live console leaves most of the match visible below it; its own
	// rows cover the HUD cards and the clock, so it is at least as opaque as the
	// frozen one (at 0.55 that text read through the log).
	constexpr float live_panel_fraction = 0.30f;
	constexpr float margin = 10.0f;
	constexpr float accent_height = 2.0f;
	const Colour panel_colour(0.02f, 0.03f, 0.05f, 0.80f);
	const Colour live_panel_colour(0.02f, 0.03f, 0.05f, 0.85f);
	const Colour echo_colour(1.0f, 1.0f, 1.0f, 1.0f);
	const Colour warning_colour(1.0f, 0.85f, 0.35f, 1.0f);
	const Colour error_colour(1.0f, 0.45f, 0.40f, 1.0f);
	const Colour strip_colour(1.0f, 0.85f, 0.35f, 1.0f);

	// The perf graph: one bar per frame, scaled to at least two 60 Hz frames.
	constexpr std::size_t graph_frames = 120;
	constexpr float graph_bar_width = 2.0f;
	constexpr float graph_height = 40.0f;
	constexpr double graph_floor_ms = 33.3;
	constexpr double reference_ms = 1000.0 / 60.0;
	const Colour graph_good(0.40f, 0.85f, 0.45f, 0.95f);
	const Colour graph_slow(1.0f, 0.80f, 0.30f, 0.95f);
	const Colour graph_bad(1.0f, 0.40f, 0.35f, 0.95f);
	const Colour graph_reference(1.0f, 1.0f, 1.0f, 0.85f);
	const Colour graph_backdrop(0.10f, 0.13f, 0.17f, 0.95f);

	// Text drawn by a panel: inside its backing by these paddings.
	constexpr float pad_x = 6.0f;
	constexpr float pad_y = 3.0f;
}

using namespace debug_console_consts;

namespace
{
	DrawList screen_list(Renderer& renderer, const GameData& data, Viewport& viewport)
	{
		if (renderer.view_count() == 0)
		{
			renderer.set_view_count(1);
		}
		DrawList list = renderer.view(renderer.view_count() - 1);
		viewport = data.viewport_manager()->fullscreen_viewport();
		list.set_viewport(viewport);
		list.set_camera(Camera::frame(
			RectangleF(0.0f, 0.0f, viewport.width, viewport.height), viewport));
		return list;
	}
}

DebugConsoleView::DebugConsoleView(bool live) : live_(live)
{
}

void DebugConsoleView::init(const GameData& data)
{
	RenderResources* resources = data.render_resources();
	this->sheet_ = resources->resolve_sprite_sheet("sprite_sheet_1");
	this->pixel_ = resources->sprite_sheet(this->sheet_)->resolve_sprite_frame("pixel");
	this->font_ = resources->resolve_sprite_font(data.presentation()->debug_overlay().font);
	// The overlay font is monospaced; one glyph's pen advance places the caret
	// and wraps every line. measure_text reports ink extent (x_offset + width
	// of the last glyph), so the advance is the difference of two runs.
	const float advance = resources->measure_text(this->font_, L"MM").x -
		resources->measure_text(this->font_, L"M").x;
	this->char_width_ = std::max(1.0f, advance * data.presentation()->debug_overlay().font_scale);
}

DebugConsoleView::Outcome DebugConsoleView::update(const Keyboard& keyboard, float dt,
	DebugDirectorImpl& director)
{
	director.forget_menu_key();

	// The grave key toggles. Its own character is dropped by returning before
	// typed() is read, and while it is held or released all text is dropped.
	if (!this->first_update_ && keyboard.pressed(Key::grave))
	{
		return Outcome::close;
	}
	// On release, so the Escape edge is over before the match reads its
	// released(escape) pause binding (the live console also blanks that frame).
	if (keyboard.released(Key::escape))
	{
		return Outcome::close;
	}

	DebugKeyEvents events;
	if (!this->first_update_)
	{
		events.typed = std::string(keyboard.typed());
	}
	this->first_update_ = false;

	const auto repeat = [this, &keyboard, dt](RepeatKey slot, Key key)
		{
			return this->repeats_[slot].fire(keyboard.held(key), keyboard.pressed(key), dt);
		};
	events.left = repeat(repeat_left, Key::left);
	events.right = repeat(repeat_right, Key::right);
	events.backspace = repeat(repeat_backspace, Key::backspace);
	events.del = repeat(repeat_delete, Key::del);
	events.up = repeat(repeat_up, Key::up);
	events.down = repeat(repeat_down, Key::down);
	events.home = keyboard.pressed(Key::home);
	events.end = keyboard.pressed(Key::end);
	events.clear_line = keyboard.held(Key::control) && keyboard.pressed(Key::u);
	// Ctrl+letter is a shortcut, never text. AltGr arrives as Ctrl+Alt and
	// is how many layouts type \ @ [ ] { } | ~, so Ctrl with Alt still types.
	// A held toggle key auto-repeats its own character, and the last repeat
	// can arrive on the frame the key is released: drop text on both.
	if ((keyboard.held(Key::control) && !keyboard.held(Key::alt)) ||
		keyboard.held(Key::grave) || keyboard.released(Key::grave))
	{
		events.typed.clear();
	}

	DebugTextField& field = director.field();
	field.apply(events);

	if (keyboard.pressed(Key::tab))
	{
		const DebugCompletion completion = director.registry().complete(field.text());
		field.set_text(completion.line);
		if (!completion.candidates.empty())
		{
			std::string line;
			const std::size_t shown = std::min<std::size_t>(completion.candidates.size(), 24);
			for (std::size_t i = 0; i < shown; ++i)
			{
				line += (i == 0 ? "" : "  ") + completion.candidates[i];
			}
			if (completion.candidates.size() > shown)
			{
				line += "  ... " + std::to_string(completion.candidates.size() - shown) + " more";
			}
			director.log().write(DebugSeverity::echo, line);
		}
	}

	if (keyboard.pressed(Key::enter))
	{
		const DebugStatus status = director.submit_console_line(field.submit());
		// A live console stays open: the match is running, so what a command
		// queued applies without it closing.
		if (status.close_console && !this->live_)
		{
			return Outcome::close;
		}
	}

	this->caret_time_ += dt;
	return Outcome::stay;
}

void DebugConsoleView::draw(Renderer& renderer, const GameData& data,
	const DebugDirectorImpl& director) const
{
	Viewport viewport;
	DrawList list = screen_list(renderer, data, viewport);

	const SpriteSheet* sheet = data.render_resources()->sprite_sheet(this->sheet_);
	const DebugOverlayPresentation& look = data.presentation()->debug_overlay();
	const float line_height = std::max(look.line_spacing, 1.0f);
	const float panel_height = std::floor(viewport.height *
		(this->live_ ? live_panel_fraction : panel_fraction));

	list.set_filter(TextureFilter::point);
	sheet->draw(list, this->pixel_, RectangleF(0.0f, 0.0f, viewport.width, panel_height),
		this->live_ ? live_panel_colour : panel_colour);
	sheet->draw(list, this->pixel_, RectangleF(0.0f, panel_height - accent_height,
		viewport.width, accent_height), data.presentation()->menu().highlight);

	list.set_filter(TextureFilter::linear);
	const auto text = [&list, &look, this](const std::string& line, float x, float y,
		const Colour& colour)
		{
			const std::wstring wide = widen(line);
			list.draw_text(this->font_, wide, Vector2F(x, y) + look.shadow_offset,
				look.shadow_colour, look.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
			list.draw_text(this->font_, wide, Vector2F(x, y), colour,
				look.font_scale, 0.0f, Vector2F::ZERO, 0.0f);
		};

	const std::size_t columns = static_cast<std::size_t>(std::max(8.0f,
		(viewport.width - 2.0f * margin) / this->char_width_));

	// Input line, bottom of the panel, scrolled so the caret stays visible.
	const DebugTextField& field = director.field();
	float y = panel_height - accent_height - margin - line_height;
	{
		const std::string prompt = this->live_ ? "live] " : "] ";
		const std::size_t room = columns > prompt.size() + 1 ? columns - prompt.size() - 1 : 1;
		const std::size_t first = field.caret() > room ? field.caret() - room : 0;
		const std::string visible = field.text().substr(first, room);
		text(prompt + visible, margin, y, look.colour);
		if (std::fmod(this->caret_time_, 1.0f) < 0.5f)
		{
			const float caret_x = margin + this->char_width_ *
				static_cast<float>(prompt.size() + (field.caret() - first));
			text("_", caret_x, y + 2.0f, echo_colour);
		}
	}

	// Non-default clock and view state, top right; the log stops below it.
	const std::string strip = director.status_strip();
	const float top = strip.empty() ? margin : margin + line_height;
	if (!strip.empty())
	{
		const float width = data.render_resources()->measure_text(this->font_,
			widen(strip)).x * look.font_scale;
		text(strip, std::max(margin, viewport.width - margin - width), margin, strip_colour);
	}

	// Log, newest at the bottom, wrapped at the panel width.
	const std::deque<DebugLogLine>& lines = director.log().lines();
	y -= line_height;
	for (auto it = lines.rbegin(); it != lines.rend() && y >= top; ++it)
	{
		const Colour colour = it->severity == DebugSeverity::error ? error_colour :
			it->severity == DebugSeverity::warning ? warning_colour :
			it->severity == DebugSeverity::echo ? echo_colour : look.colour;
		const std::string prefix = it->severity == DebugSeverity::error ? "error: " :
			it->severity == DebugSeverity::warning ? "warning: " : "";
		const std::string full = prefix + it->text;
		std::vector<std::string> wrapped;
		for (std::size_t start = 0; start < full.size() || wrapped.empty(); start += columns)
		{
			wrapped.push_back(full.substr(start, columns));
			if (full.empty())
			{
				break;
			}
		}
		for (auto part = wrapped.rbegin(); part != wrapped.rend() && y >= top; ++part)
		{
			text(*part, margin, y, colour);
			y -= line_height;
		}
	}

	list.set_filter(TextureFilter::point);
}

RectangleF DebugConsoleView::place_panel(const Vector2F& size, Anchor anchor,
	const Vector2F& screen, const std::vector<RectangleF>& avoid)
{
	Vector2F position(anchor == Anchor::bottom_left ? margin : screen.x - margin - size.x,
		screen.y - margin - size.y);
	const auto backing = [&position, &size]
		{
			return RectangleF(position.x - pad_x, position.y - pad_y,
				size.x + 2.0f * pad_x, size.y + 2.0f * pad_y);
		};
	const float highest = margin + pad_y;
	for (int pass = 0; pass < 4; ++pass)
	{
		bool lifted = false;
		for (const RectangleF& other : avoid)
		{
			if (other.width > 0.0f && other.height > 0.0f && backing().intersects(other))
			{
				// Stack this panel above the obstacle (another panel, an
				// opening card) instead of drawing over it.
				position.y = std::max(highest, other.top() - pad_y - margin - size.y);
				lifted = true;
			}
		}
		if (!lifted || position.y <= highest)
		{
			break;
		}
	}
	return backing();
}

RectangleF DebugConsoleView::draw_panel(Renderer& renderer, const GameData& data,
	const std::vector<std::string>& lines, Anchor anchor, const std::vector<RectangleF>& avoid)
{
	if (lines.empty())
	{
		return RectangleF();
	}
	RenderResources* resources = data.render_resources();
	const DebugOverlayPresentation& look = data.presentation()->debug_overlay();
	const FontHandle font = resources->resolve_sprite_font(look.font);
	const float line_height = std::max(look.line_spacing, 1.0f);

	std::vector<std::wstring> wide;
	float width = 0.0f;
	for (const std::string& line : lines)
	{
		wide.push_back(widen(line));
		width = std::max(width, resources->measure_text(font, wide.back()).x * look.font_scale);
	}
	const float height = line_height * static_cast<float>(lines.size());

	Viewport viewport;
	DrawList list = screen_list(renderer, data, viewport);
	const RectangleF background = place_panel(Vector2F(width, height), anchor,
		Vector2F(viewport.width, viewport.height), avoid);
	const Vector2F position = background.position() + Vector2F(pad_x, pad_y);

	const RenderResources::SpriteSheetHandle sheet_handle =
		resources->resolve_sprite_sheet("sprite_sheet_1");
	const SpriteSheet* sheet = resources->sprite_sheet(sheet_handle);
	list.set_filter(TextureFilter::point);
	sheet->draw(list, sheet->resolve_sprite_frame("pixel"), background, panel_colour);
	list.set_filter(TextureFilter::linear);
	for (std::size_t i = 0; i < wide.size(); ++i)
	{
		const Vector2F at = position + Vector2F(0.0f, line_height * static_cast<float>(i));
		list.draw_text(font, wide[i], at + look.shadow_offset, look.shadow_colour,
			look.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
		list.draw_text(font, wide[i], at, strip_colour, look.font_scale, 0.0f,
			Vector2F::ZERO, 0.0f);
	}
	list.set_filter(TextureFilter::point);
	return background;
}

RectangleF DebugConsoleView::draw_perf(Renderer& renderer, const GameData& data,
	const DebugPerf& perf, const std::vector<RectangleF>& avoid)
{
	RenderResources* resources = data.render_resources();
	const DebugOverlayPresentation& look = data.presentation()->debug_overlay();
	const FontHandle font = resources->resolve_sprite_font(look.font);
	const float line_height = std::max(look.line_spacing, 1.0f);
	const std::vector<std::string> lines = perf.lines();

	std::vector<std::wstring> wide;
	float width = graph_bar_width * static_cast<float>(graph_frames);
	for (const std::string& line : lines)
	{
		wide.push_back(widen(line));
		width = std::max(width, resources->measure_text(font, wide.back()).x * look.font_scale);
	}
	const float text_top = graph_height + 6.0f;
	const float height = text_top + line_height * static_cast<float>(lines.size());

	Viewport viewport;
	DrawList list = screen_list(renderer, data, viewport);
	const RectangleF background = place_panel(Vector2F(width, height), Anchor::bottom_left,
		Vector2F(viewport.width, viewport.height), avoid);
	const Vector2F origin = background.position() + Vector2F(pad_x, pad_y);

	const SpriteSheet* sheet = resources->sprite_sheet(
		resources->resolve_sprite_sheet("sprite_sheet_1"));
	const SpriteSheet::frame_handle pixel = sheet->resolve_sprite_frame("pixel");
	list.set_filter(TextureFilter::point);
	sheet->draw(list, pixel, background, panel_colour);

	// The graph's own backdrop shows its scale: bars reach the top only at
	// two 60 Hz frames or more.
	sheet->draw(list, pixel, RectangleF(origin.x, origin.y,
		graph_bar_width * static_cast<float>(graph_frames), graph_height), graph_backdrop);
	// Oldest on the left; the newest frame is the right-most bar.
	const std::size_t shown = std::min(graph_frames, perf.size());
	double top_ms = graph_floor_ms;
	for (std::size_t age = 0; age < shown; ++age)
	{
		top_ms = std::max(top_ms, perf.at(age).frame_ms);
	}
	for (std::size_t age = 0; age < shown; ++age)
	{
		const double ms = perf.at(age).frame_ms;
		const float bar = std::max(1.0f, static_cast<float>(ms / top_ms) * graph_height);
		const float x = origin.x + graph_bar_width * static_cast<float>(graph_frames - 1 - age);
		const Colour& colour = ms <= reference_ms + 1.0 ? graph_good :
			ms <= 2.0 * reference_ms + 1.0 ? graph_slow : graph_bad;
		// One pixel of each two: separate bars, so a single slow frame shows.
		sheet->draw(list, pixel, RectangleF(x, origin.y + graph_height - bar,
			graph_bar_width - 1.0f, bar), colour);
	}
	const float reference_y = origin.y + graph_height -
		static_cast<float>(reference_ms / top_ms) * graph_height;
	sheet->draw(list, pixel, RectangleF(origin.x, reference_y,
		graph_bar_width * static_cast<float>(graph_frames), 1.0f), graph_reference);

	list.set_filter(TextureFilter::linear);
	for (std::size_t i = 0; i < wide.size(); ++i)
	{
		const Vector2F at = origin + Vector2F(0.0f, text_top + line_height * static_cast<float>(i));
		list.draw_text(font, wide[i], at + look.shadow_offset, look.shadow_colour,
			look.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
		list.draw_text(font, wide[i], at, strip_colour, look.font_scale, 0.0f,
			Vector2F::ZERO, 0.0f);
	}
	list.set_filter(TextureFilter::point);
	return background;
}
#endif
