#include "game/pch.h"
#include "game/objects/projectile_spray.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

ProjectileSpray::ProjectileSpray(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	const ProjectileTuning* tuning,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	DiffusingProjectile(velocity, team, player_num, team_colour,
		ProjectileType::spray, render_resources,
		tuning,
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

	this->diffuse_collision_rectangle(this->rectangle_);

	this->rectangle_.offset(displacement.x, displacement.y);

	AnimationObject::update(dt);
}
void ProjectileSpray::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->sprite_rectangle(this->rectangle_));
}
RectangleF ProjectileSpray::bounds() const
{
	return RectangleF::union_of(this->rectangle_,
		this->sprite_rectangle(this->rectangle_));
}
const Shape* ProjectileSpray::shape() const
{
	return &this->rectangle_;
}
void ProjectileSpray::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}
