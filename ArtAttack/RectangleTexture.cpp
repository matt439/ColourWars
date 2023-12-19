#include "pch.h"
#include "RectangleTexture.h"

using namespace DirectX;
using namespace MattMath;

//const RectangleF& RectangleTexture::get_rectangle() const
//{
//	return this->_rectangle;
//}
////const RectangleI RectangleTexture::get_rectangle_i() const
////{
////	return this->_rectangle.get_rectangle_i();
////}
//void RectangleTexture::set_rectangle(const RectangleF& rectangle)
//{
//	this->_rectangle = rectangle;
//}
void RectangleTexture::draw()
{
	TextureObject::draw(RectangleObject::get_rectangle());
}