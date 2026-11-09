#include "game/pch.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/crude_drawing.h"
#include "game/input/input_slots.h"

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
		identity_text(player_index, this->team_label(team), device_label), health, ammo, timer,
		team_colour, reduced_effects, clock_running);
}

const std::wstring& InterfaceGameplay::team_label(PlayerTeam team) const
{
	return team == PlayerTeam::a ? this->look_->labels.team_a : this->look_->labels.team_b;
}

std::wstring InterfaceGameplay::identity_text(int player_index, const std::wstring& team_label,
	const std::wstring& device_label)
{
	return PlayerBadge::label(player_index) + L"  /  " + team_label + L"\n" + device_label;
}

void InterfaceGameplay::draw_gameplay_interface_prepared(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	const std::wstring& identity,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	bool reduced_effects, bool clock_running,
	bool low_paint, float low_paint_elapsed, bool show_clock, const SubHudCue& sub) const
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
	if (show_clock)
	{
		this->draw_timer(draw_list, resolution, timer, scale, reduced_effects, clock_running);
	}

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
	const float pen = this->paper_pen(scale);
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(identity_panel.position() + Vector2F(pen, pen * 3.0f),
			Vector2F(stripe_width, identity_panel.height - pen * 6.0f)), team_colour);
	this->player_badge_.draw(draw_list, RectangleF(identity_pos + Vector2F(0.0f,
		(identity_size.y - badge_size.y) / 2.0f), badge_size), player_index);
	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->player_identity_font_, identity,
		label_pos + look.player_identity_shadow_offset * scale,
		CrudeDrawing::premultiplied(look.player_identity_shadow_colour), scale,
		0.0f, Vector2F::zero, 0.0f);
	draw_list.draw_text(this->player_identity_font_, identity, label_pos,
		CrudeDrawing::premultiplied(look.player_identity_colour), scale,
		0.0f, Vector2F::zero, 0.0f);

	const float x_pos = resolution.x - resolution.x * look.right_margin;
	const Vector2F ammo_pos(x_pos,
		resolution.y * look.ammo_top_margin);
	const Vector2F health_pos(x_pos,
		resolution.y * look.health_top_margin);
	const float label_height = this->render_resources()->measure_text(
		this->meter_label_font_, look.labels.health).y * scale;
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
		health_pos, look.labels.health, scale);
	const float blink = look.low_paint.blink_period;
	const bool tagged = low_paint &&
		(reduced_effects || std::fmod(low_paint_elapsed, blink) < blink * 0.5f);
	this->draw_meter(draw_list, resolution, ammo, team_colour,
		ammo_pos, look.labels.paint, scale, tagged);
	if (sub.shown)
	{
		this->draw_sub_mark(draw_list, resolution, ammo_pos, sub, scale);
	}

	draw_list.set_filter(TextureFilter::point);
}

void InterfaceGameplay::draw_sub_mark(DrawList& draw_list, const Vector2F& resolution,
	const Vector2F& meter_position, const SubHudCue& sub, float scale) const
{
	const HudPresentation& look = *this->look_;
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->sheet_);
	const float pen = this->paper_pen(scale);
	const Vector2F meter_size(resolution.x * look.box_size.x, resolution.y * look.box_size.y);
	draw_list.set_filter(TextureFilter::point);
	// The tick: what the tank must hold for the sub, across the meter.
	const float cost = std::clamp(sub.cost, 0.0f, 1.0f);
	if (cost > 0.0f && cost < 1.0f)
	{
		sheet->draw(draw_list, this->fill_frame_, RectangleF(
			Vector2F(meter_position.x + meter_size.x * cost - pen * 0.5f, meter_position.y - pen),
			Vector2F(pen, meter_size.y + pen * 2.0f)), CrudeDrawing::premultiplied(look.paper.ink));
	}
	// The square after the PAINT label, its bottom on the label's.
	const Vector2F label_size = this->render_resources()->measure_text(
		this->meter_label_font_, look.labels.paint) * scale;
	const float side = std::max(pen * 3.0f, look.sub_mark.size * scale);
	const float label_top = meter_position.y - label_size.y - look.meter_label_gap * scale;
	const RectangleF square(
		Vector2F(meter_position.x + label_size.x + look.sub_mark.gap * scale,
			label_top + std::max(0.0f, (label_size.y - side) * 0.5f)),
		Vector2F(side, side));
	sheet->draw(draw_list, this->fill_frame_, square, CrudeDrawing::premultiplied(look.paper.ink));
	if (!sub.ready)
	{
		RectangleF face = square;
		face.inflate(-pen);
		sheet->draw(draw_list, this->fill_frame_, face, CrudeDrawing::premultiplied(look.paper.paper));
	}
}

