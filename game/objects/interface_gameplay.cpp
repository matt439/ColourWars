#include "game/pch.h"
#include "game/objects/interface_gameplay.h"

using namespace mattmath;
using namespace artattack;

InterfaceGameplay::InterfaceGameplay(RenderResources* render_resources,
	const HudPresentation* look) :
	render_resources_(render_resources),
	look_(look),
	sheet_(render_resources->resolve_sprite_sheet(look->sprite_sheet)),
	empty_box_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(look->empty_box_frame)),
	fill_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(look->fill_frame)),
	timer_font_(render_resources->resolve_sprite_font(look->timer_font)),
	respawn_timer_font_(
		render_resources->resolve_sprite_font(look->respawn_font))
{

}

RenderResources* InterfaceGameplay::render_resources() const
{
	return this->render_resources_;
}

void InterfaceGameplay::draw_gameplay_interface(DrawList& draw_list,
	const Vector2F& resolution,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	float respawn_timer,
	bool show_respawn_timer) const
{
	const HudPresentation& look = *this->look_;

	// The clock is a large face and wants linear; the boxes are pixel art and
	// want point. That was two Begin/End pairs with two samplers, one of them
	// a raw ID3D11SamplerState* the caller had cached at construction and held
	// across a device loss. It is two set_filter calls now, and the state that
	// produces the filtering belongs to the backend.
	draw_list.set_filter(TextureFilter::linear);
	this->draw_timer(draw_list, resolution, timer);

	draw_list.set_filter(TextureFilter::point);

	float x_pos = resolution.x - resolution.x * look.right_margin;

	Vector2F ammo_pos = Vector2F(x_pos,
		resolution.y * look.ammo_top_margin);
	this->draw_ammo(draw_list, resolution, ammo, team_colour, ammo_pos);

	Vector2F health_pos = Vector2F(x_pos,
		resolution.y * look.health_top_margin);
	this->draw_health(draw_list, resolution, health, health_pos);

	if (show_respawn_timer)
	{
		draw_list.set_filter(TextureFilter::linear);
		this->draw_respawn_timer(draw_list, resolution, respawn_timer);
		draw_list.set_filter(TextureFilter::point);
	}
}

void InterfaceGameplay::draw_ammo(DrawList& draw_list,
	const Vector2F& resolution,
	float ammo,
	const Colour& team_colour,
	const Vector2F& position) const
{
	const HudPresentation& look = *this->look_;

	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);

	const RectangleF empty_box_rect = RectangleF(position, size);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		empty_box_rect,
		Colour::white);

	//fill box
	float border = resolution.x * look.box_fill_border;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	fill_size.x *= ammo;

	sprite_sheet->draw(
		draw_list,
		this->fill_frame_,
		RectangleF(fill_pos, fill_size),
		team_colour);
}
void InterfaceGameplay::draw_health(DrawList& draw_list,
	const Vector2F& resolution,
	float health,
	const Vector2F& position) const
{
	const HudPresentation& look = *this->look_;

	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * look.box_size.x,
		resolution.y * look.box_size.y);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		RectangleF(position, size),
		Colour::white);

	//fill box
	float border = resolution.x * look.box_fill_border;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	fill_size.x *= health;

	sprite_sheet->draw(
		draw_list,
		this->fill_frame_,
		RectangleF(fill_pos, fill_size),
		look.health_colour);
}
void InterfaceGameplay::draw_timer(DrawList& draw_list,
	const Vector2F& resolution, float timer) const
{
	const HudPresentation& look = *this->look_;

	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * look.timer_offset.x),
		resolution.y * look.timer_offset.y);

	float scale = resolution.x / look.timer_reference_width;

	// Wide, because the narrow overload converts through a buffer owned by
	// the shared font and every player's render worker is in here.
	const std::wstring text =
		std::to_wstring(static_cast<int>(std::ceil(timer)));

	draw_list.draw_text(this->timer_font_, text,
		pos + (look.timer_shadow_offset * scale), look.timer_shadow_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, look.timer_colour, scale,
		0.0f, Vector2F::ZERO, 0.0f);
}

void InterfaceGameplay::draw_respawn_timer(DrawList& draw_list,
	const Vector2F& resolution,
	float timer) const
{
	const HudPresentation& look = *this->look_;

	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * look.respawn_offset.x),
		resolution.y * look.respawn_offset.y);

	const std::wstring text = std::to_wstring(static_cast<int>(timer) + 1);

	draw_list.draw_text(this->respawn_timer_font_, text,
		pos + look.respawn_shadow_offset, look.respawn_shadow_colour,
		1.0f, 0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->respawn_timer_font_, text, pos,
		look.respawn_colour, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
}
