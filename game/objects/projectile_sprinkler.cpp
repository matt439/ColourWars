#include "game/pch.h"
#include "game/objects/projectile_sprinkler.h"
#include "game/objects/projectile_ball.h"
#include "engine/math/scalar.h"

using namespace labrador;

ProjectileSprinkler::ProjectileSprinkler(const RectangleF& rectangle,
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
		throw std::invalid_argument("ProjectileSprinkler needs a sprinkler's sub record");
	}
	this->hits_left_ = sub->hits;
	const Vector2F size = this->details().col_rect_size;
	this->rectangle_.offset(-size.x / 2.0f, -size.y / 2.0f);
}

void ProjectileSprinkler::update(float dt)
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

void ProjectileSprinkler::draw(DrawList& draw_list) const
{
	this->colourwars::AnimationObject::draw(draw_list, this->rectangle_);
}

RectangleF ProjectileSprinkler::bounds() const
{
	return this->rectangle_;
}

const Shape* ProjectileSprinkler::shape() const
{
	return &this->rectangle_;
}

CollisionMask ProjectileSprinkler::mask() const
{
	return this->planted_ ? CollisionMask{ 0 } : Projectile::mask();
}

void ProjectileSprinkler::set_rectangle(const RectangleF& rectangle)
{
	this->rectangle_ = rectangle;
}

void ProjectileSprinkler::resolve_travel(std::span<CollisionObject* const> objects)
{
	if (!this->planted_) { Projectile::resolve_travel(objects); }
}

void ProjectileSprinkler::on_structure_contact(const Structure& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
	if (this->planted_ || this->for_deletion()) { return; }
	// The sweep has already moved it to where the cover stopped it.
	this->planted_ = true;
	this->set_velocity(Vector2F::zero);
	this->life_left_ = this->sub_->lifetime;
	this->next_spray_ = 0.0f;
}

float ProjectileSprinkler::player_damage() const
{
	return this->planted_ ? 0.0f : Projectile::player_damage();
}

bool ProjectileSprinkler::takes_hits() const
{
	return this->planted_ && !this->for_deletion();
}

void ProjectileSprinkler::take_hit()
{
	if (!this->takes_hits()) { return; }
	if (--this->hits_left_ <= 0) { this->set_for_deletion(true); }
}

std::vector<std::unique_ptr<Projectile>> ProjectileSprinkler::take_burst()
{
	if (!this->planted_ || this->for_deletion() || this->next_spray_ > 0.0f)
	{
		return {};
	}
	this->next_spray_ += this->sub_->interval;
	const int count = this->sub_->burst_count;
	const bool turned = this->sprays_ % 2 == 1;
	const RectangleF start(this->rectangle_.center(), this->sub_->burst.details.col_rect_size);
	std::vector<std::unique_ptr<Projectile>> drops;
	drops.reserve(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		// Screen y grows downward, so pi to 2pi runs from straight left over
		// the top to straight right. An even spray spans both ends; a turned
		// one sits each drop in the middle of its share of the half circle.
		const float share = turned ? (static_cast<float>(index) + 0.5f) / static_cast<float>(count) :
			count > 1 ? static_cast<float>(index) / static_cast<float>(count - 1) : 0.5f;
		const float angle = pi + pi * share;
		drops.push_back(std::make_unique<ProjectileBall>(start,
			Vector2F::unit_vec_from_angle(angle) * this->sub_->burst_speed,
			this->team(), this->player_num(), this->team_colour(),
			this->render_resources_, this->spray_strip_, &this->sub_->burst));
	}
	++this->sprays_;
	return drops;
}