float InterfaceGameplay::paper_pen(float scale) const
{
	// A quarter-size pane (four-player 720x1280 portrait: scale 0.1875) would
	// put the 2-reference-pixel keyline at 0.375 px, which the rasterizer drops
	// on most edges. The floor is a raster rule, like the cursor marker's.
	return std::max(1.0f, this->look_->paper.keyline_width * scale);
}

void InterfaceGameplay::draw_paper_panel(DrawList& draw_list,
	const RectangleF& face, float scale) const
{
	const HudPaperPresentation& art = this->look_->paper;
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->sheet_);
	const float shadow = art.shadow_offset * scale;
	draw_list.set_filter(TextureFilter::point);
	CrudeDrawing::paper_card([&](const RectangleF& box, const Colour& colour)
	{
		sheet->draw(draw_list, this->fill_frame_, box, CrudeDrawing::premultiplied(colour));
	}, RectangleF(face.position() + Vector2F(shadow), face.size()), face,
		this->paper_pen(scale), art.shade, art.ink, art.paper);
}

void InterfaceGameplay::draw_meter(DrawList& draw_list,
	const Vector2F& resolution,
	float amount,
	const Colour& colour,
	const Vector2F& position,
	const std::wstring& label,
	float scale,
	bool tagged) const
{
	const HudPresentation& look = *this->look_;
	const Vector2F size(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);
	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);
	draw_list.set_filter(TextureFilter::point);
	const float pen = this->paper_pen(scale);
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
	Colour label_colour = look.player_identity_colour;
	if (tagged)
	{
		// An ink tag with paper lettering: the low-paint warning reads by
		// lightness and shape, never by hue against the team colour.
		const Vector2F padding = look.low_paint.padding * scale;
		sprite_sheet->draw(draw_list, this->fill_frame_,
			RectangleF(label_position - padding, label_size + padding * 2.0f),
			CrudeDrawing::premultiplied(look.low_paint.tag));
		label_colour = look.low_paint.text;
	}
	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->meter_label_font_, label, label_position,
		CrudeDrawing::premultiplied(label_colour), scale, 0.0f, Vector2F::zero, 0.0f);
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
			0.0f, Vector2F::zero, 0.0f);
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
		0.0f, Vector2F::zero, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, CrudeDrawing::premultiplied(look.timer_colour), timer_scale,
		0.0f, Vector2F::zero, 0.0f);
}

void InterfaceGameplay::draw_closing_clock_panel(DrawList& draw_list,
	const RectangleF& text_bounds, float scale, float beat_age, bool reduced_effects) const
{
	const ClosingClockPresentation& art = this->look_->closing_clock;
	const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->sheet_);
	const float pen = art.keyline_width * scale;
	const float shadow = art.shadow_offset * scale;
	const Vector2F padding(std::max(this->look_->player_identity_padding.x * scale, pen * 3.0f),
		std::max(this->look_->player_identity_padding.y * scale, pen * 3.0f));
	const RectangleF face(text_bounds.position() - padding, text_bounds.size() + padding * 2.0f);
	const auto fill = [&](const RectangleF& box, const Colour& colour)
	{
		sheet->draw(draw_list, this->fill_frame_, box, CrudeDrawing::premultiplied(colour));
	};

	draw_list.set_filter(TextureFilter::point);
	CrudeDrawing::paper_card(fill, RectangleF(face.position() + Vector2F(shadow), face.size()), face,
		pen, art.shade, art.ink, art.paper);

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
