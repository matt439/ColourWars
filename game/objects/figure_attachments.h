#pragma once

#include "game/objects/crude_drawing.h"
#include "game/objects/weapon.h"
#include "game/presentation.h"
#include "engine/math/scalar.h"

#include <algorithm>
#include <array>
#include <stdexcept>

// Read-only details attached to a figure and its actual tool pivot. No pose
// writes into the weapon, so continuous aim and the launch muzzle stay exact.
class FigureAttachments final : public CrudeDrawing
{
public:
    FigureAttachments(const MovementFeedbackPresentation* fill,
        const FigurePresentation* look, labrador::RenderResources* resources,
        int teammate_index) :
        CrudeDrawing(fill->sprite_sheet, fill->fill_frame, resources), look_(look)
    {
        if (look_ == nullptr) { return; }
        if (teammate_index < 0 || teammate_index >= static_cast<int>(look_->accent_frames.size()))
        {
            throw std::runtime_error("Figure head accent requires a teammate index from 0 to 3");
        }
        sheet_ = resources->resolve_sprite_sheet(look_->sprite_sheet);
        const auto* sheet = resources->sprite_sheet(sheet_);
        accent_ = sheet->resolve_sprite_frame(look_->accent_frames[static_cast<size_t>(teammate_index)]);
        rest_arms_ = sheet->resolve_sprite_frame(look_->rest_arms_frame);
    }

    void draw_accent(labrador::DrawList& list, const mattmath::RectangleF& figure,
        bool facing_right) const
    {
        if (look_ == nullptr) { return; }
        this->art_sheet()->draw(list, accent_, this->accent_rectangle(figure, facing_right),
            labrador::Colour::white, 0.0f, mattmath::Vector2F::ZERO,
            facing_right ? labrador::SpriteFlip::none : labrador::SpriteFlip::horizontal);
    }

    void draw_rest_arms(labrador::DrawList& list, const mattmath::RectangleF& figure,
        bool facing_right) const
    {
        if (look_ == nullptr) { return; }
        this->art_sheet()->draw(list, rest_arms_, figure, labrador::Colour::white,
            0.0f, mattmath::Vector2F::ZERO, facing_right
                ? labrador::SpriteFlip::none : labrador::SpriteFlip::horizontal);
    }

    void draw_blink(labrador::DrawList& list, const mattmath::RectangleF& figure,
        bool facing_right) const
    {
        if (look_ == nullptr || !look_->quiet.enabled) { return; }
        for (const auto& eye : look_->quiet.eyes)
        {
            // Leave a short black lid at the middle of the authored square eye.
            // The eye paint-out follows the same posed canvas as the face.
            auto local = eye;
            if (!facing_right) { local.x = look_->canvas_size.x - local.right(); }
            const mattmath::Vector2F scale(figure.width / look_->canvas_size.x,
                figure.height / look_->canvas_size.y);
            const mattmath::RectangleF target(figure.x + local.x * scale.x,
                figure.y + local.y * scale.y, local.width * scale.x, local.height * scale.y);
            this->fill(list, target, look_->quiet.face_colour);
            const float lid = look_->quiet.eyelid_height * scale.y;
            this->fill(list, mattmath::RectangleF(target.x,
                target.y + (target.height - lid) / 2.0f, target.width, lid), look_->arm_colour);
        }
    }

    void draw_arms(labrador::DrawList& list, const mattmath::RectangleF& figure,
        bool facing_right, const Weapon& weapon) const
    {
        if (look_ == nullptr) { return; }
        const auto shoulders = this->shoulders(figure, facing_right);
        const auto hands = this->hands(weapon);
        const float width = look_->arm_width * this->scale(figure);
        for (size_t arm = 0; arm < hands.size(); ++arm)
        {
            const auto elbow = this->elbow(shoulders[arm], hands[arm], figure);
            this->line(list, shoulders[arm], elbow, width, look_->arm_colour);
            this->line(list, elbow, hands[arm], width, look_->arm_colour);
        }
    }

