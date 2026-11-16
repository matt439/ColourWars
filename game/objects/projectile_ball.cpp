#include "game/pch.h"
#include "game/objects/projectile_ball.h"

using namespace labrador;

ProjectileBall::ProjectileBall(const RectangleF& rectangle,
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
	float layer_depth,
	RoundShotShape shape) :
	Projectile(velocity, team, player_num, team_colour,
		ProjectileType::ball, render_resources, strip,
		tuning,
		team_colour, rotation, origin, flip, layer_depth),
	rectangle_(rectangle),
	shape_mode_(shape)
{
	Vector2F size = this->details().col_rect_size;
	
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
	this->sync_circle();
}

void ProjectileBall::update(float dt)
{
	const ProjectileDetails& details = this->details();
	
	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);

	this->rectangle_.offset(displacement.x, displacement.y);
	this->sync_circle();

	colourwars::AnimationObject::update(dt);
}
void ProjectileBall::draw(DrawList& draw_list) const
{
	this->colourwars::AnimationObject::draw(draw_list, this->rectangle_);
}
RectangleF ProjectileBall::bounds() const
{
	return this->rectangle_;
}
const Shape* ProjectileBall::shape() const
{
	if (this->shape_mode_ == RoundShotShape::circle) { return &this->circle_; }
	return &this->rectangle_;
}
std::optional<float> ProjectileBall::sweep_radius() const
{
	if (this->shape_mode_ != RoundShotShape::circle) { return std::nullopt; }
	return this->circle_.radius();
}
void ProjectileBall::sync_circle()
{
	this->circle_ = Circle(this->rectangle_.center(),
		0.5f * std::min(this->rectangle_.width, this->rectangle_.height));
}
void ProjectileBall::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
	this->sync_circle();
}
