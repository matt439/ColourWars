#pragma once

#include "game/objects/fixed_slab_geometry.h"
#include "game/objects/airbrush_shadow.h"
#include <span>
#include <utility>
#include <vector>

// Value-owned static composition: no borrowed temporary validation owner.
class FixedSlabShadow final : public labrador::GameObject, public labrador::TextureObject
{
public:
    FixedSlabShadow(const FixedSlabGeometry& geometry,
        std::span<const std::pair<float, float>> exposed_bottom,
        labrador::RenderResources* resources);
    void update(float) override {}
    void draw(labrador::DrawList& list) const override;
    mattmath::RectangleF bounds() const override;
    mattmath::RectangleF cull_bounds(float units_per_pixel) const override;
private:
    FixedSlabGeometry geometry_;
    std::vector<std::pair<float, float>> intervals_;
    std::array<mattmath::RectangleI, AirbrushShadow::VARIANTS> tiles_{};
    mattmath::RectangleF bounds_;
};
