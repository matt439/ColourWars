#include "pch.h"
#include "TextDropShadow.h"

using namespace MattMath;
using namespace DirectX;

TextDropShadow::TextDropShadow(const std::string& text,
	const std::string& font_name,
	const Vector2F& position,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	const Colour& shadow_color,
	const Vector2F& shadow_offset,
	float scale,
	float shadow_scale,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	Text(text, font_name, position, sprite_batch, resource_manager,
	     color, scale, rotation, origin, effects, layer_depth),
	_shadow_offset(shadow_offset),
	_shadow_color(shadow_color),
	_shadow_scale(shadow_scale)
{

}
void TextDropShadow::draw(const Camera& camera)
{
	// Store original values
	Colour original_color = this->get_colour();
	Vector2F original_position = this->get_position();
	float original_scale = this->get_scale();

	// Draw Shadow
	this->TextObject::set_colour(this->_shadow_color);
	this->TextObject::set_scale(this->_shadow_scale);
	this->TextObject::set_position(this->get_position() + this->_shadow_offset);
	this->TextObject::draw(camera);

	// Restore original values
	this->TextObject::set_colour(original_color);
	this->TextObject::set_scale(original_scale);
	this->TextObject::set_position(original_position);

	// Draw Text
	this->TextObject::draw(camera);
}
void TextDropShadow::draw()
{
	this->draw(Camera::DEFAULT_CAMERA);
}
Vector2F TextDropShadow::get_shadow_offset() const
{
	return this->_shadow_offset;
}
Colour TextDropShadow::get_shadow_color() const
{
	return this->_shadow_color;
}
float TextDropShadow::get_shadow_scale() const
{
	return this->_shadow_scale;
}
void TextDropShadow::set_shadow_offset(const Vector2F& offset)
{
	this->_shadow_offset = offset;
}
void TextDropShadow::set_shadow_color(const Colour& color)
{
	this->_shadow_color = color;
}
void TextDropShadow::set_shadow_scale(float scale)
{
	this->_shadow_scale = scale;
}