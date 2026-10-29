#pragma once

#include "game/objects/structure_paintable.h"
#include <span>

class Player;
struct GatePermissionPresentation;

// A team gate (ROAD#15): a solid paintable wall that one team walks through.
// It opens for the team holding more of its own paint, once that team holds
// the tuned share of its paintable area (PaintTuning::gate_open_share), and
// is closed to everyone on a tie or while unpainted. Shots of both teams hit
// and paint it whichever way it is open. Level settles it once a step, from
// the paint held before any of that step's paint lands.
//
// It never closes on a player of the team it is open for: while one of them
// is inside, the gate stays open for that team, whatever the paint says, and
// changes as soon as they are clear.
class TeamGateStructurePaintable final : public StructurePaintable
{
public:
    TeamGateStructurePaintable(const std::string& sheet_name,
        const std::string& frame_name,
        const mattmath::RectangleF& rectangle,
        labrador::RenderResources* render_resources,
        const labrador::AudioResources* audio_resources,
        const PlaySettings* preferences,
        const PaintTuning* paint,
        const TeamColour& team_colours,
        const PaintableFaces& faces,
        std::span<const mattmath::RectangleF> paint_obstacles = {},
        const labrador::Colour& colour = labrador::Colour::white,
        std::unique_ptr<const TerrainMaterial> material = nullptr,
        std::span<const PaintPolygon> paint_polygons = {},
        const GatePermissionPresentation* cue = nullptr);

    // The team that passes through now, or none.
    PlayerTeam open_for() const;
    // The team the paint held now opens it for, or none.
    PlayerTeam paint_leader() const;
    // The last settlement retained permission because a living permitted
    // player was inside. Later paint does not rewrite that settled reason.
    bool held_open() const;
    // Follow the paint leader unless a living player of the team it is open
    // for still overlaps it.
    void settle(std::span<Player* const> players);

    labrador::CollisionMask mask() const override;
    void draw(labrador::DrawList& draw_list) const override;
    void draw_miniature(labrador::DrawList& draw_list) const override;
    // Cosmetic plaque, wholly inside the body and clear of its paint bands.
    // Empty when no cue was supplied or the body has no unpainted interior.
    mattmath::RectangleF permission_bounds() const;

private:
    PlayerTeam open_for_ = PlayerTeam::none;
    bool held_open_ = false;
    float open_share_ = 0.0f;
    float paint_thickness_ = 0.0f;
    const GatePermissionPresentation* cue_ = nullptr;
    TeamColour team_colours_;
    labrador::RenderResources::SheetFrame cue_frame_;
    void draw_permission(labrador::DrawList& draw_list) const;
};
