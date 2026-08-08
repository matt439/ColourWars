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
	SpriteFlip flip,
	float layer_depth) :
	Projectile(velocity, team, player_num, team_colour,
		BALL, render_resources,
		DETAILS_BALL,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F size = this->details().col_rect_size;
	
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileBall::update(float dt)
{
	const ProjectileDetails& details = this->details();
	
	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);

	this->rectangle_.offset(displacement.x, displacement.y);

	AnimationObject::update(dt);
}
void ProjectileBall::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->rectangle_);
}
RectangleF ProjectileBall::bounds() const
{
	return this->rectangle_;
}
const Shape* ProjectileBall::shape() const
{
	return &this->rectangle_;
}