#pragma once

#include "game/objects/crude_drawing.h"
#include "game/objects/monitor.h"

// Where the person is in their few moves. Each runs 0..1.
struct SilhouettePose
{
    // Shoulders up and down: a sigh.
    float sigh = 0.0f;
    // Out of the chair.
    float stand = 0.0f;
    // Off to the side, out of the room.
    float leave = 0.0f;
};

// The person at the computer, only ever seen from behind as a silhouette:
// large, in the middle of the screen, in front of the monitor. Its build
// comes from the match's persona, like the bezel's decorations.
class Silhouette final : public CrudeDrawing
{
public:
    // The presentation is borrowed and outlives the level.
    Silhouette(const OpeningPresentation* look, labrador::RenderResources* render_resources);

    // `screen` is the window's rectangle; the person does not move with the glass.
    void draw(labrador::DrawList& draw_list, const mattmath::RectangleF& screen,
        const MonitorPersona& persona, const SilhouettePose& pose) const;

private:
    const OpeningPresentation* look_;
};
