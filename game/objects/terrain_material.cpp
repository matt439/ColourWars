#include "game/pch.h"
#include "game/objects/terrain_material.h"
#include "game/level_format.h"

#include <cmath>
#include <optional>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

#include "game/objects/terrain_material_internal.h"

namespace
{
	void validate_miniature_camera(const Camera& camera)
	{
		if (!std::isfinite(camera.scale) || camera.scale <= 0.0f ||
			!std::isfinite(camera.translation.x) || !std::isfinite(camera.translation.y))
		{
			throw std::invalid_argument("miniature terrain needs a finite camera with positive scale");
		}
	}

	RectangleF projected_pixels(const RectangleF& world, const Camera& camera)
	{
		const RectangleF projected = camera.calculate_view_rectangle(world);
		if (!std::isfinite(projected.left()) || !std::isfinite(projected.top()) ||
			!std::isfinite(projected.right()) || !std::isfinite(projected.bottom()))
		{
			throw std::invalid_argument("miniature terrain camera projection must stay finite");
		}
		// The renderer truncates the four edges independently. Preserve its
		// silhouette, including the asymmetric answer around negative zero.
		const float left = std::trunc(projected.left());
		const float top = std::trunc(projected.top());
		return RectangleF(left, top, std::trunc(projected.right()) - left,
			std::trunc(projected.bottom()) - top);
	}

	const char* collision_type_label(CollisionObjectType type)
	{
		switch (type)
		{
		case CollisionObjectType::structure: return "STRUCTURE";
		case CollisionObjectType::structure_jump_through: return "STRUCTURE_JUMP_THROUGH";
		case CollisionObjectType::structure_paintable: return "STRUCTURE_PAINTABLE";
		case CollisionObjectType::structure_ramp_left: return "STRUCTURE_RAMP_LEFT";
		case CollisionObjectType::structure_ramp_right: return "STRUCTURE_RAMP_RIGHT";
		default: return "this collision type";
		}
	}

	// The terrain tags a level record can carry; anything else carries no material.
	std::optional<level_format::Collision> format_collision(CollisionObjectType type)
	{
		switch (type)
		{
		case CollisionObjectType::structure: return level_format::Collision::structure;
		case CollisionObjectType::structure_jump_through: return level_format::Collision::structure_jump_through;
		case CollisionObjectType::structure_paintable: return level_format::Collision::structure_paintable;
		case CollisionObjectType::structure_fixed_slab: return level_format::Collision::structure_fixed_slab;
		case CollisionObjectType::structure_ramp_left: return level_format::Collision::structure_ramp_left;
		case CollisionObjectType::structure_ramp_right: return level_format::Collision::structure_ramp_right;
		default: return std::nullopt;
		}
	}
}

const char* terrain_role_name(TerrainRole role)
{
	static constexpr std::array<const char*, static_cast<size_t>(TerrainRole::count)> names{
		"keyline", "paper", "bg_far", "bg_mid", "bg_line", "bg_detail",
		"ground_fill", "ground_top", "ground_shade",
		"pencil_body", "pencil_shade", "pencil_wood", "graphite",
		"ruler_fill", "ruler_marks", "eraser_fill", "eraser_shade",
		"metal_light", "metal_dark", "shadow", "shadow_deep",
		"face", "hud_paper", "hud_ink", "highlight", "prop_accent" };
	return names.at(static_cast<size_t>(role));
}

std::unique_ptr<const TerrainMaterial> TerrainMaterial::make(const std::string& name,
	CollisionObjectType type, const Shape& shape, const TerrainFaces& faces,
	float mirror_x, RenderResources* render_resources)
{
	const bool ramp = type == CollisionObjectType::structure_ramp_left ||
		type == CollisionObjectType::structure_ramp_right;
	const auto refuse = [&]() -> std::unique_ptr<const TerrainMaterial>
	{
		throw std::invalid_argument("material '" + name + "' may not be used on " +
			collision_type_label(type));
	};
	// Which material sits on which tag is the level format's table, shared
	// with the editor; this only builds the look it names.
	const level_format::Material* material = level_format::material_named(name);
	if (material == nullptr)
	{
		if (ramp) { return refuse(); }
		throw std::invalid_argument("'" + name + "' is not a terrain material");
	}
	const std::optional<level_format::Collision> collision = format_collision(type);
	if (!collision || !level_format::sits_on(*material, *collision)) { return refuse(); }
	if (name == "set_square")
	{
		return std::make_unique<SetSquareMaterial>(shape, faces, render_resources);
	}
	const RectangleF rectangle = shape.bounding_box();
	if (name == "ground")
	{
		return std::make_unique<GroundMaterial>(rectangle, faces, render_resources);
	}
	if (name == "ruler")
	{
		return std::make_unique<RulerMaterial>(rectangle, faces, render_resources);
	}
	if (name == "pencil")
	{
		return std::make_unique<PencilMaterial>(rectangle, faces, mirror_x, render_resources);
	}
	if (name == "eraser")
	{
		return std::make_unique<EraserMaterial>(rectangle, faces, render_resources);
	}
	return std::make_unique<PaintTinMaterial>(rectangle, faces, render_resources);
}

