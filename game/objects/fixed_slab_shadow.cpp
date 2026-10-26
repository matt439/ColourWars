#include "game/pch.h"
#include "game/objects/fixed_slab_shadow.h"
#include "game/objects/terrain_material.h"
#include <cmath>

using namespace mattmath;
using namespace labrador;

FixedSlabShadow::FixedSlabShadow(const FixedSlabGeometry& geometry,
    std::span<const std::pair<float, float>> exposed_bottom, RenderResources* resources) :
    TextureObject(TerrainMaterial::SHEET_NAME, "shadow_0", resources),
    geometry_(geometry), intervals_(exposed_bottom.begin(), exposed_bottom.end()), bounds_(geometry.bounds())
{
    for (std::size_t i = 0; i < this->tiles_.size(); ++i)
    {
        this->tiles_[i] = this->sprite_sheet()->sprite_frame(
            this->sprite_sheet()->resolve_sprite_frame("shadow_" + std::to_string(i))).source_rectangle();
    }
    for (const auto& [begin, end] : this->intervals_)
    {
        const RectangleF local(begin, FixedSlabGeometry::normal_thickness,
            end - begin, static_cast<float>(this->tiles_[0].height));
        this->bounds_ = RectangleF::union_of(this->bounds_, this->geometry_.world_quad(local).bounding_box());
    }
}

void FixedSlabShadow::draw(DrawList& list) const
{
    const float width = static_cast<float>(this->tiles_[0].width);
    for (const auto& [begin, end] : this->intervals_)
    {
        for (float x = std::floor(begin / width) * width; x < end; x += width)
        {
            const auto cell = static_cast<unsigned long long>(static_cast<long long>(std::floor(x / width)));
            const auto hash = cell * 2654435761ULL;
            RectangleI source = this->tiles_[static_cast<std::size_t>((hash >> 16) % this->tiles_.size())];
            const float left = std::max(x, begin), right = std::min(x + width, end);
            source.x += static_cast<int>(std::round(left - x));
            source.width = static_cast<int>(std::round(right - left));
            if (source.width <= 0) { continue; }
            this->sprite_sheet()->draw(list, source,
                RectangleF(this->geometry_.to_world({ left, FixedSlabGeometry::normal_thickness }),
                    { right - left, static_cast<float>(source.height) }),
                Colour::white, this->geometry_.tangent().angle(), Vector2F::ZERO);
        }
    }
}

RectangleF FixedSlabShadow::bounds() const { return this->bounds_; }
RectangleF FixedSlabShadow::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->bounds_;
    result.inflate(3.0f * units_per_pixel);
    return result;
}
