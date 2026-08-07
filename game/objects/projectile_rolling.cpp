#include "game/pch.h"
#include "game/objects/projectile_rolling.h"

using namespace DirectX;
using namespace mattmath;
using namespace projectile_consts;
using namespace artattack;

ProjectileRolling::ProjectileRolling(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	const float* dt,
	RenderResources* render_resources,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	Projectile(velocity, team, player_num, team_colour,
		ROLLING, dt, render_resources,
		DETAILS_ROLLING,
		team_colour, rotation, origin, effects, layer_depth),
	rectangle_(rectangle)
{
	Vector2F col_size = this->details().col_rect_size;

	this->rectangle_.offset(-col_size.x / 2.0f, -col_size.y / 2.0f);
}
void ProjectileRolling::update()
{
	const ProjectileDetails& details = this->details();

	Projectile::update_movement(details.gravity,
		details.wind_resistance);
}
void ProjectileRolling::draw(SpriteBatch* /*sprite_batch*/,
	const Camera& /*camera*/) const
{
	// do nothing
}
void ProjectileRolling::draw(SpriteBatch* /*sprite_batch*/) const
{
	// do nothing
}
bool ProjectileRolling::is_visible_in_viewport(const RectangleF& /*view*/) const
{
	return false;
}
bool ProjectileRolling::is_colliding(const ICollisionGameObject* other) const
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
const Shape* ProjectileRolling::shape() const
{
	return &this->rectangle_;
}