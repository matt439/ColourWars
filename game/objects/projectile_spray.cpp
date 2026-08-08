#include "game/pch.h"
#include "game/objects/projectile_spray.h"

using namespace DirectX;
using namespace mattmath;
using namespace projectile_consts;
using namespace artattack;

ProjectileSpray::ProjectileSpray(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	DiffusingProjectile(velocity, team, player_num, team_colour,
		SPRAY, render_resources,
		DETAILS_SPRAY, DIFFUSION_DETAILS_SPRAY,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F size = this->details().col_rect_size;
	
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileSpray::update(float dt)
{
	const ProjectileDetails& details = this->details();
	
	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);

	this->rectangle_.inflate_to_size(calculate_diffusion_size());

	this->rectangle_.offset(displacement.x, displacement.y);

	AnimationObject::update(dt);
}
void ProjectileSpray::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->rectangle_);
}
RectangleF ProjectileSpray::bounds() const
{
	return this->rectangle_;
}
bool ProjectileSpray::is_colliding(const CollisionObject* other) const
{
	// type check
	if (!this->is_matching_collision_object_type(other))
	{
		return false;
	}

	// aabb check
	if (!this->shape()->AABB_intersects(other->shape()))
	{
		return false;
	}
	else // AABBs are intersecting
	{
		// if the other object is a rectangle, then we have a collision
		// since the AABB check passed
		ShapeType other_shape_type = other->shape()->shape_type();
		if (other_shape_type == ShapeType::rectangle)
		{
			return true;
		}
	}

	// narrow phase check
	if (this->shape()->intersects(other->shape()))
	{
		return true;
	}

	return false;
}
const Shape* ProjectileSpray::shape() const
{
	return &this->rectangle_;
}