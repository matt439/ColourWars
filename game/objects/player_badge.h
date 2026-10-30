#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"

#include <string>

// A roster number, independent of the input slot.
// The same small drawing is used in the world, pane and existing menu rows.
class PlayerBadge final : public CrudeDrawing
{
public:
    PlayerBadge(const PlayerIdentityPresentation* look,
        labrador::RenderResources* render_resources);

    // `opacity` below 1 fades the whole badge (a teammate marker fading in).
    void draw(labrador::DrawList& draw_list, const mattmath::RectangleF& rectangle,
        int roster_index, float opacity = 1.0f) const;
    static std::wstring label(int roster_index);

private:
    const PlayerIdentityPresentation* look_;
};
