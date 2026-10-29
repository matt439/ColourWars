#include "game/pch.h"
#include "game/objects/projectile_mist.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

ProjectileMist::ProjectileMist(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	RenderResources::SheetStrip strip,
	const ProjectileTuning* tuning,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	DiffusingProjectile(velocity, team, player_num, team_colour,
		ProjectileType::mist, render_resources, strip,
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

	this->diffuse_collision_rectangle(this->rectangle_);

	this->rectangle_.offset(displacement.x, displacement.y);

	colourwars::AnimationObject::update(dt);
}
void ProjectileMist::draw(DrawList& draw_list) const
{
	this->colourwars::AnimationObject::draw(draw_list, this->sprite_rectangle(this->rectangle_));
}
RectangleF ProjectileMist::bounds() const
{
	return RectangleF::union_of(this->rectangle_,
		this->sprite_rectangle(this->rectangle_));
}
const Shape* ProjectileMist::shape() const
{
	return &this->rectangle_;
}
void ProjectileMist::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}