TerrainMaterial::TerrainMaterial(const std::string& name, RenderResources* render_resources) :
	TextureObject(SHEET_NAME, "swatch_keyline", render_resources),
	name_(name)
{
	for (size_t index = 0; index < this->swatches_.size(); ++index)
	{
		this->swatches_[index] = this->sprite_sheet()->resolve_sprite_frame(
			std::string("swatch_") + terrain_role_name(static_cast<TerrainRole>(index)));
	}
}

void TerrainMaterial::paint(DrawList& draw_list) const
{
	for (const TerrainQuad& quad : this->quads_)
	{
		this->draw_with(draw_list, quad.rectangle,
			this->swatches_[static_cast<size_t>(quad.role)], Colour::white,
			Vector2F::ZERO, SpriteFlip::none, 0.0f);
	}
}

void TerrainMaterial::paint_miniature(DrawList& draw_list) const
{
	// A copy: the reference is the list's own state, which set_camera below
	// overwrites.
	const Camera camera = draw_list.camera();
	validate_miniature_camera(camera);
	if (camera.scale >= 1.0f / CELL)
	{
		this->paint(draw_list);
		return;
	}
	const std::vector<TerrainQuad> projected = this->projected_miniature_quads(camera);
	draw_list.set_camera(Camera::DEFAULT_CAMERA);
	try
	{
		for (const TerrainQuad& quad : projected)
		{
			this->draw_with(draw_list, quad.rectangle,
				this->swatches_[static_cast<size_t>(quad.role)], Colour::white,
				Vector2F::ZERO, SpriteFlip::none, 0.0f);
		}
	}
	catch (...)
	{
		draw_list.set_camera(camera);
		throw;
	}
	draw_list.set_camera(camera);
}

std::vector<TerrainQuad> TerrainMaterial::projected_miniature_quads(const Camera& camera) const
{
	validate_miniature_camera(camera);
	const bool miniature = camera.scale < 1.0f / CELL;
	const RectangleF ruler = this->miniature_ruler_
		? projected_pixels(this->ruler_rectangle_, camera) : RectangleF::ZERO;
	const float ruler_thickness = this->ruler_vertical_ ? ruler.width : ruler.height;
	std::vector<TerrainQuad> result;
	result.reserve(this->quads_.size());
	for (size_t index = 0; index < this->quads_.size(); ++index)
	{
		const TerrainQuad& quad = this->quads_[index];
		RectangleF rectangle = projected_pixels(quad.rectangle, camera);
		const MiniatureDetail& detail = this->miniature_details_[index];
		if (miniature && detail.kind == MiniatureDetail::Kind::stroke)
		{
			if (this->miniature_ruler_ && (ruler_thickness < 2.0f ||
				(ruler_thickness < 3.0f && detail.edge !=
					(this->ruler_vertical_ ? StrokeEdge::left : StrokeEdge::top))))
			{
				// One pixel is the light board. Two can hold its leading line
				// and a light row, but not a second line or black end caps.
				continue;
			}
			const RectangleF limit = projected_pixels(detail.limit, camera);
			switch (detail.edge)
			{
			case StrokeEdge::top:
				rectangle.y = limit.top();
				rectangle.height = std::max(1.0f, rectangle.height);
				break;
			case StrokeEdge::bottom:
				rectangle.height = std::max(1.0f, rectangle.height);
				rectangle.y = limit.bottom() - rectangle.height;
				break;
			case StrokeEdge::left:
				rectangle.x = limit.left();
				rectangle.width = std::max(1.0f, rectangle.width);
				break;
			case StrokeEdge::right:
				rectangle.width = std::max(1.0f, rectangle.width);
				rectangle.x = limit.right() - rectangle.width;
				break;
			}
			rectangle = intersection(rectangle, limit);
		}
		else if (miniature && (detail.kind == MiniatureDetail::Kind::minor_mark ||
			detail.kind == MiniatureDetail::Kind::major_mark))
		{
			const float period = detail.kind == MiniatureDetail::Kind::major_mark ? 80.0f : 16.0f;
			if (ruler_thickness < 3.0f || period * camera.scale < 3.0f) { continue; }
			// Marks keep a light gap between neighbours and stay between the
			// two physical outlines. Major marks survive after minors cease
			// to be distinguishable; neither can turn a thin board into a bar.
			RectangleF interior(ruler.x + 1.0f, ruler.y + 1.0f,
				ruler.width - 2.0f, ruler.height - 2.0f);
			if (this->ruler_vertical_)
			{
				rectangle.height = 1.0f;
				rectangle.width = std::max(1.0f, rectangle.width);
				rectangle.x = interior.right() - rectangle.width;
			}
			else
			{
				rectangle.width = 1.0f;
				rectangle.height = std::max(1.0f, rectangle.height);
				rectangle.y = interior.bottom() - rectangle.height;
			}
			rectangle = intersection(rectangle, interior);
		}
		if (rectangle.width > 0.0f && rectangle.height > 0.0f)
		{
			result.push_back({ rectangle, quad.role });
		}
	}
	return result;
}

const std::string& TerrainMaterial::name() const
{
	return this->name_;
}

