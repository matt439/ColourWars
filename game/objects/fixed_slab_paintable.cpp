#include "game/pch.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/projectile.h"
#include "engine/collision/narrow_phase.h"
#include "game/fixed_slab_paint_geometry.h"
#include <cmath>

using namespace mattmath;
using namespace labrador;

namespace
{
    std::vector<std::pair<float, float>> exposed_edge(const FixedSlabGeometry& geometry,
        bool along_x, bool leading, std::span<const RectangleF> obstacles)
    {
        const auto& spec = geometry.spec();
        const auto end = spec.top_left + Vector2F(160.0f, spec.rise == FixedSlabRise::right ? -80.0f : 80.0f);
        const auto& points = geometry.shape().points();
        const PaintPolygon represented{ PaintPoint{ points[0].x, points[0].y }, PaintPoint{ points[1].x, points[1].y },
            PaintPoint{ points[2].x, points[2].y }, PaintPoint{ points[3].x, points[3].y } };
        const auto normal = geometry.inward_normal();
        const auto polygon = canonical_slab_paint_polygon(represented,
            { spec.top_left.x, spec.top_left.y }, { end.x, end.y }, { normal.x, normal.y });
        std::vector<PaintFaceRectangle> flat;
        flat.reserve(obstacles.size());
        for (const auto& obstacle : obstacles)
        { flat.push_back({ obstacle.left(), obstacle.top(), obstacle.right(), obstacle.bottom() }); }
        return exposed_slab_paint_edge(polygon, along_x ? geometry.length() : 24.0f, along_x, leading, flat);
    }
    const PaintTuning* checked_paint(const PaintTuning* paint)
    {
        if (paint == nullptr || !(paint->thickness > 0.0f) ||
            paint->thickness > FixedSlabGeometry::normal_thickness / 2.0f ||
            !(paint->size.x > 0.0f && paint->size.y > 0.0f) ||
            !std::isfinite(paint->thickness) || !std::isfinite(paint->size.x) ||
            !std::isfinite(paint->size.y))
        { throw std::invalid_argument("Fixed slab paint needs finite positive tiles fitting its normal thickness"); }
        return paint;
    }
}

FixedSlabPaintable::FixedSlabPaintable(const FixedSlabSpec& spec,
    RenderResources* render_resources, const AudioResources* audio_resources,
    const PlaySettings* preferences, const PaintTuning* paint,
    const TeamColour& team_colours, const PaintableFaces& faces,
    std::span<const RectangleF> flat_obstacles) :
    Structure(checked_paint(paint)->sheet, checked_paint(paint)->frame,
        FixedSlabGeometry(spec).bounds(),
        std::make_unique<RectangleRotated>(FixedSlabGeometry(spec).shape()),
        render_resources, CollisionObjectType::structure_fixed_slab),
    geometry_(spec), paint_(checked_paint(paint)), team_colours_(team_colours),
    splash_clips_(PaintSplashClips::resolve(*this->paint_, *render_resources)),
    terrain_sheet_(render_resources->resolve_sprite_sheet(TerrainMaterial::SHEET_NAME))
{
    // Sound batching remains Level-owned, exactly as for the flat structures.
    // This fixed slice has no separate gravity runoff policy for sloped faces.
    (void)audio_resources; (void)preferences;
    TerrainFaces exposed;
    exposed.top = exposed_edge(this->geometry_, true, true, flat_obstacles);
    exposed.bottom = exposed_edge(this->geometry_, true, false, flat_obstacles);
    exposed.left = exposed_edge(this->geometry_, false, true, flat_obstacles);
    exposed.right = exposed_edge(this->geometry_, false, false, flat_obstacles);
    this->exposed_bottom_ = exposed.bottom;
    this->local_material_ = std::make_unique<EraserMaterial>(
        this->geometry_.local_rectangle(), exposed, render_resources);
    const SpriteSheet* terrain = render_resources->sprite_sheet(this->terrain_sheet_);
    for (std::size_t index = 0; index < this->terrain_swatches_.size(); ++index)
    {
        this->terrain_swatches_[index] = terrain->resolve_sprite_frame(
            std::string("swatch_") + terrain_role_name(static_cast<TerrainRole>(index)));
    }
    const RectangleF local = this->geometry_.local_rectangle();
    const auto append_face = [&](bool enabled, PaintSurfaceFace face, bool along_x, bool leading,
        const std::vector<std::pair<float, float>>& intervals)
    {
        if (!enabled) { return; }
        const float edge = along_x ? (leading ? local.top() : local.bottom())
            : (leading ? local.left() : local.right());
        for (const auto& interval : intervals)
        {
            const float length = interval.second - interval.first;
            if (!(length > 0.0f)) { continue; }
            const float target = along_x ? this->paint_->size.x : this->paint_->size.y;
            const int count = std::max(1, static_cast<int>(length / target));
            const float pitch = length / count;
            const RectangleF band = along_x
                ? RectangleF(interval.first, leading ? edge : edge - this->paint_->thickness,
                    length, this->paint_->thickness)
                : RectangleF(leading ? edge : edge - this->paint_->thickness, interval.first,
                    this->paint_->thickness, length);
            this->strips_.push_back({ band, along_x, leading,
                this->tiles_.size(), static_cast<std::size_t>(count) });
            for (int index = 0; index < count; ++index)
            {
                const float position = interval.first + pitch * index;
                const RectangleF tile = along_x
                    ? RectangleF(position, band.y, pitch, band.height)
                    : RectangleF(band.x, position, band.width, pitch);
                this->tiles_.push_back({ face, PaintTile(tile, this->paint_, &this->splash_clips_,
                    render_resources, this->team_colours_, Colour::white, 0.0f,
                    Vector2F::ZERO, SpriteFlip::none, 0.0f, along_x, leading) });
            }
        }
    };
    append_face(faces.top, PaintSurfaceFace::top, true, true, exposed.top);
    append_face(faces.bottom, PaintSurfaceFace::bottom, true, false, exposed.bottom);
    append_face(faces.left, PaintSurfaceFace::left, false, true, exposed.left);
    append_face(faces.right, PaintSurfaceFace::right, false, false, exposed.right);
}

