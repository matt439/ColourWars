#pragma once

#include "game/presentation.h"
#include "engine/render/texture_object.h"

struct WeaponFeedbackState
{
    float muzzle_remaining = 0.0f;
    float empty_remaining = 0.0f;
    float recovered_remaining = 0.0f;
    bool operator==(const WeaponFeedbackState&) const = default;
};

// Three bounded event pulses, owned and updated by one weapon. Drawing another
// view never advances them, and they never create paint or gameplay objects.
class WeaponFeedback : public labrador::TextureObject
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

    void draw(labrador::DrawList& draw_list, const mattmath::Vector2F& nozzle,
        const mattmath::Vector2F& pivot, float angle, WeaponType type,
        const labrador::Colour& team_colour) const;
    mattmath::RectangleF bounds(const mattmath::Vector2F& nozzle,
        const mattmath::Vector2F& pivot, WeaponType type) const;

private:
    void draw_block(labrador::DrawList& draw_list,
        const mattmath::Vector2F& center, const mattmath::Vector2F& size,
        float angle, const labrador::Colour& colour, float alpha) const;

    const WeaponFeedbackPresentation* presentation_ = nullptr;
    WeaponFeedbackState state_;
};
