#include "game/pch.h"
#include "game/objects/respawn_redraw.h"

using namespace labrador;
using namespace mattmath;

RespawnRedraw::RespawnRedraw(const RespawnRedrawPresentation* presentation,
    RenderResources* render_resources) :
    TextureObject(presentation->sprite_sheet, presentation->pen_frame, render_resources),
    presentation_(presentation)
{
}

bool RespawnRedraw::filled(float progress) const
{
    return progress >= this->presentation_->fill_at;
}

bool RespawnRedraw::drawing(float progress) const
{
    return progress >= this->presentation_->draw_from && !this->filled(progress);
}

float RespawnRedraw::camera_hold() const
{
    return this->presentation_->camera_hold;
}

void RespawnRedraw::draw(DrawList& draw_list, const RectangleF& figure,
    bool facing_right, float progress) const
{
    const RespawnRedrawPresentation& look = *this->presentation_;
    if (progress < look.draw_from)
    {
        return;
    }

    // The pen moves at one speed, so each stroke takes its share of the time
    // by length. The person drew it in this order; the replay keeps it.
    float total = 0.0f;
    for (const RespawnStroke& stroke : look.strokes)
    {
        for (size_t at = 1; at < stroke.points.size(); ++at)
        {
            total += (stroke.points[at] - stroke.points[at - 1]).length();
        }
    }
    float remaining = total * std::clamp((progress - look.draw_from) /
        (look.fill_at - look.draw_from), 0.0f, 1.0f);

    for (const RespawnStroke& stroke : look.strokes)
    {
        for (size_t at = 1; at < stroke.points.size(); ++at)
        {
            if (remaining <= 0.0f)
            {
                return;
            }
            const Vector2F& from = stroke.points[at - 1];
            const Vector2F& to = stroke.points[at];
            const float length = (to - from).length();
            this->draw_pen(draw_list, figure, facing_right, from,
                remaining >= length ? to : from + (to - from) * (remaining / length));
            remaining -= length;
        }
    }
}

void RespawnRedraw::draw_pen(DrawList& draw_list, const RectangleF& figure,
    bool facing_right, const Vector2F& from, const Vector2F& to) const
{
    const RespawnRedrawPresentation& look = *this->presentation_;
    // Every line is horizontal or vertical, so the nib's sweep is a rectangle.
    Vector2F top_left(std::min(from.x, to.x), std::min(from.y, to.y));
    const Vector2F size(std::abs(to.x - from.x) + look.pen_width,
        std::abs(to.y - from.y) + look.pen_width);
    if (!facing_right)
    {
        // As SpriteFlip::horizontal mirrors the sprite across its frame.
        top_left.x = look.canvas_size.x - top_left.x - size.x;
    }
    const Vector2F scale(figure.width / look.canvas_size.x,
        figure.height / look.canvas_size.y);
    this->draw_with(draw_list,
        RectangleF(figure.position() + Vector2F(top_left.x * scale.x, top_left.y * scale.y),
            Vector2F(size.x * scale.x, size.y * scale.y)),
        this->frame(), look.pen_colour, Vector2F::ZERO, SpriteFlip::none, 0.0f);
}
