#pragma once

#include "game/tuning.h"
#include "engine/render/animation_object.h"

#include <optional>

// A second animation object keeps the team fill on the body's exact pose.
// Player selects, resets and advances both clips together; draws are read-only.
class FigureClothing final : public labrador::AnimationObject
{
public:
    FigureClothing(const PlayerAnimationTuning& clip,
        labrador::RenderResources* resources, const labrador::Colour& colour,
        float rotation, const mattmath::Vector2F& origin,
        labrador::SpriteFlip flip, float layer_depth) :
        AnimationObject(clip.sheet, clip.paint_strip, resources, colour,
            rotation, origin, flip, layer_depth)
    {
        if (clip.frame_time.has_value()) { this->set_frame_time(*clip.frame_time); }
    }

    // paint_strip is the clip's paint strip, resolved by Player at construction.
    void select(labrador::RenderResources::SheetStrip paint_strip,
        std::optional<float> frame_time)
    {
        this->set_animation_strip_and_reset(paint_strip);
        if (frame_time.has_value()) { this->set_frame_time(*frame_time); }
        this->play();
    }

    void advance(float dt) { AnimationObject::update(dt); }
    void select_frame(int frame) { this->set_frame_index(frame); }

    void draw(labrador::DrawList& list, const mattmath::RectangleF& rectangle,
        const labrador::Colour& colour, labrador::SpriteFlip flip) const
    {
        this->draw_with(list, rectangle, colour, flip);
    }
};
