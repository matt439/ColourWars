#pragma once

#include "game/objects/horizontal_motion.h"
#include "game/objects/structure_paintable.h"

class LevelStructures;
class Projectile;
struct FerryDirectionPresentation;

// A lift's end stops are its own (ROAD#28): top_stop at phase 0, bottom_stop
// at phase 0.5, where a horizontal mover has left_stop and right_stop.
enum class FerryMotionCue { unstepped, held, moving, left_stop, right_stop, top_stop, bottom_stop };

struct FerryCueState
{
    bool paint_driven = false;
    PlayerTeam demand_team = PlayerTeam::none;
    int demand_heading = 0;
    int committed_heading = 0;
    // Sign of committed world-body displacement on the mover's own axis, after
    // pose rounding: +1 right (a lift down), -1 left (a lift up).
    int movement_heading = 0;
    FerryMotionCue motion = FerryMotionCue::unstepped;
};

// Rigid translation of anchored scoring/material geometry on one axis: a
// horizontal mover, or a vertical lift (ROAD#28), which is solid.
// This capability is fixed for a Level lifetime; it is not a rotating body.
// A paint-driven one (a paint ferry, or a paint lift) reads its own held paint
// each step, before any of the step's paint lands, to choose which way it travels.
class TranslatingStructurePaintable final : public StructurePaintable
{
public:
    TranslatingStructurePaintable(const std::string& sheet_name,
        const std::string& frame_name,
        const HorizontalMotionSpec& motion,
        labrador::RenderResources* render_resources,
        const PlaySettings* preferences,
        const PaintTuning* paint,
        CollisionObjectType collision_type,
        const TeamColour& team_colours,
        const PaintableFaces& faces,
        const labrador::Colour& colour = labrador::Colour::white,
        const labrador::Vector2F& origin = labrador::Vector2F::zero,
        labrador::SpriteFlip flip = labrador::SpriteFlip::none,
        float layer_depth = 0.0f,
        std::unique_ptr<const TerrainMaterial> material = nullptr,
        const FerryDirectionPresentation* cue = nullptr);

    HorizontalMotionId motion_id() const;
    HorizontalMotionState motion_state() const;
    HorizontalMotionStep collision_step() const;
    labrador::Vector2F translation() const;
    // +1 right (a lift down), -1 left (a lift up) or 0 still, from the paint
    // held now; 0 for a cycle.
    int drive_heading() const;
    // Current paint demand and last positive committed step are separate
    // clocks. Drawing and zero steps never turn new paint into claimed motion.
    FerryCueState cue_state() const;
    // World-space hanging plaque; empty for cycles and uncued objects.
    labrador::RectangleF cue_bounds() const;
    void append_anchor_paint_surfaces(std::vector<PaintSurfaceSample>& samples) const;

    void draw(labrador::DrawList& draw_list) const override;
    void draw_miniature(labrador::DrawList& draw_list) const override;
    labrador::RectangleF bounds() const override;
    labrador::RectangleF cull_bounds(float units_per_pixel) const override;
    const labrador::Shape* shape() const override;
    // Paint lands in the rest frame: the splash is shifted back by the pose.
    void on_projectile_contact(const Projectile& other,
        const labrador::Vector2F& normal, float penetration) override;
    PlayerTeam top_paint_team(float world_x) const override;
    std::vector<PaintRunoffVisual> runoff_visuals() const override;
    void append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
        std::size_t source_index = 0) const override;
    bool for_deletion() const override;
    void set_for_deletion(bool requested) override;
    TranslatingStructurePaintable* mover() override { return this; }
    const TranslatingStructurePaintable* mover() const override { return this; }

private:
    friend class LevelStructures;
    friend class Projectile;
    void bind_motion_id(HorizontalMotionId id);
    // A lift's sign is no wider than the clear column admission keeps beside
    // its shaft (the lift plus a body width each side); Level admission sets it.
    void fit_cue_to_column(float width) noexcept;
    HorizontalPreparedStep prepare_motion(float dt) const;
    HorizontalPreparedStep hold_motion(float dt) const;
    void commit_motion(const HorizontalPreparedStep& prepared) noexcept;
    void publish_zero_step() noexcept;
    // Accepted relative-sweep geometry stays local; it is never re-gated by
    // a final-world narrow phase or reconstructed from a rounded world copy.
    void paint_projectile_at(const Projectile& source,
        const labrador::RectangleF& anchor_rectangle);
    static const labrador::RectangleF& checked_anchor(const HorizontalMotionSpec& spec,
        CollisionObjectType collision_type);

    HorizontalMotion motion_;
    labrador::RectangleF world_rectangle_ = labrador::RectangleF::zero;
    HorizontalMotionStep collision_step_;
    HorizontalMotionId motion_id_ = 0;
    const FerryDirectionPresentation* cue_ = nullptr;
    labrador::RenderResources::SheetFrame cue_frame_;
    float cue_column_ = 0.0f; // Zero until admission: no column limit yet.
    int committed_heading_ = 0;
    int movement_heading_ = 0;
    FerryMotionCue motion_cue_ = FerryMotionCue::unstepped;
    labrador::RectangleF anchor_cue_bounds() const;
    void draw_direction(labrador::DrawList& draw_list) const;
};
