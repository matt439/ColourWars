#include "game/pch.h"
#include "game/objects/structure.h"

using namespace DirectX;
using namespace mattmath;

Structure::Structure(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	const mattmath::Shape* collision_shape,
	RenderResources* render_resources,
	collision_object_type collision_type,
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

void Structure::update()
{
	
}
void Structure::draw(SpriteBatch* sprite_batch, const Camera& camera)
{
	this->TextureObject::draw(sprite_batch, this->sprite_rectangle_, camera);
}
void Structure::draw(SpriteBatch* sprite_batch)
{
	this->TextureObject::draw(sprite_batch, this->sprite_rectangle_);
}
bool Structure::is_visible_in_viewport(const RectangleF& view) const
{
	return this->sprite_rectangle_.intersects(view);
}
bool Structure::is_colliding(const ICollisionGameObject* /*other*/) const
{
	return false;
}
void Structure::on_collision(const ICollisionGameObject* /*other*/)
{
	// do nothing
}
collision_object_type Structure::get_collision_object_type() const
{
	return this->collision_type_;
}
const Shape* Structure::get_shape() const
{
	return this->collision_shape_.get();
}
bool Structure::get_for_deletion() const
{
	return false;
}
const RectangleF& Structure::get_rectangle() const
{
	return this->sprite_rectangle_;
}