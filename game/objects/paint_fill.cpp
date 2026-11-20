#include "game/pch.h"
#include "game/objects/paint_fill.h"
#include "game/contracts/team_gate_contract.h"
#include "game/objects/structure_paintable.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PaintFill::PaintFill(StructurePaintable& trigger, std::vector<PaintSurfaceFace> trigger_faces,
    StructurePaintable& target, PaintSurfaceFace target_face, float qualifying_share, float cooldown) :
    trigger_(trigger), trigger_faces_(std::move(trigger_faces)), target_(target),
    target_face_(target_face), qualifying_share_(qualifying_share), cooldown_(cooldown)
{
    team_gate_contract::validate_open_share(qualifying_share);
    if (trigger.mover() || target.mover())
    { throw std::invalid_argument("paint_fill structures must be static"); }
    if (!std::isfinite(cooldown) || cooldown <= 0.0f)
    { throw std::invalid_argument("paint_fill cooldown must be positive"); }
    trigger_geometry_ = trigger_tiles();
    target_geometry_ = target_tiles();
    if (trigger_faces_.empty() || target_geometry_.empty())
    { throw std::invalid_argument("paint_fill trigger and target need scoring tiles"); }
    for (const auto face : trigger_faces_)
    {
        if ((&trigger == &target && face == target_face) ||
            std::count(trigger_faces_.begin(), trigger_faces_.end(), face) != 1)
        { throw std::invalid_argument("paint_fill faces must be distinct"); }
        if (std::none_of(trigger_geometry_.begin(), trigger_geometry_.end(),
            [face](const auto& tile) { return tile.face == face; }))
        { throw std::invalid_argument("paint_fill trigger face has no scoring tiles"); }
    }
    for (const auto& tile : trigger_geometry_)
    {
        total_ += tile.scored_area;
        if (tile.team == PlayerTeam::a) { area_a_ += tile.scored_area; }
        if (tile.team == PlayerTeam::b) { area_b_ += tile.scored_area; }
    }
    leader_ = qualifying_leader();
    trigger_.set_paint_contact_observer([this](PaintSurfaceFace face, PlayerTeam before,
        PlayerTeam after, int owner, float area) { contact(face, before, after, owner, area); });
}

PaintFill::~PaintFill()
{
    trigger_.set_paint_contact_observer({});
}

labrador::RectangleF PaintFill::trigger_bounds() const { return trigger_.bounds(); }
labrador::RectangleF PaintFill::target_bounds() const { return target_.bounds(); }

std::vector<PaintSurfaceSample> PaintFill::trigger_tiles() const
{
    std::vector<PaintSurfaceSample> samples;
    trigger_.append_paint_surfaces(samples);
    std::erase_if(samples, [this](const auto& tile) {
        return std::find(trigger_faces_.begin(), trigger_faces_.end(), tile.face) == trigger_faces_.end();
    });
    return samples;
}

std::vector<PaintSurfaceSample> PaintFill::target_tiles() const
{
    std::vector<PaintSurfaceSample> samples;
    target_.append_paint_surfaces(samples);
    std::erase_if(samples, [this](const auto& tile) { return tile.face != target_face_; });
    return samples;
}

float PaintFill::trigger_area(PlayerTeam team) const
{
    if (team == PlayerTeam::a) { return area_a_; }
    if (team == PlayerTeam::b) { return area_b_; }
    return total_ - area_a_ - area_b_;
}

PlayerTeam PaintFill::qualifying_leader() const
{
    using namespace team_gate_contract;
    switch (team_gate_contract::leader(area_a_, area_b_, total_, qualifying_share_))
    {
    case Leader::team_a: return PlayerTeam::a;
    case Leader::team_b: return PlayerTeam::b;
    default: return PlayerTeam::none;
    }
}

void PaintFill::contact(PaintSurfaceFace face, PlayerTeam before, PlayerTeam after, int owner, float area)
{
    if (std::find(trigger_faces_.begin(), trigger_faces_.end(), face) == trigger_faces_.end()) { return; }
    if (before == PlayerTeam::a) { area_a_ -= area; }
    if (before == PlayerTeam::b) { area_b_ -= area; }
    if (after == PlayerTeam::a) { area_a_ += area; }
    if (after == PlayerTeam::b) { area_b_ += area; }
    const PlayerTeam next = qualifying_leader();
    if (next != leader_)
    {
        pending_owner_ = next != PlayerTeam::none && next == after ? owner : -1;
        leader_ = next;
    }
}

void PaintFill::settle(float live_dt)
{
    if (!(live_dt > 0.0f)) { return; }
    cooldown_remaining_ = std::max(0.0, cooldown_remaining_ - live_dt);
    if (last_team_ != PlayerTeam::none) { fill_age_ += live_dt; }
    if (cooldown_remaining_ > 0.0 || leader_ == PlayerTeam::none || leader_ == last_team_) { return; }

    changed_tiles_ = target_tiles();
    std::erase_if(changed_tiles_, [this](const auto& tile) { return tile.team == leader_; });
    target_.fill_paint_face(target_face_, leader_, pending_owner_);
    for (auto& tile : changed_tiles_)
    {
        tile.team = leader_;
        tile.paint_owner = pending_owner_;
    }
    last_team_ = leader_;
    cooldown_remaining_ = cooldown_;
    fill_age_ = 0.0f;
}
