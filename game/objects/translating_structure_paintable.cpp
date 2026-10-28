#include "game/pch.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/projectile.h"
#include "game/objects/translated_draw_scope.h"
#include "game/presentation.h"
#include <array>
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
    float layer_depth, std::unique_ptr<const TerrainMaterial> material,
    const FerryDirectionPresentation* cue) :
    StructurePaintable(sheet_name, frame_name, checked_anchor(motion, collision_type),
        std::make_unique<RectangleF>(motion.anchor), render_resources, audio_resources,
        preferences, paint, collision_type, team_colours, faces, {}, colour,
        0.0f, origin, flip, layer_depth, std::move(material)),
    motion_(motion), world_rectangle_(this->motion_.bounds()),
    cue_(motion.drive == HorizontalDrive::paint ? cue : nullptr), team_colours_(team_colours)
{
    this->publish_zero_step();
    if (this->cue_ != nullptr)
    { this->cue_frame_ = render_resources->resolve_sheet_frame("sprite_sheet_1", "pixel"); }
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
    return this->motion_.prepare(dt, this->drive_heading());
}
int TranslatingStructurePaintable::drive_heading() const
{
    const HorizontalMotionSpec& spec = this->motion_.state_.spec;
    if (spec.drive != HorizontalDrive::paint) { return 0; }
    const PaintTotal paint = this->paint_total();
    return HorizontalMotion::paint_heading(spec, paint.team_a, paint.team_b);
}
FerryCueState TranslatingStructurePaintable::cue_state() const
{
    FerryCueState state;
    const HorizontalMotionSpec& spec = this->motion_.state_.spec;
    state.paint_driven = spec.drive == HorizontalDrive::paint;
    if (!state.paint_driven) { return state; }
    const PaintTotal paint = this->paint_total();
    state.demand_heading = HorizontalMotion::paint_heading(spec, paint.team_a, paint.team_b);
    state.demand_team = paint.team_a == paint.team_b ? PlayerTeam::none :
        paint.team_a > paint.team_b ? PlayerTeam::a : PlayerTeam::b;
    state.committed_heading = this->committed_heading_;
    state.movement_heading = this->movement_heading_;
    state.motion = this->motion_cue_;
    return state;
}

RectangleF TranslatingStructurePaintable::anchor_cue_bounds() const
{
    if (this->cue_ == nullptr) { return RectangleF::ZERO; }
    const RectangleF& body = this->motion_.state_.spec.anchor;
    const float scale = std::min(1.0f, body.width / this->cue_->size.x);
    const Vector2F size = this->cue_->size * scale;
    return RectangleF(body.center().x - size.x * 0.5f,
        body.bottom() + this->cue_->gap, size.x, size.y);
}

RectangleF TranslatingStructurePaintable::cue_bounds() const
{
    RectangleF result = this->anchor_cue_bounds();
    if (result.width > 0.0f) { result.offset(this->translation()); }
    return result;
}

