// The six leaf materials: each works out its drawing once, in its constructor.
#include "game/pch.h"
#include "game/objects/terrain_material.h"
#include "game/contracts/level_format.h"
#include "game/ai/bot_random.h"
#include "engine/math/triangle.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

using namespace labrador;

#include "game/objects/terrain_material_internal.h"

namespace
{
	constexpr float cell = TerrainMaterial::cell;

	// Whole cells, never less than one.
	float cells(float units)
	{
		return std::max(cell, std::round(units / cell) * cell);
	}

	// The first multiple of `period` at or after `world`: interior detail sits
	// on the world grid, so neighbouring pieces' ticks and joints line up.
	float grid_from(float world, float period)
	{
		return std::ceil(world / period) * period;
	}

	// A hash of a world cell and a seed: neighbouring pieces agree and every
	// build of one match draws the same wall. The seed is a constant per kind
	// of slip, mixed with the match's hand (MatchRandom::hand); hand 0 is the
	// house hand. Never a draw from a gameplay stream: the key is the first
	// splitmix64 output of a private BotRandom.
	std::uint64_t cell_key(float x, float y, std::uint64_t seed)
	{
		return BotRandom((static_cast<std::uint64_t>(static_cast<std::int64_t>(std::lround(x / cell))) << 32) ^
			static_cast<std::uint64_t>(static_cast<std::int64_t>(std::lround(y / cell))) ^ seed).next();
	}

	// The person laid the bricks by hand (docs/drawing-mistakes-research.md,
	// stage 1). Decoration, so the stage's mirrored halves need not slip alike.
	//
	// A few joints landed off the bond.
	constexpr unsigned brick_slip_one_in = 5;
	constexpr float brick_slip_cells = 1.0f;

	float brick_slip(float joint_x, float course_top, std::uint64_t hand)
	{
		const std::uint64_t z = cell_key(joint_x, course_top, 0x6272696B736C6970ULL ^ hand);
		if (z % brick_slip_one_in != 0) { return 0.0f; }
		return ((z >> 32) & 1u) != 0 ? brick_slip_cells * cell : -brick_slip_cells * cell;
	}

	// A long mortar line went down in several strokes, and some strokes landed
	// a row off: each line drifts one way, up or down, in bands of the world. Where
	// two strokes meet at different rows they overlap by up to two cells or touch
	// only at a corner, never leave a gap a bucket fill would leak through.
	constexpr float mortar_band = 40.0f * cell;
	constexpr unsigned mortar_drift_one_in = 3;
	constexpr float mortar_min_stroke = 8.0f * cell;
	constexpr unsigned mortar_join_cells = 3;

	// A joint sometimes ran a cell or two past the line it was meant to stop at.
	constexpr unsigned joint_overshoot_one_in = 4;

	// Each brick is its own rectangle, and a few were drawn a cell or two off the
	// edge they should have shared, so that edge came out doubled: two lines side
	// by side, or with a one-cell sliver of brick between them.
	constexpr unsigned joint_doubled_one_in = 8;
	constexpr unsigned brick_top_doubled_one_in = 10;

	struct MortarStroke
	{
		float left, right, y;
	};

	std::vector<MortarStroke> mortar_strokes(const RectangleF& rectangle, float y, std::uint64_t hand)
	{
		float drift = (cell_key(0.0f, y, 0x6D6F727461726479ULL ^ hand) & 1u) != 0 ? cell : -cell;
		if (y + drift < rectangle.top() + cell * 3.0f || y + drift + cell > rectangle.bottom() - cell * 3.0f)
		{
			drift = 0.0f;
		}
		std::vector<MortarStroke> bands;
		for (float band = std::floor(rectangle.left() / mortar_band) * mortar_band;
			band < rectangle.right(); band += mortar_band)
		{
			const bool off = cell_key(band, y, 0x6D6F72746172626EULL ^ hand) % mortar_drift_one_in == 0;
			bands.push_back({ std::max(band, rectangle.left()), std::min(band + mortar_band, rectangle.right()),
				y + (off ? drift : 0.0f) });
		}
		// A sliver against the outline belongs to the stroke beside it.
		if (bands.size() > 1 && bands.front().right - bands.front().left < mortar_min_stroke)
		{
			bands.front().y = bands[1].y;
		}
		if (bands.size() > 1 && bands.back().right - bands.back().left < mortar_min_stroke)
		{
			bands.back().y = bands[bands.size() - 2].y;
		}
		std::vector<MortarStroke> strokes;
		for (const MortarStroke& band : bands)
		{
			if (!strokes.empty() && strokes.back().y == band.y) { strokes.back().right = band.right; }
			else { strokes.push_back(band); }
		}
		for (size_t i = 1; i < strokes.size(); ++i)
		{
			const std::uint64_t join = cell_key(strokes[i].left, y, 0x6D6F72746172636AULL ^ hand);
			strokes[i - 1].right += static_cast<float>(join % mortar_join_cells) * cell;
			strokes[i].left -= static_cast<float>((join / mortar_join_cells) % mortar_join_cells) * cell;
		}
		return strokes;
	}

