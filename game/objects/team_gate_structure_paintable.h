#pragma once

#include "game/objects/structure_paintable.h"
#include <span>

class Player;

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
        std::span<const PaintPolygon> paint_polygons = {});

    // The team that passes through now, or none.
    PlayerTeam open_for() const;
    // The team the paint held now opens it for, or none.
    PlayerTeam paint_leader() const;
    // Follow the paint leader unless a living player of the team it is open
    // for still overlaps it.
    void settle(std::span<Player* const> players);

    labrador::CollisionMask mask() const override;

private:
    PlayerTeam open_for_ = PlayerTeam::none;
    float open_share_ = 0.0f;
};
