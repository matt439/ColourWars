#include "game/pch.h"
#include "game/objects/paint_fill_draw.h"
#include "game/config/presentation/presentation_match.h"
#include "game/objects/paint_fill.h"
#include "game/objects/paper_card.h"

using namespace labrador;

namespace
{
    bool horizontal(PaintSurfaceFace face)
    {
        return face == PaintSurfaceFace::top || face == PaintSurfaceFace::bottom;
    }

    RectangleF tile_bounds(const PaintSurfaceSample& tile)
    {
        // The fill contract admits only fixed, axis-aligned faces.
        return { tile.corners[0], tile.corners[2] - tile.corners[0] };
    }

    // One mark for each exposed run, broken wherever another surface covers a
    // gap. Adjacent scoring tiles are already ordered along their shared face.
    template<class DrawRun>
    void for_each_run(const std::vector<PaintSurfaceSample>& tiles, DrawRun draw)
    {
        if (tiles.empty()) { return; }
        PaintSurfaceFace face = tiles.front().face;
        RectangleF run = tile_bounds(tiles.front());
        for (std::size_t index = 1; index < tiles.size(); ++index)
        {
            const PaintSurfaceSample& tile = tiles[index];
            const RectangleF box = tile_bounds(tile);
            const bool adjacent = horizontal(face) ? box.x == run.right() : box.y == run.bottom();
            if (face == tile.face && adjacent)
            {
                run = RectangleF::union_of(run, box);
                continue;
            }
            draw(run, face);
            run = box;
            face = tile.face;
        }
        draw(run, face);
    }

    RectangleF marker(const RectangleF& run, PaintSurfaceFace face, const RectangleF& body, float size)
    {
        const bool across = horizontal(face);
        const bool leading = face == PaintSurfaceFace::top || face == PaintSurfaceFace::left;
        const float inside = across ? (leading ? body.bottom() - run.bottom() : run.top() - body.top()) :
            (leading ? body.right() - run.right() : run.left() - body.left());
        size = std::min({ size, across ? run.width : run.height, inside });
        if (size <= 0.0f) { return RectangleF::zero; }
        const float x = across ? run.center().x - size / 2.0f : (leading ? run.right() : run.left() - size);
        const float y = across ? (leading ? run.bottom() : run.top() - size) : run.center().y - size / 2.0f;
        return { x, y, size, size };
    }
}

void draw_paint_fill(DrawList& list, const PaintFill& fill,
    const PaperCard& paper, const PaintFillPresentation& look, bool reduce_effects)
{
    const SpriteSheet* sheet = paper.sheet();
    const auto rect = [&](const RectangleF& box, const Colour& colour)
    {
        sheet->draw(list, paper.fill(), box, colour);
    };
    const auto draw_marks = [&](const std::vector<PaintSurfaceSample>& tiles, const RectangleF& body)
    {
        for_each_run(tiles, [&](const RectangleF& run, PaintSurfaceFace face)
        {
            const RectangleF box = marker(run, face, body, look.mark_size);
            if (box.width <= 0.0f) { return; }
            const float cell = box.width / 9.0f;
            const auto glyph = [&](int x, int y, int w, int h, const Colour& colour)
            {
                rect({ box.x + static_cast<float>(x) * cell, box.y + static_cast<float>(y) * cell,
                    static_cast<float>(w) * cell, static_cast<float>(h) * cell }, colour);
            };
            rect(box, look.ink);
            glyph(1, 1, 7, 7, look.paper);
            // A poured drop over an open tray. During cooldown a horizontal
            // lid closes the tray. Both states read in ink on paper.
            glyph(4, 2, 1, 1, look.ink);
            glyph(3, 3, 3, 2, look.ink);
            glyph(2, 6, 5, 1, look.ink);
            if (fill.cooldown_remaining() > 0.0) { glyph(2, 5, 5, 1, look.ink); }
            else { glyph(2, 5, 1, 1, look.ink); glyph(6, 5, 1, 1, look.ink); }
        });
    };
    draw_marks(fill.trigger_geometry(), fill.trigger_bounds());
    draw_marks(fill.target_geometry(), fill.target_bounds());

    const auto& changed = fill.changed_tiles();
    if (changed.empty() || fill.fill_age() >= look.sweep_seconds) { return; }
    const bool across = horizontal(changed.front().face);
    const RectangleF first = tile_bounds(changed.front()), last = tile_bounds(changed.back());
    const float start = across ? first.left() : first.top();
    const float end = across ? last.right() : last.bottom();
    const float depth = across ? first.height : first.width;
    const float head = start + (end - start) * (fill.fill_age() / look.sweep_seconds);
    for_each_run(changed, [&](const RectangleF& run, PaintSurfaceFace)
    {
        // Reduced effects holds the completed stroke stationary for the same
        // beat. No pulse travels over friendly or buried tiles in either mode.
        const float low = across ? run.left() : run.top();
        const float high = across ? run.right() : run.bottom();
        const float left = reduce_effects ? low : std::max(low, head - depth * 0.5f);
        const float right = reduce_effects ? high : std::min(high, head + depth * 0.5f);
        if (right <= left) { return; }
        const float length = right - left;
        const float pen = depth * 0.5f;
        const RectangleF stroke = across ? RectangleF(left, run.center().y - pen / 2.0f, length, pen) :
            RectangleF(run.center().x - pen / 2.0f, left, pen, length);
        rect(stroke, look.ink);
        rect(across ? RectangleF(stroke.x, stroke.y, length, pen / 2.0f) :
            RectangleF(stroke.x, stroke.y, pen / 2.0f, length), look.paper);
    });
}
