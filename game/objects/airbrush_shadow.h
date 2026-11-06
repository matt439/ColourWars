#pragma once

#include "engine/core/game_object.h"
#include "engine/render/texture_object.h"
#include "engine/math/rectanglef.h"

#include <algorithm>
#include <array>
#include <cmath>

// The airbrush speckle under one exposed underside of a floating platform:
// baked tiles from the terrain sheet laid along the span, the last one
// cropped rather than stretched. It is drawn before every structure, so it
// lands on the backdrop and never on terrain or paint. One object per span,
// so each view culls it on its own (docs/polish-terrain.md).
class AirbrushShadow final : public labrador::GameObject, public labrador::TextureObject
{
public:
	static constexpr int variants = 4;
	// The terrain sheet's first baked tile, `shadow_0`; the rest follow it.
	static constexpr const char* first_tile = "shadow_0";
	using Tiles = std::array<labrador::RectangleI, variants>;

	// The source rectangles of the baked tiles, `shadow_0` onwards.
	static Tiles tiles(const labrador::SpriteSheet& sheet);
	// Lays the tiles along [begin, end) on the world grid: each tile picks a
	// variant from its grid index, so neighbouring platforms never repeat in
	// lockstep and nothing varies between frames or views. The tiles at the
	// ends are cropped, never stretched. `place(source, left, right)` draws
	// each non-empty tile.
	template <typename Place>
	static void lay(const Tiles& tiles, float begin, float end, const Place& place)
	{
		const float width = static_cast<float>(tiles[0].width);
		for (float x = std::floor(begin / width) * width; x < end; x += width)
		{
			const long long cell = static_cast<long long>(std::floor(x / width));
			const unsigned long long hash = static_cast<unsigned long long>(cell) * 2654435761ULL;
			labrador::RectangleI source = tiles[static_cast<size_t>((hash >> 16) % variants)];
			const float left = std::max(x, begin);
			const float right = std::min(x + width, end);
			const int skip = static_cast<int>(std::round(left - x));
			const int keep = static_cast<int>(std::round(right - left));
			if (keep <= 0) { continue; }
			source.x += skip;
			source.width = keep;
			place(source, left, right);
		}
	}

	// Under the exposed interval [left, right) of an underside at `top`; one
	// tile tall.
	AirbrushShadow(float left, float right, float top,
		labrador::RenderResources* render_resources);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF bounds() const override;
	labrador::RectangleF cull_bounds(float units_per_pixel) const override;

private:
	labrador::RectangleF span_;
	Tiles tiles_{};
};
