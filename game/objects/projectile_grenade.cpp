#include "game/pch.h"
#include "game/objects/projectile_grenade.h"
#include "game/objects/projectile_ball.h"
#include "engine/math/scalar.h"

using namespace labrador;

ProjectileGrenade::ProjectileGrenade(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	RenderResources::SheetStrip body_strip,
	const ProjectileTuning* body,
	RenderResources::SheetStrip burst_strip,
	const ProjectileTuning* burst,
	int burst_count,
	float burst_speed,
	RoundShotShape shape) :
	Projectile(velocity, team, player_num, team_colour,
		ProjectileType::ball, render_resources, body_strip, body, team_colour),
	rectangle_(rectangle),
	shape_mode_(shape),
	render_resources_(render_resources),
	burst_strip_(burst_strip),
	burst_tuning_(burst),
	burst_count_(burst_count),
	burst_speed_(burst_speed)
{
	if (burst == nullptr || burst_count < 1)
	{
		throw std::invalid_argument("ProjectileGrenade needs a burst record and at least one drop");
	}
	const Vector2F size = this->details().col_rect_size;
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
	this->sync_circle();
}

void ProjectileGrenade::update(float dt)
{
	const ProjectileDetails& details = this->details();
	const Vector2F displacement = Projectile::update_movement(
		details.gravity, details.wind_resistance, dt);
	this->rectangle_.offset(displacement.x, displacement.y);
	this->sync_circle();
	colourwars::AnimationObject::update(dt);
}

void ProjectileGrenade::draw(DrawList& draw_list) const
{
	this->colourwars::AnimationObject::draw(draw_list, this->rectangle_);
}

RectangleF ProjectileGrenade::bounds() const
{
	return this->rectangle_;
}

const Shape* ProjectileGrenade::shape() const
{
	if (this->shape_mode_ == RoundShotShape::circle) { return &this->circle_; }
	return &this->rectangle_;
}

std::optional<float> ProjectileGrenade::sweep_radius() const
{
	if (this->shape_mode_ != RoundShotShape::circle) { return std::nullopt; }
	return this->circle_.radius();
}

void ProjectileGrenade::sync_circle()
{
	this->circle_ = Circle(this->rectangle_.center(),
		0.5f * std::min(this->rectangle_.width, this->rectangle_.height));
}

void ProjectileGrenade::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
	this->sync_circle();
}

std::vector<std::unique_ptr<Projectile>> ProjectileGrenade::take_burst()
{
	if (!this->for_deletion() || this->burst_)
	{
		return {};
	}
	this->burst_ = true;

	const Vector2F centre = this->rectangle_.center();
	const RectangleF start(centre, this->burst_tuning_->details.col_rect_size);
	std::vector<std::unique_ptr<Projectile>> drops;
	drops.reserve(static_cast<size_t>(this->burst_count_));
	for (int index = 0; index < this->burst_count_; ++index)
	{
		// Screen y grows downward, so -pi/2 is straight up.
		const float angle = -pi / 2.0f +
			2.0f * pi * static_cast<float>(index) / static_cast<float>(this->burst_count_);
		drops.push_back(std::make_unique<ProjectileBall>(start,
			Vector2F::unit_vec_from_angle(angle) * this->burst_speed_,
			this->team(), this->player_num(), this->team_colour(),
			this->render_resources_, this->burst_strip_, this->burst_tuning_,
			0.0f, Vector2F::zero, SpriteFlip::none, 0.0f, this->shape_mode_));
	}
	return drops;
}
