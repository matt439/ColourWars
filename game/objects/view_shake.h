#pragma once

#include "game/presentation_match.h"

// One bounded, deterministic camera accent per player. It never touches the
// base follow camera, input, physics or random stream. Rendering only reads it.
class ViewShake
{
public:
    explicit ViewShake(const ViewShakePresentation* presentation = nullptr);
    void update(float dt, bool enabled);
    void hurt();
    void eliminate();
    void land(float speed);
    void clear();
    mattmath::Vector2F offset() const;
    float remaining() const;

private:
    const ViewShakePresentation* presentation_;
    float remaining_ = 0.0f;
    float amplitude_ = 0.0f;
    float hurt_delay_ = 0.0f;
    float duration_ = 0.0f;
    float cycles_ = 0.0f;

    void impulse(float amplitude);
};
