#pragma once

#include "game/presentation.h"
#include "engine/render/texture_object.h"

// A splatted figure coming back. It has been painted over, so it is gone from
// the picture, but the undo history still holds the strokes the person drew it
// with: they replay fast-forward at the spawn, line by line, and the length of
// the replay is the respawn wait (docs/backstory.md, "Respawn").
//
// It keeps no state of its own. The player's respawn timer is the only clock,
// so every view draws the same picture and a replay draws it again exactly.
class RespawnRedraw final : public labrador::TextureObject
{
public:
    // The presentation is borrowed and outlives the player; `tune` edits it live.
    RespawnRedraw(const RespawnRedrawPresentation* presentation,
        labrador::RenderResources* render_resources);

    // `progress` runs from 0 at the pop to 1 when the figure is back. Draws the
    // outline drawn so far over `figure`, mirrored when facing left. Nothing
    // before draw_from; the whole outline from fill_at, when filled() is true.
    void draw(labrador::DrawList& draw_list, const mattmath::RectangleF& figure,
        bool facing_right, float progress) const;

    // The outline is finished and the figure is filled in: its own sprite takes over.
    bool filled(float progress) const;

    // The part of the wait during which the outline's pen is moving.
    bool drawing(float progress) const;

    // Seconds after the pop that the camera cuts to the spawn.
    float camera_hold() const;

private:
    const RespawnRedrawPresentation* presentation_;

    void draw_pen(labrador::DrawList& draw_list, const mattmath::RectangleF& figure,
        bool facing_right, const mattmath::Vector2F& from,
        const mattmath::Vector2F& to) const;
};
