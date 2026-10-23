// The six leaf materials: each works out its drawing once, in its constructor.
#include "game/pch.h"
#include "game/objects/terrain_material.h"
#include "engine/math/triangle.h"

#include <cmath>
#include <optional>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

#include "game/objects/terrain_material_internal.h"

namespace
{
	constexpr float CELL = TerrainMaterial::CELL;

	// Whole cells, never less than one.
	float cells(float units)
	{
		return std::max(CELL, std::round(units / CELL) * CELL);
	}

	// The first multiple of `period` at or after `world`: interior detail sits
	// on the world grid, so neighbouring pieces' ticks and joints line up.
	float grid_from(float world, float period)
	{
		return std::ceil(world / period) * period;
	}

	bool overlaps(const std::vector<std::pair<float, float>>& spans, float low, float high)
	{
		for (const auto& span : spans)
		{
			if (span.first < high && span.second > low) { return true; }
		}
		return false;
	}

	// An axis-aligned right triangle: the right angle, its neighbour along the
	// horizontal leg and its neighbour along the vertical leg.
	struct RightTriangle
	{
		Vector2F corner, across, apex;

		static RightTriangle of(const Triangle& triangle)
		{
			const Vector2F* p = triangle.points;
			for (int i = 0; i < 3; ++i)
			{
				const Vector2F& a = p[(i + 1) % 3];
				const Vector2F& b = p[(i + 2) % 3];
				if (a.y == p[i].y && b.x == p[i].x) { return { p[i], a, b }; }
				if (b.y == p[i].y && a.x == p[i].x) { return { p[i], b, a }; }
			}
			throw std::invalid_argument("a set square needs an axis-aligned right triangle");
		}

		RightTriangle scaled(float factor) const
		{
			const Vector2F centre = (this->corner + this->across + this->apex) / 3.0f;
			return { centre + (this->corner - centre) * factor,
				centre + (this->across - centre) * factor,
				centre + (this->apex - centre) * factor };
		}

		float hypotenuse_x(float y) const
		{
			const float t = (y - this->across.y) / (this->apex.y - this->across.y);
			return this->across.x + (this->apex.x - this->across.x) * std::clamp(t, 0.0f, 1.0f);
		}

		bool hypotenuse_left() const { return this->across.x < this->corner.x; }

		// The part of the band [y0, y1] that is inside at both its edges.
		std::optional<std::pair<float, float>> span(float y0, float y1) const
		{
			const float low = std::min(this->corner.y, this->apex.y);
			const float high = std::max(this->corner.y, this->apex.y);
			if (y0 < low - 0.001f || y1 > high + 0.001f) { return std::nullopt; }
			const float a = this->hypotenuse_x(y0);
			const float b = this->hypotenuse_x(y1);
			const float hyp = this->hypotenuse_left() ? std::max(a, b) : std::min(a, b);
			const float left = std::min(hyp, this->corner.x);
			const float right = std::max(hyp, this->corner.x);
			if (!(right > left)) { return std::nullopt; }
			return std::make_pair(left, right);
		}
	};
}

GroundMaterial::GroundMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	TerrainMaterial("ground", render_resources)
{
	constexpr float course = 16.0f * CELL;
	constexpr float brick = 32.0f * CELL;
	constexpr int tiled_courses = 4;

	const float tiled_bottom = std::min(rectangle.bottom(), rectangle.top() + course * tiled_courses);
	this->add(RectangleF(rectangle.x, rectangle.y, rectangle.width, tiled_bottom - rectangle.y),
		TerrainRole::ground_fill, rectangle);
	// Darker toward the underside: only the top courses are laid as tiles.
	this->add(RectangleF(rectangle.x, tiled_bottom, rectangle.width, rectangle.bottom() - tiled_bottom),
		TerrainRole::ground_shade, rectangle);
	for (int index = 0; index < tiled_courses; ++index)
	{
		const float top = rectangle.y + course * static_cast<float>(index);
		if (top >= tiled_bottom) { break; }
		const float bottom = std::min(top + course, tiled_bottom);
		if (index > 0)
		{
			this->add(RectangleF(rectangle.x, top, rectangle.width, CELL),
				TerrainRole::keyline, rectangle);
		}
		// Halved bond: every other course starts half a tile along.
		const float offset = (index % 2 == 0) ? 0.0f : brick / 2.0f;
		// Joints keep clear of the outline, or they read as a doubled edge.
		for (float x = grid_from(rectangle.left() - offset + CELL * 3.0f, brick) + offset;
			x < rectangle.right() - CELL * 3.0f; x += brick)
		{
			this->add(RectangleF(x, top, CELL, bottom - top), TerrainRole::keyline, rectangle);
		}
	}
	this->outline(rectangle, faces);
}

