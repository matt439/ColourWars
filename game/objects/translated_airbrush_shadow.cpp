#include "game/pch.h"
#include "game/objects/translated_airbrush_shadow.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/translated_draw_scope.h"
#include "game/objects/terrain_material.h"
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace mattmath;
using namespace labrador;

float TranslatedAirbrushShadow::checked_left(const TranslatingStructurePaintable& source,
    RenderResources* resources)
{
    if (resources == nullptr) { throw std::invalid_argument("A translated shadow needs render resources."); }
    const RectangleF rectangle = source.motion_state().spec.anchor;
    const SpriteSheet* sheet = resources->sprite_sheet(resources->resolve_sprite_sheet(TerrainMaterial::SHEET_NAME));
    const RectangleI tile = sheet->sprite_frame(sheet->resolve_sprite_frame("shadow_0")).source_rectangle();
    const float pitch = static_cast<float>(tile.width);
    if (!(pitch > 0.0f)) { throw std::invalid_argument("A translated shadow needs a positive anchor grid pitch."); }
    const float first = std::floor(rectangle.left() / pitch) * pitch;
    const float last = std::floor(rectangle.right() / pitch) * pitch;
    // The composed legacy recipe iterates float grid positions and casts a
    // cell to int64. Refuse coordinates at which that recipe cannot progress.
    const double lower = static_cast<double>((std::numeric_limits<long long>::min)());
    const double upper = -lower;
    if (!(pitch > 0.0f) || !std::isfinite(first) || !std::isfinite(last) ||
        !(first + pitch > first) || !(last + pitch > last) ||
        static_cast<double>(std::floor(first / pitch)) < lower ||
        !(static_cast<double>(std::floor(last / pitch)) < upper))
    {
        throw std::invalid_argument("A translated shadow needs a representable progressing anchor grid.");
    }
    return rectangle.left();
}
TranslatedAirbrushShadow::TranslatedAirbrushShadow(const TranslatingStructurePaintable& source,
    RenderResources* render_resources) :
    source_(source),
    anchor_(checked_left(source, render_resources), source.motion_state().spec.anchor.right(),
        source.motion_state().spec.anchor.bottom(), render_resources)
{
}
void TranslatedAirbrushShadow::update(float /*dt*/)
{
}
void TranslatedAirbrushShadow::draw(DrawList& list) const
{
    const TranslatedDrawScope camera(list, this->source_.translation());
    this->anchor_.draw(list);
}
RectangleF TranslatedAirbrushShadow::bounds() const
{
    RectangleF result = this->anchor_.bounds();
    const Vector2F offset = this->source_.translation();
    if (offset.x != 0.0f) { result.offset(offset.x, 0.0f); }
    return result;
}
RectangleF TranslatedAirbrushShadow::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->anchor_.cull_bounds(units_per_pixel);
    const Vector2F offset = this->source_.translation();
    if (offset.x != 0.0f) { result.offset(offset.x, 0.0f); }
    return result;
}
