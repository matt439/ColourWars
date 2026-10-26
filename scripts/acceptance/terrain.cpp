#include "game/pch.h"
#include "game/tuning.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/drifting_visual.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/structure.h"
#include "game/objects/terrain_material.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/translated_airbrush_shadow.h"
#include "game/objects/fixed_slab_paintable.h"
#include "game/objects/fixed_slab_shadow.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/camera.h"
#include "engine/render/dds_file.h"
#include "engine/render/visual.h"
#include "engine/math/triangle.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <limits>

using namespace labrador;
using namespace mattmath;

// The Tool Props kit on every shipped stage (docs/polish-terrain.md): every
// collision object wears a material that fits its collision type, its drawing
// never leaves the collision shape, it is the same on every build, and the
// shadows sit under real undersides. No device is created.
namespace
{
	void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	constexpr float EPSILON = 0.01f;
	// Generous; a ruined budget is a material drawing per cell, not per run.
	constexpr size_t QUAD_BUDGET = 1600;

	struct World
	{
		RenderResources render;
		AudioResources audio;
		PlaySettings preferences;

		explicit World(const std::filesystem::path& content)
		{
			for (const char* sheet : { "sprite_sheet_1", "paint_fx", "terrain" })
			{
				render.add_sprite_sheet(sheet, read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
					TextureHandle{}));
			}
			audio.add_sound_bank("paint_exe", SoundBank::silent());
		}
	};

	std::vector<std::filesystem::path> shipped_levels(const std::filesystem::path& content)
	{
		const JsonDocument manifest = read_json_file((content / "manifest.json").string().c_str());
		const JsonValue assets = manifest.root().array("assets");
		std::vector<std::filesystem::path> levels;
		for (size_t group = 0; group < assets.size(); ++group)
		{
			const JsonValue asset = assets.at(group);
			if (asset.string("kind") != "level") { continue; }
			const JsonValue names = asset.array("names");
			for (size_t index = 0; index < names.size(); ++index)
			{
				levels.push_back(content / asset.string("directory") /
					(names.at(index).as_string() + ".json"));
			}
		}
		require(levels.size() >= 4, "Acceptance needs the four shipped stages");
		return levels;
	}

	bool inside(const RectangleF& inner, const RectangleF& outer)
	{
		return inner.left() >= outer.left() - EPSILON && inner.right() <= outer.right() + EPSILON &&
			inner.top() >= outer.top() - EPSILON && inner.bottom() <= outer.bottom() + EPSILON;
	}

	bool inside(const Vector2F& point, const Triangle& triangle)
	{
		const auto side = [&](const Vector2F& a, const Vector2F& b)
		{
			return (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
		};
		const Vector2F* p = triangle.points;
		const float d0 = side(p[0], p[1]);
		const float d1 = side(p[1], p[2]);
		const float d2 = side(p[2], p[0]);
		const float slack = 0.5f;
		const bool negative = d0 < -slack || d1 < -slack || d2 < -slack;
		const bool positive = d0 > slack || d1 > slack || d2 > slack;
		return !(negative && positive);
	}

	RectangleF authored_rectangle(const JsonValue& json)
	{
		if (json.has("rectangle"))
		{
			const JsonValue r = json.object("rectangle");
			return RectangleF(r.number("x"), r.number("y"), r.number("width"), r.number("height"));
		}
		const JsonValue r = json.object("rectangle_tlbr");
		return RectangleF::from_top_left_bottom_right(
			Vector2F(r.number("left"), r.number("top")),
			Vector2F(r.number("right"), r.number("bottom")));
	}

	void same_rectangle(const RectangleF& actual, const RectangleF& expected,
		const std::string& where)
	{
		require(std::abs(actual.x - expected.x) <= EPSILON &&
			std::abs(actual.y - expected.y) <= EPSILON &&
			std::abs(actual.width - expected.width) <= EPSILON &&
			std::abs(actual.height - expected.height) <= EPSILON, where);
	}

	// Every quad lies inside the collision shape: the art draws no ledge, lip
	// or overhang that collision does not have.
	void check_drawing(const Structure& structure, const std::string& where)
	{
		const TerrainMaterial* material = structure.material();
		const auto* mover = dynamic_cast<const TranslatingStructurePaintable*>(&structure);
		// A mover retains its material in anchor space; drawing translates the
		// camera. Static structures keep their original collision-shape path.
		const RectangleF anchor = mover != nullptr ? mover->motion_state().spec.anchor : RectangleF::ZERO;
		const Shape* material_shape = mover != nullptr ? &anchor : structure.shape();
		const auto& quads = material->quads();
		require(!quads.empty(), where + " draws nothing");
		require(quads.size() <= QUAD_BUDGET, where + " draws " + std::to_string(quads.size()) +
			" quads; the budget is " + std::to_string(QUAD_BUDGET));
		const Triangle* triangle = dynamic_cast<const Triangle*>(material_shape);
		for (const TerrainQuad& quad : quads)
		{
			const RectangleF& r = quad.rectangle;
			require(r.width > 0.0f && r.height > 0.0f, where + " has an empty quad");
			require(inside(r, material_shape->bounding_box()),
				where + " draws outside its collision rectangle");
			if (mover != nullptr)
			{
				RectangleF world = r;
				const float offset = mover->translation().x;
				if (offset != 0.0f) { world.offset(offset, 0.0f); }
				require(inside(world, structure.shape()->bounding_box()),
					where + ": translated material leaves its actual world collider");
			}
			if (triangle != nullptr)
			{
				const Vector2F corners[]{ r.top_left(), Vector2F(r.right(), r.top()),
					Vector2F(r.left(), r.bottom()), r.bottom_right() };
				for (const Vector2F& corner : corners)
				{
					require(inside(corner, *triangle), where + " draws outside its ramp triangle");
				}
			}
		}
	}

	// The registered fixed pair has an independently specified rigid frame.
	// Local material/tile rectangles are authoritative; the world AABB contains
	// empty corners and is never a substitute for the sloping solid or its score.
	Vector2F slab_world(const JsonValue& json, double along, double inward)
	{
		const JsonValue spec = json.object("slab");
		const double sign = spec.string("rise") == "right" ? -1.0 : 1.0;
		const double unit = 1.0 / std::sqrt(5.0);
		return { static_cast<float>(spec.number("x") + unit * (2.0 * along - sign * inward)),
			static_cast<float>(spec.number("y") + unit * (sign * along + 2.0 * inward)) };
	}

	void same_point(const Vector2F& actual, const Vector2F& expected, const std::string& where)
	{
		require(std::abs(actual.x - expected.x) <= EPSILON && std::abs(actual.y - expected.y) <= EPSILON, where);
	}

	void check_slab_drawing(const FixedSlabPaintable& slab, const FixedSlabPaintable& again,
		const JsonValue& json, const PaintTuning& paint, const std::string& where)
	{
		const double length = 80.0 * std::sqrt(5.0);
		const RectangleF local(0.0f, 0.0f, static_cast<float>(length), 24.0f);
		require(slab.local_material().name() == "eraser" && json.string("material") == "eraser",
			where + ": fixed slab lost its dedicated local material");
		same_point(slab.geometry().top_start(), slab_world(json, 0.0, 0.0), where + ": wrong top endpoint");
		same_point(slab.geometry().top_end(), slab_world(json, length, 0.0), where + ": wrong top endpoint");
		const auto& quads = slab.local_material().quads();
		const auto& repeated = again.local_material().quads();
		require(!quads.empty() && quads.size() <= QUAD_BUDGET && quads.size() == repeated.size(),
			where + ": missing, excessive or nondeterministic local material quads");
		for (size_t index = 0; index < quads.size(); ++index)
		{
			const RectangleF& r = quads[index].rectangle;
			require(r.width > 0.0f && r.height > 0.0f && inside(r, local), where + ": local material leaves the solid");
			require(quads[index].role == repeated[index].role && r == repeated[index].rectangle,
				where + ": local material is not deterministic");
			const auto world = slab.geometry().world_quad(r);
			const std::array<Vector2F, 4> actual{ world.point_0(), world.point_1(), world.point_2(), world.point_3() };
			const std::array<Vector2F, 4> expected{ slab_world(json, r.left(), r.top()), slab_world(json, r.right(), r.top()),
				slab_world(json, r.right(), r.bottom()), slab_world(json, r.left(), r.bottom()) };
			for (size_t corner = 0; corner < actual.size(); ++corner)
			{ same_point(actual[corner], expected[corner], where + ": local art lost its independently specified world frame"); }
		}
		std::vector<PaintSurfaceSample> samples, repeated_samples;
		slab.append_paint_surfaces(samples); again.append_paint_surfaces(repeated_samples);
		require(!samples.empty() && samples.size() == repeated_samples.size(), where + ": missing or nondeterministic local paint tiles");
		std::array<double, 4> areas{};
		for (size_t index = 0; index < samples.size(); ++index)
		{
			const auto& sample = samples[index]; const auto& peer = repeated_samples[index];
			require(sample.team == PlayerTeam::none && sample.paint_owner == -1 &&
				std::isfinite(sample.scored_area) && sample.scored_area > 0.0f,
				where + ": new local paint is owned or has an invalid authoritative area");
			require(sample.corners == peer.corners && sample.face == peer.face && sample.scored_area == peer.scored_area,
				where + ": local paint construction is not deterministic");
			areas.at(static_cast<size_t>(sample.face)) += sample.scored_area;
			for (const auto& corner : sample.corners)
			{
				const Vector2F origin = slab_world(json, 0.0, 0.0);
				const double sign = json.object("slab").string("rise") == "right" ? -1.0 : 1.0;
				const double dx = static_cast<double>(corner.x) - origin.x, dy = static_cast<double>(corner.y) - origin.y;
				const double along = (2.0 * dx + sign * dy) / std::sqrt(5.0);
				const double normal = (-sign * dx + 2.0 * dy) / std::sqrt(5.0);
				require(along >= -EPSILON && along <= length + EPSILON && normal >= -EPSILON && normal <= 24.0 + EPSILON,
					where + ": actual scoring tile leaves the independently projected collider");
			}
		}
		// This registered pair joins the unchanged crossover: one normal cap is
		// buried, and twelve local units of underside meet it. The diagonal top
		// stays whole. These lengths are analytic, not a sum of world AABB areas.
		const bool right = json.object("slab").string("rise") == "right";
		const auto faces = json.object("paintable_faces");
		for (const char* face : { "top", "bottom", "left", "right" })
		{ require(faces.boolean(face), where + ": registered scored slab lost an intended paint face"); }
		require(std::abs(areas[0] - length * paint.thickness) <= 0.05 &&
			std::abs(areas[1] - (length - 12.0) * paint.thickness) <= 0.05 &&
			std::abs(areas[right ? 2 : 3] - 24.0 * paint.thickness) <= 0.05 && areas[right ? 3 : 2] == 0.0,
			where + ": registered slab local-face score differs from the independent clipped-edge census");
		require(slab.exposed_bottom().size() == 1, where + ": underside shadow interval inventory changed");
		const auto underside = slab.exposed_bottom().front();
		require(std::abs(underside.first - (right ? 0.0 : 12.0)) <= EPSILON &&
			std::abs(underside.second - (right ? length - 12.0 : length)) <= EPSILON,
			where + ": shadow interval is not the actual exposed underside");
	}

	RectangleF slab_shadow_bounds(World& world, const FixedSlabPaintable& slab, const JsonValue& json)
	{
		const auto* sheet = world.render.sprite_sheet(world.render.resolve_sprite_sheet(TerrainMaterial::SHEET_NAME));
		const auto tile = sheet->sprite_frame(sheet->resolve_sprite_frame("shadow_0")).source_rectangle();
		require(tile.height > 0, "Fixed slab shadow needs a positive source tile height");
		const double length = 80.0 * std::sqrt(5.0);
		const bool right = json.object("slab").string("rise") == "right";
		const double begin = right ? 0.0 : 12.0, end = right ? length - 12.0 : length;
		const std::array<Vector2F, 4> corners{ slab_world(json, begin, 24.0), slab_world(json, end, 24.0),
			slab_world(json, end, 24.0 + tile.height), slab_world(json, begin, 24.0 + tile.height) };
		return RectangleF::union_of(slab.bounds(), RectangleF::bounding_box_of(corners));
	}

	// This is the renderer's ordinary edge quantisation, independently applied
	// to the original art. It is the unchanged native-size reference and the
	// occupancy reference for a miniature; strokes may move inward, never add
	// pixels outside the shape the old drawing already occupied.
	RectangleF ordinary_projection(const RectangleF& rectangle, const Camera& camera)
	{
		const RectangleF view = camera.calculate_view_rectangle(rectangle);
		const float left = std::trunc(view.left());
		const float top = std::trunc(view.top());
		const float right = std::trunc(view.right());
		const float bottom = std::trunc(view.bottom());
		return RectangleF(left, top, right - left, bottom - top);
	}

	std::vector<TerrainQuad> ordinary_projection(const TerrainMaterial& material, const Camera& camera)
	{
		std::vector<TerrainQuad> projected;
		for (const TerrainQuad& quad : material.quads())
		{
			const RectangleF rectangle = ordinary_projection(quad.rectangle, camera);
			if (rectangle.width > 0.0f && rectangle.height > 0.0f)
			{
				projected.push_back({ rectangle, quad.role });
			}
		}
		return projected;
	}
}