PencilMaterial::PencilMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	float mirror_x, RenderResources* render_resources) :
	OrientedTerrainMaterial("pencil", rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();

	this->place(0.0f, 0.0f, length, thickness, TerrainRole::pencil_body);
	if (thickness >= 10.0f * CELL)
	{
		// Light from the top left: the lower facet is in shade.
		const float facet = cells(thickness * 0.65f);
		this->place(0.0f, facet, length, thickness - facet, TerrainRole::pencil_shade);
	}
	else if (thickness >= 6.0f * CELL)
	{
		this->place(0.0f, cells(thickness * 0.55f), length, CELL, TerrainRole::pencil_shade);
	}

	const bool start_open = this->vertical()
		? exposed(faces.top, rectangle.left(), rectangle.right())
		: exposed(faces.left, rectangle.top(), rectangle.bottom());
	const bool end_open = this->vertical()
		? exposed(faces.bottom, rectangle.left(), rectangle.right())
		: exposed(faces.right, rectangle.top(), rectangle.bottom());

	if (length >= std::max(4.0f * thickness, 15.0f * CELL))
	{
		// The eraser points away from the stage's mirror line, the cut wood
		// end toward it, so mirrored halves read as mirrored. A pencil lying
		// across the line is cut at both ends, so it stays symmetric; one
		// standing up has its eraser at the top.
		const bool across_mirror = !this->vertical() &&
			rectangle.left() < mirror_x && rectangle.right() > mirror_x;
		const bool eraser_at_start = this->vertical() || rectangle.center().x < mirror_x;
		const float eraser = cells(std::clamp(thickness * 1.25f, 3.0f * CELL, length / 5.0f));
		const float ferrule = cells(thickness * 0.6f);
		const float wood = cells(thickness * 0.5f);
		const float core = cells(thickness / 3.0f);
		const float core_at = cells((thickness - core) / 2.0f);
		const float shade_at = cells(thickness * 0.65f);

		// at = where the cap begins; inward = +1 from the start, -1 from the end.
		const auto eraser_cap = [&](bool at_start)
		{
			const auto span = [&](float from, float size, float across, float across_size, TerrainRole role)
			{
				this->place(at_start ? from : length - from - size, across, size, across_size, role);
			};
			span(0.0f, eraser, 0.0f, thickness, TerrainRole::eraser_fill);
			span(0.0f, eraser, shade_at, thickness - shade_at, TerrainRole::eraser_shade);
			span(eraser, CELL, 0.0f, thickness, TerrainRole::keyline);
			span(eraser + CELL, ferrule, 0.0f, thickness, TerrainRole::metal_light);
			span(eraser + CELL + cells(ferrule / 2.0f) - CELL, CELL, 0.0f, thickness, TerrainRole::metal_dark);
			span(eraser + CELL + ferrule, CELL, 0.0f, thickness, TerrainRole::keyline);
		};
		const auto wood_end = [&](bool at_start)
		{
			const auto span = [&](float from, float size, float across, float across_size, TerrainRole role)
			{
				this->place(at_start ? from : length - from - size, across, size, across_size, role);
			};
			span(0.0f, wood, 0.0f, thickness, TerrainRole::pencil_wood);
			span(0.0f, wood, core_at, core, TerrainRole::graphite);
			span(wood, CELL, 0.0f, thickness, TerrainRole::keyline);
		};
		if (start_open)
		{
			if (eraser_at_start && !across_mirror) { eraser_cap(true); }
			else { wood_end(true); }
		}
		if (end_open)
		{
			if (!eraser_at_start && !across_mirror) { eraser_cap(false); }
			else { wood_end(false); }
		}
	}
	this->outline(rectangle, faces);
}

