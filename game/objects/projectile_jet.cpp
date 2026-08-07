#include "game/pch.h"
#include "game/objects/projectile_jet.h"

using namespace DirectX;
using namespace mattmath;
using namespace projectile_consts;

ProjectileJet::ProjectileJet(const RectangleF& rectangle,
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
		JET, dt, render_resources,
		DETAILS_JET,
		team_colour, rotation, origin, effects, layer_depth),
	rectangle_(rectangle)
{
	Vector2F col_size = this->get_details().col_rect_size;
	
	this->rectangle_.offset(-col_size.x / 2.0f, -col_size.y / 2.0f);
}

void ProjectileJet::update()
{
	const ProjectileDetails& details = this->get_details();

	Projectile::update_movement(details.gravity,
		details.wind_resistance);

	this->rectangle_.offset(MovingObject::get_dx_x(),
		MovingObject::get_dx_y());

	AnimationObject::update();
}
void ProjectileJet::draw(SpriteBatch* sprite_batch, const Camera& camera)
{
	this->AnimationObject::draw(sprite_batch, this->rectangle_, camera);
}
void ProjectileJet::draw(SpriteBatch* sprite_batch)
{
	this->AnimationObject::draw(sprite_batch, this->rectangle_);
}
bool ProjectileJet::is_visible_in_viewport(const RectangleF& view) const
{
	return this->rectangle_.intersects(view);
}
bool ProjectileJet::is_colliding(const ICollisionGameObject* other) const
{
	// type check
	if (!this->is_matching_collision_object_type(other))
	{
		return false;
	}

	// aabb check
	if (!this->get_shape()->AABB_intersects(other->get_shape()))
	{
		return false;
	}
	else // AABBs are intersecting
	{
		// if the other object is a rectangle, then we have a collision
		// since the AABB check passed
		ShapeType other_shape_type = other->get_shape()->get_shape_type();
		if (other_shape_type == ShapeType::rectangle)
		{
			return true;
		}
	}

	// narrow phase check
	if (this->get_shape()->intersects(other->get_shape()))
	{
		return true;
	}

	return false;
}
const Shape* ProjectileJet::get_shape() const
{
	return &this->rectangle_;
}
