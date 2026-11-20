#pragma once

#include "game/objects/projectile.h"

class ProjectilePellet final : public Projectile
{
public:
    ProjectilePellet(const labrador::Vector2F& centre, const labrador::Vector2F& velocity,
        PlayerTeam team, int owner, const labrador::Colour& colour,
        labrador::RenderResources* resources, labrador::RenderResources::SheetStrip strip,
        const ProjectileTuning* tuning);
    void update(float dt) override;
    void draw(labrador::DrawList& list) const override;
    labrador::RectangleF bounds() const override { return body_; }
    labrador::RectangleF cull_bounds(float units_per_pixel) const override;
    const labrador::Shape* shape() const override { return &body_; }
private:
    void set_rectangle(const labrador::RectangleF& rectangle) override { body_ = rectangle; }
    labrador::RectangleF body_;
};