#include "scripts/acceptance/terrain/cloud_checks.h"
#include "scripts/acceptance/terrain/miniature_checks.h"
#include "scripts/acceptance/terrain/surface_finish_checks.h"
#include "scripts/acceptance/terrain/landmark_checks.h"
#include "scripts/acceptance/terrain/moving_checks.h"

namespace
{
	void check_stage(World& world, const GameTuning& tuning, const std::filesystem::path& path)
	{
		const std::string stage = path.stem().string();
		const JsonDocument document = read_json_file(path.string().c_str());
		const JsonValue collision_json = document.root().array("collision_objects");
		const LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences,
			&tuning.paint());
		const auto first = builder.build_collision_objects(collision_json, TeamColour{});
		const auto second = builder.build_collision_objects(collision_json, TeamColour{});
		require(first.size() == collision_json.size(), stage + ": object count changed");

		size_t casters = 0;
		std::vector<std::pair<const FixedSlabPaintable*, size_t>> slab_casters;
		double total_scored = 0.0;
		for (size_t index = 0; index < first.size(); ++index)
		{
			const JsonValue json = collision_json.at(index);
			const std::string where = stage + " " + (json.has("name") ? json.string("name") : std::to_string(index));
			const auto* structure = dynamic_cast<const Structure*>(first[index].get());
			require(structure != nullptr, where + " is not a structure");
			if (const auto* paintable = dynamic_cast<const PaintableObject*>(structure))
			{ total_scored += paintable->paint_total().total; }
			if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(structure))
			{
				const auto* peer = dynamic_cast<const FixedSlabPaintable*>(second[index].get());
				require(peer != nullptr, where + ": fixed slab construction changed type");
				check_slab_drawing(*slab, *peer, json, tuning.paint(), where);
				slab_casters.emplace_back(slab, index);
				continue;
			}
			require(structure->material() != nullptr, where + " has no material");
			require(structure->material()->name() == json.string("material"),
				where + " built a different material than it names");
			check_drawing(*structure, where);
			if (const auto* mover = dynamic_cast<const TranslatingStructurePaintable*>(structure))
			{
				const RectangleF anchor = mover->motion_state().spec.anchor;
				check_miniature_projection(*structure->material(), anchor, where);
			}
			else { check_miniature_projection(*structure->material(), *structure->shape(), where); }

