#include "game/pch.h"
#include "game/objects/projectile_pellet.h"

using namespace labrador;

ProjectilePellet::ProjectilePellet(const Vector2F& centre, const Vector2F& velocity,
    PlayerTeam team, int owner, const Colour& colour, RenderResources* resources,
    RenderResources::SheetStrip strip, const ProjectileTuning* tuning) :
    Projectile(velocity, team, owner, colour, ProjectileType::pellet, resources, strip, tuning, colour),
    body_(centre - tuning->details.col_rect_size * 0.5f, tuning->details.col_rect_size)
{
}

void ProjectilePellet::update(float dt)
{
    const auto displacement = update_movement(details().gravity, details().wind_resistance, dt);
    body_.offset(displacement.x, displacement.y);
    colourwars::AnimationObject::update(dt);
}

void ProjectilePellet::draw(DrawList& list) const
{
    colourwars::AnimationObject::draw(list, RectangleF(body_.center() - details().size * 0.5f, details().size));
}

RectangleF ProjectilePellet::cull_bounds(float /*units_per_pixel*/) const
{
    return RectangleF::union_of(body_, RectangleF(body_.center() - details().size * 0.5f, details().size));
}
