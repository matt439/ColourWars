#include "game/pch.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/crude_drawing.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace labrador;

namespace
{
	// Format remaining time as mm:ss, rounding up and clamping at zero.
	std::wstring format_clock(float seconds)
	{
		const int total = std::max(0, static_cast<int>(std::ceil(seconds)));
		const int minutes = total / 60;
		const int remainder = total % 60;

		return std::to_wstring(minutes) + (remainder < 10 ? L":0" : L":") +
			std::to_wstring(remainder);
	}
}

InterfaceGameplay::InterfaceGameplay(RenderResources* render_resources,
	const HudPresentation* look, const PlayerIdentityPresentation* identity) :
	render_resources_(render_resources),
	look_(look),
	identity_(identity),
	player_badge_(identity, render_resources),
	sheet_(render_resources->resolve_sprite_sheet(look->sprite_sheet)),
	fill_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(look->fill_frame)),
	meter_label_font_(render_resources->resolve_sprite_font(look->meter_label_font)),
	timer_font_(render_resources->resolve_sprite_font(look->timer_font)),
	player_identity_font_(render_resources->resolve_sprite_font(look->player_identity_font))
{

}

RenderResources* InterfaceGameplay::render_resources() const
{
	return this->render_resources_;
}

void InterfaceGameplay::draw_gameplay_interface(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	int input_slot,
	PlayerTeam team,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	bool reduced_effects, bool clock_running) const
{
	this->draw_gameplay_interface(draw_list, resolution, player_index,
		input_slots::device_name(input_slot), team, health, ammo, timer, team_colour,
		reduced_effects, clock_running);
}

void InterfaceGameplay::draw_gameplay_interface(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	const std::wstring& device_label,
	PlayerTeam team,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	bool reduced_effects, bool clock_running) const
{
	this->draw_gameplay_interface_prepared(draw_list, resolution, player_index,
		identity_text(player_index, team, device_label), health, ammo, timer,
		team_colour, reduced_effects, clock_running);
}

std::wstring InterfaceGameplay::identity_text(int player_index, PlayerTeam team,
	const std::wstring& device_label)
{
	return PlayerBadge::label(player_index) +
		(team == PlayerTeam::a ? L"  /  TEAM A\n" : L"  /  TEAM B\n") +
		device_label;
}

void InterfaceGameplay::draw_gameplay_interface_prepared(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	const std::wstring& identity,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	bool reduced_effects, bool clock_running) const
{
	const HudPresentation& look = *this->look_;
	const float scale = std::min(
		resolution.x / look.player_identity_reference_width,
		resolution.y / look.reference_height);
	const Vector2F padding = look.player_identity_padding * scale;
	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	// Scale text to both pane dimensions: a stacked two-player view is wide,
	// but has only half the height available for its HUD.
	draw_list.set_filter(TextureFilter::linear);
	this->draw_timer(draw_list, resolution, timer, scale, reduced_effects, clock_running);

	const Vector2F identity_pos(resolution.x * look.player_identity_offset.x,
		resolution.y * look.player_identity_offset.y);
	const Vector2F identity_size = this->render_resources()->measure_text(
		this->player_identity_font_, identity) * scale;
	const Vector2F badge_size = this->identity_->ui_size * scale;
	const float badge_width = this->identity_->enabled
		? badge_size.x + this->identity_->ui_gap * scale : 0.0f;
	const Vector2F label_pos = identity_pos + Vector2F(badge_width, 0.0f);
	const RectangleF identity_panel(identity_pos - padding,
		Vector2F(identity_size.x + badge_width,
			std::max(identity_size.y, this->identity_->enabled ? badge_size.y : 0.0f)) +
			padding * 2.0f);
	draw_list.set_filter(TextureFilter::point);
	this->draw_paper_panel(draw_list, identity_panel, scale);
	const float stripe_width = look.box_fill_border *
		look.player_identity_reference_width * scale;
	const float pen = look.paper.keyline_width * scale;
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(identity_panel.position() + Vector2F(pen, pen * 3.0f),
			Vector2F(stripe_width, identity_panel.height - pen * 6.0f)), team_colour);
	this->player_badge_.draw(draw_list, RectangleF(identity_pos + Vector2F(0.0f,
		(identity_size.y - badge_size.y) / 2.0f), badge_size), player_index);
	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->player_identity_font_, identity,
		label_pos + look.player_identity_shadow_offset * scale,
		look.player_identity_shadow_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);
	draw_list.draw_text(this->player_identity_font_, identity, label_pos,
		look.player_identity_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);

	const float x_pos = resolution.x - resolution.x * look.right_margin;
	const Vector2F ammo_pos(x_pos,
		resolution.y * look.ammo_top_margin);
	const Vector2F health_pos(x_pos,
		resolution.y * look.health_top_margin);
	const float label_height = this->render_resources()->measure_text(
		this->meter_label_font_, L"HEALTH").y * scale;
	const float panel_top = std::min(ammo_pos.y, health_pos.y) -
		label_height - look.meter_label_gap * scale;
	const float panel_bottom = std::max(ammo_pos.y, health_pos.y) +
		resolution.y * look.box_size.y;
	draw_list.set_filter(TextureFilter::point);
	this->draw_paper_panel(draw_list,
		RectangleF(Vector2F(x_pos, panel_top) - padding,
			Vector2F(resolution.x * look.box_size.x,
				panel_bottom - panel_top) + padding * 2.0f), scale);
	this->draw_meter(draw_list, resolution, health, CrudeDrawing::premultiplied(look.health_colour),
		health_pos, L"HEALTH", scale);
	this->draw_meter(draw_list, resolution, ammo, team_colour,
		ammo_pos, L"PAINT", scale);

	draw_list.set_filter(TextureFilter::point);
}

