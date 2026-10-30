#include "game/pch.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/player.h"
#include "game/team_gate_contract.h"
#include "game/presentation.h"
#include <array>

using namespace mattmath;
using namespace labrador;

namespace
{
    float checked_share(const PaintTuning* paint)
    {
        if (paint == nullptr) { throw std::invalid_argument("A team gate needs paint tuning."); }
        team_gate_contract::validate_open_share(paint->gate_open_share);
        return paint->gate_open_share;
    }

    team_gate_contract::Rectangle contract_rectangle(const RectangleF& rectangle)
    {
        return { rectangle.x, rectangle.y, rectangle.width, rectangle.height };
    }
}

TeamGateStructurePaintable::TeamGateStructurePaintable(const std::string& sheet_name,
    const std::string& frame_name, const RectangleF& rectangle,
    RenderResources* render_resources, const AudioResources* audio_resources,
    const PlaySettings* preferences, const PaintTuning* paint,
    const TeamColour& team_colours, const PaintableFaces& faces,
    std::span<const RectangleF> paint_obstacles, const Colour& colour,
    std::unique_ptr<const TerrainMaterial> material,
    std::span<const PaintPolygon> paint_polygons,
    const GatePermissionPresentation* cue) :
    StructurePaintable(sheet_name, frame_name, rectangle, std::make_unique<RectangleF>(rectangle),
        render_resources, audio_resources, preferences, paint,
        CollisionObjectType::structure_paintable, team_colours, faces, paint_obstacles,
        colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f, std::move(material), paint_polygons),
    open_share_(checked_share(paint)), paint_thickness_(paint->thickness),
    cue_(cue), team_colours_(team_colours)
{
    if (!faces.left && !faces.top && !faces.right && !faces.bottom)
    { throw std::invalid_argument("A team gate needs a paintable face."); }
    if (cue != nullptr)
    { this->cue_frame_ = render_resources->resolve_sheet_frame("sprite_sheet_1", "pixel"); }
}

PlayerTeam TeamGateStructurePaintable::open_for() const
{
    return this->open_for_;
}

bool TeamGateStructurePaintable::passable_for(PlayerTeam team) const
{
    return team != PlayerTeam::none && this->open_for_ == team;
}

PlayerTeam TeamGateStructurePaintable::paint_leader() const
{
    const PaintTotal paint = this->paint_total();
    switch (team_gate_contract::leader(paint.team_a, paint.team_b, paint.total, this->open_share_))
    {
    case team_gate_contract::Leader::team_a: return PlayerTeam::a;
    case team_gate_contract::Leader::team_b: return PlayerTeam::b;
    case team_gate_contract::Leader::none:
    default: return PlayerTeam::none;
    }
}

void TeamGateStructurePaintable::settle(std::span<Player* const> players)
{
    this->held_open_ = false;
    const PlayerTeam leader = this->paint_leader();
    if (leader == this->open_for_) { return; }
    if (this->open_for_ != PlayerTeam::none)
    {
        const team_gate_contract::Rectangle gate = contract_rectangle(this->rectangle());
        for (const Player* player : players)
        {
            if (player != nullptr && player->team() == this->open_for_ &&
                player->state() == PlayerState::alive &&
                team_gate_contract::overlaps(gate, contract_rectangle(player->rectangle())))
            {
                this->held_open_ = true;
                return;
            }
        }
    }
    this->open_for_ = leader;
}

bool TeamGateStructurePaintable::held_open() const
{
    return this->held_open_;
}

RectangleF TeamGateStructurePaintable::permission_bounds() const
{
    if (this->cue_ == nullptr) { return RectangleF::ZERO; }
    const RectangleF& body = this->rectangle();
    const float inset = std::max(this->cue_->inset, this->paint_thickness_);
    const float width = body.width - inset * 2.0f;
    const float height = body.height - inset * 2.0f;
    if (!(width > 0.0f && height > 0.0f)) { return RectangleF::ZERO; }
    const float scale = std::min({ 1.0f, width / this->cue_->size.x,
        height / this->cue_->size.y });
    const Vector2F size = this->cue_->size * scale;
    return RectangleF(body.center().x - size.x * 0.5f, body.top() + inset, size.x, size.y);
}