	// The rows a mortar line inks in the column at `x`, as [top, bottom).
	std::pair<float, float> mortar_at(const std::vector<MortarStroke>& strokes, float x, float y)
	{
		float top = std::numeric_limits<float>::max();
		float bottom = std::numeric_limits<float>::lowest();
		for (const MortarStroke& stroke : strokes)
		{
			if (x >= stroke.left && x < stroke.right)
			{
				top = std::min(top, stroke.y);
				bottom = std::max(bottom, stroke.y + cell);
			}
		}
		if (top > bottom) { return { y, y + cell }; }
		return { top, bottom };
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
	RenderResources* render_resources, std::uint64_t hand) :
	TerrainMaterial(level_format::material_name::ground, render_resources)
{
	constexpr float course = 16.0f * cell;
	constexpr float brick = 32.0f * cell;
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
		const std::vector<MortarStroke> above = index > 0
			? mortar_strokes(rectangle, top, hand) : std::vector<MortarStroke>{};
		const std::vector<MortarStroke> below = bottom < tiled_bottom
			? mortar_strokes(rectangle, bottom, hand) : std::vector<MortarStroke>{};
		for (const MortarStroke& stroke : above)
		{
			this->add(RectangleF(stroke.left, stroke.y, stroke.right - stroke.left, cell),
				TerrainRole::keyline, rectangle);
		}
		// Halved bond: every other course starts half a tile along.
		const float offset = (index % 2 == 0) ? 0.0f : brick / 2.0f;
		// Joints keep clear of the outline, or they read as a doubled edge.
		std::optional<float> previous;
		for (float x = grid_from(rectangle.left() - offset + cell * 3.0f, brick) + offset;
			x < rectangle.right() - cell * 3.0f; x += brick)
		{
			float joint = x + brick_slip(x, top, hand);
			if (joint < rectangle.left() + cell * 3.0f || joint >= rectangle.right() - cell * 3.0f)
			{
				joint = x;
			}
			// A joint meets the lines where they were drawn, not where they were meant to be.
			float joint_top = above.empty() ? top : mortar_at(above, joint, top).first;
			float joint_bottom = below.empty() ? bottom : mortar_at(below, joint, bottom).second;
			const std::uint64_t slip = cell_key(joint, top, 0x6A6F696E746F7672ULL ^ hand);
			if (slip % joint_overshoot_one_in == 0)
			{
				const float tail = static_cast<float>(1u + ((slip >> 32) & 1u)) * cell;
				const bool up = ((slip >> 40) & 1u) != 0;
				if (up && !above.empty() && joint_top - tail >= rectangle.top() + cell * 3.0f)
				{
					joint_top -= tail;
				}
				else if (!up && !below.empty() && joint_bottom + tail <= rectangle.bottom() - cell * 3.0f)
				{
					joint_bottom += tail;
				}
			}
			this->add(RectangleF(joint, joint_top, cell, joint_bottom - joint_top), TerrainRole::keyline, rectangle);

			const std::uint64_t doubled = cell_key(joint, top, 0x646F75626C656A74ULL ^ hand);
			if (doubled % joint_doubled_one_in == 0)
			{
				const float apart = static_cast<float>(1u + ((doubled >> 32) & 1u)) * cell;
				const float twin = ((doubled >> 40) & 1u) != 0 ? joint + apart : joint - apart;
				if (twin >= rectangle.left() + cell * 3.0f && twin < rectangle.right() - cell * 3.0f)
				{
					const float twin_top = above.empty() ? top : mortar_at(above, twin, top).first;
					const float twin_bottom = below.empty() ? bottom : mortar_at(below, twin, bottom).second;
					this->add(RectangleF(twin, twin_top, cell, twin_bottom - twin_top), TerrainRole::keyline, rectangle);
				}
			}

			// The brick between the last joint and this one: its top edge drawn
			// again, a row or two under the line it should have shared.
			if (previous && !above.empty())
			{
				const std::uint64_t edge = cell_key(*previous, top, 0x646F75626C65746FULL ^ hand);
				if (edge % brick_top_doubled_one_in == 0)
				{
					const float line_bottom = std::max(mortar_at(above, *previous, top).second,
						mortar_at(above, joint, top).second);
					const float y = line_bottom + static_cast<float>((edge >> 32) & 1u) * cell;
					if (y + cell <= bottom - cell * 3.0f && y + cell <= rectangle.bottom() - cell * 3.0f)
					{
						this->add(RectangleF(*previous, y, joint + cell - *previous, cell), TerrainRole::keyline, rectangle);
					}
				}
			}
			previous = joint;
		}
	}
	this->outline(rectangle, faces);
}