RulerMaterial::RulerMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	OrientedTerrainMaterial("ruler", rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->configure_miniature_ruler(rectangle, this->vertical());
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::ruler_fill);

	if (thickness >= 6.0f * CELL)
	{
		// Short marks every 16 units, long every 80, rising from the lower edge.
		const float origin = this->vertical() ? rectangle.top() : rectangle.left();
		const float bottom_keyline = CELL;
		for (float world = grid_from(origin + CELL * 2.0f, 16.0f);
			world < origin + length - CELL * 2.0f; world += 16.0f)
		{
			const bool major = std::fmod(std::abs(world), 80.0f) < 0.5f;
			const float mark = (major ? 3.0f : 2.0f) * CELL;
			const float along = world - origin;
			const float across = thickness - bottom_keyline - mark;
			this->add_ruler_mark(this->vertical()
				? RectangleF(rectangle.x + across, rectangle.y + along, mark, CELL)
				: RectangleF(rectangle.x + along, rectangle.y + across, CELL, mark), rectangle, major);
		}
	}

	TerrainFaces drawn = faces;
	if (thickness < 5.0f * CELL)
	{
		// Too thin for two keylines: the lower (or right) edge stays open.
		(this->vertical() ? drawn.right : drawn.bottom).clear();
	}
	this->outline(rectangle, drawn);
}

EraserMaterial::EraserMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	OrientedTerrainMaterial("eraser", rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::eraser_fill);
	if (thickness >= 6.0f * CELL)
	{
		const float shade = cells(thickness * 0.65f);
		this->place(0.0f, shade, length, CELL, TerrainRole::keyline);
		this->place(0.0f, shade + CELL, length, thickness - shade - CELL, TerrainRole::eraser_shade);
	}
	if (thickness >= 10.0f * CELL && length >= 20.0f * CELL)
	{
		// One shine dash, below the 12-unit paint band so paint never hides it.
		this->place(3.0f * CELL, 4.0f * CELL, std::min(length / 3.0f, 40.0f * CELL), CELL,
			TerrainRole::highlight);
	}
	this->outline(rectangle, faces);
}

PaintTinMaterial::PaintTinMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	OrientedTerrainMaterial("paint_tin", rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::metal_light);
	// The rim sits just under the paint band; the far side is in shade.
	this->place(0.0f, 3.0f * CELL, length, CELL, TerrainRole::metal_dark);
	const float side = cells(length * 0.15f);
	this->place(length - side, 4.0f * CELL, side, thickness - 4.0f * CELL, TerrainRole::metal_dark);
	const float label_top = cells(thickness * 0.35f);
	const float label_bottom = cells(thickness * 0.75f);
	if (label_bottom - label_top >= 3.0f * CELL)
	{
		this->place(0.0f, label_top, length, CELL, TerrainRole::keyline);
		this->place(0.0f, label_top + CELL, length, label_bottom - label_top - CELL * 2.0f,
			TerrainRole::prop_accent);
		this->place(0.0f, label_bottom - CELL, length, CELL, TerrainRole::keyline);
	}
	this->outline(rectangle, faces);
}

