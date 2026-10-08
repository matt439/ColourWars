#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_console_state.h"
#include "engine/core/state_context.h"
#include "engine/input/keyboard.h"
#include "engine/render/camera.h"
#include "engine/render/renderer.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_director_impl.h"
#include "game/game_data.h"

using namespace labrador;
using namespace mattmath;

// Developer-only styling; presentation.json ships, this console does not.
namespace debug_console_consts
{
	constexpr float panel_fraction = 0.45f;
	constexpr float margin = 10.0f;
	constexpr float accent_height = 2.0f;
	const Colour panel_colour(0.02f, 0.03f, 0.05f, 0.80f);
	const Colour echo_colour(1.0f, 1.0f, 1.0f, 1.0f);
	const Colour warning_colour(1.0f, 0.85f, 0.35f, 1.0f);
	const Colour error_colour(1.0f, 0.45f, 0.40f, 1.0f);
	const Colour candidate_colour(0.55f, 0.85f, 1.0f, 1.0f);
	const Colour strip_colour(1.0f, 0.85f, 0.35f, 1.0f);
}

using namespace debug_console_consts;

DebugConsoleState::DebugConsoleState(GameData* data, DebugDirectorImpl* director) :
	data_(data), director_(director)
{
	this->director_->set_console_open(true);
}

DebugConsoleState::~DebugConsoleState()
{
	this->director_->set_console_open(false);
}

void DebugConsoleState::init()
{
	RenderResources* resources = this->data_->render_resources();
	this->sheet_ = resources->resolve_sprite_sheet("sprite_sheet_1");
	this->pixel_ = resources->sprite_sheet(this->sheet_)->resolve_sprite_frame("pixel");
	this->font_ = resources->resolve_sprite_font(
		this->data_->presentation()->debug_overlay().font);
	// The overlay font is monospaced; one glyph's pen advance places the caret
	// and wraps every line. measure_text reports ink extent (x_offset + width
	// of the last glyph), so the advance is the difference of two runs.
	const float advance = resources->measure_text(this->font_, L"MM").x -
		resources->measure_text(this->font_, L"M").x;
	this->char_width_ = std::max(1.0f, advance *
		this->data_->presentation()->debug_overlay().font_scale);
}

void DebugConsoleState::close()
{
	if (!this->closing_)
	{
		this->closing_ = true;
		this->context()->pop();
	}
}

void DebugConsoleState::update(float dt)
{
	if (this->closing_)
	{
		return;
	}
	const Keyboard& keyboard = *this->data_->keyboard();

	// The grave key toggles. Its own character is dropped by returning before
	// typed() is read, and while it is held or released all text is dropped.
	if (!this->first_update_ && keyboard.pressed(Key::grave))
	{
		this->close();
		return;
	}
	// On release, so the Escape edge is over before the match reads its
	// released(escape) pause binding.
	if (keyboard.released(Key::escape))
	{
		this->close();
		return;
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

	DebugTextField& field = this->director_->field();
	field.apply(events);

	if (keyboard.pressed(Key::tab))
	{
		const DebugCompletion completion =
			this->director_->registry().complete(field.text());
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
			this->director_->log().write(DebugSeverity::echo, line);
		}
	}

	if (keyboard.pressed(Key::enter))
	{
		const std::string line = field.submit();
		this->director_->log().write(DebugSeverity::echo, "] " + line);
		if (this->director_->execute(line).close_console)
		{
			this->close();
			return;
		}
	}

	this->caret_time_ += dt;
}

void DebugConsoleState::draw(Renderer& renderer) const
{
	if (renderer.view_count() == 0)
	{
		renderer.set_view_count(1);
	}
	DrawList list = renderer.view(renderer.view_count() - 1);
	const Viewport viewport = this->data_->viewport_manager()->fullscreen_viewport();
	list.set_viewport(viewport);
	list.set_camera(Camera::frame(
		RectangleF(0.0f, 0.0f, viewport.width, viewport.height), viewport));

	const SpriteSheet* sheet = this->data_->render_resources()->sprite_sheet(this->sheet_);
	const DebugOverlayPresentation& look = this->data_->presentation()->debug_overlay();
	const float line_height = std::max(look.line_spacing, 1.0f);
	const float panel_height = std::floor(viewport.height * panel_fraction);

	list.set_filter(TextureFilter::point);
	sheet->draw(list, this->pixel_, RectangleF(0.0f, 0.0f, viewport.width, panel_height),
		panel_colour);
	sheet->draw(list, this->pixel_, RectangleF(0.0f, panel_height - accent_height,
		viewport.width, accent_height), this->data_->presentation()->menu().highlight);

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
	const DebugTextField& field = this->director_->field();
	float y = panel_height - accent_height - margin - line_height;
	{
		const std::string prompt = "] ";
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
	const std::string strip = this->director_->status_strip();
	const float top = strip.empty() ? margin : margin + line_height;
	if (!strip.empty())
	{
		const float width = this->data_->render_resources()->measure_text(this->font_,
			widen(strip)).x * look.font_scale;
		text(strip, std::max(margin, viewport.width - margin - width), margin, strip_colour);
	}

	// Log, newest at the bottom, wrapped at the panel width.
	const std::deque<DebugLogLine>& lines = this->director_->log().lines();
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

void DebugConsoleState::draw_strip(Renderer& renderer, const GameData& data,
	const std::string& strip)
{
	if (strip.empty())
	{
		return;
	}
	if (renderer.view_count() == 0)
	{
		renderer.set_view_count(1);
	}
	RenderResources* resources = data.render_resources();
	const DebugOverlayPresentation& look = data.presentation()->debug_overlay();
	const FontHandle font = resources->resolve_sprite_font(look.font);
	const std::wstring wide = widen(strip);
	const Vector2F size = resources->measure_text(font, wide) * look.font_scale;

	DrawList list = renderer.view(renderer.view_count() - 1);
	const Viewport viewport = data.viewport_manager()->fullscreen_viewport();
	list.set_viewport(viewport);
	list.set_camera(Camera::frame(
		RectangleF(0.0f, 0.0f, viewport.width, viewport.height), viewport));
	// Bottom right: clear of every pane's HUD (top) and the opening cards
	// (bottom centre), in any layout.
	const Vector2F position(viewport.width - margin - size.x,
		viewport.height - margin - size.y);

	const RenderResources::SpriteSheetHandle sheet_handle =
		resources->resolve_sprite_sheet("sprite_sheet_1");
	const SpriteSheet* sheet = resources->sprite_sheet(sheet_handle);
	list.set_filter(TextureFilter::point);
	sheet->draw(list, sheet->resolve_sprite_frame("pixel"),
		RectangleF(position.x - 6.0f, position.y - 3.0f, size.x + 12.0f, size.y + 6.0f),
		panel_colour);
	list.set_filter(TextureFilter::linear);
	list.draw_text(font, wide, position + look.shadow_offset, look.shadow_colour,
		look.shadow_scale, 0.0f, Vector2F::ZERO, 0.0f);
	list.draw_text(font, wide, position, strip_colour, look.font_scale, 0.0f,
		Vector2F::ZERO, 0.0f);
	list.set_filter(TextureFilter::point);
}
#endif
