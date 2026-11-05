#pragma once

#include "game/config/presentation/presentation_match.h"
#include "game/objects/crude_drawing.h"

enum class WeaponReadiness
{
    ready,
    recovering,
    empty,
};

struct WeaponFeedbackState
{
    float muzzle_remaining = 0.0f;
    float empty_remaining = 0.0f;
    float recovered_remaining = 0.0f;
    bool operator==(const WeaponFeedbackState&) const = default;
};

// POL#38: whether the tool is low on paint, how much of the warning band is
// left (ammunition over the low line, [0, 1)) and its simulated low age.
struct LowPaintCue
{
    bool low = false;
    float level = 0.0f;
    float elapsed = 0.0f;
    // The player's centre. The tank's horizontal offset is taken from it, not
    // the tool pivot, so it clears the identity badge in every facing.
    float body_x = 0.0f;
};

// Three bounded event pulses and a stateless readiness marker. Drawing another
// view never advances them, and they never create paint or gameplay objects.
class WeaponFeedback : public CrudeDrawing
{
public:
    WeaponFeedback(const WeaponFeedbackPresentation* presentation,
        labrador::RenderResources* render_resources);

    void update(float dt);
    void shot(WeaponType type);
    void empty();
    void recovered();
    void clear();
    const WeaponFeedbackState& state() const;

    // The low-paint tank shares the status slot and yields to the pulses.
    void draw(labrador::DrawList& draw_list, const labrador::Vector2F& nozzle,
        const labrador::Vector2F& pivot, float angle, WeaponType type,
        const labrador::Colour& team_colour, bool reduce_effects = false,
        const LowPaintCue& low = {}) const;
    labrador::RectangleF bounds(const labrador::Vector2F& nozzle,
        const labrador::Vector2F& pivot, WeaponType type, const LowPaintCue& low = {}) const;

    // Persistent, directional information, separate from the event pulses.
    // Level draws it only in the owning player's pane while that player can act.
    void draw_aim_marker(labrador::DrawList& draw_list,
        const labrador::Vector2F& nozzle, float angle, WeaponReadiness readiness,
        const labrador::Colour& team_colour) const;
    labrador::RectangleF aim_marker_bounds(const labrador::Vector2F& nozzle,
        float angle) const;

private:
    void draw_block(labrador::DrawList& draw_list,
        const labrador::Vector2F& center, const labrador::Vector2F& size,
        float angle, const labrador::Colour& colour, float alpha) const;

    const WeaponFeedbackPresentation* presentation_ = nullptr;
    WeaponFeedbackState state_;
};
