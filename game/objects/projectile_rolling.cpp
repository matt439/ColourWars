#include "game/pch.h"
#include "game/objects/projectile_rolling.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

ProjectileRolling::ProjectileRolling(const RectangleF& rectangle,
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
	Projectile(velocity, team, player_num, team_colour,
		ProjectileType::rolling, render_resources,
		tuning,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F col_size = this->details().col_rect_size;

	this->rectangle_.offset(-col_size.x / 2.0f, -col_size.y / 2.0f);
}
void ProjectileRolling::update(float dt)
{
	const ProjectileDetails& details = this->details();

	// Advance lifetime without moving this one-step volume from its spawn position.
	std::ignore = Projectile::update_movement(details.gravity,
		details.wind_resistance, dt);
}
void ProjectileRolling::on_contact(const CollisionObject& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
	// A rolling volume paints and damages everything it touches in its one
	// collision sweep. The floor must not consume it before a nearby player.
	// Its short lifetime retires it on the following update.
}
void ProjectileRolling::draw(DrawList& /*draw_list*/) const
{
}
RectangleF ProjectileRolling::bounds() const
{
	return this->rectangle_;
}
const Shape* ProjectileRolling::shape() const
{
	return &this->rectangle_;
}
void ProjectileRolling::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}
