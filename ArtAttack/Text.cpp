#include "pch.h"
#include "Text.h"

using namespace MattMath;
using namespace DirectX;

Text::Text(const std::string& text,
	const std::string& font_name,
	const Vector2F& position,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	float scale,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	TextObject(text, font_name, position, sprite_batch, resource_manager,
		color, scale, rotation, origin, effects, layer_depth)
{

}
void Text::set_text(const std::string& text)
{
	this->TextObject::set_text(text);
}
void Text::set_colour(const Colour& colour)
{
	this->TextObject::set_colour(colour);
}
void Text::set_scale(float scale)
{
	this->TextObject::set_scale(scale);
}
void Text::set_position(const Vector2F& position)
{
	this->TextObject::set_position(position);
}