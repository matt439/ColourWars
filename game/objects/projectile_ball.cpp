#include "game/pch.h"
#include "game/objects/projectile_ball.h"

using namespace mattmath;
using namespace DirectX;
using namespace projectile_consts;
using namespace artattack;

ProjectileBall::ProjectileBall(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	Projectile(velocity, team, player_num, team_colour,
		BALL, render_resources,
		DETAILS_BALL,
		team_colour, rotation, origin, effects, layer_depth),
	rectangle_(rectangle)
{
	Vector2F size = this->details().col_rect_size;
	
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileBall::update(float dt)
{
	const ProjectileDetails& details = this->details();
	
	Projectile::update_movement(details.gravity,
		details.wind_resistance, dt);

	this->rectangle_.offset(MovingObject::dx_x(),
		MovingObject::dx_y());

	AnimationObject::update(dt);
}
void ProjectileBall::draw(SpriteBatch* sprite_batch,
	const Camera& camera) const
{
	this->AnimationObject::draw(sprite_batch, this->rectangle_, camera);
}
RectangleF ProjectileBall::bounds() const
{
	return this->rectangle_;
}
bool ProjectileBall::is_colliding(const ICollisionGameObject* other) const
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
const Shape* ProjectileBall::shape() const
{
	return &this->rectangle_;
}