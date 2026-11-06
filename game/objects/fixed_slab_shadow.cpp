#include "game/pch.h"
#include "game/objects/fixed_slab_shadow.h"
#include "game/objects/terrain_material.h"
#include <cmath>

using namespace labrador;

FixedSlabShadow::FixedSlabShadow(const FixedSlabGeometry& geometry,
    std::span<const std::pair<float, float>> exposed_bottom, RenderResources* resources) :
    TextureObject(TerrainMaterial::sheet_name, AirbrushShadow::first_tile, resources),
    geometry_(geometry), intervals_(exposed_bottom.begin(), exposed_bottom.end()),
    tiles_(AirbrushShadow::tiles(*this->sprite_sheet())), bounds_(geometry.bounds())
{
    for (const auto& [begin, end] : this->intervals_)
    {
        const RectangleF local(begin, FixedSlabGeometry::normal_thickness,
            end - begin, static_cast<float>(this->tiles_[0].height));
        this->bounds_ = RectangleF::union_of(this->bounds_, this->geometry_.world_quad(local).bounding_box());
    }
}

void FixedSlabShadow::draw(DrawList& list) const
{
    for (const auto& [begin, end] : this->intervals_)
    {
        AirbrushShadow::lay(this->tiles_, begin, end,
            [&](const RectangleI& source, float left, float right)
        {
            this->sprite_sheet()->draw(list, source,
                RectangleF(this->geometry_.to_world({ left, FixedSlabGeometry::normal_thickness }),
                    { right - left, static_cast<float>(source.height) }),
                Colour::white, this->geometry_.tangent().angle(), Vector2F::zero);
        });
    }
}

RectangleF FixedSlabShadow::bounds() const { return this->bounds_; }
RectangleF FixedSlabShadow::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->bounds_;
    result.inflate(3.0f * units_per_pixel);
    return result;
}
