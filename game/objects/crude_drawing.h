#pragma once

#include "engine/render/texture_object.h"
#include "engine/render/colour.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"

// Shapes drawn the way the person draws: a mouse, the rectangle tool and a
// shaky hand. Everything is one stretched solid frame, so round things come
// out stepped, which is the look (docs/backstory.md; the art is Paint-style).
// The drawings of the opening and the reveal derive from it.
class CrudeDrawing : public labrador::TextureObject
{
public:
    // A translucent tint for the premultiplied blend: the engine multiplies
    // the texel by the tint and blends ONE, INV_SRC_ALPHA.
    static labrador::Colour premultiplied(const labrador::Colour& colour);

protected:
    CrudeDrawing(const std::string& sheet_name, const std::string& fill_frame,
        labrador::RenderResources* render_resources);

    void fill(labrador::DrawList& draw_list, const mattmath::RectangleF& rectangle,
        const labrador::Colour& colour) const;
    // An outline drawn inside the rectangle.
    void frame_rectangle(labrador::DrawList& draw_list,
        const mattmath::RectangleF& rectangle, float width,
        const labrador::Colour& colour) const;
    // A filled circle or ellipse in horizontal bands `step` high.
    void blob(labrador::DrawList& draw_list, const mattmath::Vector2F& centre,
        const mattmath::Vector2F& radius, float step, const labrador::Colour& colour) const;
    // Only the top half of blob(): shoulders, a mound.
    void dome(labrador::DrawList& draw_list, const mattmath::Vector2F& base_centre,
        const mattmath::Vector2F& radius, float step, const labrador::Colour& colour) const;
    // A ring of square dabs.
    void ring(labrador::DrawList& draw_list, const mattmath::Vector2F& centre,
        float radius, float width, const labrador::Colour& colour) const;
    // A line of square dabs, as a mouse drags it.
    void line(labrador::DrawList& draw_list, const mattmath::Vector2F& from,
        const mattmath::Vector2F& to, float width, const labrador::Colour& colour) const;
};
