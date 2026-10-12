#include "game/pch.h"
#include "game/objects/screen_cursor.h"

using namespace labrador;
using namespace mattmath;

ScreenCursor::ScreenCursor(const OpeningPresentation* look, RenderResources* render_resources) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, render_resources),
    look_(look)
{
}

void ScreenCursor::draw(DrawList& draw_list, const Vector2F& tip, float screen_height) const
{
    const float size = this->look_->cursor_size * screen_height;
    const float row = std::max(1.0f, size / 14.0f);
    const float outline = row;

    // The tail first, so the head covers its join.
    const Vector2F tail_from = tip + Vector2F(size * 0.3f, size * 0.62f);
    const Vector2F tail_to = tip + Vector2F(size * 0.52f, size * 1.0f);
    this->line(draw_list, tail_from, tail_to, size * 0.22f, Colour::black);
    this->line(draw_list, tail_from, tail_to, size * 0.22f - outline * 2.0f, Colour::white);

    // The head: a triangle in rows, black, then white inside it.
    const int rows = 11;
    for (int index = 0; index < rows; ++index)
    {
        const float width = (static_cast<float>(index) + 1.0f) * row * 0.75f;
        this->fill(draw_list, RectangleF(tip.x, tip.y + static_cast<float>(index) * row,
            width + outline, row), Colour::black);
    }
    for (int index = 1; index < rows - 1; ++index)
    {
        const float width = static_cast<float>(index) * row * 0.75f - outline;
        this->fill(draw_list, RectangleF(tip.x + outline,
            tip.y + static_cast<float>(index) * row, width, row), Colour::white);
    }
}
