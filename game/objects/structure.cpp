#include "game/pch.h"
#include "game/objects/structure.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

Structure::Structure(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	const mattmath::Shape* collision_shape,
	RenderResources* render_resources,
	CollisionObjectType collision_type,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	TextureObject(sheet_name, frame_name, render_resources,
		color, rotation, origin, effects, layer_depth),
	collision_type_(collision_type),
	sprite_rectangle_(sprite_rectangle),
	collision_shape_(collision_shape->clone())
{

}

void Structure::update(float /*dt*/)
{
	
}
void Structure::draw(SpriteBatch* sprite_batch, const Camera& camera) const
{
	this->TextureObject::draw(sprite_batch, this->sprite_rectangle_, camera);
}
RectangleF Structure::bounds() const
{
	return this->sprite_rectangle_;
}
bool Structure::is_colliding(const CollisionObject* /*other*/) const
{
	return false;
}
void Structure::on_collision(const CollisionObject* /*other*/)
{
	// do nothing
}
CollisionObjectType Structure::collision_object_type() const
{
	return this->collision_type_;
}
const Shape* Structure::shape() const
{
	return this->collision_shape_.get();
}
bool Structure::for_deletion() const
{
	return false;
}
const RectangleF& Structure::rectangle() const
{
	return this->sprite_rectangle_;
}