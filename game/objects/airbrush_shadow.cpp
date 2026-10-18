#include "game/pch.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/terrain_material.h"

#include <cmath>

using namespace labrador;
using namespace mattmath;

AirbrushShadow::AirbrushShadow(float left, float right, float top,
	RenderResources* render_resources) :
	TextureObject(TerrainMaterial::SHEET_NAME, "shadow_0", render_resources)
{
	const SpriteSheet* sheet = this->sprite_sheet();
	for (int index = 0; index < VARIANTS; ++index)
	{
		this->tiles_[static_cast<size_t>(index)] = sheet->sprite_frame(
			sheet->resolve_sprite_frame("shadow_" + std::to_string(index))).source_rectangle();
	}
	this->span_ = RectangleF(left, top, right - left,
		static_cast<float>(this->tiles_[0].height));
}

void AirbrushShadow::update(float /*dt*/)
{
}

void AirbrushShadow::draw(DrawList& draw_list) const
{
	const SpriteSheet* sheet = this->sprite_sheet();
	const float tile_width = static_cast<float>(this->tiles_[0].width);
	// Tiles sit on the world grid and pick a variant from their grid index,
	// so neighbouring platforms never repeat in lockstep and nothing varies
	// between frames or views.
	const float first = std::floor(this->span_.left() / tile_width) * tile_width;
	for (float x = first; x < this->span_.right(); x += tile_width)
	{
		const long long cell = static_cast<long long>(std::floor(x / tile_width));
		const unsigned long long hash = static_cast<unsigned long long>(cell) * 2654435761ULL;
		RectangleI source = this->tiles_[static_cast<size_t>((hash >> 16) % VARIANTS)];
		const float left = std::max(x, this->span_.left());
		const float right = std::min(x + tile_width, this->span_.right());
		const int skip = static_cast<int>(std::round(left - x));
		const int keep = static_cast<int>(std::round(right - left));
		if (keep <= 0) { continue; }
		source.x += skip;
		source.width = keep;
		sheet->draw(draw_list, source, RectangleF(left, this->span_.top(),
			right - left, this->span_.height));
	}
}

RectangleF AirbrushShadow::bounds() const
{
	return this->span_;
}

RectangleF AirbrushShadow::cull_bounds(float units_per_pixel) const
{
	RectangleF result = this->span_;
	result.inflate(units_per_pixel);
	return result;
}
