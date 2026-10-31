#include "game/pch.h"
#include "game/objects/level_bot_world.h"
#include "game/objects/collider.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/level_structures.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/weapon.h"
#include "engine/math/intersects.h"
#include "engine/math/segment.h"
#include "engine/scene/scene.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

using namespace mattmath;
using namespace labrador;

namespace
{
    bool passable_gate(const CollisionObject* object, PlayerTeam team)
    {
        if (team == PlayerTeam::none) { return false; }
        const Structure* structure = Collider::of(*object).structure();
        return structure != nullptr && structure->passable_for(team);
    }
}

LevelBotWorld::LevelBotWorld(const Scene& scene, const std::vector<Player*>& players,
    const LevelStructures& structures) :
    scene_(scene), players_(players), structures_(structures)
{
}

int LevelBotWorld::participant_count() const
{
    return static_cast<int>(this->players_.size());
}

BotBody LevelBotWorld::body(int participant) const
{
    const Player& player = *this->players_.at(static_cast<std::size_t>(participant));
    const Weapon& weapon = player.primary_weapon();
    const PlayerMoveState move = player.move_state();
    const PlayerPaintUnderfootState underfoot = player.paint_underfoot_state();
    BotBody body;
    body.center = player.center();
    body.velocity = player.velocity();
    body.team = player.team();
    body.alive = player.state() == PlayerState::alive;
    body.health = player.health();
    body.grounded = move == PlayerMoveState::on_ground || move == PlayerMoveState::on_drop_down_ground ||
        move == PlayerMoveState::on_ramp_left || move == PlayerMoveState::on_ramp_right;
    body.on_platform = move == PlayerMoveState::on_drop_down_ground;
    body.facing_right = player.facing_right();
    body.ammo = player.weapon_ammo();
    body.handicap = player.handicap();
    body.underfoot_enabled = underfoot.enabled;
    body.underfoot_team = underfoot.step_team;
    body.weapon = weapon.type();
    const WeaponReadiness readiness = weapon.readiness();
    body.tool_ready = readiness == WeaponReadiness::ready;
    body.tool_empty = readiness == WeaponReadiness::empty;
    body.muzzle_reach = Vector2F::distance(weapon.nozzle_pos(), player.center());
    return body;
}

std::vector<PlayerTeam> LevelBotWorld::gate_permissions() const
{
    const auto gates = this->structures_.team_gates();
    std::vector<PlayerTeam> permissions;
    permissions.reserve(gates.size());
    for (const TeamGateStructurePaintable* gate : gates) { permissions.push_back(gate->open_for()); }
    return permissions;
}

std::vector<PaintSurfaceSample> LevelBotWorld::paint_surfaces() const
{
    std::vector<PaintSurfaceSample> samples;
    std::size_t source_index = 0;
    for (const CollisionObject* object : this->scene_.collision_objects())
    {
        const PaintableObject* paintable = Collider::of(*object).paintable();
        if (paintable == nullptr)
        {
            if (to_collision_type(object->tag()) == CollisionObjectType::structure_paintable)
            {
                throw std::runtime_error("Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
            }
            continue;
        }
        paintable->append_paint_surfaces(samples, source_index++);
    }
    return samples;
}

std::vector<RectangleF> LevelBotWorld::obstacles(PlayerTeam permitted) const
{
    std::vector<RectangleF> bounds;
    for (const CollisionObject* object : this->structures_.projectile_targets())
    {
        if (is_structure(to_collision_type(object->tag())) && object->shape() != nullptr &&
            !passable_gate(object, permitted))
        { bounds.push_back(object->shape()->bounding_box()); }
    }
    return bounds;
}

std::vector<MatchBotSupport> LevelBotWorld::supports(PlayerTeam permitted) const
{
    std::vector<MatchBotSupport> supports;
    // The target list is fixed for the match, so a position in it is a stable
    // identity even for a support that moves.
    std::uint32_t id = 0;
    for (const CollisionObject* object : this->structures_.projectile_targets())
    {
        ++id;
        const Structure* structure = Collider::of(*object).structure();
        if (structure != nullptr && structure->flat() && object->shape() != nullptr &&
            !passable_gate(object, permitted))
        {
            supports.push_back({ object->shape()->bounding_box(),
                structure->jump_through(), structure->paintable() != nullptr, id,
                structure->mover() != nullptr });
        }
    }
    return supports;
}

std::vector<MatchBotRamp> LevelBotWorld::ramps() const
{
    std::vector<MatchBotRamp> ramps;
    for (const CollisionObject* object : this->structures_.projectile_targets())
    {
        const CollisionObjectType type = to_collision_type(object->tag());
        if (is_structure_ramp(type) && object->shape() != nullptr)
        {
            ramps.push_back({ object->shape()->bounding_box(),
                type == CollisionObjectType::structure_ramp_right });
        }
    }
    return ramps;
}

bool LevelBotWorld::line_of_sight(Vector2F from, Vector2F to) const
{
    return this->traversal_line_of_sight(from, to, PlayerTeam::none);
}

bool LevelBotWorld::traversal_line_of_sight(Vector2F from, Vector2F to, PlayerTeam team) const
{
    if (!std::isfinite(from.x) || !std::isfinite(from.y) ||
        !std::isfinite(to.x) || !std::isfinite(to.y)) { return false; }
    const Segment sight(from, to);
    for (const CollisionObject* object : this->structures_.projectile_targets())
    {
        if (is_structure(to_collision_type(object->tag())) && object->shape() != nullptr &&
            !passable_gate(object, team) &&
            rectangle_segment_intersect(object->shape()->bounding_box(), sight)) { return false; }
    }
    return true;
}
