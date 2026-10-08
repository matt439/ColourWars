#pragma once

#include "game/workshop_visual_mode.h"
#include "game/objects/weapon_type.h"
#include "engine/render/texture_object.h"

#include <array>
#include <string>

class Weapon;

struct WorkshopToolPresentation
{
    std::string frame;
    std::string team_frame;
    mattmath::Vector2F size;
    // World-space vectors from the authored frame pivot, before mirroring.
    mattmath::Vector2F muzzle_offset;
    mattmath::Vector2F support_grip_offset;
    float aim_angle = 0.0f;
};

struct WorkshopPlayerPresentation
{
    std::string sprite_sheet;
    mattmath::Vector2F body_size;
    std::array<std::string, 2> idle_frames;
    std::array<std::string, 4> run_frames;
    std::string air_frame;
    std::string landing_frame;
    float idle_frame_time = 0.0f;
    float run_frame_time = 0.0f;
    float landing_duration = 0.0f;
    float pixel_size = 0.0f;
    std::string pixel_frame;
    mattmath::Vector2F shoulder_offset;
    mattmath::Vector2F rear_shoulder_offset;
    float elbow_drop = 0.0f;
    float sleeve_width = 0.0f;
    float arm_outline_width = 0.0f;
    labrador::Colour sleeve_colour;
    labrador::Colour arm_outline_colour;
    std::string glove_frame;
    mattmath::Vector2F glove_size;
    std::array<std::string, 4> badge_frames;
    mattmath::Vector2F badge_size;
    // From the top-centre of the visual body to the centre of the badge.
    mattmath::Vector2F badge_offset;
    WorkshopToolPresentation sprayer;
    std::array<WorkshopToolPresentation, 5> sprayer_poses;
    WorkshopToolPresentation roller;
};

// Game-owned cosmetic attachment. A single body-frame index selects its matching
// team mask, while gameplay retains ownership of aim, muzzle and collision shape.
class PlayerVisual final : public labrador::TextureObject
{
public:
    PlayerVisual(const WorkshopPlayerPresentation* presentation,
        labrador::RenderResources* render_resources, PlayerVisualMode mode,
        int view_index);

    void update(float dt, bool grounded, bool moving);
    void land();
    void takeoff();
    void reset();
    void draw(labrador::DrawList& draw_list,
        const mattmath::RectangleF& collision_rectangle, bool facing_right,
        const labrador::Colour& team_colour, const Weapon& weapon,
        bool debug = false) const;
    mattmath::RectangleF bounds(const mattmath::RectangleF& collision_rectangle,
        const Weapon& weapon) const;

    // Small observable presentation state for consumer acceptance.
    size_t body_frame_index() const;
    float animation_time() const;
    PlayerVisualMode mode() const;

private:
    struct FramePair
    {
        labrador::SpriteSheet::frame_handle base;
        labrador::SpriteSheet::frame_handle team;
    };
    struct Tool
    {
        const WorkshopToolPresentation* presentation = nullptr;
        FramePair frames;
    };
    struct Attachment
    {
        const Tool* tool = nullptr;
        mattmath::Vector2F grip;
        mattmath::Vector2F support_grip;
        mattmath::Vector2F muzzle;
        float rotation = 0.0f;
        labrador::SpriteFlip flip = labrador::SpriteFlip::none;
    };

    const WorkshopPlayerPresentation* presentation_;
    PlayerVisualMode mode_;
    std::array<FramePair, 8> body_frames_;
    Tool sprayer_;
    std::array<Tool, 5> sprayer_poses_;
    Tool roller_;
    labrador::SpriteSheet::frame_handle glove_frame_;
    labrador::SpriteSheet::frame_handle badge_frame_;
    size_t body_frame_index_ = 0;
    float animation_time_ = 0.0f;
    float idle_phase_offset_ = 0.0f;
    float landing_remaining_ = 0.0f;
    bool was_grounded_ = false;
    bool was_moving_ = false;

    FramePair resolve_pair(const std::string& frame, const std::string& team) const;
    Attachment attachment(const Weapon& weapon) const;
    mattmath::RectangleF body_rectangle(
        const mattmath::RectangleF& collision_rectangle) const;
    mattmath::RectangleF badge_rectangle(
        const mattmath::RectangleF& body_rectangle) const;
    void draw_pair(labrador::DrawList& draw_list, const FramePair& frames,
        const mattmath::Vector2F& position, const mattmath::Vector2F& size,
        const labrador::Colour& team_colour, labrador::SpriteFlip flip,
        float rotation) const;
    void draw_frame(labrador::DrawList& draw_list,
        labrador::SpriteSheet::frame_handle frame,
        const mattmath::Vector2F& position, const mattmath::Vector2F& size,
        const labrador::Colour& colour, labrador::SpriteFlip flip,
        float rotation) const;
    void draw_arm(labrador::DrawList& draw_list,
        const mattmath::Vector2F& shoulder, const mattmath::Vector2F& grip,
        const mattmath::Vector2F& grid_origin) const;
    void draw_segment(labrador::DrawList& draw_list,
        const mattmath::Vector2F& start, const mattmath::Vector2F& finish,
        const mattmath::Vector2F& grid_origin, float width,
        const labrador::Colour& colour) const;
};
