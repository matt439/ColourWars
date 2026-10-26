#include "game/pch.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/projectile.h"
#include "game/objects/translated_draw_scope.h"
#include <limits>
#include <stdexcept>

using namespace mattmath;
using namespace labrador;

const RectangleF& TranslatingStructurePaintable::checked_anchor(
    const HorizontalMotionSpec& spec, CollisionObjectType collision_type)
{
    HorizontalMotion::validate_spec(spec);
    if (collision_type != CollisionObjectType::structure_paintable &&
        collision_type != CollisionObjectType::structure_jump_through)
    {
        throw std::invalid_argument("A translating paintable needs solid or jump-through flat collision.");
    }
    return spec.anchor;
}

TranslatingStructurePaintable::TranslatingStructurePaintable(
    const std::string& sheet_name, const std::string& frame_name,
    const HorizontalMotionSpec& motion, RenderResources* render_resources,
    const AudioResources* audio_resources, const PlaySettings* preferences,
    const PaintTuning* paint, CollisionObjectType collision_type,
    const TeamColour& team_colours, const PaintableFaces& faces,
    const Colour& colour, const Vector2F& origin, SpriteFlip flip,
    float layer_depth, std::unique_ptr<const TerrainMaterial> material) :
    StructurePaintable(sheet_name, frame_name, checked_anchor(motion, collision_type),
        std::make_unique<RectangleF>(motion.anchor), render_resources, audio_resources,
        preferences, paint, collision_type, team_colours, faces, {}, colour,
        0.0f, origin, flip, layer_depth, std::move(material)),
    motion_(motion), world_rectangle_(this->motion_.bounds())
{
    this->publish_zero_step();
}

HorizontalMotionId TranslatingStructurePaintable::motion_id() const
{
    return this->motion_id_;
}
HorizontalMotionState TranslatingStructurePaintable::motion_state() const
{
    return this->motion_.state();
}
HorizontalMotionStep TranslatingStructurePaintable::collision_step() const
{
    return this->collision_step_;
}
Vector2F TranslatingStructurePaintable::translation() const
{
    return this->motion_.state().current_offset;
}
void TranslatingStructurePaintable::bind_motion_id(HorizontalMotionId id)
{
    if (this->motion_id_ != 0 || id == 0 ||
        id > static_cast<HorizontalMotionId>((std::numeric_limits<int>::max)()))
    {
        throw std::invalid_argument("A translating structure's positive Level ID is bound exactly once.");
    }
    this->motion_id_ = id;
}
HorizontalPreparedStep TranslatingStructurePaintable::prepare_motion(float dt) const
{
    return this->motion_.prepare(dt);
}
void TranslatingStructurePaintable::commit_motion(const HorizontalPreparedStep& prepared) noexcept
{
    this->motion_.apply(prepared);
    this->world_rectangle_ = prepared.collision.after;
    this->collision_step_ = prepared.collision;
}
void TranslatingStructurePaintable::publish_zero_step() noexcept
{
    this->collision_step_ = {};
    this->collision_step_.before = this->world_rectangle_;
    this->collision_step_.after = this->world_rectangle_;
}
void TranslatingStructurePaintable::paint_projectile_at(const Projectile& source,
    const RectangleF& anchor_rectangle)
{
    this->paint_from_projectile(source, anchor_rectangle);
}
void TranslatingStructurePaintable::draw(DrawList& list) const
{
    const TranslatedDrawScope camera(list, this->translation());
    this->StructurePaintable::draw(list);
}
void TranslatingStructurePaintable::draw_miniature(DrawList& list) const
{
    const TranslatedDrawScope camera(list, this->translation());
    this->StructurePaintable::draw_miniature(list);
}
RectangleF TranslatingStructurePaintable::bounds() const
{
    return this->world_rectangle_;
}
RectangleF TranslatingStructurePaintable::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->StructurePaintable::cull_bounds(units_per_pixel);
    const Vector2F offset = this->translation();
    if (offset.x != 0.0f) { result.offset(offset.x, 0.0f); }
    return result;
}
const Shape* TranslatingStructurePaintable::shape() const
{
    return &this->world_rectangle_;
}
void TranslatingStructurePaintable::on_contact(const CollisionObject& other,
    const Vector2F& /*normal*/, float /*penetration*/)
{
    const CollisionLayer layer = other.layer();
    if (layer != collision_layers::PROJECTILE_TEAM_A &&
        layer != collision_layers::PROJECTILE_TEAM_B) { return; }
    const Projectile& source = dynamic_cast<const Projectile&>(other);
    if (this->motion_id_ != 0 && source.projectile_type() == ProjectileType::rolling &&
        source.moving_paint_completed(this->motion_id_)) { return; }
    RectangleF splash = source.shape()->bounding_box();
    const float offset = this->translation().x;
    if (offset != 0.0f) { splash.x -= offset; }
    this->paint_from_projectile(source, splash);
}
PlayerTeam TranslatingStructurePaintable::top_paint_team(float world_x) const
{
    const float offset = this->translation().x;
    return this->StructurePaintable::top_paint_team_translated(world_x, offset);
}
std::vector<PaintRunoffVisual> TranslatingStructurePaintable::runoff_visuals() const
{
    std::vector<PaintRunoffVisual> visuals = this->StructurePaintable::runoff_visuals();
    const float offset = this->translation().x;
    if (offset == 0.0f) { return visuals; }
    for (PaintRunoffVisual& visual : visuals)
    {
        visual.owned_band.offset(offset, 0.0f);
        if (visual.trail.width > 0.0f && visual.trail.height > 0.0f)
        { visual.trail.offset(offset, 0.0f); }
        if (visual.bead.width > 0.0f && visual.bead.height > 0.0f)
        { visual.bead.offset(offset, 0.0f); }
    }
    return visuals;
}
void TranslatingStructurePaintable::append_anchor_paint_surfaces(
    std::vector<PaintSurfaceSample>& samples) const
{
    this->StructurePaintable::append_paint_surfaces(samples, 0);
}
void TranslatingStructurePaintable::append_paint_surfaces(
    std::vector<PaintSurfaceSample>& samples, std::size_t source_index) const
{
    const std::size_t begin = samples.size();
    this->StructurePaintable::append_paint_surfaces(samples, source_index);
    const float offset = this->translation().x;
    if (offset != 0.0f)
    {
        for (std::size_t index = begin; index < samples.size(); ++index)
        {
            for (Vector2F& corner : samples[index].corners) { corner.x += offset; }
        }
    }
}
bool TranslatingStructurePaintable::for_deletion() const
{
    return false;
}
void TranslatingStructurePaintable::set_for_deletion(bool /*requested*/)
{
}
