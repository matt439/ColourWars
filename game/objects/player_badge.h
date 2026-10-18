#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation.h"

#include <string>

// A roster number, independent of the input slot.
// The same small drawing is used in the world, pane and existing menu rows.
class PlayerBadge final : public CrudeDrawing
{
public:
    PlayerBadge(const PlayerIdentityPresentation* look,
        labrador::RenderResources* render_resources);

    void draw(labrador::DrawList& draw_list, const mattmath::RectangleF& rectangle,
        int roster_index) const;
    static std::wstring label(int roster_index);

private:
    const PlayerIdentityPresentation* look_;
};
