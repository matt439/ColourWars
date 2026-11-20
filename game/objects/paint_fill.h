#pragma once

#include "game/objects/paintable_object.h"
#include "engine/math/rectanglef.h"

class StructurePaintable;

// Observes ordinary trigger paint in contact order. Only settle() can paint
// the receiving face, after all contacts for the live step are complete.
class PaintFill
{
public:
    PaintFill(StructurePaintable& trigger, std::vector<PaintSurfaceFace> trigger_faces,
        StructurePaintable& target, PaintSurfaceFace target_face,
        float qualifying_share, float cooldown);
    ~PaintFill();
    PaintFill(const PaintFill&) = delete;
    PaintFill& operator=(const PaintFill&) = delete;

    void settle(float live_dt);
    PlayerTeam leader() const { return leader_; }
    PlayerTeam last_team() const { return last_team_; }
    int pending_owner() const { return pending_owner_; }
    double cooldown_remaining() const { return cooldown_remaining_; }
    float fill_age() const { return fill_age_; }
    float trigger_total() const { return total_; }
    float qualifying_share() const { return qualifying_share_; }
    float trigger_area(PlayerTeam team) const;
    std::vector<PaintSurfaceSample> trigger_tiles() const;
    std::vector<PaintSurfaceSample> target_tiles() const;
    labrador::RectangleF trigger_bounds() const;
    labrador::RectangleF target_bounds() const;
    const std::vector<PaintSurfaceSample>& trigger_geometry() const { return trigger_geometry_; }
    const std::vector<PaintSurfaceSample>& target_geometry() const { return target_geometry_; }
    const std::vector<PaintSurfaceSample>& changed_tiles() const { return changed_tiles_; }

private:
    StructurePaintable& trigger_;
    std::vector<PaintSurfaceFace> trigger_faces_;
    StructurePaintable& target_;
    PaintSurfaceFace target_face_;
    float qualifying_share_;
    float cooldown_;
    double cooldown_remaining_ = 0.0;
    float fill_age_ = 0.0f;
    float total_ = 0.0f;
    float area_a_ = 0.0f;
    float area_b_ = 0.0f;
    PlayerTeam leader_ = PlayerTeam::none;
    PlayerTeam last_team_ = PlayerTeam::none;
    int pending_owner_ = -1;
    std::vector<PaintSurfaceSample> trigger_geometry_;
    std::vector<PaintSurfaceSample> target_geometry_;
    std::vector<PaintSurfaceSample> changed_tiles_;

    void contact(PaintSurfaceFace face, PlayerTeam before, PlayerTeam after, int owner, float area);
    PlayerTeam qualifying_leader() const;
};
