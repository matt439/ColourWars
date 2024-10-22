#include "pch.h"
#include "Visual.h"

using namespace MattMath;

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