#include "game/pch.h"
#include "game/objects/structure.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

Structure::Structure(const std::string& sheet_name,
	const std::string& frame_name,
	const RectangleF& sprite_rectangle,
	std::unique_ptr<mattmath::Shape> collision_shape,
	RenderResources* render_resources,
	CollisionObjectType collision_type,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth,
	float tile_world_units_per_texel) :
	TextureObject(sheet_name, frame_name, render_resources,
		color, rotation, origin, flip, layer_depth),
	collision_type_(collision_type),
	sprite_rectangle_(sprite_rectangle),
	collision_shape_(std::move(collision_shape))
{
	if (this->collision_shape_ == nullptr)
	{
		throw std::invalid_argument("A structure requires a collision shape.");
	}
	if (!std::isfinite(tile_world_units_per_texel) || tile_world_units_per_texel < 0.0f)
	{
		throw std::invalid_argument("A structure tile density must be finite and nonnegative.");
	}
	if (tile_world_units_per_texel > 0.0f)
	{
		if (rotation != 0.0f || origin != Vector2F::ZERO || flip != SpriteFlip::none)
		{
			throw std::invalid_argument("Tiled structures require an unrotated, unflipped rectangle.");
		}
		this->build_texture_tiles(tile_world_units_per_texel);
	}
}

void Structure::update(float /*dt*/)
{
	
}
void Structure::draw(DrawList& draw_list) const
{
	if (this->texture_tiles_.empty())
	{
		this->TextureObject::draw(draw_list, this->sprite_rectangle_);
		return;
	}
	for (const TextureTile& tile : this->texture_tiles_)
	{
		this->sprite_sheet()->draw(draw_list, tile.source, tile.destination,
			this->colour(), 0.0f, Vector2F::ZERO, SpriteFlip::none, this->layer_depth());
	}
}

void Structure::build_texture_tiles(float world_units_per_texel)
{
	const RectangleI& frame_rectangle =
		this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle();
	const RectangleF& rectangle = this->sprite_rectangle_;
	if (!std::isfinite(rectangle.x) || !std::isfinite(rectangle.y) ||
		!std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
		rectangle.width <= 0.0f || rectangle.height <= 0.0f ||
		frame_rectangle.width <= 0 || frame_rectangle.height <= 0)
	{
		throw std::invalid_argument("A tiled structure requires finite positive dimensions.");
	}
	// This is a content-work limit, not a change to terrain dimensions or scale.
	// Reject unreasonable authored density/geometry before allocating either
	// axis. A partial module can add one final single-texel record per axis;
	// check the exact cross-product again after the bounded axis construction.
	constexpr size_t maximum_texture_tiles = 65536;
	const double column_modules = std::ceil(static_cast<double>(rectangle.width) /
		(static_cast<double>(frame_rectangle.width) * world_units_per_texel));
	const double row_modules = std::ceil(static_cast<double>(rectangle.height) /
		(static_cast<double>(frame_rectangle.height) * world_units_per_texel));
	if (column_modules * row_modules > static_cast<double>(maximum_texture_tiles))
	{
		throw std::invalid_argument("A tiled structure exceeds the 65536 draw-tile limit.");
	}

	struct AxisTile
	{
		int source_offset;
		int source_length;
		float destination_offset;
		float destination_length;
	};
	const auto split_axis = [world_units_per_texel](float length, int texels)
	{
		std::vector<AxisTile> tiles;
		const float module_length = static_cast<float>(texels) * world_units_per_texel;
		if (!std::isfinite(module_length) || module_length <= 0.0f)
		{
			throw std::invalid_argument("A structure tile must have a finite positive world size.");
		}
		for (float offset = 0.0f; offset < length;)
		{
			const float remaining = std::min(module_length, length - offset);
			const int whole_texels = std::min(texels,
				static_cast<int>(std::floor(remaining / world_units_per_texel)));
			const float whole_length = static_cast<float>(whole_texels) * world_units_per_texel;
			if (whole_texels > 0)
			{
				tiles.push_back({ 0, whole_texels, offset, whole_length });
			}
			if (remaining > whole_length)
			{
				// The engine accepts integer source rectangles. A final fraction of
				// one point-sampled texel has constant colour, so sampling that one
				// texel into its shorter remainder is exactly the cropped result.
				// No multi-texel detail is compressed to fit the authored geometry.
				tiles.push_back({ whole_texels, 1, offset + whole_length,
					remaining - whole_length });
			}
			const float next = offset + remaining;
			if (next <= offset)
			{
				throw std::invalid_argument("Structure tile density cannot advance across this rectangle.");
			}
			offset = next;
		}
		return tiles;
	};
	const std::vector<AxisTile> columns = split_axis(rectangle.width, frame_rectangle.width);
	const std::vector<AxisTile> rows = split_axis(rectangle.height, frame_rectangle.height);
	if (columns.size() > maximum_texture_tiles / rows.size())
	{
		throw std::invalid_argument("A tiled structure exceeds the 65536 draw-tile limit.");
	}
	this->texture_tiles_.reserve(columns.size() * rows.size());
	for (const AxisTile& row : rows)
	{
		for (const AxisTile& column : columns)
		{
			this->texture_tiles_.push_back({
				RectangleI(frame_rectangle.x + column.source_offset,
					frame_rectangle.y + row.source_offset,
					column.source_length, row.source_length),
				RectangleF(rectangle.x + column.destination_offset,
					rectangle.y + row.destination_offset,
					column.destination_length, row.destination_length) });
		}
	}
}
RectangleF Structure::bounds() const
{
	return this->sprite_rectangle_;
}
void Structure::on_contact(const CollisionObject& /*other*/,
	const Vector2F& /*normal*/, float /*penetration*/)
{
}
CollisionLayer Structure::layer() const
{
	return collision_layer(this->collision_type_);
}
CollisionMask Structure::mask() const
{
	return collision_mask(this->collision_type_);
}
CollisionTag Structure::tag() const
{
	return to_collision_tag(this->collision_type_);
}
CollisionObjectType Structure::collision_type() const
{
	return this->collision_type_;
}
const Shape* Structure::shape() const
{
	return this->collision_shape_.get();
}
bool Structure::for_deletion() const
{
	return false;
}
const RectangleF& Structure::rectangle() const
{
	return this->sprite_rectangle_;
}
