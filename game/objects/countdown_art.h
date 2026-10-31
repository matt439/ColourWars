#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"
#include <string_view>

// The shared start signal, drawn with the same coarse rectangle tool as the
// person and monitor. The match clock owns its age; drawing never advances it.
class CountdownArt final : public CrudeDrawing
{
public:
    CountdownArt(const MatchPresentation* look, const OpeningPresentation* source,
        labrador::RenderResources* render_resources);

    // The complete drawing, including shade and detached ticks, fits bounds.
    void draw(labrador::DrawList& list, const mattmath::RectangleF& bounds,
        std::wstring_view text, float beat_age, bool reduced_effects) const;
    void draw_card(labrador::DrawList& list, const mattmath::RectangleF& bounds,
        float scale) const;

private:
    const MatchPresentation* look_;
};
