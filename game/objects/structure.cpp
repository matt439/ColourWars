#include "game/pch.h"
#include "game/objects/structure.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

Structure::Structure(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	std::unique_ptr<mattmath::Shape> collision_shape,
	RenderResources* render_resources,
	CollisionObjectType collision_type,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth,
	std::unique_ptr<const TerrainMaterial> material) :
	TextureObject(sheet_name, frame_name, render_resources,
		color, rotation, origin, flip, layer_depth),
	collision_type_(collision_type),
	sprite_rectangle_(sprite_rectangle),
	collision_shape_(std::move(collision_shape)),
	material_(std::move(material))
{
	if (this->collision_shape_ == nullptr)
	{
		throw std::invalid_argument("A structure requires a collision shape.");
	}
}

void Structure::update(float /*dt*/)
{
	
}
void Structure::draw(DrawList& draw_list) const
{
	if (this->material_ != nullptr)
	{
		this->material_->paint(draw_list);
		return;
	}
	this->TextureObject::draw(draw_list, this->sprite_rectangle_);
}
RectangleF Structure::bounds() const
{
	return this->sprite_rectangle_;
}
void Structure::on_contact(const CollisionObject& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
}
CollisionLayer Structure::layer() const
{
	return collision_layer(this->collision_type_);
}
CollisionMask Structure::mask() const
{
	return collision_mask(this->collision_type_);
}
CollisionTag Structure::tag() const
{
	return to_collision_tag(this->collision_type_);
}
CollisionObjectType Structure::collision_type() const
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
const TerrainMaterial* Structure::material() const
{
	return this->material_.get();
}
const RectangleF& Structure::rectangle() const
{
	return this->sprite_rectangle_;
}
