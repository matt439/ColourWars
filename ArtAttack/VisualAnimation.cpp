#include "pch.h"
#include "VisualAnimation.h"

//using namespace MattMath;
//using namespace DirectX;
//
//VisualAnimation::VisualAnimation(const float* dt,
//	const std::string& sheet_name,
//	const std::string& animation_strip_name,
//	const RectangleF& rectangle,
//	SpriteBatch* sprite_batch,
//	ResourceManager* resource_manager,
//	const Colour& color,
//	float rotation,
//	const Vector2F& origin,
//	SpriteEffects effects,
//	float layer_depth) :
//	AnimationObject(dt, sheet_name, animation_strip_name,
//		sprite_batch, resource_manager,
//		color, rotation, origin, effects, layer_depth),
//	_rectangle(rectangle)
//{
//
//}
//void VisualAnimation::update()
//{
//	this->AnimationObject::update();
//}
//void VisualAnimation::draw(const Camera& camera)
//{
//	this->AnimationObject::draw(this->_rectangle, camera);
//}
//void VisualAnimation::draw()
//{
//	this->AnimationObject::draw(this->_rectangle);
//}
//bool VisualAnimation::is_visible_in_viewport(const RectangleF& view) const
//{
//	return this->_rectangle.intersects(view);
//}