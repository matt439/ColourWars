#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"

// What a figure is doing while the person is in the room, or has just left
// (docs/backstory.md): looking at its hands and across at the other side as
// it comes alive, and, under the person's eye, freezing but cheating.
enum class FigureAct
{
    none,
    look_at_hands,
    look_across,
    cheat,
    celebrate,
    slump,
};

// Draws the parts of the acting that are not the sprite itself: the eyes
// looking somewhere, and sweat. The eyes are found from the respawn
// strokes, so they sit exactly where the person drew them.
class FigureActing final : public CrudeDrawing
{
public:
    // Both presentations are borrowed and outlive the level.
    FigureActing(const OpeningPresentation* look, const RespawnRedrawPresentation* strokes,
        labrador::RenderResources* render_resources);

    // `glance` is where the pupils go within the eyes, -1..1 on each axis.
    void draw_eyes(labrador::DrawList& draw_list, const mattmath::RectangleF& figure,
        bool facing_right, const mattmath::Vector2F& glance) const;
    // A drop running down beside the head; `time` loops it.
    void draw_sweat(labrador::DrawList& draw_list, const mattmath::RectangleF& figure,
        bool facing_right, float time) const;

private:
    const OpeningPresentation* look_;
    const RespawnRedrawPresentation* strokes_;
};
