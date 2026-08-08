#include "game/pch.h"
#include "game/objects/projectile_jet.h"

using namespace DirectX;
using namespace mattmath;
using namespace projectile_consts;
using namespace artattack;

ProjectileJet::ProjectileJet(const RectangleF& rectangle,
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
		JET, render_resources,
		DETAILS_JET,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F col_size = this->details().col_rect_size;
	
	this->rectangle_.offset(-col_size.x / 2.0f, -col_size.y / 2.0f);
}

void ProjectileJet::update(float dt)
{
	const ProjectileDetails& details = this->details();

	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);

	this->rectangle_.offset(displacement.x, displacement.y);

	AnimationObject::update(dt);
}
void ProjectileJet::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->rectangle_);
}
RectangleF ProjectileJet::bounds() const
{
	return this->rectangle_;
}
const Shape* ProjectileJet::shape() const
{
	return &this->rectangle_;
}
