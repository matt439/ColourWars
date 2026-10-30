#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"

// The person's mouse pointer, an arrow drawn as crudely as everything else.
// It is the person's only presence inside the picture.
class ScreenCursor final : public CrudeDrawing
{
public:
    // The presentation is borrowed and outlives the level.
    ScreenCursor(const OpeningPresentation* look, labrador::RenderResources* render_resources);

    // The arrow's fixed tip at `tip`, sized by the screen's height. A negative
    // click age is resting; the existing reveal timeline owns the click age.
    void draw(labrador::DrawList& draw_list, const mattmath::Vector2F& tip,
        float screen_height, float click_age = -1.0f, bool reduced_effects = false) const;

private:
    const OpeningPresentation* look_;
};
