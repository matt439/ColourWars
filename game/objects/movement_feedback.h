#pragma once

#include "game/presentation.h"
#include "engine/render/texture_object.h"

#include <array>
#include <span>

enum class MovementFeedbackKind
{
    takeoff,
    landing
};

// A player's bounded cosmetic trail. It owns no physics or collision objects;
// the player advances it once per tick, and every view reads the same snapshot.
class MovementFeedback final : public labrador::TextureObject
{
public:
    struct Effect
    {
        MovementFeedbackKind kind = MovementFeedbackKind::takeoff;
        mattmath::Vector2F position;
        float age = 0.0f;
        float strength = 1.0f;
    };

    MovementFeedback(const MovementFeedbackPresentation* presentation,
        labrador::RenderResources* render_resources);

    void update(float dt);
    void takeoff(const mattmath::Vector2F& position);
    void land(const mattmath::Vector2F& position, float speed);
    void clear();
    void draw(labrador::DrawList& draw_list) const;
    mattmath::RectangleF bounds(const mattmath::RectangleF& player_bounds) const;
    std::span<const Effect> effects() const;

private:
    // Storage capacity is an implementation bound, not a visual authoring knob.
    std::array<Effect, 8> effects_{};
    size_t count_ = 0;
    const MovementFeedbackPresentation* presentation_;

    const MovementBurstPresentation& style(MovementFeedbackKind kind) const;
    void emit(MovementFeedbackKind kind, const mattmath::Vector2F& position,
        float strength);
};