			// The same content draws the same quads on every build.
			const auto* again = dynamic_cast<const Structure*>(second[index].get());
			const auto& a = structure->material()->quads();
			const auto& b = again->material()->quads();
			require(a.size() == b.size(), where + " is not deterministic");
			for (size_t quad = 0; quad < a.size(); ++quad)
			{
				require(a[quad].role == b[quad].role && a[quad].rectangle == b[quad].rectangle,
					where + " is not deterministic");
			}
			const RectangleF box = structure->shape()->bounding_box();
			if (json.string("type") != "StructureRamp" && box.width > box.height && box.height < 200.0f)
			{
				++casters;
			}
		}

		// Shadows hang under undersides, tile-tall, and cull by their own extent.
		const auto shadows = builder.build_shadows(collision_json);
		require(!shadows.empty() || casters == 0, stage + " casts no airbrush shadow");
		size_t slab_shadow_count = 0;
		for (const auto& object : shadows)
		{
			if (const auto* shadow = dynamic_cast<const FixedSlabShadow*>(object.get()))
			{
				require(slab_shadow_count < slab_casters.size(), stage + ": fixed shadow has no actual slab owner");
				const auto& [owner, source_index] = slab_casters[slab_shadow_count++];
				const auto expected = slab_shadow_bounds(world, *owner, collision_json.at(source_index));
				same_rectangle(shadow->bounds(), expected, stage + ": fixed shadow is not along its real sloping underside");
				for (float units : { 0.5f, 1.0f, 2.0f })
				{
					auto cull = shadow->bounds(); cull.inflate(3.0f * units);
					same_rectangle(shadow->cull_bounds(units), cull, stage + ": fixed shadow lost its rotated submission margin");
					require(inside(expected, shadow->cull_bounds(units)), stage + ": fixed shadow escapes its cull bounds");
				}
				continue;
			}
			const auto* shadow = dynamic_cast<const AirbrushShadow*>(object.get());
			require(shadow != nullptr, stage + ": a shadow is not an AirbrushShadow");
			const RectangleF bounds = shadow->bounds();
			require(bounds.width > 0.0f && bounds.height > 0.0f, stage + ": empty shadow");
			require(inside(bounds, shadow->cull_bounds(1.0f)), stage + ": shadow escapes its cull bounds");
			bool under = false;
			for (const auto& candidate : first)
			{
				if (dynamic_cast<const FixedSlabPaintable*>(candidate.get()) != nullptr) { continue; }
				const RectangleF caster = candidate->shape()->bounding_box();
				under |= std::abs(caster.bottom() - bounds.top()) < EPSILON &&
					bounds.left() >= caster.left() - EPSILON && bounds.right() <= caster.right() + EPSILON;
			}
			require(under, stage + ": a shadow is not under any underside");
		}
		require(slab_shadow_count == slab_casters.size(), stage + ": a fixed slab lost its value-owned shadow");
		if (stage == "turbulence_slab_trial")
		{
			require(slab_casters.size() == 2 && tuning.paint().thickness == 12.0f,
				stage + ": registered comparison lost the qualified pair or unchanged paint depth");
			const double length = 80.0 * std::sqrt(5.0);
			const double expected = 165120.0 + 2.0 * (12.0 * (2.0 * length + 12.0) - 144.0 * std::sqrt(5.0));
			require(std::abs(total_scored - expected) <= 0.15,
				stage + ": total score lost the preserved mover baseline or reciprocal crossover clipping");
		}
		check_moving_shadows(world, builder, collision_json, first, stage);

		// Backdrop hues come from terrain texels; every authored tint stays grey.
		const JsonValue decorations = document.root().array("non_collision_objects");
		bool paper = false;
		for (size_t index = 0; index < decorations.size(); ++index)
		{
			const JsonValue json = decorations.at(index);
			if (json.string("sheet_name") == TerrainMaterial::SHEET_NAME)
			{
				require(json.string("colour") == "WHITE",
					stage + ": a terrain backdrop is tinted; its colour belongs in the texels");
			}
			paper |= json.string("frame_name") == "swatch_paper";
		}
		require(paper, stage + " has no paper behind it");
		check_shipped_landmarks(world, builder, decorations, stage);
		check_shipped_clouds(world, builder, decorations, stage);
	}

	// The material table refuses what it does not draw.
	void check_rejections(World& world)
	{
		const RectangleF rectangle(0.0f, 0.0f, 400.0f, 40.0f);
		const TriangleRightAxisAligned ramp(0.0f, 100.0f, 300.0f, 100.0f, 300.0f, 0.0f);
		TerrainFaces faces;
		faces.top = { { 0.0f, 400.0f } };
		const auto refused = [&](const char* name, CollisionObjectType type, const Shape& shape)
		{
			try
			{
				TerrainMaterial::make(name, type, shape, faces, 0.0f, &world.render);
			}
			catch (const std::invalid_argument&)
			{
				return true;
			}
			return false;
		};
		require(refused("pencil", CollisionObjectType::structure, rectangle), "a solid took a pencil");
		require(refused("ruler", CollisionObjectType::structure_paintable, rectangle), "paint took a ruler");
		require(refused("ground", CollisionObjectType::structure_jump_through, rectangle), "a jump-through took ground");
		require(refused("set_square", CollisionObjectType::structure_paintable, rectangle), "a set square left the ramps");
		require(refused("ground", CollisionObjectType::structure_ramp_right, ramp), "a ramp took ground");
		require(refused("marble", CollisionObjectType::structure, rectangle), "an unknown material was accepted");
		require(!refused("ground", CollisionObjectType::structure_paintable, rectangle), "paintable ground was refused");
		require(!refused("set_square", CollisionObjectType::structure_ramp_left, ramp), "a ramp refused its set square");
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: terrain.exe <game/content directory>");
		const std::filesystem::path content = argv[1];
		World world(content);
		check_landmark_atlas(world, content);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		size_t stages = 0;
		const auto levels = shipped_levels(content);
		require(levels.size() == 9, "Terrain acceptance needs all nine registered stages");
		for (const char* stage : {"movement_course", "king_of_the_hill", "turbulence", "close_quarters", "crossfeed", "press_tower",
			"turbulence_blockout", "turbulence_mover_trial", "turbulence_slab_trial"})
		{
			require(std::count_if(levels.begin(), levels.end(), [&](const auto& path) { return path.stem().string() == stage; }) == 1,
				std::string("Landmark acceptance needs each shipped arena exactly once: ") + stage);
		}
		for (const std::filesystem::path& level : levels)
		{
			check_stage(world, tuning, level);
			++stages;
		}
		check_rejections(world);
		check_surface_finish(world);
		check_miniature_rulers(world);
		check_miniature_faces(world);
		check_miniature_ramps(world);
		check_miniature_rejections(world);
		check_cloud_clock(world);
		check_cloud_rejections(world, tuning,
			std::filesystem::absolute(argv[0]).parent_path() / "terrain-fixtures");
		std::cout << "PASS terrain: " << stages << " stages fully materialled, every drawing inside "
			"its collision shape and deterministic, shadows under real undersides, untinted "
			"backdrops on paper, shared sealed finishes clipped to exposed faces, quantized miniature silhouettes, "
			"qualified native-size static landmarks with stable bounds/RNG/reduction, paper-only provisional blockouts, "
			"bounded deterministic cloud drift with live reduction, and material/drift/camera refusals.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL terrain: " << error.what() << '\n';
		return 1;
	}
}