void InterfaceGameplay::draw_paper_panel(DrawList& draw_list,
	const RectangleF& face, float scale) const
{
	const HudPaperPresentation& art = this->look_->paper;
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->sheet_);
	const float pen = art.keyline_width * scale;
	const float step = pen * 2.0f;
	const float shadow = art.shadow_offset * scale;
	const auto fill = [&](const RectangleF& box, const Colour& colour)
	{
		sheet->draw(draw_list, this->fill_frame_, box, CrudeDrawing::premultiplied(colour));
	};
	const auto paper = [&](const RectangleF& box, const Colour& colour)
	{
		fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), colour);
		fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), colour);
	};
	draw_list.set_filter(TextureFilter::point);
	paper(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
	paper(face, art.ink);
	const RectangleF inside(face.x + pen, face.y + pen,
		face.width - pen * 2.0f, face.height - pen * 2.0f);
	paper(inside, art.paper);
	for (int pass = 3; pass > 0; --pass)
	{
		Colour shade = art.shade;
		shade.a *= 0.12f;
		const float width = pen * static_cast<float>(pass);
		fill(RectangleF(inside.x + step, inside.bottom() - width,
			inside.width - step * 2.0f, width), shade);
		fill(RectangleF(inside.right() - width, inside.y + step,
			width, inside.height - step * 2.0f), shade);
	}
}

