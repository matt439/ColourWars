#include "game/pch.h"
#include "game/objects/projectile_jet.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

ProjectileJet::ProjectileJet(const RectangleF& rectangle,
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
		ProjectileType::jet, render_resources,
		tuning,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	Vector2F col_size = this->details().col_rect_size;
	
	this->rectangle_.offset(-col_size.x / 2.0f, -col_size.y / 2.0f);

	// The pressure streak follows its travel, independently of the square
	// collision volume. Sprite origins are source texels, not world units.
	const RectangleI& source = this->sprite_sheet()->animation_strip(
		this->sprite_sheet()->resolve_animation_strip(
			this->details().animation_strip_name)).frame_rect(0);
	this->set_origin(Vector2F(static_cast<float>(source.width) / 2.0f,
		static_cast<float>(source.height) / 2.0f));
	if (velocity.length_squared() > 0.0f)
	{
		this->set_draw_rotation(std::atan2(velocity.y, velocity.x));
	}
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
	this->AnimationObject::draw(draw_list,
		RectangleF(this->rectangle_.center(), this->details().size));
}
RectangleF ProjectileJet::bounds() const
{
	return this->rectangle_;
}
RectangleF ProjectileJet::cull_bounds(float units_per_pixel) const
{
	// A half diagonal encloses the visual rectangle at every aim angle.
	const float radius = this->details().size.length() / 2.0f +
		std::max(0.0f, units_per_pixel);
	return RectangleF::union_of(this->rectangle_, RectangleF(
		this->rectangle_.center() - Vector2F(radius, radius),
		Vector2F(radius * 2.0f, radius * 2.0f)));
}
const Shape* ProjectileJet::shape() const
{
	return &this->rectangle_;
}

void ProjectileJet::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}