const FixedSlabGeometry& FixedSlabPaintable::geometry() const { return this->geometry_; }
const TerrainMaterial& FixedSlabPaintable::local_material() const { return *this->local_material_; }
const std::vector<std::pair<float, float>>& FixedSlabPaintable::exposed_bottom() const { return this->exposed_bottom_; }
RectangleF FixedSlabPaintable::bounds() const { return this->geometry_.bounds(); }

void FixedSlabPaintable::update(float dt)
{
    for (Tile& tile : this->tiles_) { tile.paint.update(dt); }
}

void FixedSlabPaintable::draw_local(DrawList& draw_list,
    const RectangleF& rectangle, const Colour& colour) const
{
    this->sprite_sheet()->draw(draw_list,
        this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle(),
        RectangleF(this->geometry_.to_world(rectangle.top_left()), rectangle.size()),
        colour, this->geometry_.tangent().angle(), Vector2F::ZERO);
}

void FixedSlabPaintable::draw(DrawList& draw_list) const
{
    const SpriteSheet* terrain = this->render_resources()->sprite_sheet(this->terrain_sheet_);
    for (const TerrainQuad& quad : this->local_material_->quads())
    {
        terrain->draw(draw_list, terrain->sprite_frame(this->terrain_swatches_[
            static_cast<std::size_t>(quad.role)]).source_rectangle(),
            RectangleF(this->geometry_.to_world(quad.rectangle.top_left()), quad.rectangle.size()),
            Colour::white, this->geometry_.tangent().angle(), Vector2F::ZERO);
    }
    for (const Strip& strip : this->strips_)
    {
        this->draw_local(draw_list, strip.band, Colour::white);
        const float depth = strip.along_x ? strip.band.height : strip.band.width;
        const float edge = std::min(this->paint_->edge_width, depth / 2.0f);
        const RectangleF inner = strip.along_x
            ? RectangleF(strip.band.x, strip.leading ? strip.band.bottom() - edge : strip.band.top(),
                strip.band.width, edge)
            : RectangleF(strip.leading ? strip.band.right() - edge : strip.band.left(), strip.band.y,
                edge, strip.band.height);
        this->draw_local(draw_list, inner, Colour::black);
    }
    for (const Tile& tile : this->tiles_) { tile.paint.draw_splash_rigid(draw_list, this->geometry_); }
    for (const Strip& strip : this->strips_)
    {
        const std::size_t strip_end = strip.first_tile + strip.tile_count;
        for (std::size_t begin = strip.first_tile; begin < strip_end;)
        {
            const PaintTile& first = this->tiles_[begin].paint;
            if (first.team() == PlayerTeam::none) { ++begin; continue; }
            std::size_t end = begin + 1;
            while (end < strip_end && this->tiles_[end].paint.team() == first.team()) { ++end; }
            const RectangleF band = RectangleF::union_of(first.bounds(), this->tiles_[end - 1].paint.bounds());
            // Quantizing each rotated tile separately exposes primer at their
            // joins. One same-team opaque base covers the contiguous face;
            // all original tiles, owner credit and wet ages remain untouched.
            first.draw_surface_base_rigid(draw_list, this->geometry_, band);
            for (std::size_t index = begin; index < end; ++index)
            { this->tiles_[index].paint.draw_surface_wet_rigid(draw_list, this->geometry_); }
            begin = end;
        }
    }
}

