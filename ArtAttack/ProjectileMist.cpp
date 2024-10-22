#include "pch.h"
#include "ProjectileMist.h"

using namespace DirectX;
using namespace MattMath;
using namespace projectile_consts;

ProjectileMist::ProjectileMist(const RectangleF& rectangle,
	const Vector2F& velocity,
	player_team team,
	int player_num,
	const Colour& team_colour,
	const float* dt,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	DiffusingProjectile(velocity, team, player_num, team_colour,
		SPRAY, dt,
		sprite_batch, resource_manager,
		DETAILS_MIST, DIFFUSION_DETAILS_MIST,
		team_colour, rotation, origin, effects, layer_depth),
	_rectangle(rectangle)
{
	Vector2F size = this->get_details().col_rect_size;

	this->_rectangle.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileMist::update()
{
	const ProjectileDetails& details = this->get_details();

	Projectile::update_movement(details.gravity,
		details.wind_resistance);

	this->_rectangle.inflate_to_size(calculate_diffusion_size());

	this->_rectangle.offset(MovingObject::get_dx_x(),
		MovingObject::get_dx_y());

	AnimationObject::update();
}
void ProjectileMist::draw(const Camera& camera)
{
	this->AnimationObject::draw(this->_rectangle, camera);
}
void ProjectileMist::draw()
{
	this->AnimationObject::draw(this->_rectangle);
}
bool ProjectileMist::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}
bool ProjectileMist::is_colliding(const ICollisionGameObject* other) const
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
		shape_type other_shape_type = other->get_shape()->get_shape_type();
		if (other_shape_type == shape_type::RECTANGLE)
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
const Shape* ProjectileMist::get_shape() const
{
	return &this->_rectangle;
}