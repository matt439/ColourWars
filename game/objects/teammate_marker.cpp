#include "game/pch.h"
#include "game/objects/teammate_marker.h"
#include "game/objects/player_badge.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
    Colour faded(const Colour& colour, float opacity)
    {
        return CrudeDrawing::premultiplied(Colour(colour.r, colour.g, colour.b,
            colour.a * std::clamp(opacity, 0.0f, 1.0f)));
    }
}

TeammateMarker::TeammateMarker(const TeammateMarkerPresentation* look,
    RenderResources* render_resources) :
    CrudeDrawing("sprite_sheet_1", "pixel", render_resources), look_(look)
{
    if (look_ == nullptr)
    {
        throw std::invalid_argument("Teammate marker requires its presentation");
    }
}

std::optional<TeammateMarkerPlacement> TeammateMarker::place(
    const TeammateMarkerPresentation& look, const Vector2F& pane_size,
    const Camera& camera, const RectangleF& body, float scale)
{
    if (pane_size.x <= 0.0f || pane_size.y <= 0.0f || !(scale > 0.0f)) { return std::nullopt; }
    const RectangleF visible = camera.visible_rectangle(
        Viewport(RectangleF(Vector2F::ZERO, pane_size)));
    if (body.intersects(visible)) { return std::nullopt; }

    const Vector2F middle = pane_size * 0.5f;
    const Vector2F offset = camera.calculate_view_position(body.center()) - middle;
    const float distance = offset.length();
    if (!std::isfinite(distance) || distance <= 0.0f) { return std::nullopt; }
    const Vector2F direction = offset / distance;

    // The chip's half-extent plus the pointer: how far the marker reaches
    // from its centre along any one axis, so the whole of it stays inside.
    const Vector2F chip_size = look.size * scale;
    const float leave = std::max(chip_size.x, chip_size.y) * 0.5f;
    const float reach = look.margin * scale + leave + look.pointer_length * scale;
    const Vector2F room(std::max(0.0f, middle.x - reach), std::max(0.0f, middle.y - reach));
    float along = distance;
    if (std::abs(direction.x) > 0.0f) { along = std::min(along, room.x / std::abs(direction.x)); }
    if (std::abs(direction.y) > 0.0f) { along = std::min(along, room.y / std::abs(direction.y)); }

    TeammateMarkerPlacement placement;
    placement.centre = middle + direction * along;
    placement.direction = direction;
    placement.chip = RectangleF(placement.centre - chip_size * 0.5f, chip_size);
    placement.tip = placement.centre + direction * (leave + look.pointer_length * scale);
    placement.scale = scale;
    return placement;
}

void TeammateMarker::draw(DrawList& draw_list, const TeammateMarkerPlacement& placement,
    const Colour& team_colour, const PlayerBadge& badge) const
{
    const TeammateMarkerPresentation& look = *this->look_;
    if (!look.enabled || placement.opacity <= 0.0f || placement.participant < 0) { return; }
    const float scale = placement.scale;
    const float keyline = look.keyline_width * scale;
    const float pointer = look.pointer_width * scale;
    const Colour ink = faded(look.ink, placement.opacity);
    const Colour team = faded(Colour(team_colour.r, team_colour.g, team_colour.b, 1.0f),
        placement.opacity);

    draw_list.set_filter(TextureFilter::point);
    // The pointer's last dab is centred half its width short of the tip, so
    // the square dab ends at the tip and never leaves the pane margin.
    const Vector2F end = placement.tip - placement.direction * (pointer * 0.5f);
    this->line(draw_list, placement.centre, end, pointer, ink);
    const float core = pointer - keyline * 2.0f;
    if (core > 0.0f)
    {
        this->line(draw_list, placement.centre, end - placement.direction * keyline, core, team);
    }
    this->fill(draw_list, placement.chip, ink);
    const RectangleF inner(placement.chip.position() + Vector2F(keyline),
        placement.chip.size() - Vector2F(keyline * 2.0f));
    this->fill(draw_list, inner, team);
    const float inset = look.badge_inset * scale;
    badge.draw(draw_list, RectangleF(inner.position() + Vector2F(inset),
        inner.size() - Vector2F(inset * 2.0f)), placement.participant, placement.opacity);
}
