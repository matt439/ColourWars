#pragma once

#include "game/objects/aim_facing.h"
#include "game/objects/crude_drawing.h"
#include "game/objects/weapon.h"
#include "game/config/presentation/presentation_match.h"
#include "engine/math/scalar.h"

#include <algorithm>
#include <array>

// Read-only details attached to a figure and its actual tool pivot. No pose
// writes into the weapon, so continuous aim and the launch muzzle stay exact.
// Every figure rectangle here is the whole canvas, overhang included
// (FigurePresentation::canvas_rectangle), not the hitbox.
class FigureAttachments final : public CrudeDrawing
{
public:
    FigureAttachments(const MovementFeedbackPresentation* fill,
        const FigurePresentation* look, labrador::RenderResources* resources) :
        CrudeDrawing(fill->sprite_sheet, fill->fill_frame, resources), look_(look)
    {
        if (look_ == nullptr) { return; }
        sheet_ = resources->resolve_sprite_sheet(look_->sprite_sheet);
        const auto* sheet = resources->sprite_sheet(sheet_);
        rest_arms_ = { sheet->resolve_sprite_frame(look_->rest_arms_frame),
            sheet->resolve_sprite_frame(look_->rest_arms_paint_frame) };
        sleeves_ = { sheet->resolve_sprite_frame(look_->sleeves_frame),
            sheet->resolve_sprite_frame(look_->sleeves_paint_frame) };
    }

    // Arms and hands while no tool is held; the sleeves take the team colour.
    void draw_rest_arms(labrador::DrawList& list, const labrador::RectangleF& figure,
        bool facing_right, const labrador::Colour& team) const
    {
        if (look_ == nullptr) { return; }
        this->draw_layers(list, rest_arms_, figure, facing_right, team);
    }

    // The sleeves alone while a tool is held, over the roots of the live arms.
    void draw_sleeves(labrador::DrawList& list, const labrador::RectangleF& figure,
        bool facing_right, const labrador::Colour& team) const
    {
        if (look_ == nullptr) { return; }
        this->draw_layers(list, sleeves_, figure, facing_right, team);
    }

    void draw_blink(labrador::DrawList& list, const labrador::RectangleF& figure,
        bool facing_right) const
    {
        if (look_ == nullptr) { return; }
        this->draw_blink(list, figure, facing_right, look_->quiet.eyes);
    }

    // The same, over eyes the match's person drew elsewhere (FigurePerson).
    void draw_blink(labrador::DrawList& list, const labrador::RectangleF& figure,
        bool facing_right, const std::array<labrador::RectangleF, 2>& eyes) const
    {
        if (look_ == nullptr || !look_->quiet.enabled) { return; }
        for (const auto& eye : eyes)
        {
            // Leave a short ink lid at the middle of the authored eye.
            // The eye paint-out follows the same posed canvas as the face.
            auto local = eye;
            if (!facing_right) { local.x = look_->canvas_size.x - local.right(); }
            const labrador::Vector2F scale(figure.width / look_->canvas_size.x,
                figure.height / look_->canvas_size.y);
            const labrador::RectangleF target(figure.x + local.x * scale.x,
                figure.y + local.y * scale.y, local.width * scale.x, local.height * scale.y);
            this->fill(list, target, premultiplied(look_->quiet.face_colour));
            const float lid = look_->quiet.eyelid_height * scale.y;
            this->fill(list, labrador::RectangleF(target.x,
                target.y + (target.height - lid) / 2.0f, target.width, lid), premultiplied(look_->arm_colour));
        }
    }

    void draw_arms(labrador::DrawList& list, const labrador::RectangleF& figure,
        bool facing_right, const Weapon& weapon) const
    {
        if (look_ == nullptr) { return; }
        const auto shoulders = this->shoulders(figure, facing_right);
        const auto hands = this->hands(weapon);
        const float width = look_->arm_width * this->scale(figure);
        for (size_t arm = 0; arm < hands.size(); ++arm)
        {
            const auto elbow = this->elbow(shoulders[arm], hands[arm], figure);
            this->line(list, shoulders[arm], elbow, width, premultiplied(look_->arm_colour));
            this->line(list, elbow, hands[arm], width, premultiplied(look_->arm_colour));
        }
    }

