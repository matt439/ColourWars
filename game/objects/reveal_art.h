#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"
#include <string_view>

// The percentages on the person's monitor. The existing reveal owns when
// these appear; the drawing is immutable and keeps every mark inside bounds.
class RevealArt final : public CrudeDrawing
{
public:
    RevealArt(const OpeningPresentation* look, labrador::RenderResources* resources);
    void draw(labrador::DrawList& list, const mattmath::RectangleF& bounds,
        std::wstring_view text, const labrador::Colour& team,
        float click_age, bool reduced_effects) const;

private:
    const OpeningPresentation* look_;
    labrador::RenderResources* resources_;
    labrador::FontHandle font_;
};
