#include "game/pch.h"
#include "game/objects/weapon_umbrella.h"

using namespace labrador;

ToolMotionPose WeaponUmbrella::motion_pose() const
{
    const auto pose = Weapon::motion_pose();
    // The pleats spring on the paid volley; the tightened runner holds the
    // canopy up between volleys, including while the tank is empty.
    return pose == ToolMotionPose::idle && state_.open ? ToolMotionPose::lift : pose;
}

void WeaponUmbrella::load_canopy_art()
{
    const auto& canopy = this->weapon_tuning_->canopy;
    const RenderResources& render = *this->render_resources();
    this->canopy_frames_ = { render.resolve_sheet_frame(canopy.sheet, canopy.open_frame),
        render.resolve_sheet_frame(canopy.sheet, canopy.cracked_frame),
        render.resolve_sheet_frame(canopy.sheet, canopy.shut_frame) };
}

const RenderResources::SheetFrame* WeaponUmbrella::visible_canopy_frame() const
{
    const auto state = this->canopy_state();
    if (state.open)
    {
        return &this->canopy_frames_[state.hits_left < this->weapon_tuning_->canopy.hits ? 1 : 0];
    }
    // The idle weapon already carries folded ribs. A broken canopy leaves a
    // short crumpled remnant at its mount while the holder can still fire.
    return state.recovery_remaining > 0.0f ? &this->canopy_frames_[2] : nullptr;
}

void WeaponUmbrella::draw(DrawList& list, bool debug) const
{
    Weapon::draw(list, debug);
    if (const auto* frame = this->visible_canopy_frame())
    {
        const auto* sheet = this->render_resources()->sprite_sheet(frame->sheet());
        const SpriteFlip flip = this->invert_y() ? SpriteFlip::horizontal : SpriteFlip::none;
        sheet->draw(list, frame->frame(), this->canopy_bounds(), this->displayed_paint_colour(),
            0.0f, Vector2F::zero, flip);
    }
}

RectangleF WeaponUmbrella::draw_bounds() const
{
    const RectangleF weapon = Weapon::draw_bounds();
    return this->visible_canopy_frame() ? RectangleF::union_of(weapon, this->canopy_bounds()) : weapon;
}
