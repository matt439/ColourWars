#pragma once

#include "game/presentation.h"
#include "engine/render/texture_object.h"

// One player's bounded cosmetic hit cues. Simulation updates the timers once;
// every render view reads the same hurt accent and detached elimination burst.
class CombatFeedback final : public labrador::TextureObject
{
public:
    CombatFeedback(const CombatFeedbackPresentation* presentation,
        labrador::RenderResources* render_resources);

    void update(float dt);
    // Accepted damage may arrive more often than a readable visual accent.
    // Returns whether this hit started a new accent.
    bool hurt();
    void eliminate(const mattmath::RectangleF& death_rectangle);
    void clear();

    void draw(labrador::DrawList& draw_list,
        const mattmath::RectangleF& current_body,
        const labrador::Colour& team_colour) const;
    mattmath::RectangleF bounds(const mattmath::RectangleF& current_body) const;

    float hurt_remaining() const;
    float elimination_remaining() const;
    const mattmath::RectangleF& death_rectangle() const;

private:
    const CombatFeedbackPresentation* presentation_;
    float hurt_remaining_ = 0.0f;
    float hurt_delay_remaining_ = 0.0f;
    float elimination_remaining_ = 0.0f;
    mattmath::RectangleF death_rectangle_ = mattmath::RectangleF::ZERO;

    void draw_fill(labrador::DrawList& draw_list,
        const mattmath::RectangleF& rectangle,
        const labrador::Colour& colour, float alpha) const;
    void draw_keyline(labrador::DrawList& draw_list,
        const mattmath::RectangleF& rectangle, float alpha) const;
};
