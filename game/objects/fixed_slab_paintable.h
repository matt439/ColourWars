#pragma once

#include "game/objects/fixed_slab_geometry.h"
#include "game/objects/structure_paintable.h"
#include <span>

// A fixed sloping solid with paint held in its own rigid frame. The ordinary
// axis-aligned StructurePaintable paths keep their original geometry/arithmetic.
class FixedSlabPaintable final : public Structure, public PaintableObject
{
public:
    FixedSlabPaintable(const FixedSlabSpec& spec,
        labrador::RenderResources* render_resources,
        const labrador::AudioResources* audio_resources,
        const PlaySettings* preferences, const PaintTuning* paint,
        const TeamColour& team_colours, const PaintableFaces& faces,
        std::span<const mattmath::RectangleF> flat_obstacles = {});
    FixedSlabPaintable(const FixedSlabPaintable&) = delete;
    FixedSlabPaintable& operator=(const FixedSlabPaintable&) = delete;

    const FixedSlabGeometry& geometry() const;
    PlayerTeam top_paint_team(float world_x) const;
    std::optional<labrador::Colour> top_paint_colour(float world_x) const;
    const TerrainMaterial& local_material() const;
    const std::vector<std::pair<float, float>>& exposed_bottom() const;

    void update(float dt) override;
    void draw(labrador::DrawList& draw_list) const override;
    void draw_miniature(labrador::DrawList& draw_list) const override;
    mattmath::RectangleF bounds() const override;
    mattmath::RectangleF cull_bounds(float units_per_pixel) const override;
    void on_contact(const labrador::CollisionObject& other,
        const mattmath::Vector2F& normal, float penetration) override;
    PaintTotal paint_total() const override;
    void append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
        std::size_t source_index = 0) const override;

private:
    struct Tile
    {
        PaintSurfaceFace face;
        PaintTile paint;
    };
    struct Strip
    {
        mattmath::RectangleF band;
        bool along_x;
        bool leading;
    };

    FixedSlabGeometry geometry_;
    const PaintTuning* paint_;
    TeamColour team_colours_;
    PaintSplashClips splash_clips_;
    std::vector<Tile> tiles_;
    std::vector<Strip> strips_;
    std::vector<std::pair<float, float>> exposed_bottom_;
    std::unique_ptr<const TerrainMaterial> local_material_;
    labrador::RenderResources::SpriteSheetHandle terrain_sheet_;
    std::array<labrador::SpriteSheet::frame_handle,
        static_cast<std::size_t>(TerrainRole::count)> terrain_swatches_{};

    void draw_local(labrador::DrawList& draw_list,
        const mattmath::RectangleF& rectangle, const labrador::Colour& colour) const;
};
