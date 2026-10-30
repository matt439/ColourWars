#include "game/pch.h"
#include "game/objects/structure.h"

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
		if (this->miniature_) { this->material_->paint_miniature(draw_list); }
		else { this->material_->paint(draw_list); }
		return;
	}
	this->TextureObject::draw(draw_list, this->sprite_rectangle_);
}
void Structure::draw_miniature(DrawList& draw_list) const
{
	if (this->material_ != nullptr)
	{
		this->material_->paint_miniature(draw_list);
		return;
	}
	this->TextureObject::draw(draw_list, this->sprite_rectangle_);
}
RectangleF Structure::bounds() const
{
	return this->sprite_rectangle_;
}
void Structure::touch(Collider& receiver, const Vector2F& normal, float penetration) const
{
	receiver.on_structure_contact(*this, normal, penetration);
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
bool Structure::jump_through() const
{
	return this->collision_type_ == CollisionObjectType::structure_jump_through;
}
bool Structure::ramp() const
{
	return is_structure_ramp(this->collision_type_);
}
bool Structure::ramp_rises_right() const
{
	return this->collision_type_ == CollisionObjectType::structure_ramp_right;
}
bool Structure::flat() const
{
	return !this->ramp() && this->slab_geometry() == nullptr;
}
PlayerTeam Structure::top_paint_team(float /*x*/) const
{
	return PlayerTeam::none;
}
std::optional<Colour> Structure::top_paint_colour(float /*x*/) const
{
	return std::nullopt;
}
bool Structure::passable_for(PlayerTeam /*team*/) const
{
	return false;
}
Vector2F Structure::cover_point(const Vector2F& shot_centre, const Vector2F& normal) const
{
	// The normal points from the shot into this cover. Anchor the cue on its
	// actual face, with tangential position clamped to the face (including
	// corner hits and very thin walls).
	const RectangleF cover = this->shape()->bounding_box();
	Vector2F point(std::clamp(shot_centre.x, cover.left(), cover.right()),
		std::clamp(shot_centre.y, cover.top(), cover.bottom()));
	if (std::abs(normal.x) >= std::abs(normal.y))
	{
		point.x = normal.x > 0.0f ? cover.left() : cover.right();
	}
	else
	{
		point.y = normal.y > 0.0f ? cover.top() : cover.bottom();
	}
	return point;
}
const RectangleF& Structure::rectangle() const
{
	return this->sprite_rectangle_;
}
