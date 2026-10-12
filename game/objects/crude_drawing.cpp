#include "game/pch.h"
#include "game/objects/crude_drawing.h"

using namespace labrador;
using namespace mattmath;

namespace
{
    // Bounded cosmetic work whatever the content asks for.
    constexpr int maximum_dabs = 256;
    constexpr int maximum_bands = 256;
}

Colour CrudeDrawing::premultiplied(const Colour& colour)
{
    return Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a);
}

CrudeDrawing::CrudeDrawing(const std::string& sheet_name, const std::string& fill_frame,
    RenderResources* render_resources) :
    TextureObject(sheet_name, fill_frame, render_resources)
{
}

void CrudeDrawing::fill(DrawList& draw_list, const RectangleF& rectangle,
    const Colour& colour) const
{
    if (rectangle.width <= 0.0f || rectangle.height <= 0.0f)
    {
        return;
    }
    this->draw_with(draw_list, rectangle, this->frame(), colour, Vector2F::ZERO,
        SpriteFlip::none, 0.0f);
}

void CrudeDrawing::frame_rectangle(DrawList& draw_list, const RectangleF& rectangle,
    float width, const Colour& colour) const
{
    this->fill(draw_list, RectangleF(rectangle.x, rectangle.y, rectangle.width, width), colour);
    this->fill(draw_list, RectangleF(rectangle.x, rectangle.bottom() - width,
        rectangle.width, width), colour);
    this->fill(draw_list, RectangleF(rectangle.x, rectangle.y + width,
        width, rectangle.height - width * 2.0f), colour);
    this->fill(draw_list, RectangleF(rectangle.right() - width, rectangle.y + width,
        width, rectangle.height - width * 2.0f), colour);
}

void CrudeDrawing::blob(DrawList& draw_list, const Vector2F& centre,
    const Vector2F& radius, float step, const Colour& colour) const
{
    this->dome(draw_list, centre, radius, step, colour);
    // The bottom half, mirrored.
    const int bands = std::clamp(static_cast<int>(std::ceil(radius.y / step)), 1, maximum_bands);
    for (int band = 0; band < bands; ++band)
    {
        const float y = (static_cast<float>(band) + 0.5f) * step;
        const float half = radius.x * std::sqrt(std::max(0.0f, 1.0f - (y * y) / (radius.y * radius.y)));
        this->fill(draw_list, RectangleF(centre.x - half, centre.y + band * step,
            half * 2.0f, step), colour);
    }
}

void CrudeDrawing::dome(DrawList& draw_list, const Vector2F& base_centre,
    const Vector2F& radius, float step, const Colour& colour) const
{
    const int bands = std::clamp(static_cast<int>(std::ceil(radius.y / step)), 1, maximum_bands);
    for (int band = 0; band < bands; ++band)
    {
        const float y = (static_cast<float>(band) + 0.5f) * step;
        const float half = radius.x * std::sqrt(std::max(0.0f, 1.0f - (y * y) / (radius.y * radius.y)));
        this->fill(draw_list, RectangleF(base_centre.x - half,
            base_centre.y - (band + 1) * step, half * 2.0f, step), colour);
    }
}

void CrudeDrawing::ring(DrawList& draw_list, const Vector2F& centre, float radius,
    float width, const Colour& colour) const
{
    const int dabs = std::clamp(static_cast<int>(radius * 6.2832f / (width * 0.8f)), 8, maximum_dabs);
    for (int dab = 0; dab < dabs; ++dab)
    {
        const float angle = 6.2832f * static_cast<float>(dab) / static_cast<float>(dabs);
        const Vector2F at = centre + Vector2F(std::cos(angle), std::sin(angle)) * radius;
        this->fill(draw_list, RectangleF(at - Vector2F(width / 2.0f), Vector2F(width)), colour);
    }
}

void CrudeDrawing::line(DrawList& draw_list, const Vector2F& from, const Vector2F& to,
    float width, const Colour& colour) const
{
    const Vector2F delta = to - from;
    const float length = delta.length();
    if (!std::isfinite(length))
    {
        return;
    }
    const int dabs = std::clamp(static_cast<int>(length / (width * 0.6f)) + 1, 1, maximum_dabs);
    for (int dab = 0; dab <= dabs; ++dab)
    {
        const Vector2F at = from + delta * (static_cast<float>(dab) / static_cast<float>(dabs));
        this->fill(draw_list, RectangleF(at - Vector2F(width / 2.0f), Vector2F(width)), colour);
    }
}
