#include "game/pch.h"
#include "game/objects/reveal_art.h"

using namespace labrador;
using namespace mattmath;

RevealArt::RevealArt(const OpeningPresentation* look, RenderResources* resources) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, resources),
    look_(look), resources_(resources), font_(resources->resolve_sprite_font(look->percent_font))
{
}

void RevealArt::draw(DrawList& list, const RectangleF& bounds, std::wstring_view text,
    const Colour& team, float click_age, bool reduced_effects) const
{
    if (text.empty() || bounds.width <= 0.0f || bounds.height <= 0.0f) { return; }
    const RevealArtPresentation& art = this->look_->reveal_art;
    const float pen = bounds.height * art.keyline_width;
    const float shade = bounds.height * art.shadow_offset;
    const float reach = bounds.height * art.tick_length;
    // Reserve the full response even in the settled drawing: neither the
    // text nor the card moves when Reduce effects changes during a pause.
    const float margin = reach + pen * 3.0f;
    const RectangleF face(bounds.x + margin, bounds.y,
        bounds.width - margin * 2.0f - shade, bounds.height - shade);
    // The text's ink outline also clears the deepest stepped paper corner,
    // including the widest accepted pen with the smallest authored padding.
    const float padding = std::max(bounds.height * art.padding, pen * 3.6f);
    const Vector2F available = face.size() - Vector2F(padding * 2.0f);
    if (available.x <= 0.0f || available.y <= 0.0f) { return; }

    const auto paper = [&](const RectangleF& box, const Colour& colour)
    {
        const float step = pen * 2.0f;
        this->fill(list, RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), colour);
        this->fill(list, RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), colour);
    };
    list.set_filter(TextureFilter::point);
    paper(RectangleF(face.position() + Vector2F(shade), face.size()), premultiplied(art.shade));
    paper(face, premultiplied(art.ink));
    const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
    paper(inside, premultiplied(art.paper));
    for (int pass = 3; pass > 0; --pass)
    {
        Colour colour = art.shade;
        colour.a *= 0.12f;
        const float width = pen * static_cast<float>(pass);
        this->fill(list, RectangleF(inside.x + pen * 2.0f, inside.bottom() - width,
            inside.width - pen * 4.0f, width), premultiplied(colour));
        this->fill(list, RectangleF(inside.right() - width, inside.y + pen * 2.0f,
            width, inside.height - pen * 4.0f), premultiplied(colour));
    }

    const std::wstring label(text);
    const Vector2F measured = this->resources_->measure_text(this->font_, label);
    if (measured.x <= 0.0f || measured.y <= 0.0f) { return; }
    const float scale = std::min(available.x / measured.x, available.y / measured.y);
    const Vector2F at = face.center() - measured * scale * 0.5f;
    const float outline = pen * 0.6f;
    list.set_filter(TextureFilter::linear);
    // Keep the exact team colour, with ink beneath even the brightest yellow.
    for (const Vector2F offset : { Vector2F(-outline, 0), Vector2F(outline, 0),
        Vector2F(0, -outline), Vector2F(0, outline), Vector2F(-outline, -outline),
        Vector2F(outline, -outline), Vector2F(-outline, outline), Vector2F(outline, outline) })
    {
        list.draw_text(this->font_, label, at + offset, premultiplied(art.ink),
            scale, 0.0f, Vector2F::ZERO, 0.0f);
    }
    list.draw_text(this->font_, label, at, premultiplied(team), scale, 0.0f, Vector2F::ZERO, 0.0f);
    list.set_filter(TextureFilter::point);

    if (!reduced_effects && click_age >= 0.0f && click_age < art.beat_duration)
    {
        const float remaining = 1.0f - click_age / art.beat_duration;
        const float beat = remaining * remaining;
        Colour ink = art.ink;
        Colour paint = team;
        ink.a *= beat;
        paint.a *= beat;
        for (const float side : { -1.0f, 1.0f })
        {
            const float x = side < 0.0f ? face.x - pen * 2.0f : face.right() + pen * 2.0f;
            for (const float row : { -1.0f, 1.0f })
            {
                const Vector2F from(x, face.center().y + row * face.height * 0.2f);
                const Vector2F to = from + Vector2F(side * reach * beat, 0);
                const float left = std::min(from.x, to.x);
                const float length = std::abs(to.x - from.x);
                this->fill(list, RectangleF(left - pen * 0.5f, from.y - pen * 0.5f,
                    length + pen, pen), premultiplied(ink));
                this->fill(list, RectangleF(left - pen * 0.2f, from.y - pen * 0.2f,
                    length + pen * 0.4f, pen * 0.4f), premultiplied(paint));
            }
        }
    }
}
