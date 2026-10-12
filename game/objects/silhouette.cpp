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
    // Seated height, from the bottom of the screen to the top of the head.
    const float height = (persona.child ? look.child_height : look.adult_height) * screen.height;
    const float step = height * 0.025f;

    // Standing lifts the whole figure; a sigh lifts it a little and drops it.
    const float lift = pose.stand * screen.height * 0.5f + pose.sigh * height * 0.05f;
    const Vector2F base(screen.x + screen.width * 0.5f + pose.leave * screen.width * 0.8f,
        screen.bottom() + height * 0.12f - lift);

    // Shoulders and back, a mound rising out of the bottom of the screen.
    const Vector2F shoulders(height * (persona.child ? 0.62f : 0.72f), height * 0.6f);
    this->dome(draw_list, base, shoulders, step, look.silhouette);
    // Below the screen as the person stands, the body continues down.
    this->fill(draw_list, RectangleF(base.x - shoulders.x * 0.8f, base.y - step,
        shoulders.x * 1.6f, lift + height * 0.2f), look.silhouette);

    const float neck_top = base.y - shoulders.y - height * 0.08f;
    this->fill(draw_list, RectangleF(base.x - height * 0.09f, neck_top,
        height * 0.18f, height * 0.14f), look.silhouette);

    const Vector2F head_radius(height * (persona.child ? 0.2f : 0.17f),
        height * (persona.child ? 0.21f : 0.2f));
    const Vector2F head(base.x, neck_top - head_radius.y * 0.8f);
    this->blob(draw_list, head, head_radius, step, look.silhouette);

    if (persona.child)
    {
        // Two bunches of hair: a child, at a glance.
        const Vector2F bunch(head_radius.x * 0.38f, head_radius.y * 0.34f);
        this->blob(draw_list, head + Vector2F(-head_radius.x * 1.1f, -head_radius.y * 0.2f),
            bunch, step, look.silhouette);
        this->blob(draw_list, head + Vector2F(head_radius.x * 1.1f, -head_radius.y * 0.2f),
            bunch, step, look.silhouette);
    }
    if (persona.mug)
    {
        // Held at the shoulder: the same person whose ring is on the bezel.
        const RectangleF mug(base.x + shoulders.x * 0.82f, base.y - shoulders.y * 0.55f,
            height * 0.13f, height * 0.16f);
        this->fill(draw_list, mug, look.silhouette);
        this->ring(draw_list, Vector2F(mug.right(), mug.y + mug.height * 0.45f),
            mug.width * 0.32f, step * 1.2f, look.silhouette);
    }
}
