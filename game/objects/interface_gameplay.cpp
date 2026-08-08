#include "game/pch.h"
#include "game/objects/interface_gameplay.h"

using namespace mattmath;
using namespace interface_consts;
using namespace artattack;

InterfaceGameplay::InterfaceGameplay(RenderResources* render_resources) :
	render_resources_(render_resources),
	sheet_(render_resources->resolve_sprite_sheet(SPRITE_SHEET)),
	empty_box_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(EMPTY_BOX_FRAME)),
	fill_frame_(render_resources->sprite_sheet(this->sheet_)->
		resolve_sprite_frame(FILL_FRAME)),
	timer_font_(render_resources->resolve_sprite_font(TIMER_FONT)),
	respawn_timer_font_(
		render_resources->resolve_sprite_font(RESPAWN_TIMER_FONT))
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
	// The clock is a large face and wants linear; the boxes are pixel art and
	// want point. That was two Begin/End pairs with two samplers, one of them
	// a raw ID3D11SamplerState* the caller had cached at construction and held
	// across a device loss. It is two set_filter calls now, and the state that
	// produces the filtering belongs to the backend.
	draw_list.set_filter(TextureFilter::linear);
	this->draw_timer(draw_list, resolution, timer);

	draw_list.set_filter(TextureFilter::point);

	float x_pos = resolution.x - resolution.x * RIGHT_MARGIN;

	Vector2F ammo_pos = Vector2F(x_pos,
		resolution.y * AMMO_TOP_MARGIN);
	this->draw_ammo(draw_list, resolution, ammo, team_colour, ammo_pos);

	Vector2F health_pos = Vector2F(x_pos,
		resolution.y * HEALTH_TOP_MARGIN);
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
	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * TOP_RIGHT_BOX_SIZE.x,
		resolution.y * TOP_RIGHT_BOX_SIZE.y);

	const RectangleF empty_box_rect = RectangleF(position, size);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		empty_box_rect,
		Colour::white);

	//fill box
	float border = resolution.x * BOX_FILL_BORDER;
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
	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * TOP_RIGHT_BOX_SIZE.x,
		resolution.y * TOP_RIGHT_BOX_SIZE.y);

	const SpriteSheet* sprite_sheet = this->render_resources()->
		sprite_sheet(this->sheet_);

	sprite_sheet->draw(
		draw_list,
		this->empty_box_frame_,
		RectangleF(position, size),
		Colour::white);

	//fill box
	float border = resolution.x * BOX_FILL_BORDER;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	fill_size.x *= health;

	sprite_sheet->draw(
		draw_list,
		this->fill_frame_,
		RectangleF(fill_pos, fill_size),
		HEALTH_COLOUR);
}
void InterfaceGameplay::draw_timer(DrawList& draw_list,
	const Vector2F& resolution, float timer) const
{
	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * TIMER_OFFSET.x),
		resolution.y * TIMER_OFFSET.y);

	float scale = resolution.x / TIMER_SCALE_FACTOR;

	// Wide, because the narrow overload converts through a buffer owned by
	// the shared font and every player's render worker is in here.
	const std::wstring text =
		std::to_wstring(static_cast<int>(std::ceil(timer)));

	draw_list.draw_text(this->timer_font_, text,
		pos + (TIMER_SHADOW_OFFSET * scale), TIMER_SHADOW_COLOUR, scale,
		0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->timer_font_, text, pos, TIMER_COLOUR, scale,
		0.0f, Vector2F::ZERO, 0.0f);
}

void InterfaceGameplay::draw_respawn_timer(DrawList& draw_list,
	const Vector2F& resolution,
	float timer) const
{
	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * RESPAWN_TIMER_OFFSET.x),
		resolution.y * RESPAWN_TIMER_OFFSET.y);

	const std::wstring text = std::to_wstring(static_cast<int>(timer) + 1);

	draw_list.draw_text(this->respawn_timer_font_, text,
		pos + RESPAWN_TIMER_SHADOW_OFFSET, RESPAWN_TIMER_SHADOW_COLOUR,
		1.0f, 0.0f, Vector2F::ZERO, 0.0f);

	draw_list.draw_text(this->respawn_timer_font_, text, pos,
		RESPAWN_TIMER_COLOUR, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
}