    // Hands are drawn after the tool, so the grip reads as held, not pasted
    // over the forearm. Both are centred on the same points the arms reach.
    void draw_hands(labrador::DrawList& list, const labrador::RectangleF& figure,
        const Weapon& weapon) const
    {
        if (look_ == nullptr) { return; }
        const float scale = this->scale(figure);
        const float size = look_->hand_size * scale;
        const float edge = look_->hand_edge * scale;
        for (const auto& hand : this->hands(weapon))
        {
            this->fill(list, labrador::RectangleF(hand - labrador::Vector2F(size / 2.0f),
                labrador::Vector2F(size)), premultiplied(look_->arm_colour));
            this->fill(list, labrador::RectangleF(hand - labrador::Vector2F(size / 2.0f - edge),
                labrador::Vector2F(size - edge * 2.0f)), premultiplied(look_->hand_colour));
        }
    }

    labrador::RectangleF bounds(const labrador::RectangleF& figure, bool facing_right,
        const Weapon* held_weapon) const
    {
        if (look_ == nullptr || held_weapon == nullptr) { return figure; }
        auto result = figure;
        const auto shoulders = this->shoulders(figure, facing_right);
        const auto hands = this->hands(*held_weapon);
        const float radius = std::max(look_->hand_size, look_->arm_width) *
            this->scale(figure) / 2.0f;
        for (size_t arm = 0; arm < hands.size(); ++arm)
        {
            for (const auto& point : { shoulders[arm], hands[arm],
                this->elbow(shoulders[arm], hands[arm], figure) })
            {
                result = labrador::RectangleF::union_of(result,
                    labrador::RectangleF(point - labrador::Vector2F(radius),
                        labrador::Vector2F(radius * 2.0f)));
            }
        }
        return result;
    }

private:
    const FigurePresentation* look_ = nullptr;
    labrador::RenderResources::SpriteSheetHandle sheet_;
    // Neutral, then team paint.
    std::array<labrador::SpriteSheet::frame_handle, 2> rest_arms_;
    std::array<labrador::SpriteSheet::frame_handle, 2> sleeves_;

    const labrador::SpriteSheet* art_sheet() const
    {
        return this->render_resources()->sprite_sheet(sheet_);
    }

    void draw_layers(labrador::DrawList& list,
        const std::array<labrador::SpriteSheet::frame_handle, 2>& layers,
        const labrador::RectangleF& figure, bool facing_right, const labrador::Colour& team) const
    {
        const auto flip = facing_right ? labrador::SpriteFlip::none : labrador::SpriteFlip::horizontal;
        this->art_sheet()->draw(list, layers[1], figure, team, 0.0f, labrador::Vector2F::zero, flip);
        this->art_sheet()->draw(list, layers[0], figure, labrador::Colour::white,
            0.0f, labrador::Vector2F::zero, flip);
    }

    float scale(const labrador::RectangleF& figure) const
    {
        return std::min(figure.width / look_->canvas_size.x,
            figure.height / look_->canvas_size.y);
    }

    labrador::Vector2F figure_point(const labrador::RectangleF& figure,
        bool facing_right, labrador::Vector2F point) const
    {
        if (!facing_right) { point.x = look_->canvas_size.x - point.x; }
        return figure.position() + labrador::Vector2F(point.x * figure.width / look_->canvas_size.x,
            point.y * figure.height / look_->canvas_size.y);
    }

    std::array<labrador::Vector2F, 2> shoulders(const labrador::RectangleF& figure,
        bool facing_right) const
    {
        return { this->figure_point(figure, facing_right, look_->rear_shoulder),
            this->figure_point(figure, facing_right, look_->front_shoulder) };
    }

    std::array<labrador::Vector2F, 2> hands(const Weapon& weapon) const
    {
        const auto& grip = look_->grips.at(static_cast<size_t>(weapon.type()));
        const float rotation = weapon.rotation();
        const bool flip = aim_faces_left(rotation);
        const auto point = [&](labrador::Vector2F local)
        {
            if (flip) { local.y = -local.y; }
            return weapon.draw_pos() + labrador::Vector2F::rotate_vector(local, rotation);
        };
        return { point(grip.rear), point(grip.front) };
    }

    labrador::Vector2F elbow(const labrador::Vector2F& shoulder,
        const labrador::Vector2F& hand, const labrador::RectangleF& figure) const
    {
        return (shoulder + hand) / 2.0f +
            labrador::Vector2F(0.0f, look_->elbow_drop * this->scale(figure));
    }
};