PencilMaterial::PencilMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	float mirror_x, RenderResources* render_resources) :
	OrientedTerrainMaterial(level_format::material_name::pencil, rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();

	this->place(0.0f, 0.0f, length, thickness, TerrainRole::pencil_body);
	if (thickness >= 10.0f * cell)
	{
		// Light from the top left: the lower facet is in shade.
		const float facet = cells(thickness * 0.65f);
		this->place(0.0f, facet, length, thickness - facet, TerrainRole::pencil_shade);
	}
	else if (thickness >= 6.0f * cell)
	{
		this->place(0.0f, cells(thickness * 0.55f), length, cell, TerrainRole::pencil_shade);
	}

	const bool start_open = this->vertical()
		? exposed(faces.top, rectangle.left(), rectangle.right())
		: exposed(faces.left, rectangle.top(), rectangle.bottom());
	const bool end_open = this->vertical()
		? exposed(faces.bottom, rectangle.left(), rectangle.right())
		: exposed(faces.right, rectangle.top(), rectangle.bottom());

	if (length >= std::max(4.0f * thickness, 15.0f * cell))
	{
		// The eraser points away from the stage's mirror line, the cut wood
		// end toward it, so mirrored halves read as mirrored. A pencil lying
		// across the line is cut at both ends, so it stays symmetric; one
		// standing up has its eraser at the top.
		const bool across_mirror = !this->vertical() &&
			rectangle.left() < mirror_x && rectangle.right() > mirror_x;
		const bool eraser_at_start = this->vertical() || rectangle.center().x < mirror_x;
		const float eraser = cells(std::clamp(thickness * 1.25f, 3.0f * cell, length / 5.0f));
		const float ferrule = cells(thickness * 0.6f);
		const float wood = cells(thickness * 0.5f);
		const float core = cells(thickness / 3.0f);
		const float core_at = cells((thickness - core) / 2.0f);
		const float shade_at = cells(thickness * 0.65f);

		// at_start: the cap is laid from the start (along = 0); otherwise it is
		// mirrored to end at the far end (along = length).
		const auto eraser_cap = [&](bool at_start)
		{
			const auto span = [&](float from, float size, float across, float across_size, TerrainRole role)
			{
				this->place(at_start ? from : length - from - size, across, size, across_size, role);
			};
			span(0.0f, eraser, 0.0f, thickness, TerrainRole::eraser_fill);
			span(0.0f, eraser, shade_at, thickness - shade_at, TerrainRole::eraser_shade);
			span(eraser, cell, 0.0f, thickness, TerrainRole::keyline);
			span(eraser + cell, ferrule, 0.0f, thickness, TerrainRole::metal_light);
			span(eraser + cell + cells(ferrule / 2.0f) - cell, cell, 0.0f, thickness, TerrainRole::metal_dark);
			span(eraser + cell + ferrule, cell, 0.0f, thickness, TerrainRole::keyline);
		};
		const auto wood_end = [&](bool at_start)
		{
			const auto span = [&](float from, float size, float across, float across_size, TerrainRole role)
			{
				this->place(at_start ? from : length - from - size, across, size, across_size, role);
			};
			span(0.0f, wood, 0.0f, thickness, TerrainRole::pencil_wood);
			span(0.0f, wood, core_at, core, TerrainRole::graphite);
			span(wood, cell, 0.0f, thickness, TerrainRole::keyline);
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
	OrientedTerrainMaterial(level_format::material_name::ruler, rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->configure_miniature_ruler(rectangle, this->vertical());
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::ruler_fill);

	if (thickness >= 6.0f * cell)
	{
		// Short marks every minor period, long every major, rising from the lower edge.
		const float origin = this->vertical() ? rectangle.top() : rectangle.left();
		const float bottom_keyline = cell;
		for (float world = grid_from(origin + cell * 2.0f, ruler_minor_period);
			world < origin + length - cell * 2.0f; world += ruler_minor_period)
		{
			const bool major = std::fmod(std::abs(world), ruler_major_period) < 0.5f;
			const float mark = (major ? 3.0f : 2.0f) * cell;
			const float along = world - origin;
			const float across = thickness - bottom_keyline - mark;
			this->add_ruler_mark(this->vertical()
				? RectangleF(rectangle.x + across, rectangle.y + along, mark, cell)
				: RectangleF(rectangle.x + along, rectangle.y + across, cell, mark), rectangle, major);
		}
	}

	TerrainFaces drawn = faces;
	if (thickness < 5.0f * cell)
	{
		// Too thin for two keylines: the lower (or right) edge stays open.
		(this->vertical() ? drawn.right : drawn.bottom).clear();
	}
	this->outline(rectangle, drawn);
}

EraserMaterial::EraserMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	OrientedTerrainMaterial(level_format::material_name::eraser, rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::eraser_fill);
	if (thickness >= 6.0f * cell)
	{
		const float shade = cells(thickness * 0.65f);
		this->place(0.0f, shade, length, cell, TerrainRole::keyline);
		this->place(0.0f, shade + cell, length, thickness - shade - cell, TerrainRole::eraser_shade);
	}
	if (thickness >= 10.0f * cell && length >= 20.0f * cell)
	{
		// One shine dash, below the 12-unit paint band so paint never hides it.
		this->place(3.0f * cell, 4.0f * cell, std::min(length / 3.0f, 40.0f * cell), cell,
			TerrainRole::highlight);
	}
	this->outline(rectangle, faces);
}

PaintTinMaterial::PaintTinMaterial(const RectangleF& rectangle, const TerrainFaces& faces,
	RenderResources* render_resources) :
	OrientedTerrainMaterial(level_format::material_name::paint_tin, rectangle, render_resources)
{
	const float length = this->length();
	const float thickness = this->thickness();
	this->place(0.0f, 0.0f, length, thickness, TerrainRole::metal_light);
	// The rim sits just under the paint band; the far side is in shade.
	this->place(0.0f, 3.0f * cell, length, cell, TerrainRole::metal_dark);
	const float side = cells(length * 0.15f);
	this->place(length - side, 4.0f * cell, side, thickness - 4.0f * cell, TerrainRole::metal_dark);
	const float label_top = cells(thickness * 0.35f);
	const float label_bottom = cells(thickness * 0.75f);
	if (label_bottom - label_top >= 3.0f * cell)
	{
		this->place(0.0f, label_top, length, cell, TerrainRole::keyline);
		this->place(0.0f, label_top + cell, length, label_bottom - label_top - cell * 2.0f,
			TerrainRole::prop_accent);
		this->place(0.0f, label_bottom - cell, length, cell, TerrainRole::keyline);
	}
	this->outline(rectangle, faces);
}

SetSquareMaterial::SetSquareMaterial(const Shape& shape, const TerrainFaces& faces,
	RenderResources* render_resources) :
	TerrainMaterial(level_format::material_name::set_square, render_resources)
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
		const int count = static_cast<int>(std::ceil(height / cell - 0.001f));
		for (int row = 0; row < count; ++row)
		{
			const float leg_near = static_cast<float>(row) * cell;
			const float leg_far = std::min(height, leg_near + cell);
			const float y0 = corner_below ? t.corner.y - leg_far : t.corner.y + leg_near;
			const float y1 = corner_below ? t.corner.y - leg_near : t.corner.y + leg_far;
			const auto span = t.span(y0, y1);
			if (!span) { continue; }
			const RectangleF band(span->first, y0, span->second - span->first, y1 - y0);
			if (outer_edge) { this->add(band, TerrainRole::metal_light, box); }

			// The stair edge: each row covers the step to the next.
			const float step = std::max(cell,
				std::abs(t.hypotenuse_x(y0) - t.hypotenuse_x(y1)) + cell);
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
					constexpr float period = 8.0f * cell;
					const float phase = std::floor(y0 / cell) * cell;
					for (float x = grid_from(seal.left() + phase - cell, period) - phase;
						x < seal.right(); x += period)
					{
						this->add(RectangleF(x, y0, cell, y1 - y0), TerrainRole::metal_light, seal);
					}
				};
				const float seal_width = step + 2.0f * cell;
				seal_row(RectangleF(t.hypotenuse_left() ? span->first : span->second - seal_width,
					y0, seal_width, y1 - y0));
				if (overlaps(vertical_leg, y0, y1))
				{
					seal_row(RectangleF(corner_right ? span->second - 3.0f * cell : span->first,
						y0, 3.0f * cell, y1 - y0));
				}
				if (leg_near < 3.0f * cell)
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
				const RectangleF leg(corner_right ? span->second - cell : span->first, y0,
					cell, y1 - y0);
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
	if (std::abs(inner.corner.y - inner.apex.y) >= 6.0f * cell)
	{
		rows(inner, false, TerrainRole::metal_dark);
	}
}
