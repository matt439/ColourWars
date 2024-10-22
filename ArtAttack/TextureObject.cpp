#include "pch.h"
#include "TextureObject.h"

using namespace DirectX;
using namespace MattMath;

void TextureObject::draw(const RectangleI& destination_rectangle) const
{
	SpriteSheet* sprite_sheet = SpriteSheetObject::get_sprite_sheet();

	sprite_sheet->draw(this->get_sprite_batch(),
						this->get_element_name(),
						destination_rectangle,
						this->get_colour(),
						this->get_draw_rotation(),
						this->get_origin(),
						this->get_effects(),
						this->get_layer_depth());
}
void TextureObject::draw(const RectangleF& destination_rectangle)const
{
	this->draw(destination_rectangle.get_rectangle_i());
}
void TextureObject::draw(const Vector2F& position, float scale) const
{
	SpriteSheet* sprite_sheet = SpriteSheetObject::get_sprite_sheet();
	
	sprite_sheet->draw(this->get_sprite_batch(),
								this->get_element_name(),
								position,
								this->get_colour(),
								this->get_draw_rotation(),
								this->get_origin(),
								scale,
								this->get_effects(),
								this->get_layer_depth());
}
void TextureObject::draw(const RectangleF& destination_rectangle,
	const Camera& camera) const
{
	RectangleF rect = camera.calculate_view_rectangle(destination_rectangle);
	this->draw(rect);
}
void TextureObject::draw(const Vector2F& position,
	const Camera& camera, float scale) const
{
	Vector2F view_pos = camera.calculate_view_position(position);
	float view_scale = camera.calculate_view_scale(scale);
	this->draw(view_pos, view_scale);
}