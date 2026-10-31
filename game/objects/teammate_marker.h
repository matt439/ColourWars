#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation_match.h"

#include "engine/render/camera.h"

#include <optional>

class PlayerBadge;

// Where one off-screen teammate's marker sits in a pane, in pane-local pixels
// (the HUD's coordinates). `direction` is the unit vector from the pane centre
// toward the teammate's body centre; `centre` lies on that line, clamped so
// the chip and its pointer stay `margin` inside the pane edge.
struct TeammateMarkerPlacement
{
    int participant = -1;
    mattmath::Vector2F centre;
    mattmath::Vector2F direction;
    // The chip; the pointer leaves it along `direction` and ends at `tip`.
    mattmath::RectangleF chip;
    mattmath::Vector2F tip;
    // The HUD scale it was placed at, so drawing uses the same sizes.
    float scale = 1.0f;
    // 0 when the teammate has only just left the view, 1 once faded in.
    float opacity = 1.0f;
};

// ROAD#18: the edge chip that says where a teammate is when their body is
// outside this pane's view. A Paint.exe rectangle in the teammate's team
// colour, an ink keyline, their roster badge and a stepped pointer.
class TeammateMarker final : public CrudeDrawing
{
public:
    TeammateMarker(const TeammateMarkerPresentation* look,
        labrador::RenderResources* render_resources);

    // None while any of `body` is inside what `camera` shows of a pane of
    // `pane_size`; otherwise where the marker goes. `scale` is the HUD's.
    static std::optional<TeammateMarkerPlacement> place(
        const TeammateMarkerPresentation& look, const mattmath::Vector2F& pane_size,
        const labrador::Camera& camera, const mattmath::RectangleF& body, float scale);

    // Draws under whatever camera the list holds; the HUD's identity camera
    // makes the placement's pane-local pixels exact.
    void draw(labrador::DrawList& draw_list, const TeammateMarkerPlacement& placement,
        const labrador::Colour& team_colour, const PlayerBadge& badge) const;

private:
    const TeammateMarkerPresentation* look_;
};