void InterfaceGameplay::draw_meter(DrawList& draw_list,
	const Vector2F& resolution,
	float amount,
	const Colour& colour,
	const Vector2F& position,
	const std::wstring& label,
	float scale) const
{
	const HudPresentation& look = *this->look_;
	const Vector2F size(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);
	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);
	draw_list.set_filter(TextureFilter::point);
	const float pen = look.paper.keyline_width * scale;
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(position - Vector2F(pen), size + Vector2F(pen * 2.0f)), look.paper.ink);
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(position, size), CrudeDrawing::premultiplied(look.meter_track_colour));
	// Draw exactly the owned fraction of the track, including an empty
	// meter at zero. Presentation never changes health or ammunition state.
	const float fill_width = size.x * std::clamp(amount, 0.0f, 1.0f);
	if (fill_width > 0.0f)
	{
		sprite_sheet->draw(draw_list, this->fill_frame_,
			RectangleF(position, Vector2F(fill_width, size.y)), colour);
	}
	const Vector2F label_size = this->render_resources()->measure_text(
		this->meter_label_font_, label) * scale;
	const Vector2F label_position = position -
		Vector2F(0.0f, label_size.y + look.meter_label_gap * scale);
	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->meter_label_font_, label, label_position,
		look.player_identity_colour, scale, 0.0f, Vector2F::ZERO, 0.0f);
}
void InterfaceGameplay::draw_timer(DrawList& draw_list,
	const Vector2F& resolution, float timer, float scale, bool reduced_effects, bool clock_running) const
{
	const HudPresentation& look = *this->look_;
	const std::wstring text = format_clock(timer);
	const float timer_scale = scale * look.player_identity_reference_width /
		look.timer_reference_width;
	const Vector2F text_size = this->render_resources()->measure_text(
		this->timer_font_, text) * timer_scale;
	const Vector2F pos(
		(resolution.x - text_size.x) / 2.0f - resolution.x * look.timer_offset.x,
		resolution.y * look.timer_offset.y);
	if (clock_running && timer > 0.0f && std::ceil(timer) <= 10.0f)
	{
		this->draw_closing_clock_panel(draw_list, RectangleF(pos, text_size), scale,
			std::ceil(timer) - timer, reduced_effects);
		draw_list.set_filter(TextureFilter::linear);
		draw_list.draw_text(this->timer_font_, text, pos, look.closing_clock.text, timer_scale,
			0.0f, Vector2F::ZERO, 0.0f);
		return;
	}
	const Vector2F padding = look.player_identity_padding * scale;
	draw_list.set_filter(TextureFilter::point);
	this->draw_paper_panel(draw_list,
		RectangleF(pos - padding, text_size + padding * 2.0f), scale);

	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->timer_font_, text,
		pos + (look.timer_shadow_offset * timer_scale),
		CrudeDrawing::premultiplied(look.timer_shadow_colour), timer_scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, CrudeDrawing::premultiplied(look.timer_colour), timer_scale,
		0.0f, Vector2F::ZERO, 0.0f);
}

void InterfaceGameplay::draw_closing_clock_panel(DrawList& draw_list,
	const RectangleF& text_bounds, float scale, float beat_age, bool reduced_effects) const
{
	const ClosingClockPresentation& art = this->look_->closing_clock;
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->sheet_);
	const float pen = art.keyline_width * scale;
	const float step = pen * 2.0f;
	const float shadow = art.shadow_offset * scale;
	const Vector2F padding(std::max(this->look_->player_identity_padding.x * scale, pen * 3.0f),
		std::max(this->look_->player_identity_padding.y * scale, pen * 3.0f));
	const RectangleF face(text_bounds.position() - padding, text_bounds.size() + padding * 2.0f);
	const auto fill = [&](const RectangleF& box, const Colour& colour)
	{
		sheet->draw(draw_list, this->fill_frame_, box, CrudeDrawing::premultiplied(colour));
	};
	const auto paper = [&](const RectangleF& box, const Colour& colour)
	{
		fill(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), colour);
		fill(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), colour);
	};

	draw_list.set_filter(TextureFilter::point);
	paper(RectangleF(face.position() + Vector2F(shadow), face.size()), art.shade);
	paper(face, art.ink);
	const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f,
		face.height - pen * 2.0f);
	paper(inside, art.paper);
	for (int pass = 3; pass > 0; --pass)
	{
		Colour shade = art.shade;
		shade.a *= 0.12f;
		const float width = pen * static_cast<float>(pass);
		fill(RectangleF(inside.x + step, inside.bottom() - width,
			inside.width - step * 2.0f, width), shade);
		fill(RectangleF(inside.right() - width, inside.y + step,
			width, inside.height - step * 2.0f), shade);
	}

	if (reduced_effects || beat_age < 0.0f || beat_age >= art.beat_duration) { return; }
	const float remaining = 1.0f - beat_age / art.beat_duration;
	const float reach = art.tick_reach * scale * remaining * remaining;
	// The occupied panel expands by tick_reach + three pen widths at each side,
	// plus the shade offset at the right/bottom. The clock itself never moves.
	for (const float side : { -1.0f, 1.0f })
	{
		const float edge = side < 0.0f ? face.x - pen * 2.0f :
			face.right() + shadow + pen * 2.0f;
		const float x = side < 0.0f ? edge - reach : edge;
		for (const float row : { 0.35f, 0.65f })
		{
			const float y = face.y + face.height * row;
			fill(RectangleF(x, y - pen, reach + pen, pen * 2.0f), art.ink);
			fill(RectangleF(x + pen * 0.5f, y - pen * 0.25f,
				reach, pen * 0.5f), art.accent);
		}
	}
}
