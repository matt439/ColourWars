#include "game/pch.h"
#include "game/objects/figure_acting.h"

#include <vector>

using namespace labrador;
using namespace mattmath;

namespace
{
    // Each eye, in texels of the canvas, from the strokes that drew it.
    std::vector<RectangleF> eye_rectangles(const RespawnRedrawPresentation& strokes)
    {
        std::vector<RectangleF> eyes;
        for (const RespawnStroke& stroke : strokes.strokes)
        {
            if (stroke.part != "eye")
            {
                continue;
            }
            for (size_t at = 1; at < stroke.points.size(); ++at)
            {
                const Vector2F& from = stroke.points[at - 1];
                const Vector2F& to = stroke.points[at];
                const RectangleF sweep(std::min(from.x, to.x), std::min(from.y, to.y),
                    std::abs(to.x - from.x) + strokes.pen_width,
                    std::abs(to.y - from.y) + strokes.pen_width);
                // Strokes that touch are one eye.
                bool merged = false;
                for (RectangleF& eye : eyes)
                {
                    if (sweep.x <= eye.right() && eye.x <= sweep.right() &&
                        sweep.y <= eye.bottom() && eye.y <= sweep.bottom())
                    {
                        eye = RectangleF::union_of(eye, sweep);
                        merged = true;
                    }
                }
                if (!merged)
                {
                    eyes.push_back(sweep);
                }
            }
        }
        return eyes;
    }
}

FigureActing::FigureActing(const OpeningPresentation* look,
    const RespawnRedrawPresentation* strokes, RenderResources* render_resources) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, render_resources),
    look_(look),
    strokes_(strokes)
{
}

void FigureActing::draw_eyes(DrawList& draw_list, const RectangleF& figure,
    bool facing_right, const Vector2F& glance) const
{
    const RespawnRedrawPresentation& strokes = *this->strokes_;
    const Vector2F scale(figure.width / strokes.canvas_size.x,
        figure.height / strokes.canvas_size.y);
    for (RectangleF eye : eye_rectangles(strokes))
    {
        if (!facing_right)
        {
            eye.x = strokes.canvas_size.x - eye.right();
        }
        const RectangleF world(figure.x + eye.x * scale.x, figure.y + eye.y * scale.y,
            eye.width * scale.x, eye.height * scale.y);
        // Paint the eye out, then put a narrower one where it is looking.
        this->fill(draw_list, world, this->look_->face);
        const Vector2F pupil(world.width * 0.55f, world.height * 0.6f);
        const float x = world.x + (world.width - pupil.x) *
            (0.5f + 0.5f * std::clamp(glance.x, -1.0f, 1.0f));
        const float y = world.y + (world.height - pupil.y) *
            (0.5f + 0.5f * std::clamp(glance.y, -1.0f, 1.0f));
        this->fill(draw_list, RectangleF(Vector2F(x, y), pupil), Colour::black);
    }
}

void FigureActing::draw_sweat(DrawList& draw_list, const RectangleF& figure,
    bool facing_right, float time) const
{
    const float loop = time / 0.8f - std::floor(time / 0.8f);
    const float side = facing_right ? 0.92f : 0.08f;
    const Vector2F at(figure.x + figure.width * side,
        figure.y + figure.height * (0.08f + 0.22f * loop));
    const float radius = figure.height * 0.03f;
    this->blob(draw_list, at, Vector2F(radius, radius * 1.3f), std::max(1.0f, radius / 3.0f),
        this->look_->sweat);
}
