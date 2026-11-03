#pragma once

#include "game/objects/airbrush_shadow.h"

class TranslatingStructurePaintable;

// The actual admitted mover outlives every draw. Scene destroys collision
// owners before plain shadows, so this default destructor never observes it.
class TranslatedAirbrushShadow final : public labrador::GameObject
{
public:
    TranslatedAirbrushShadow(const TranslatingStructurePaintable& source,
        labrador::RenderResources* render_resources);
    ~TranslatedAirbrushShadow() override = default;
    void update(float dt) override;
    void draw(labrador::DrawList& list) const override;
    labrador::RectangleF bounds() const override;
    labrador::RectangleF cull_bounds(float units_per_pixel) const override;
private:
    static float checked_left(const TranslatingStructurePaintable& source,
        labrador::RenderResources* resources);
    const TranslatingStructurePaintable& source_;
    AirbrushShadow anchor_;
};
