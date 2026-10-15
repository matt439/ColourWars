#include "game/pch.h"
#include "game/objects/silhouette.h"

using namespace labrador;
using namespace mattmath;

Silhouette::Silhouette(const OpeningPresentation* look, RenderResources* render_resources) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, render_resources),
    look_(look)
{
}

void Silhouette::draw(DrawList& draw_list, const RectangleF& screen,
    const MonitorPersona& persona, const SilhouettePose& pose) const
{
    const OpeningPresentation& look = *this->look_;
    const SilhouetteArtPresentation& art = look.silhouette_art;
    // Seated height, from the bottom of the screen to the top of the head.
    const float height = (persona.child ? look.child_height : look.adult_height) * screen.height;
    if (height <= 0.0f) { return; }
    const float step = height * 0.025f;
    const float pen = height * art.pen_width;
    const float shade_width = height * art.shade_width;

    // Standing lifts the whole figure; a sigh lifts it a little and drops it.
    const float lift = pose.stand * screen.height * 0.5f + pose.sigh * height * 0.05f;
    const Vector2F base(screen.x + screen.width * 0.5f + pose.leave * screen.width * 0.8f,
        screen.bottom() + height * 0.12f - lift);

    // Shoulders and back, a mound rising out of the bottom of the screen.
    const Vector2F shoulders(height * (persona.child ? 0.62f : 0.72f), height * 0.6f);
    // Below the screen as the person stands, the body continues down.
    const RectangleF back(base.x - shoulders.x * 0.8f, base.y - step,
        shoulders.x * 1.6f, lift + height * 0.2f);
    const float neck_top = base.y - shoulders.y - height * 0.08f;
    const RectangleF neck(base.x - height * 0.09f, neck_top,
        height * 0.18f, height * 0.14f);
    const Vector2F head_radius(height * (persona.child ? 0.2f : 0.17f),
        height * (persona.child ? 0.21f : 0.2f));
    const Vector2F head(base.x, neck_top - head_radius.y * 0.8f);
    const Vector2F bunch(head_radius.x * 0.38f, head_radius.y * 0.34f);
    const Vector2F left_bunch = head + Vector2F(-head_radius.x * 1.1f, -head_radius.y * 0.2f);
    const Vector2F right_bunch = head + Vector2F(head_radius.x * 1.1f, -head_radius.y * 0.2f);
    const RectangleF mug(base.x + shoulders.x * 0.82f, base.y - shoulders.y * 0.55f,
        height * 0.13f, height * 0.16f);

    // Draw the entire outline before any fill, so the joined head, neck and
    // shoulders stay one mass rather than acquiring dark internal seams.
    const auto mass = [&](const Colour& colour, const Vector2F& offset)
    {
        this->dome(draw_list, base + offset, shoulders, step, colour);
        this->fill(draw_list, RectangleF(back.position() + offset, back.size()), colour);
        this->fill(draw_list, RectangleF(neck.position() + offset, neck.size()), colour);
        this->blob(draw_list, head + offset, head_radius, step, colour);
        if (persona.child)
        {
            // Two bunches of hair: a child, at a glance.
            this->blob(draw_list, left_bunch + offset, bunch, step, colour);
            this->blob(draw_list, right_bunch + offset, bunch, step, colour);
        }
        if (persona.mug)
        {
            // Held at the shoulder: the same person whose ring is on the bezel.
            this->fill(draw_list, RectangleF(mug.position() + offset, mug.size()), colour);
            this->ring(draw_list, Vector2F(mug.right(), mug.y + mug.height * 0.45f) + offset,
                mug.width * 0.32f, step * 1.2f, colour);
        }
    };
    const Colour ink = premultiplied(art.ink);
    mass(ink, Vector2F(-pen, -pen));
    mass(ink, Vector2F(pen, -pen));
    mass(ink, Vector2F(-pen, pen));
    mass(ink, Vector2F(pen, pen));
    mass(look.silhouette, Vector2F::ZERO);

    // A few translucent, square-ended passes sit inside the same stepped
    // edge. There is no face, clothing or new persona information: only the
    // monitor's dim light catching one side of an otherwise dark silhouette.
    const Colour rim = premultiplied(art.rim);
    const Colour shade = premultiplied(Colour(art.shade.r, art.shade.g,
        art.shade.b, art.shade.a / 4.0f));
    const auto edges = [&](const Vector2F& centre, const Vector2F& radius,
        bool full, bool left_rim, bool right_shade, bool child_head)
    {
        const int bands = std::clamp(static_cast<int>(std::ceil(radius.y / step)), 1, 32);
        const int first = full ? -bands : 0;
        for (int band = first; band < bands; ++band)
        {
            const float y = (static_cast<float>(band) + 0.5f) * step;
            // The head's lower edge merges into the neck. The bunches also
            // merge into the child's head; do not draw a lit seam through them.
            if (full && y < -radius.y * 0.45f) { continue; }
            if (child_head && y > -radius.y * 0.14f && y < radius.y * 0.54f) { continue; }
            const float half = radius.x * std::sqrt(std::max(0.0f,
                1.0f - (y * y) / (radius.y * radius.y)));
            const float top = centre.y - static_cast<float>(band + 1) * step;
            if (left_rim)
            {
                this->fill(draw_list, RectangleF(centre.x - half, top,
                    std::min(pen, half * 2.0f), step), rim);
            }
            if (right_shade)
            {
                for (int pass = 4; pass > 0; --pass)
                {
                    const float spread = std::min(shade_width * static_cast<float>(pass) / 4.0f,
                        half * 2.0f);
                    this->fill(draw_list, RectangleF(centre.x + half - spread, top,
                        spread, step), shade);
                }
            }
        }
    };
    edges(base, shoulders, false, true, true, false);
    edges(head, head_radius, true, true, true, persona.child);
    if (persona.child)
    {
        edges(left_bunch, bunch, true, true, false, false);
        edges(right_bunch, bunch, true, false, true, false);
    }
    if (persona.mug)
    {
        this->fill(draw_list, RectangleF(mug.x, mug.y, mug.width, pen), rim);
        this->fill(draw_list, RectangleF(mug.x, mug.y, pen, mug.height * 0.6f), rim);
        for (int pass = 4; pass > 0; --pass)
        {
            const float spread = std::min(shade_width * static_cast<float>(pass) / 4.0f, mug.width);
            this->fill(draw_list, RectangleF(mug.right() - spread, mug.y,
                spread, mug.height), shade);
        }
    }
}
