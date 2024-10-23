#include "pch.h"
#include "Visual.h"

using namespace MattMath;

Visual::Visual(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& rectangle,
	DirectX::SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	DirectX::SpriteEffects effects,
	float layer_depth) :
	TextureObject(sheet_name, frame_name, sprite_batch, resource_manager,
		color, rotation, origin, effects, layer_depth),
	_rectangle(rectangle)
{
}

void Visual::update()
{
	// do nothing
}
void Visual::draw(const Camera& camera)
{
	this->TextureObject::draw(this->_rectangle, camera);
}
void Visual::draw()
{
	this->TextureObject::draw(this->_rectangle);
}
bool Visual::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}