const std::vector<TerrainQuad>& TerrainMaterial::quads() const
{
	return this->quads_;
}

void TerrainMaterial::add(const RectangleF& rectangle, TerrainRole role,
	const RectangleF& limit)
{
	const RectangleF clipped = intersection(rectangle, limit);
	if (clipped.width > 0.0f && clipped.height > 0.0f)
	{
		this->quads_.push_back({ clipped, role });
		this->miniature_details_.push_back({ MiniatureDetail::Kind::fill, StrokeEdge::top, limit });
	}
}

void TerrainMaterial::add_stroke(const RectangleF& rectangle, TerrainRole role,
	const RectangleF& limit, StrokeEdge edge)
{
	this->add_stroke(rectangle, role, limit, edge, limit);
}

void TerrainMaterial::add_stroke(const RectangleF& rectangle, TerrainRole role,
	const RectangleF& limit, StrokeEdge edge, const RectangleF& miniature_limit)
{
	const size_t before = this->quads_.size();
	this->add(rectangle, role, limit);
	if (this->quads_.size() != before)
	{
		this->miniature_details_.back() = { MiniatureDetail::Kind::stroke, edge, miniature_limit };
	}
}

void TerrainMaterial::configure_miniature_ruler(const RectangleF& rectangle, bool vertical)
{
	this->ruler_rectangle_ = rectangle;
	this->ruler_vertical_ = vertical;
	this->miniature_ruler_ = true;
}

void TerrainMaterial::add_ruler_mark(const RectangleF& rectangle,
	const RectangleF& limit, bool major)
{
	const size_t before = this->quads_.size();
	this->add(rectangle, TerrainRole::ruler_marks, limit);
	if (this->quads_.size() != before)
	{
		this->miniature_details_.back().kind = major
			? MiniatureDetail::Kind::major_mark : MiniatureDetail::Kind::minor_mark;
	}
}

void TerrainMaterial::outline(const RectangleF& rectangle, const TerrainFaces& faces)
{
	const auto seal = [&](const std::vector<std::pair<float, float>>& spans,
		bool along_x, bool leading, StrokeEdge edge)
	{
		const float thickness = std::min(3.0f * CELL,
			along_x ? rectangle.height : rectangle.width);
		const auto band = [&](float low, float high, float offset, float depth)
		{
			return along_x
				? RectangleF(low, leading ? rectangle.top() + offset : rectangle.bottom() - offset - depth,
					high - low, depth)
				: RectangleF(leading ? rectangle.left() + offset : rectangle.right() - offset - depth,
					low, depth, high - low);
		};
		for (const auto& [low, high] : spans)
		{
			const RectangleF limit = band(low, high, 0.0f, thickness);
			this->add(limit, TerrainRole::metal_dark, rectangle);
			// Sparse, world-aligned diagonal cuts. The smooth white scoring
			// band masks these; disabled faces retain the same sealed finish.
			constexpr float period = 8.0f * CELL;
			for (float at = std::floor(low / period) * period - thickness;
				at < high; at += period)
			{
				for (float inset = CELL; inset < thickness; inset += CELL)
				{
					this->add(band(at + inset, at + inset + CELL, inset,
						std::min(CELL, thickness - inset)), TerrainRole::metal_light, limit);
				}
			}
			this->add_stroke(band(low, high, 0.0f, std::min(CELL, thickness)),
				TerrainRole::keyline, rectangle, edge);
		}
	};
	seal(faces.top, true, true, StrokeEdge::top);
	seal(faces.bottom, true, false, StrokeEdge::bottom);
	seal(faces.left, false, true, StrokeEdge::left);
	seal(faces.right, false, false, StrokeEdge::right);
}

OrientedTerrainMaterial::OrientedTerrainMaterial(const std::string& name,
	const RectangleF& rectangle, RenderResources* render_resources) :
	TerrainMaterial(name, render_resources),
	rectangle_(rectangle),
	vertical_(rectangle.height > rectangle.width)
{
}

float OrientedTerrainMaterial::length() const
{
	return this->vertical_ ? this->rectangle_.height : this->rectangle_.width;
}

float OrientedTerrainMaterial::thickness() const
{
	return this->vertical_ ? this->rectangle_.width : this->rectangle_.height;
}

bool OrientedTerrainMaterial::vertical() const
{
	return this->vertical_;
}

const RectangleF& OrientedTerrainMaterial::rectangle() const
{
	return this->rectangle_;
}

void OrientedTerrainMaterial::place(float along, float across, float along_size,
	float across_size, TerrainRole role)
{
	const RectangleF& r = this->rectangle_;
	this->add(this->vertical_
		? RectangleF(r.x + across, r.y + along, across_size, along_size)
		: RectangleF(r.x + along, r.y + across, along_size, across_size), role, r);
}

bool OrientedTerrainMaterial::exposed(const std::vector<std::pair<float, float>>& face,
	float low, float high)
{
	float total = 0.0f;
	for (const auto& span : face)
	{
		total += std::max(0.0f, std::min(span.second, high) - std::max(span.first, low));
	}
	return total * 2.0f >= high - low;
}
