#include "game/pch.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/terrain_material.h"

#include <cmath>

using namespace labrador;

AirbrushShadow::AirbrushShadow(float left, float right, float top,
	RenderResources* render_resources) :
	TextureObject(TerrainMaterial::sheet_name, first_tile, render_resources),
	tiles_(tiles(*this->sprite_sheet()))
{
	this->span_ = RectangleF(left, top, right - left,
		static_cast<float>(this->tiles_[0].height));
}

AirbrushShadow::Tiles AirbrushShadow::tiles(const SpriteSheet& sheet)
{
	Tiles result{};
	for (int index = 0; index < variants; ++index)
	{
		result[static_cast<size_t>(index)] = sheet.sprite_frame(
			sheet.resolve_sprite_frame("shadow_" + std::to_string(index))).source_rectangle();
	}
	return result;
}

void AirbrushShadow::update(float /*dt*/)
{
}

void AirbrushShadow::draw(DrawList& draw_list) const
{
	const SpriteSheet* sheet = this->sprite_sheet();
	lay(this->tiles_, this->span_.left(), this->span_.right(),
		[&](const RectangleI& source, float left, float right)
	{
		sheet->draw(draw_list, source, RectangleF(left, this->span_.top(),
			right - left, this->span_.height));
	});
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
