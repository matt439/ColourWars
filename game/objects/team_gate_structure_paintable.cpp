#include "game/pch.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/player.h"
#include "game/team_gate_contract.h"

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
    std::span<const PaintPolygon> paint_polygons) :
    StructurePaintable(sheet_name, frame_name, rectangle, std::make_unique<RectangleF>(rectangle),
        render_resources, audio_resources, preferences, paint,
        CollisionObjectType::structure_paintable, team_colours, faces, paint_obstacles,
        colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f, std::move(material), paint_polygons),
    open_share_(checked_share(paint))
{
    if (!faces.left && !faces.top && !faces.right && !faces.bottom)
    { throw std::invalid_argument("A team gate needs a paintable face."); }
}

PlayerTeam TeamGateStructurePaintable::open_for() const
{
    return this->open_for_;
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
                return;
            }
        }
    }
    this->open_for_ = leader;
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
