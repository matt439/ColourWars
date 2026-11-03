#include "game/pch.h"
#include "game/objects/figure_acting.h"
#include "game/objects/figure_person.h"

#include <vector>

using namespace labrador;

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
    this->draw_eyes(draw_list, figure, facing_right, glance, this->strokes_->strokes);
}

void FigureActing::draw_eyes(DrawList& draw_list, const RectangleF& figure,
    bool facing_right, const Vector2F& glance, const std::vector<RespawnStroke>& drawn) const
{
    const RespawnRedrawPresentation& strokes = *this->strokes_;
    const Vector2F scale(figure.width / strokes.canvas_size.x,
        figure.height / strokes.canvas_size.y);
    for (RectangleF eye : eye_rectangles(drawn, strokes.pen_width))
    {
        if (!facing_right)
        {
            eye.x = strokes.canvas_size.x - eye.right();
        }
        const RectangleF world(figure.x + eye.x * scale.x, figure.y + eye.y * scale.y,
            eye.width * scale.x, eye.height * scale.y);
        // Paint the eye out, then put a narrower one where it is looking.
        this->fill(draw_list, world, premultiplied(this->look_->face));
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
    // Just clear of the front ear on the painter's whole canvas.
    const float side = facing_right ? 0.88f : 0.12f;
    const Vector2F at(figure.x + figure.width * side,
        figure.y + figure.height * (0.08f + 0.22f * loop));
    const float radius = figure.height * 0.03f;
    this->blob(draw_list, at, Vector2F(radius, radius * 1.3f), std::max(1.0f, radius / 3.0f),
        premultiplied(this->look_->sweat));
}