void TranslatingStructurePaintable::draw_direction(DrawList& list) const
{
    const RectangleF plaque = this->anchor_cue_bounds();
    if (!(plaque.width > 0.0f && plaque.height > 0.0f)) { return; }
    const FerryDirectionPresentation& look = *this->cue_;
    const FerryCueState state = this->cue_state();
    const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->cue_frame_.sheet());
    const auto fill = [&](const RectangleF& rectangle, const Colour& colour)
    { sheet->draw(list, this->cue_frame_.frame(), rectangle, colour); };
    const float border = look.keyline_width * plaque.width / look.size.x;
    fill(plaque, look.ink);
    fill(RectangleF(plaque.x + border, plaque.y + border,
        plaque.width - border * 2.0f, plaque.height - border * 2.0f), look.paper);

    // A fixed 17-column/11-row grammar. The two opposed team arrows are
    // always present, so an idle rider can see which repaint reverses demand.
    // The independently sampled lower glyph describes the committed step.
    const float cell = std::min((plaque.width - border * 4.0f) / 18.0f,
        (plaque.height - border * 4.0f) / 12.0f);
    const Vector2F origin(plaque.center().x - cell * 8.5f, plaque.center().y - cell * 5.5f);
    const auto mark = [&](float x, float y, float width, float height, const Colour& colour)
    {
        fill(RectangleF(origin.x + x * cell, origin.y + y * cell,
            width * cell, height * cell), colour);
    };
    constexpr std::array<unsigned int, 5> a{ 2u, 5u, 7u, 5u, 5u };
    constexpr std::array<unsigned int, 5> b{ 6u, 5u, 6u, 5u, 6u };
    const auto glyph = [&](const std::array<unsigned int, 5>& bits, int x, int y, const Colour& colour)
    {
        for (int row = 0; row < 5; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                if ((bits[static_cast<size_t>(row)] & (1u << (2 - column))) != 0u)
                { mark(static_cast<float>(x + column), static_cast<float>(y + row), 1, 1, colour); }
            }
        }
    };
    const auto arrow = [&](int x, int y, int heading, const Colour& colour)
    {
        constexpr std::array<unsigned int, 5> right{ 4u, 2u, 1u, 2u, 4u };
        constexpr std::array<unsigned int, 5> left{ 1u, 2u, 4u, 2u, 1u };
        glyph(heading > 0 ? right : left, x, y, colour);
    };
    for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
    {
        const int x = team == PlayerTeam::a ? 0 : 10;
        const bool demanded = state.demand_team == team;
        if (demanded) { mark(static_cast<float>(x) - 0.5f, -0.5f, 8, 6, look.ink); }
        const Colour ink = demanded ? look.paper : look.ink;
        glyph(team == PlayerTeam::a ? a : b, x, 0, ink);
        const int heading = this->motion_.state_.spec.team_a_heading * (team == PlayerTeam::a ? 1 : -1);
        arrow(x + 4, 0, heading, ink);
        // Hue reinforces the letter; it is never the only team distinction.
        mark(static_cast<float>(x), -0.5f, 7, 0.5f, this->team_colours_.team_colour(team));
    }
    switch (state.motion)
    {
    case FerryMotionCue::unstepped:
        mark(6, 8, 1, 1, look.ink); mark(8, 8, 1, 1, look.ink); mark(10, 8, 1, 1, look.ink);
        break;
    case FerryMotionCue::held:
        mark(6, 6, 1, 5, look.ink); mark(10, 6, 1, 5, look.ink);
        break;
    case FerryMotionCue::moving:
        arrow(7, 6, state.movement_heading, look.ink);
        break;
    case FerryMotionCue::left_stop:
        mark(5, 6, 1, 5, look.ink); arrow(7, 6, -1, look.ink);
        break;
    case FerryMotionCue::right_stop:
        arrow(7, 6, 1, look.ink); mark(11, 6, 1, 5, look.ink);
        break;
    }
}
void TranslatingStructurePaintable::commit_motion(const HorizontalPreparedStep& prepared) noexcept
{
    this->motion_.apply(prepared);
    this->world_rectangle_ = prepared.collision.after;
    this->collision_step_ = prepared.collision;
    if (this->motion_.state_.spec.drive == HorizontalDrive::paint && prepared.collision.dt > 0.0f)
    {
        this->committed_heading_ = prepared.paint_heading;
        // Carry and collision use these rounded world poses. A positive local
        // offset advance can still round to the same world X at a distant
        // anchor, so it must not claim actual displacement here.
        const float delta = prepared.collision.after.x - prepared.collision.before.x;
        this->movement_heading_ = delta > 0.0f ? 1 : delta < 0.0f ? -1 : 0;
        if (prepared.paint_heading < 0 && prepared.after_state.phase == 0.0)
        { this->motion_cue_ = FerryMotionCue::left_stop; }
        else if (prepared.paint_heading > 0 && prepared.after_state.phase == 0.5)
        { this->motion_cue_ = FerryMotionCue::right_stop; }
        else
        { this->motion_cue_ = delta == 0.0f ? FerryMotionCue::held : FerryMotionCue::moving; }
    }
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
    this->draw_direction(list);
}
void TranslatingStructurePaintable::draw_miniature(DrawList& list) const
{
    const TranslatedDrawScope camera(list, this->translation());
    this->StructurePaintable::draw_miniature(list);
    this->draw_direction(list);
}
RectangleF TranslatingStructurePaintable::bounds() const
{
    return this->world_rectangle_;
}
RectangleF TranslatingStructurePaintable::cull_bounds(float units_per_pixel) const
{
    RectangleF result = this->StructurePaintable::cull_bounds(units_per_pixel);
    const RectangleF cue = this->anchor_cue_bounds();
    if (cue.width > 0.0f && cue.height > 0.0f)
    {
        RectangleF expanded = cue;
        expanded.inflate(units_per_pixel);
        result = RectangleF::union_of(result, expanded);
    }
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
