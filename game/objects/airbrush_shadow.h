#pragma once

#include "engine/core/game_object.h"
#include "engine/render/texture_object.h"
#include "engine/math/rectanglef.h"

#include <array>

// The airbrush speckle under one exposed underside of a floating platform:
// baked tiles from the terrain sheet laid along the span, the last one
// cropped rather than stretched. It is drawn before every structure, so it
// lands on the backdrop and never on terrain or paint. One object per span,
// so each view culls it on its own (docs/polish-terrain.md).
class AirbrushShadow final : public labrador::GameObject, public labrador::TextureObject
{
public:
	static constexpr int VARIANTS = 4;

	// Under the exposed interval [left, right) of an underside at `top`; one
	// tile tall.
	AirbrushShadow(float left, float right, float top,
		labrador::RenderResources* render_resources);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;
	mattmath::RectangleF cull_bounds(float units_per_pixel) const override;

private:
	mattmath::RectangleF span_;
	std::array<mattmath::RectangleI, VARIANTS> tiles_{};
};
