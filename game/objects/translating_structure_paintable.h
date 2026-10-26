#pragma once

#include "game/objects/horizontal_motion.h"
#include "game/objects/structure_paintable.h"

class Level;
class Projectile;

// Rigid horizontal translation of anchored scoring/material geometry. This
// capability is fixed for a Level lifetime; it is not a lift or a rotating body.
class TranslatingStructurePaintable final : public StructurePaintable
{
public:
    TranslatingStructurePaintable(const std::string& sheet_name,
        const std::string& frame_name,
        const HorizontalMotionSpec& motion,
        labrador::RenderResources* render_resources,
        const labrador::AudioResources* audio_resources,
        const PlaySettings* preferences,
        const PaintTuning* paint,
        CollisionObjectType collision_type,
        const TeamColour& team_colours,
        const PaintableFaces& faces,
        const labrador::Colour& colour = labrador::Colour::white,
        const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
        labrador::SpriteFlip flip = labrador::SpriteFlip::none,
        float layer_depth = 0.0f,
        std::unique_ptr<const TerrainMaterial> material = nullptr);

    HorizontalMotionId motion_id() const;
    HorizontalMotionState motion_state() const;
    HorizontalMotionStep collision_step() const;
    mattmath::Vector2F translation() const;
    void append_anchor_paint_surfaces(std::vector<PaintSurfaceSample>& samples) const;

    void draw(labrador::DrawList& draw_list) const override;
    void draw_miniature(labrador::DrawList& draw_list) const override;
    mattmath::RectangleF bounds() const override;
    mattmath::RectangleF cull_bounds(float units_per_pixel) const override;
    const mattmath::Shape* shape() const override;
    void on_contact(const labrador::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    PlayerTeam top_paint_team(float world_x) const override;
    std::vector<PaintRunoffVisual> runoff_visuals() const override;
    void append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
        std::size_t source_index = 0) const override;
    bool for_deletion() const override;
    void set_for_deletion(bool requested) override;

private:
    friend class Level;
    friend class Projectile;
    void bind_motion_id(HorizontalMotionId id);
    HorizontalPreparedStep prepare_motion(float dt) const;
    void commit_motion(const HorizontalPreparedStep& prepared) noexcept;
    void publish_zero_step() noexcept;
    // Accepted relative-sweep geometry stays local; it is never re-gated by
    // a final-world narrow phase or reconstructed from a rounded world copy.
    void paint_projectile_at(const Projectile& source,
        const mattmath::RectangleF& anchor_rectangle);
    static const mattmath::RectangleF& checked_anchor(const HorizontalMotionSpec& spec,
        CollisionObjectType collision_type);

    HorizontalMotion motion_;
    mattmath::RectangleF world_rectangle_ = mattmath::RectangleF::ZERO;
    HorizontalMotionStep collision_step_;
    HorizontalMotionId motion_id_ = 0;
};
