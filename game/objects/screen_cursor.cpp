#include "game/pch.h"
#include "game/objects/screen_cursor.h"

#include <array>
#include <string_view>

using namespace labrador;
using namespace mattmath;

ScreenCursor::ScreenCursor(const OpeningPresentation* look, RenderResources* render_resources) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, render_resources),
    look_(look)
{
}

void ScreenCursor::draw(DrawList& draw_list, const Vector2F& tip, float screen_height,
    float click_age, bool reduced_effects) const
{
    const CursorArtPresentation& art = this->look_->cursor_art;
    const float size = this->look_->cursor_size * screen_height;
    if (!std::isfinite(size) || size <= 0.0f) { return; }

    // Editable mouse-drawn source cells, like the painter's coarse canvas.
    // The first cell is always the hotspot; neither the shadow nor the click
    // response translates the arrow away from the selected swatch/button.
    static constexpr std::array<std::string_view, 20> cells = {
        "#.............",
        "##............",
        "#W#...........",
        "#WW#..........",
        "#WWW#.........",
        "#WWWW#........",
        "#WWWWW#.......",
        "#WWWWWW#......",
        "#WWWWWWW#.....",
        "#WWWWWWWW#....",
        "#WWWWWWWWW#...",
        "#WWWWWW#####..",
        "#WWWW#WW#.....",
        "#WWW##WW#.....",
        "#WW#.#SWW#....",
        "#W#...#SWW#...",
        "##....#SWW#...",
        ".......#SS#...",
        ".......#SS#...",
        "........##...."
    };
    const float cell = size / static_cast<float>(cells.size());
    const Colour ink = premultiplied(art.ink);
    const Colour fill = premultiplied(art.fill);
    const Colour shade = premultiplied(art.shade);
    const auto arrow = [&](const Vector2F& origin, bool shadow)
    {
        for (size_t y = 0; y < cells.size(); ++y)
        {
            const std::string_view row = cells[y];
            for (size_t x = 0; x < row.size();)
            {
                const char material = row[x];
                const size_t start = x++;
                // Merge adjacent cells of the same material into one dab.
                while (x < row.size() && row[x] == material) { ++x; }
                if (material == '.') { continue; }
                const Colour colour = shadow ? premultiplied(art.shadow) :
                    (material == '#' ? ink : (material == 'S' ? shade : fill));
                this->fill(draw_list, RectangleF(origin.x + static_cast<float>(start) * cell,
                    origin.y + static_cast<float>(y) * cell,
                    static_cast<float>(x - start) * cell, cell), colour);
            }
        }
    };
    arrow(tip + Vector2F(size * art.shadow_offset), true);
    arrow(tip, false);

    if (!reduced_effects && click_age >= 0.0f && click_age < art.click_duration)
    {
        const float progress = click_age / art.click_duration;
        const float distance = size * art.click_reach * (0.55f + progress * 0.45f);
        const float length = size * art.click_reach * (0.65f - progress * 0.25f);
        // Three short rectangle-tool ticks outside the pointer. The swatch
        // remains visible at the fixed tip and no expanding ring covers it.
        for (const Vector2F& direction : { Vector2F(-1, 0), Vector2F(0, -1), Vector2F(-0.7f, -0.7f) })
        {
            const Vector2F from = tip + direction * distance;
            const Vector2F to = from + direction * length;
            this->line(draw_list, from, to, cell * 2.5f, ink);
            this->line(draw_list, from, to, cell, fill);
        }
    }
}
