#include "game/pch.h"
#include "game/objects/projectile_mist.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

ProjectileMist::ProjectileMist(const RectangleF& rectangle,
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
	// MIST, not SPRAY. It passed SPRAY from the day it was written, so every
	// mist particle reported itself as spray to collision_type() - the one
	// reader of type() - and was tagged projectile_spray_team_*. Invisible
	// while the two shared a layer, a mask and a damage number to four decimal
	// places; not invisible now that the damage is a number in a file keyed by
	// exactly this enum, and a mist tuned differently from spray would have
	// been applied as spray.
	DiffusingProjectile(velocity, team, player_num, team_colour,
		MIST, render_resources,
		tuning,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F size = this->details().col_rect_size;

	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileMist::update(float dt)
{
	const ProjectileDetails& details = this->details();

	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);

	this->rectangle_.inflate_to_size(calculate_diffusion_size());

	this->rectangle_.offset(displacement.x, displacement.y);

	AnimationObject::update(dt);
}
void ProjectileMist::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->rectangle_);
}
RectangleF ProjectileMist::bounds() const
{
	return this->rectangle_;
}
const Shape* ProjectileMist::shape() const
{
	return &this->rectangle_;
}