#include "game/pch.h"
#include "game/objects/projectile_planted.h"
#include "game/objects/projectile_ball.h"
#include "engine/math/scalar.h"

using namespace labrador;

ProjectilePlanted::ProjectilePlanted(const RectangleF& rectangle,
	const Vector2F& velocity,
	PlayerTeam team,
	int player_num,
	const Colour& team_colour,
	RenderResources* render_resources,
	RenderResources::SheetStrip body_strip,
	RenderResources::SheetStrip spray_strip,
	const SubTuning* sub) :
	Projectile(velocity, team, player_num, team_colour, ProjectileType::ball,
		render_resources, body_strip, sub != nullptr ? &sub->body : nullptr, team_colour),
	rectangle_(rectangle),
	render_resources_(render_resources),
	spray_strip_(spray_strip),
	sub_(sub)
{
	if (sub == nullptr || sub->burst_count < 1 || !(sub->interval > 0.0f) || sub->hits < 1)
	{
		throw std::invalid_argument("A planted sub needs its sub record, with a schedule and hits");
	}
	this->hits_left_ = sub->hits;
	const Vector2F size = this->details().col_rect_size;
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectilePlanted::update(float dt)
{
	if (this->planted_)
	{
		if (dt > 0.0f && !this->for_deletion())
		{
			this->next_spray_ -= dt;
			this->life_left_ -= dt;
			if (this->life_left_ <= 0.0f) { this->set_for_deletion(true); }
		}
	}
	else
	{
		const ProjectileDetails& details = this->details();
		const Vector2F displacement = Projectile::update_movement(
			details.gravity, details.wind_resistance, dt);
		this->rectangle_.offset(displacement.x, displacement.y);
	}
	colourwars::AnimationObject::update(dt);
}

void ProjectilePlanted::draw(DrawList& draw_list) const
{
	this->colourwars::AnimationObject::draw(draw_list, this->rectangle_);
}

RectangleF ProjectilePlanted::bounds() const
{
	return this->rectangle_;
}

const Shape* ProjectilePlanted::shape() const
{
	return &this->rectangle_;
}

CollisionMask ProjectilePlanted::mask() const
{
	return this->planted_ ? CollisionMask{ 0 } : Projectile::mask();
}

void ProjectilePlanted::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}

void ProjectilePlanted::resolve_travel(std::span<CollisionObject* const> objects,
	std::optional<ProjectileTargetFlags> known)
{
	if (!this->planted_) { Projectile::resolve_travel(objects, known); }
}

void ProjectilePlanted::on_structure_contact(const Structure& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
	if (this->planted_ || this->for_deletion()) { return; }
	// The sweep has already moved it to where the cover stopped it.
	this->planted_ = true;
	this->set_velocity(Vector2F::zero);
	this->life_left_ = this->sub_->lifetime;
	this->next_spray_ = 0.0f;
}

float ProjectilePlanted::player_damage() const
{
	return this->planted_ ? 0.0f : Projectile::player_damage();
}

bool ProjectilePlanted::takes_hits() const
{
	return this->planted_ && !this->for_deletion();
}

void ProjectilePlanted::take_hit()
{
	if (!this->takes_hits()) { return; }
	if (--this->hits_left_ <= 0) { this->set_for_deletion(true); }
}

std::unique_ptr<Projectile> ProjectilePlanted::drop(const Vector2F& centre, const Vector2F& velocity) const
{
	return std::make_unique<ProjectileBall>(RectangleF(centre, this->sub_->burst.details.col_rect_size),
		velocity, this->team(), this->player_num(), this->team_colour(),
		this->render_resources_, this->spray_strip_, &this->sub_->burst);
}

std::vector<std::unique_ptr<Projectile>> ProjectilePlanted::take_burst()
{
	if (!this->planted_ || this->for_deletion() || this->next_spray_ > 0.0f)
	{
		return {};
	}
	this->next_spray_ += this->sub_->interval;
	std::vector<std::unique_ptr<Projectile>> drops = this->spray(this->sprays_);
	++this->sprays_;
	return drops;
}