    // Hands are drawn after the tool, so the grip reads as held, not pasted
    // over the forearm. Both are centred on the same points the arms reach.
    void draw_hands(labrador::DrawList& list, const mattmath::RectangleF& figure,
        const Weapon& weapon) const
    {
        if (look_ == nullptr) { return; }
        const float scale = this->scale(figure);
        const float size = look_->hand_size * scale;
        const float edge = look_->hand_edge * scale;
        for (const auto& hand : this->hands(weapon))
        {
            this->fill(list, mattmath::RectangleF(hand - mattmath::Vector2F(size / 2.0f),
                mattmath::Vector2F(size)), look_->arm_colour);
            this->fill(list, mattmath::RectangleF(hand - mattmath::Vector2F(size / 2.0f - edge),
                mattmath::Vector2F(size - edge * 2.0f)), look_->hand_colour);
        }
    }

    mattmath::RectangleF bounds(const mattmath::RectangleF& figure, bool facing_right,
        const Weapon* held_weapon) const
    {
        if (look_ == nullptr) { return figure; }
        auto result = mattmath::RectangleF::union_of(figure,
            this->accent_rectangle(figure, facing_right));
        if (held_weapon == nullptr) { return result; }
        const auto shoulders = this->shoulders(figure, facing_right);
        const auto hands = this->hands(*held_weapon);
        const float radius = std::max(look_->hand_size, look_->arm_width) *
            this->scale(figure) / 2.0f;
        for (size_t arm = 0; arm < hands.size(); ++arm)
        {
            for (const auto& point : { shoulders[arm], hands[arm],
                this->elbow(shoulders[arm], hands[arm], figure) })
            {
                result = mattmath::RectangleF::union_of(result,
                    mattmath::RectangleF(point - mattmath::Vector2F(radius),
                        mattmath::Vector2F(radius * 2.0f)));
            }
        }
        return result;
    }

private:
    const FigurePresentation* look_ = nullptr;
    labrador::RenderResources::SpriteSheetHandle sheet_;
    labrador::SpriteSheet::frame_handle accent_;
    labrador::SpriteSheet::frame_handle rest_arms_;

    const labrador::SpriteSheet* art_sheet() const
    {
        return this->render_resources()->sprite_sheet(sheet_);
    }

    float scale(const mattmath::RectangleF& figure) const
    {
        return std::min(figure.width / look_->canvas_size.x,
            figure.height / look_->canvas_size.y);
    }

    mattmath::Vector2F figure_point(const mattmath::RectangleF& figure,
        bool facing_right, mattmath::Vector2F point) const
    {
        if (!facing_right) { point.x = look_->canvas_size.x - point.x; }
        return figure.position() + mattmath::Vector2F(point.x * figure.width / look_->canvas_size.x,
            point.y * figure.height / look_->canvas_size.y);
    }

    mattmath::RectangleF accent_rectangle(const mattmath::RectangleF& figure,
        bool facing_right) const
    {
        auto offset = look_->accent_offset;
        if (!facing_right) { offset.x += look_->accent_size.x; }
        return mattmath::RectangleF(this->figure_point(figure, facing_right, offset),
            mattmath::Vector2F(look_->accent_size.x * figure.width / look_->canvas_size.x,
                look_->accent_size.y * figure.height / look_->canvas_size.y));
    }

    std::array<mattmath::Vector2F, 2> shoulders(const mattmath::RectangleF& figure,
        bool facing_right) const
    {
        return { this->figure_point(figure, facing_right, look_->rear_shoulder),
            this->figure_point(figure, facing_right, look_->front_shoulder) };
    }

    std::array<mattmath::Vector2F, 2> hands(const Weapon& weapon) const
    {
        const auto& grip = look_->grips.at(static_cast<size_t>(weapon.type()));
        const float rotation = weapon.rotation();
        const bool flip = rotation > mattmath::PI_OVER_2 || rotation < -mattmath::PI_OVER_2;
        const auto point = [&](mattmath::Vector2F local)
        {
            if (flip) { local.y = -local.y; }
            return weapon.draw_pos() + mattmath::Vector2F::rotate_vector(local, rotation);
        };
        return { point(grip.rear), point(grip.front) };
    }

    mattmath::Vector2F elbow(const mattmath::Vector2F& shoulder,
        const mattmath::Vector2F& hand, const mattmath::RectangleF& figure) const
    {
        return (shoulder + hand) / 2.0f +
            mattmath::Vector2F(0.0f, look_->elbow_drop * this->scale(figure));
    }
};
