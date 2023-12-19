#include "pch.h"
#include "Visual.h"

using namespace MattMath;

void Visual::update()
{
	return;
}
void Visual::draw(const Camera& camera)
{
	this->TextureObject::draw(this->_rectangle, camera);
}
void Visual::draw()
{
	this->TextureObject::draw(this->_rectangle);
}
//void Visual::draw(const Viewport& viewport)
//{
//	RectangleI bounds = this->_rectangle.get_rectangle_i();
//	bounds.x -= static_cast<int>(viewport.x);
//	bounds.y -= static_cast<int>(viewport.y);
//	this->TextureObject::draw(bounds);
//}
bool Visual::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}