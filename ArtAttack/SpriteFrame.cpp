#include "pch.h"
#include "SpriteFrame.h"

//using namespace DirectX;
//using namespace DirectX::SimpleMath;

using namespace MattMath;

//SpriteFrame::SpriteFrame(const RectangleI& source_RectangleI)
//{
//	this->set_source_RectangleI(source_RectangleI);
//}
SpriteFrame::SpriteFrame(const RectangleI& source_rectangle,
	const Vector2F& origin,
	bool rotated)
{
	this->set_source_rectangle(source_rectangle);
	this->_origin = origin;
	this->_rotated = rotated;
}
SpriteFrame::SpriteFrame(const Vector2F& position, const Vector2F& size,
	const Vector2F& origin, bool rotated)
{
	this->set_source_rectangle(position, size);
	this->_origin = origin;
	this->_rotated = rotated;
}
//SpriteFrame::SpriteFrame(const Vector2F& position, const Vector2F& size)
//{
//	this->set_source_RectangleI(position, size);
//}
//RECT* SpriteFrame::get_source_RectangleI() const
//{
//	return this->_source_RectangleI.get();
//}
const RECT* SpriteFrame::get_source_rectangle() const
{
	return &this->_source_rectangle;
}
//const RECT* SpriteFrame::get_source_rectangle2() const
//{
//	return &this->_source_rectangle2.get_win_rect();
//}

void SpriteFrame::set_source_rectangle(const RectangleI& source_rectangle)
{
	//RectangleI source_rectan = source_rectangle;
	//source_rectan.operator RECT();
	this->_source_rectangle = source_rectangle.get_win_rect();
}

void SpriteFrame::set_source_rectangle(const Vector2F& position,
	const Vector2F& size)
{
	RectangleI source_rectan = RectangleI(position, size);
	this->_source_rectangle = source_rectan.get_win_rect();
}