void FixedSlabPaintable::draw_miniature(DrawList& draw_list) const { this->draw(draw_list); }

RectangleF FixedSlabPaintable::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->bounds();
    for (const Tile& tile : this->tiles_)
    {
        result = RectangleF::union_of(result,
            this->geometry_.world_quad(tile.paint.visual_bounds()).bounding_box());
    }
    // Sprite destinations quantize their anchor and both dimensions before
    // rotation. Bound that submission error as well as the local artwork;
    // a single cardinal pixel is insufficient for a diagonal corner.
    result.inflate(3.0f * units_per_pixel);
    return result;
}

void FixedSlabPaintable::on_projectile_contact(const Projectile& projectile,
    const Vector2F& /*normal*/, float /*penetration*/)
{
    if (projectile.layer() != collision_layers::PROJECTILE_TEAM_A &&
        projectile.layer() != collision_layers::PROJECTILE_TEAM_B) { return; }
    if (projectile.shape()->shape_type() != ShapeType::rectangle)
    { throw std::invalid_argument("Fixed slab paint currently requires rectangular projectile footprints"); }
    const PlayerTeam team = projectile.layer() == collision_layers::PROJECTILE_TEAM_A ? PlayerTeam::a : PlayerTeam::b;
    const Quad footprint = this->geometry_.local_quad(projectile.shape()->bounding_box());
    bool painted = false, captured = false;
    for (Tile& tile : this->tiles_)
    {
        if (!narrow_phase(tile.paint.bounds(), footprint)) { continue; }
        const PlayerTeam before = tile.paint.team();
        captured |= before != team;
        tile.paint.paint(team, projectile.player_num(), projectile.projectile_type());
        this->credit_.add(projectile.player_num(), before, team, tile.paint.area());
        painted = true;
    }
    Projectile::answer_paint_contact(projectile, painted, captured);
}

PaintTotal FixedSlabPaintable::paint_total() const
{
    PaintTotal result;
    result.credit = this->credit_;
    for (const Tile& tile : this->tiles_)
    {
        const float area = tile.paint.area();
        result.total += area;
        if (tile.paint.team() == PlayerTeam::a) { result.team_a += area; }
        else if (tile.paint.team() == PlayerTeam::b) { result.team_b += area; }
        const int owner = tile.paint.player_num();
        if (owner >= 0 && static_cast<std::size_t>(owner) < result.player_areas.size())
        { result.player_areas[static_cast<std::size_t>(owner)] += area; }
    }
    return result;
}

void FixedSlabPaintable::append_paint_surfaces(std::vector<PaintSurfaceSample>& samples,
    std::size_t source_index) const
{
    samples.reserve(samples.size() + this->tiles_.size());
    for (const Tile& tile : this->tiles_)
    {
        const Quad quad = this->geometry_.world_quad(tile.paint.bounds());
        samples.push_back({ { quad.point_0(), quad.point_1(), quad.point_2(), quad.point_3() },
            tile.face, tile.paint.team(), tile.paint.player_num(), tile.paint.area(), source_index });
    }
}

PlayerTeam FixedSlabPaintable::top_paint_team(float world_x) const
{
    for (const Tile& tile : this->tiles_)
    {
        if (tile.face != PaintSurfaceFace::top) { continue; }
        const RectangleF rectangle = tile.paint.bounds();
        const float left = this->geometry_.to_world(rectangle.top_left()).x;
        const float right = this->geometry_.to_world(rectangle.top_right()).x;
        if (world_x >= left && world_x < right) { return tile.paint.team(); }
    }
    return PlayerTeam::none;
}

std::optional<Colour> FixedSlabPaintable::top_paint_colour(float world_x) const
{
    const PlayerTeam team = this->top_paint_team(world_x);
    if (team == PlayerTeam::none) { return std::nullopt; }
    return this->team_colours_.team_colour(team);
}

const FixedSlabGeometry* FixedSlabPaintable::slab_geometry() const
{
    return &this->geometry_;
}

Vector2F FixedSlabPaintable::cover_point(const Vector2F& shot_centre, const Vector2F& normal) const
{
    // Clamp into the slab's local box, then put the point on the face the
    // normal enters: an end cap along the tangent, else the top or underside.
    const auto& geometry = this->geometry_;
    Vector2F local = geometry.to_local(shot_centre);
    local.x = std::clamp(local.x, 0.0f, geometry.length());
    local.y = std::clamp(local.y, 0.0f, FixedSlabGeometry::normal_thickness);
    const float along = normal.dot(geometry.tangent());
    const float across = normal.dot(geometry.inward_normal());
    if (std::abs(along) > std::abs(across)) { local.x = along > 0.0f ? 0.0f : geometry.length(); }
    else { local.y = across > 0.0f ? 0.0f : FixedSlabGeometry::normal_thickness; }
    return geometry.to_world(local);
}
