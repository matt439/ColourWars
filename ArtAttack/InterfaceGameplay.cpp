#include "pch.h"
#include "InterfaceGameplay.h"

using namespace DirectX;
using namespace MattMath;
using namespace interface_consts;
using namespace colour_consts;

void InterfaceGameplay::draw_gameplay_interface(const Vector2F& resolution,
	float health,
	float ammo,
	float timer,
	const Colour& team_colour,
	ID3D11SamplerState* sampler_state,
	float respawn_timer,
	bool show_respawn_timer)
{
	
	this->get_sprite_batch()->Begin();
	this->draw_timer(resolution, timer);
	this->get_sprite_batch()->End();

	this->get_sprite_batch()->Begin(SpriteSortMode_Deferred, nullptr, sampler_state);

	float x_pos = resolution.x - resolution.x * RIGHT_MARGIN;

	Vector2F ammo_pos = Vector2F(x_pos,
		resolution.y * AMMO_TOP_MARGIN);
	this->draw_ammo(resolution, ammo, team_colour, ammo_pos);

	Vector2F health_pos = Vector2F(x_pos,
		resolution.y * HEALTH_TOP_MARGIN);
	this->draw_health(resolution, health, health_pos);

	if (show_respawn_timer)
	{
		this->draw_respawn_timer(resolution, respawn_timer);
	}
	this->get_sprite_batch()->End();
}

void InterfaceGameplay::draw_ammo(
	const Vector2F& resolution,
	float ammo,
	const Colour& team_colour,
	const Vector2F& position) const
{
	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * TOP_RIGHT_BOX_SIZE.x,
		resolution.y * TOP_RIGHT_BOX_SIZE.y);

	RectangleI empty_box_rect = RectangleI(
		static_cast<int>(position.x),
		static_cast<int>(position.y),
		static_cast<int>(size.x),
		static_cast<int>(size.y));

	SpriteSheet* sprite_sheet = this->get_resource_manager()->
		get_sprite_sheet(SPRITE_SHEET);

	sprite_sheet->draw(
		this->get_sprite_batch(),
		EMPTY_BOX_FRAME,
		empty_box_rect,
		WHITE);

	//fill box
	float border = resolution.x * BOX_FILL_BORDER;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	fill_size.x *= ammo;

	RectangleI fill_rect = RectangleI(
		static_cast<int>(fill_pos.x),
		static_cast<int>(fill_pos.y),
		static_cast<int>(fill_size.x),
		static_cast<int>(fill_size.y));

	sprite_sheet = this->get_resource_manager()->
		get_sprite_sheet(SPRITE_SHEET);

	sprite_sheet->draw(
		this->get_sprite_batch(),
		FILL_FRAME,
		fill_rect,
		team_colour);
}
void InterfaceGameplay::draw_health(
	const Vector2F& resolution,
	float health,
	const Vector2F& position) const
{
	//draw empty box
	Vector2F size = Vector2F(
		resolution.x * TOP_RIGHT_BOX_SIZE.x,
		resolution.y * TOP_RIGHT_BOX_SIZE.y);

	RectangleI empty_rect = RectangleI(
		static_cast<int>(position.x),
		static_cast<int>(position.y),
		static_cast<int>(size.x),
		static_cast<int>(size.y));

	SpriteSheet* sprite_sheet = this->get_resource_manager()->
		get_sprite_sheet(SPRITE_SHEET);

	sprite_sheet->draw(
		this->get_sprite_batch(),
		EMPTY_BOX_FRAME,
		empty_rect,
		WHITE);

	//fill box
	float border = resolution.x * BOX_FILL_BORDER;
	Vector2F fill_pos = position + Vector2F(border, border);
	Vector2F fill_size = Vector2F(size.x - 2.0f * border,
		size.y - 2.0f * border);
	fill_size.x *= health;

	RectangleI fill_rect = RectangleI(
		static_cast<int>(fill_pos.x),
		static_cast<int>(fill_pos.y),
		static_cast<int>(fill_size.x),
		static_cast<int>(fill_size.y));

	sprite_sheet = this->get_resource_manager()->
		get_sprite_sheet(SPRITE_SHEET);

	sprite_sheet->draw(
		this->get_sprite_batch(),
		FILL_FRAME,
		fill_rect,
		HEALTH_COLOUR);
}
void InterfaceGameplay::draw_timer(const Vector2F& resolution, float timer) const
{
	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * TIMER_OFFSET.x),
		resolution.y * TIMER_OFFSET.y);

	float scale = resolution.x / TIMER_SCALE_FACTOR;

	SpriteFont* sprite_font = this->get_resource_manager()->
		get_sprite_font(TIMER_FONT);

	sprite_font->DrawString(
		this->get_sprite_batch(),
		std::to_string(static_cast<int>(std::ceil(timer))).c_str(),
		(pos + (TIMER_SHADOW_OFFSET * scale)).get_xm_vector(),
		TIMER_SHADOW_COLOUR.get_xm_vector(),
		0.0f,
		XMFLOAT2(0.0f, 0.0f),
		scale);

	sprite_font->DrawString(
		this->get_sprite_batch(),
		std::to_string(static_cast<int>(std::ceil(timer))).c_str(),
		pos.get_xm_vector(),
		TIMER_COLOUR.get_xm_vector(),
		0.0f,
		XMFLOAT2(0.0f, 0.0f),
		scale);
}

void InterfaceGameplay::draw_respawn_timer(const MattMath::Vector2F& resolution,
	float timer) const
{
	Vector2F pos = Vector2F(
		resolution.x / 2.0f - (resolution.x * RESPAWN_TIMER_OFFSET.x),
		resolution.y * RESPAWN_TIMER_OFFSET.y);

	SpriteFont* sprite_font = this->get_resource_manager()->
		get_sprite_font(RESPAWN_TIMER_FONT);

	std::string text = std::to_string(static_cast<int>(timer) + 1);

	sprite_font->DrawString(
		this->get_sprite_batch(),
		text.c_str(),
		(pos + RESPAWN_TIMER_SHADOW_OFFSET).get_xm_vector(),
		RESPAWN_TIMER_SHADOW_COLOUR.get_xm_vector());

	sprite_font->DrawString(
		this->get_sprite_batch(),
		text.c_str(),
		pos.get_xm_vector(),
		RESPAWN_TIMER_COLOUR.get_xm_vector());
}
