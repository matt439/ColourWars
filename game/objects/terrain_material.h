#pragma once

#include "engine/render/texture_object.h"
#include "engine/math/rectanglef.h"
#include "engine/math/shape.h"
#include "game/objects/collision_object_type.h"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// The Muted Day roles the terrain sheet carries as `swatch_<role>` frames.
// Hues live in those texels; every authored JSON tint stays grey
// (docs/polish-terrain.md). Order is the swatch table, not a ranking.
enum class TerrainRole
{
	keyline, paper, bg_far, bg_mid, bg_line, bg_detail,
	ground_fill, ground_top, ground_shade,
	pencil_body, pencil_shade, pencil_wood, graphite,
	ruler_fill, ruler_marks, eraser_fill, eraser_shade,
	metal_light, metal_dark, shadow, shadow_deep,
	face, hud_paper, hud_ink, highlight, prop_accent,
	count
};

const char* terrain_role_name(TerrainRole role);

// One solid swatch stretched over a world rectangle.
struct TerrainQuad
{
	mattmath::RectangleF rectangle;
	TerrainRole role = TerrainRole::keyline;
};

// The spans of each face that nothing else covers, in world units along the
// face. Outlines, lips and end caps are drawn only there, so abutting and
// stacked rectangles read as one surface rather than a seam.
struct TerrainFaces
{
	std::vector<std::pair<float, float>> top, bottom, left, right;
};

// A Tool Props material: how a walkable rectangle (or a ramp's triangle) is
// drawn. The drawing is worked out once, at construction, into a fixed list of
// swatch quads that never leaves the collision shape; draw() only replays it,
// so it is const, deterministic and identical in every view.
class TerrainMaterial : public labrador::TextureObject
{
public:
	static constexpr const char* SHEET_NAME = "terrain";
	// One Paint pixel at Zoom 400%: four world units, as the painter's cells.
	static constexpr float CELL = 4.0f;

	// Throws std::invalid_argument for an unknown name, or a name the
	// collision type may not wear (the table is in docs/level-format.md).
	static std::unique_ptr<const TerrainMaterial> make(const std::string& name,
		CollisionObjectType type, const mattmath::Shape& shape,
		const TerrainFaces& faces, float mirror_x,
		labrador::RenderResources* render_resources);

	virtual ~TerrainMaterial() = default;

	void paint(labrador::DrawList& draw_list) const;

	const std::string& name() const;
	const std::vector<TerrainQuad>& quads() const;

protected:
	TerrainMaterial(const std::string& name, labrador::RenderResources* render_resources);

	// Adds a quad clipped to `limit`; an empty result is dropped.
	void add(const mattmath::RectangleF& rectangle, TerrainRole role,
		const mattmath::RectangleF& limit);
	// A keyline CELL wide inside each exposed span of each face.
	void outline(const mattmath::RectangleF& rectangle, const TerrainFaces& faces);

private:
	std::string name_;
	std::array<labrador::SpriteSheet::frame_handle,
		static_cast<size_t>(TerrainRole::count)> swatches_{};
	std::vector<TerrainQuad> quads_;
};

// Drawn in a frame along the rectangle's long axis: `along` runs left to right
// (or top to bottom when the rectangle is taller than wide), `across` from the
// lit face (top, or left) to the shaded one. A vertical pencil is the same
// drawing turned a quarter, eraser at the top and shade on the right.
class OrientedTerrainMaterial : public TerrainMaterial
{
protected:
	OrientedTerrainMaterial(const std::string& name, const mattmath::RectangleF& rectangle,
		labrador::RenderResources* render_resources);

	float length() const;
	float thickness() const;
	bool vertical() const;
	const mattmath::RectangleF& rectangle() const;
	// along, across, along size, across size -> a quad in world space.
	void place(float along, float across, float along_size, float across_size,
		TerrainRole role);
	// The exposed spans of the start (along = 0) and end faces' far ends.
	static bool exposed(const std::vector<std::pair<float, float>>& face,
		float low, float high);

private:
	mattmath::RectangleF rectangle_;
	bool vertical_ = false;
};

// Solid ground and the big paintable floors: halved-bond tile courses with
// black joints near the top, darkening to ground_shade below. Paintable
// ground carries a light lip under the top keyline; solid ground does not.
class GroundMaterial final : public TerrainMaterial
{
public:
	GroundMaterial(const mattmath::RectangleF& rectangle, const TerrainFaces& faces,
		bool paintable, labrador::RenderResources* render_resources);
};

// A long paintable pencil: eraser and ferrule at one end, a cut wood end with
// its graphite core at the other, a facet line along the body.
class PencilMaterial final : public OrientedTerrainMaterial
{
public:
	PencilMaterial(const mattmath::RectangleF& rectangle, const TerrainFaces& faces,
		float mirror_x, labrador::RenderResources* render_resources);
};

// A jump-through ruler: light, with tick marks rising from its lower edge.
class RulerMaterial final : public OrientedTerrainMaterial
{
public:
	RulerMaterial(const mattmath::RectangleF& rectangle, const TerrainFaces& faces,
		labrador::RenderResources* render_resources);
};

// A big paintable eraser block, shaded lower third under a keyline.
class EraserMaterial final : public OrientedTerrainMaterial
{
public:
	EraserMaterial(const mattmath::RectangleF& rectangle, const TerrainFaces& faces,
		labrador::RenderResources* render_resources);
};

// A paintable paint tin seen side on: rim, label band, shaded side.
class PaintTinMaterial final : public OrientedTerrainMaterial
{
public:
	PaintTinMaterial(const mattmath::RectangleF& rectangle, const TerrainFaces& faces,
		labrador::RenderResources* render_resources);
};

// A ramp as a set square: metal, stepped in whole cells inside the triangle,
// keylined along exposed edges, with its engraved inner triangle drawn as a
// line rather than a hole (a hole would draw a ledge that is not there).
class SetSquareMaterial final : public TerrainMaterial
{
public:
	SetSquareMaterial(const mattmath::Shape& triangle, const TerrainFaces& faces,
		labrador::RenderResources* render_resources);
};
