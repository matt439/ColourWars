#pragma once

#include "engine/render/texture_object.h"
#include "engine/render/colour.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"

// Shapes drawn the way the person draws: a mouse and the rectangle tool.
// Everything is one stretched solid frame, so round things come out stepped,
// which is the look (docs/backstory.md; the art is Paint-style). There is no
// jitter: any slip is authored where it is drawn
// (docs/drawing-mistakes-research.md).
// The drawings of the opening and the reveal derive from it.
class CrudeDrawing : public labrador::TextureObject
{
public:
    // A translucent tint for the premultiplied blend: the engine multiplies
    // the texel by the tint and blends ONE, INV_SRC_ALPHA.
    static labrador::Colour premultiplied(const labrador::Colour& colour);

    // The stepped paper card every drawn panel shares: a shade card at
    // `shadow`, an ink card at `face`, paper one pen inside it, then three
    // quiet airbrush passes inside the paper's lower and right edges. Each
    // card is two overlapping rectangles, so its corners step in by two pens.
    // Colours are straight; `draw(rectangle, colour)` premultiplies, which
    // lets a drawing that is not a CrudeDrawing share the card.
    template <typename Fill>
    static void paper_card(const Fill& draw, const labrador::RectangleF& shadow,
        const labrador::RectangleF& face, float pen, const labrador::Colour& shade,
        const labrador::Colour& ink, const labrador::Colour& paper)
    {
        const float step = pen * 2.0f;
        const auto card = [&](const labrador::RectangleF& box, const labrador::Colour& colour)
        {
            draw(labrador::RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), colour);
            draw(labrador::RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), colour);
        };
        card(shadow, shade);
        card(face, ink);
        const labrador::RectangleF inside(face.x + pen, face.y + pen,
            face.width - pen * 2.0f, face.height - pen * 2.0f);
        card(inside, paper);
        for (int pass = 3; pass > 0; --pass)
        {
            labrador::Colour airbrush = shade;
            airbrush.a *= 0.12f;
            const float width = pen * static_cast<float>(pass);
            draw(labrador::RectangleF(inside.x + step, inside.bottom() - width,
                inside.width - step * 2.0f, width), airbrush);
            draw(labrador::RectangleF(inside.right() - width, inside.y + step,
                width, inside.height - step * 2.0f), airbrush);
        }
    }

protected:
    CrudeDrawing(const std::string& sheet_name, const std::string& fill_frame,
        labrador::RenderResources* render_resources);

    void fill(labrador::DrawList& draw_list, const labrador::RectangleF& rectangle,
        const labrador::Colour& colour) const;
    // An outline drawn inside the rectangle.
    void frame_rectangle(labrador::DrawList& draw_list,
        const labrador::RectangleF& rectangle, float width,
        const labrador::Colour& colour) const;
    // A filled circle or ellipse in horizontal bands `step` high.
    void blob(labrador::DrawList& draw_list, const labrador::Vector2F& centre,
        const labrador::Vector2F& radius, float step, const labrador::Colour& colour) const;
    // Only the top half of blob(): shoulders, a mound.
    void dome(labrador::DrawList& draw_list, const labrador::Vector2F& base_centre,
        const labrador::Vector2F& radius, float step, const labrador::Colour& colour) const;
    // A ring of square dabs.
    void ring(labrador::DrawList& draw_list, const labrador::Vector2F& centre,
        float radius, float width, const labrador::Colour& colour) const;
    // A line of square dabs, as a mouse drags it.
    void line(labrador::DrawList& draw_list, const labrador::Vector2F& from,
        const labrador::Vector2F& to, float width, const labrador::Colour& colour) const;
    // paper_card() through fill(), premultiplying each colour.
    void paper_card(labrador::DrawList& draw_list, const labrador::RectangleF& shadow,
        const labrador::RectangleF& face, float pen, const labrador::Colour& shade,
        const labrador::Colour& ink, const labrador::Colour& paper) const;
    // A block of the fill frame centred on `center` and turned `angle` about
    // it, over a keyline `keyline_width` wide on every side. Colours are
    // straight and premultiplied here, before `alpha` fades both.
    void keyed_block(labrador::DrawList& draw_list, const labrador::Vector2F& center,
        const labrador::Vector2F& size, float angle, const labrador::Colour& colour,
        const labrador::Colour& keyline, float keyline_width, float alpha) const;
};
