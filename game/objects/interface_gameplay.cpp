#include "game/pch.h"
#include "game/objects/interface_gameplay.h"
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
	const HudPresentation* look) :
	render_resources_(render_resources),
	look_(look),
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
	const Colour& team_colour) const
{
	this->draw_gameplay_interface(draw_list, resolution, player_index,
		input_slots::device_name(input_slot), team, health, ammo, timer, team_colour);
}

void InterfaceGameplay::draw_gameplay_interface(DrawList& draw_list,
	const Vector2F& resolution,
	int player_index,
	const std::wstring& device_label,
	PlayerTeam team,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour) const
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
	this->draw_timer(draw_list, resolution, timer, scale);

	const std::wstring identity = L"P" + std::to_wstring(player_index + 1) +
		(team == PlayerTeam::a ? L"  /  TEAM A\n" : L"  /  TEAM B\n") +
		device_label;
	const Vector2F identity_pos(resolution.x * look.player_identity_offset.x,
		resolution.y * look.player_identity_offset.y);
	const Vector2F identity_size = this->render_resources()->measure_text(
		this->player_identity_font_, identity) * scale;
	const RectangleF identity_panel(identity_pos - padding,
		identity_size + padding * 2.0f);
	draw_list.set_filter(TextureFilter::point);
	sprite_sheet->draw(draw_list, this->fill_frame_, identity_panel,
		look.player_identity_background_colour);
	const float stripe_width = look.box_fill_border *
		look.player_identity_reference_width * scale;
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(identity_panel.position(),
			Vector2F(stripe_width, identity_panel.height)), team_colour);
	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->player_identity_font_, identity,
		identity_pos + look.player_identity_shadow_offset * scale,
		look.player_identity_shadow_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);
	draw_list.draw_text(this->player_identity_font_, identity, identity_pos,
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
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(Vector2F(x_pos, panel_top) - padding,
			Vector2F(resolution.x * look.box_size.x,
				panel_bottom - panel_top) + padding * 2.0f),
		look.player_identity_background_colour);
	this->draw_meter(draw_list, resolution, health, look.health_colour,
		health_pos, L"HEALTH", scale);
	this->draw_meter(draw_list, resolution, ammo, team_colour,
		ammo_pos, L"PAINT", scale);

	draw_list.set_filter(TextureFilter::point);
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
	sprite_sheet->draw(draw_list, this->fill_frame_,
		RectangleF(position, size), look.meter_track_colour);
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
	const Vector2F& resolution, float timer, float scale) const
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
	const Vector2F padding = look.player_identity_padding * scale;
	draw_list.set_filter(TextureFilter::point);
	this->render_resources()->sprite_sheet(this->sheet_)->draw(draw_list,
		this->fill_frame_, RectangleF(pos - padding, text_size + padding * 2.0f),
		look.player_identity_background_colour);

	draw_list.set_filter(TextureFilter::linear);
	draw_list.draw_text(this->timer_font_, text,
		pos + (look.timer_shadow_offset * timer_scale),
		look.timer_shadow_colour, timer_scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, look.timer_colour, timer_scale,
		0.0f, Vector2F::ZERO, 0.0f);
}