SetSquareMaterial::SetSquareMaterial(const Shape& shape, const TerrainFaces& faces,
	RenderResources* render_resources) :
	TerrainMaterial("set_square", render_resources)
{
	const Triangle* triangle = dynamic_cast<const Triangle*>(&shape);
	if (triangle == nullptr)
	{
		throw std::invalid_argument("a set square needs a triangle");
	}
	const RectangleF box = shape.bounding_box();
	const RightTriangle outer = RightTriangle::of(*triangle);
	const RightTriangle inner = outer.scaled(0.45f);
	const bool corner_below = outer.corner.y > outer.apex.y;
	const bool corner_right = outer.corner.x > outer.across.x;
	// The horizontal leg is the box's top or bottom face, the vertical leg its
	// left or right. The hypotenuse is always the walked surface: always lined.
	const auto& horizontal_leg = corner_below ? faces.bottom : faces.top;
	const auto& vertical_leg = corner_right ? faces.right : faces.left;

	// Rows run from the horizontal leg toward the apex, so the leg row is a
	// whole cell and any partial cell is at the point. `outer` lines only the
	// exposed legs; the engraving lines all three edges of its triangle.
	const auto rows = [&](const RightTriangle& t, bool outer_edge, TerrainRole line)
	{
		const float height = std::abs(t.corner.y - t.apex.y);
		const int count = static_cast<int>(std::ceil(height / CELL - 0.001f));
		for (int row = 0; row < count; ++row)
		{
			const float leg_near = static_cast<float>(row) * CELL;
			const float leg_far = std::min(height, leg_near + CELL);
			const float y0 = corner_below ? t.corner.y - leg_far : t.corner.y + leg_near;
			const float y1 = corner_below ? t.corner.y - leg_near : t.corner.y + leg_far;
			const auto span = t.span(y0, y1);
			if (!span) { continue; }
			const RectangleF band(span->first, y0, span->second - span->first, y1 - y0);
			if (outer_edge) { this->add(band, TerrainRole::metal_light, box); }

			// The stair edge: each row covers the step to the next.
			const float step = std::max(CELL,
				std::abs(t.hypotenuse_x(y0) - t.hypotenuse_x(y1)) + CELL);
			const float hyp_x = t.hypotenuse_left() ? span->first : span->second - step;
			const RectangleF hypotenuse = intersection(RectangleF(hyp_x, y0, step, y1 - y0), band);
			if (outer_edge)
			{
				// Ramps reject paint too: the same dark sealed finish follows
				// their slope and exposed legs, clipped to occupied stair rows.
				const auto seal_row = [&](const RectangleF& edge_band)
				{
					const RectangleF seal = intersection(edge_band, band);
					this->add(seal, TerrainRole::metal_dark, band);
					constexpr float period = 8.0f * CELL;
					const float phase = std::floor(y0 / CELL) * CELL;
					for (float x = grid_from(seal.left() + phase - CELL, period) - phase;
						x < seal.right(); x += period)
					{
						this->add(RectangleF(x, y0, CELL, y1 - y0), TerrainRole::metal_light, seal);
					}
				};
				const float seal_width = step + 2.0f * CELL;
				seal_row(RectangleF(t.hypotenuse_left() ? span->first : span->second - seal_width,
					y0, seal_width, y1 - y0));
				if (overlaps(vertical_leg, y0, y1))
				{
					seal_row(RectangleF(corner_right ? span->second - 3.0f * CELL : span->first,
						y0, 3.0f * CELL, y1 - y0));
				}
				if (leg_near < 3.0f * CELL)
				{
					for (const auto& [low, high] : horizontal_leg)
					{
						seal_row(RectangleF(low, y0, high - low, y1 - y0));
					}
				}
				// A surviving pixel row may borrow one pixel only from its own
				// filled band, never from the triangle's empty side or tip.
				this->add_stroke(hypotenuse, line, box,
					t.hypotenuse_left() ? StrokeEdge::left : StrokeEdge::right, band);
			}
			else { this->add(hypotenuse, line, box); }

			if (!outer_edge || overlaps(vertical_leg, y0, y1))
			{
				const RectangleF leg(corner_right ? span->second - CELL : span->first, y0,
					CELL, y1 - y0);
				if (outer_edge)
				{
					this->add_stroke(leg, line, box,
						corner_right ? StrokeEdge::right : StrokeEdge::left, band);
				}
				else { this->add(leg, line, box); }
			}
			if (row == 0)
			{
				if (!outer_edge) { this->add(band, line, box); }
				else for (const auto& [low, high] : horizontal_leg)
				{
					this->add_stroke(intersection(RectangleF(low, y0, high - low, y1 - y0), band),
						line, box, corner_below ? StrokeEdge::bottom : StrokeEdge::top, band);
				}
			}
		}
	};
	rows(outer, true, TerrainRole::keyline);
	if (std::abs(inner.corner.y - inner.apex.y) >= 6.0f * CELL)
	{
		rows(inner, false, TerrainRole::metal_dark);
	}
}
