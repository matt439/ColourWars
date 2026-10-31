#pragma once

#include "game/objects/player_badge.h"
#include "engine/ui/widget.h"

// Identity contrast is fixed, so focus colours do not recolour this UiObject.
class UiPlayerBadge final : public labrador::UiObject
{
public:
    UiPlayerBadge(const std::string& name, const mattmath::RectangleF& rectangle,
        int roster_index, const PlayerIdentityPresentation* look,
        labrador::RenderResources* render_resources, bool hidden = false) :
        UiObject(name, hidden), badge_(look, render_resources), rectangle_(rectangle),
        roster_index_(roster_index)
    {
    }

    void update(float /*dt*/) override {}
    void draw(labrador::DrawList& draw_list) const override
    {
        if (!this->hidden()) { this->badge_.draw(draw_list, rectangle_, roster_index_); }
    }
    mattmath::RectangleF bounds() const override { return rectangle_; }

    // Existing labels retain their geometry; the same badge sits just before them.
    // The geometry is PlayerIdentityPresentation's, which the content checks share.
    static mattmath::RectangleF before(const mattmath::RectangleF& label,
        const PlayerIdentityPresentation& look)
    {
        return look.ui_badge_before(label);
    }

private:
    PlayerBadge badge_;
    mattmath::RectangleF rectangle_;
    int roster_index_;
};