void TeamGateStructurePaintable::draw(DrawList& draw_list) const
{
    this->StructurePaintable::draw(draw_list);
    this->draw_permission(draw_list);
}

void TeamGateStructurePaintable::draw_miniature(DrawList& draw_list) const
{
    this->StructurePaintable::draw_miniature(draw_list);
    this->draw_permission(draw_list);
}

void TeamGateStructurePaintable::draw_permission(DrawList& draw_list) const
{
    const RectangleF plaque = this->permission_bounds();
    if (!(plaque.width > 0.0f && plaque.height > 0.0f)) { return; }
    const GatePermissionPresentation& look = *this->cue_;
    const SpriteSheet* sheet = this->render_resources()->sprite_sheet(this->cue_frame_.sheet());
    const auto fill = [&](const RectangleF& rectangle, const Colour& colour)
    { sheet->draw(draw_list, this->cue_frame_.frame(), rectangle, colour); };
    const float scale = plaque.width / look.size.x;
    const float border = look.keyline_width * scale;
    fill(plaque, look.ink);
    fill(RectangleF(plaque.x + border, plaque.y + border,
        plaque.width - border * 2.0f, plaque.height - border * 2.0f), look.paper);

    // A fixed five-column, eleven-row sign: its grammar is code, its physical
    // size and keyline are presentation. A/B remain different without hue.
    const float cell = std::min((plaque.width - border * 4.0f) / 5.0f,
        (plaque.height - border * 4.0f) / 11.0f);
    const Vector2F origin(plaque.center().x - cell * 2.5f,
        plaque.center().y - cell * 5.5f);
    const auto mark = [&](int x, int y, int width, int height, const Colour& colour)
    {
        fill(RectangleF(origin.x + static_cast<float>(x) * cell,
            origin.y + static_cast<float>(y) * cell,
            static_cast<float>(width) * cell, static_cast<float>(height) * cell), colour);
    };
    if (this->open_for_ == PlayerTeam::none)
    {
        // A barred passage. The X is not a neutral team-colour guess.
        for (int row = 0; row < 5; ++row)
        {
            mark(row, row + 3, 1, 1, look.ink);
            if (row != 2) { mark(4 - row, row + 3, 1, 1, look.ink); }
        }
        return;
    }

    // Keep permission and hold together above an occupant's body/badge. The
    // team accent is a separate header, leaving all glyphs on plain paper.
    fill(RectangleF(plaque.x + border, plaque.y + border,
        plaque.width - border * 2.0f, border), this->team_colours_.team_colour(this->open_for_));
    constexpr std::array<unsigned int, 5> a{ 2u, 5u, 7u, 5u, 5u };
    constexpr std::array<unsigned int, 5> b{ 6u, 5u, 6u, 5u, 6u };
    const auto& glyph = this->open_for_ == PlayerTeam::a ? a : b;
    for (int row = 0; row < 5; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            if ((glyph[static_cast<size_t>(row)] & (1u << (2 - column))) != 0u)
            { mark(column + 1, row, 1, 1, look.ink); }
        }
    }
    // Two jambs with a clear horizontal passage: passage is bidirectional,
    // so neither team's sign implies a travel direction.
    mark(0, 6, 1, 3, look.ink);
    mark(4, 6, 1, 3, look.ink);
    mark(1, 7, 3, 1, look.ink);
    if (this->held_open_)
    {
        // Pause bars flank the permitted letter with half-cell gaps. Lower
        // marks can be behind the occupant, so never put the hold down there.
        fill(RectangleF(origin.x, origin.y, cell * 0.5f, cell * 5.0f), look.ink);
        fill(RectangleF(origin.x + cell * 4.5f, origin.y, cell * 0.5f, cell * 5.0f), look.ink);
    }
}

CollisionMask TeamGateStructurePaintable::mask() const
{
    const CollisionMask all = this->StructurePaintable::mask();
    switch (this->open_for_)
    {
    case PlayerTeam::a: return all & ~collision_layers::PLAYER_TEAM_A;
    case PlayerTeam::b: return all & ~collision_layers::PLAYER_TEAM_B;
    case PlayerTeam::none:
    default: return all;
    }
}
