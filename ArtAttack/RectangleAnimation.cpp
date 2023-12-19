#include "pch.h"
#include "RectangleAnimation.h"

using namespace DirectX;
using namespace MattMath;

//RectangleAnimation::RectangleAnimation(const float* dt,
//	const std::string& sheet_name,
//	const std::string& animation_strip_name,
//	const MattMath::RectangleF& rectangle,
//	DirectX::SpriteBatch* sprite_batch,
//	ResourceManager* resource_manager,
//	const MattMath::Colour& color,
//	float rotation,
//	const MattMath::Vector2F& origin,
//	DirectX::SpriteEffects effects,
//	float layer_depth) :
//	AnimationObject(sheet_name, animation_strip_name, sprite_batch,
//		resource_manager, color, rotation, origin, effects,
//		layer_depth),
//	_dt(dt),
//	_rectangle(rectangle)
//{
//	std::make_unique<AnimatedSprite>(this->_sprite_sheet.get(),
//				this->_animation_strip_name, this->_dt);
//}

//const RectangleF& RectangleAnimation::get_rectangle() const
//{
//	return this->_rectangle;
//}
////const RectangleI RectangleAnimation::get_rectangle_i() const
////{
////	return this->_rectangle.get_rectangle_i();
////}
//void RectangleAnimation::set_rectangle(const RectangleF& rectangle)
//{
//	this->_rectangle = rectangle;
//}
//

void RectangleAnimation::draw()
{
	this->AnimationObject::draw(RectangleObject::get_rectangle());
}