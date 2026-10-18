#include "game/pch.h"
#include "game/tuning.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/drifting_visual.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/structure.h"
#include "game/objects/terrain_material.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/camera.h"
#include "engine/math/triangle.h"

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

	void check_cloud_pose(const DriftingVisual& cloud, const RectangleF& anchor,
		float distance, const RectangleF& cull, const std::string& where)
	{
		const RectangleF r = cloud.rectangle();
		require(std::isfinite(r.x) && std::abs(r.x - anchor.x) <= distance + 2.0f + EPSILON,
			where + ": cloud left its finite horizontal excursion");
		require(r.y == anchor.y && r.width == anchor.width && r.height == anchor.height,
			where + ": cloud changed its height or size");
		const float cells = (r.x - anchor.x) / 4.0f;
		require(std::abs(cells - std::round(cells)) <= EPSILON,
			where + ": cloud drift is not on the four-unit art grid");
		require(inside(r, cloud.bounds()) && inside(cloud.bounds(), cull),
			where + ": current geometry escaped its bounds or cull extent");
		require(inside(r, cull) && inside(anchor, cull),
			where + ": cloud escaped its original cull extent");
	}

	void check_shipped_clouds(World& world, const LevelObjectBuilder& builder,
		const JsonValue& decorations, const std::string& stage)
	{
		world.preferences.reduce_effects = false;
		const auto first = builder.build_non_collision_objects(decorations);
		const auto restarted = builder.build_non_collision_objects(decorations);
		require(first.size() == decorations.size(), stage + ": decoration count changed");
		size_t clouds = 0;
		bool varied_phase = false;
		float first_phase = 0.0f;
		for (size_t index = 0; index < decorations.size(); ++index)
		{
			const JsonValue json = decorations.at(index);
			const bool is_cloud = json.string("frame_name").starts_with("cloud_");
			auto* cloud = dynamic_cast<DriftingVisual*>(first[index].get());
			if (!is_cloud)
			{
				require(cloud == nullptr && !json.has("drift"),
					stage + ": drift reached scenery other than the clouds");
				continue;
			}
			const std::string where = stage + " " + json.string("name");
			require(cloud != nullptr && json.has("drift"), where + ": cloud is static");
			const JsonValue drift = json.object("drift");
			const float distance = drift.number("distance");
			const float period = drift.number("period");
			const float phase = drift.number("phase");
			if (clouds == 0) { first_phase = phase; }
			else { varied_phase |= phase != first_phase; }
			++clouds;
			const RectangleF anchor = authored_rectangle(json);
			const RectangleF initial = cloud->rectangle();
			const auto* again = dynamic_cast<const DriftingVisual*>(restarted[index].get());
			require(again != nullptr, where + ": restart changed the cloud type");
			same_rectangle(initial, again->rectangle(), where + ": restart changed its authored phase");

			// Compare the builder's authored phase against elapsed time from phase
			// zero. The clock's known quarter-cycle poses are checked separately.
			DriftingVisual phase_zero(json.string("sheet_name"), json.string("frame_name"),
				anchor, &world.render, &world.preferences, Colour::white, distance, period, 0.0f);
			phase_zero.update(period * phase);
			same_rectangle(initial, phase_zero.rectangle(), where + ": builder lost its authored phase");

			const RectangleF cull = cloud->cull_bounds(0.0f);
			for (int step = 0; step <= 72; ++step)
			{
				check_cloud_pose(*cloud, anchor, distance, cull, where);
				cloud->update(period / 72.0f);
			}
			world.preferences.reduce_effects = true;
			same_rectangle(cloud->rectangle(), anchor, where + ": reduced effects did not restore the anchor immediately");
			require(inside(anchor, cloud->bounds()) && inside(anchor, cull),
				where + ": reduced cloud escaped its bounds");
			world.preferences.reduce_effects = false;
		}
		require(clouds >= 4 && varied_phase, stage + ": needs several clouds with staggered phases");
	}

	void check_cloud_clock(World& world)
	{
		world.preferences.reduce_effects = false;
		const RectangleF anchor(100.0f, 200.0f, 256.0f, 96.0f);
		const auto make = [&](float distance = 64.0f, float phase = 0.0f)
		{
			return DriftingVisual("terrain", "cloud_big", anchor, &world.render,
				&world.preferences, Colour::white, distance, 32.0f, phase);
		};
		// Construction and visual time must not consume the gameplay rand stream.
		std::srand(7321);
		const int expected_random = std::rand();
		std::srand(7321);
		auto cloud = make();
		const RectangleF cull = cloud.cull_bounds(0.0f);
		const float offsets[]{ 0.0f, 64.0f, 0.0f, -64.0f, 0.0f };
		for (size_t quarter = 0; quarter < std::size(offsets); ++quarter)
		{
			if (quarter != 0) { cloud.update(8.0f); }
			same_rectangle(cloud.rectangle(), RectangleF(anchor.x + offsets[quarter], anchor.y,
				anchor.width, anchor.height), "cloud missed a known quarter-cycle pose");
			check_cloud_pose(cloud, anchor, 64.0f, cull, "quarter-cycle cloud");
		}
		require(std::rand() == expected_random, "cloud construction/update consumed gameplay randomness");

		auto phased = make(64.0f, 0.25f);
		same_rectangle(phased.rectangle(), RectangleF(164.0f, 200.0f, 256.0f, 96.0f),
			"initial cloud phase was delayed until its first update");
		const RectangleF still = phased.rectangle();
		for (float dt : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
			-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
		{
			phased.update(dt);
			same_rectangle(phased.rectangle(), still, "nonpositive/nonfinite dt moved the cloud clock");
		}

		auto large_tick = make();
		large_tick.update(1048584.0f); // 32768 complete periods and one quarter.
		same_rectangle(large_tick.rectangle(), still, "large dt did not wrap by the authored period");
		large_tick.update(std::numeric_limits<float>::max());
		check_cloud_pose(large_tick, anchor, 64.0f, cull, "maximum finite dt");
		auto whole = make();
		auto split = make();
		whole.update(7.25f);
		for (int step = 0; step < 58; ++step) { split.update(0.125f); }
		same_rectangle(split.rectangle(), whole.rectangle(), "equal elapsed time depends on frame partitioning");

		const RectangleF moving = phased.rectangle();
		world.preferences.reduce_effects = true;
		same_rectangle(phased.rectangle(), anchor, "live reduction needs an update before restoring the anchor");
		same_rectangle(phased.bounds(), anchor, "live reduction left bounds at the moving pose");
		require(inside(moving, phased.cull_bounds(0.0f)) && inside(anchor, phased.cull_bounds(0.0f)),
			"live reduction invalidated moving/static cull coverage");
		phased.update(8.0f);
		world.preferences.reduce_effects = false;
		same_rectangle(phased.rectangle(), moving, "reduced effects advanced the hidden cloud clock");
		phased.update(8.0f);
		same_rectangle(phased.rectangle(), anchor, "cloud did not resume from its held phase");
		world.preferences.reduce_effects = true;
		auto reduced_start = make(64.0f, 0.25f);
		reduced_start.update(20.0f);
		same_rectangle(reduced_start.rectangle(), anchor, "reduced initial construction did not stay static");
		world.preferences.reduce_effects = false;
		same_rectangle(reduced_start.rectangle(), moving, "reduced initial construction lost its authored phase");

		// Rounding an authored distance can exceed that distance. The cull
		// extent must include the snapped endpoints before the first update.
		auto snapped = make(63.0f);
		const RectangleF snapped_cull = snapped.cull_bounds(0.0f);
		snapped.update(8.0f);
		require(snapped.rectangle().x == 164.0f, "cloud peak did not snap to the art grid");
		check_cloud_pose(snapped, anchor, 63.0f, snapped_cull, "snapped positive peak");
		snapped.update(16.0f);
		require(snapped.rectangle().x == 36.0f, "cloud negative peak did not snap to the art grid");
		check_cloud_pose(snapped, anchor, 63.0f, snapped_cull, "snapped negative peak");
		for (float zoom : { 0.5f, 1.0f, 4.0f, 16.0f })
		{
			const RectangleF zoom_cull = snapped.cull_bounds(zoom);
			require(inside(snapped_cull, zoom_cull), "view-pixel cull padding shrank the cloud excursion");
			require(zoom_cull.left() < snapped_cull.left() && zoom_cull.right() > snapped_cull.right() &&
				zoom_cull.top() < snapped_cull.top() && zoom_cull.bottom() > snapped_cull.bottom(),
				"cloud culling omitted view-pixel snap padding");
		}
	}

	void check_cloud_rejections(World& world, const GameTuning& tuning,
		const std::filesystem::path& scratch)
	{
		std::filesystem::create_directories(scratch);
		const LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &tuning.paint());
		const auto fixture = [&](const std::string& drift, size_t index)
		{
			const auto path = scratch / ("cloud-drift-" + std::to_string(index) + ".json");
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			file << "[{\"type\":\"Visual\",\"sheet_name\":\"terrain\",\"frame_name\":\"cloud_big\","
				"\"colour\":\"WHITE\",\"rectangle\":{\"x\":100,\"y\":200,\"width\":256,\"height\":96},"
				"\"drift\":" << drift << "}]";
			file.close();
			require(file.good(), "cannot write cloud drift fixture");
			return path;
		};
		struct Rejection { const char* drift; const char* field; };
		const Rejection rejected[]{
			{ "null", "drift" }, { "[]", "drift" },
			{ "{\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":0,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":-1,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":257,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":1e100,\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":\"64\",\"period\":32,\"phase\":0}", "distance" },
			{ "{\"distance\":64,\"period\":0.5,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":3601,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":1e100,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":false,\"phase\":0}", "period" },
			{ "{\"distance\":64,\"period\":32,\"phase\":-0.01}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":1}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":1e100}", "phase" },
			{ "{\"distance\":64,\"period\":32,\"phase\":[]}", "phase" }
		};
		for (size_t index = 0; index < std::size(rejected); ++index)
		{
			const auto path = fixture(rejected[index].drift, index);
			const JsonDocument document = read_json_file(path.string().c_str());
			bool named = false;
			try { builder.build_non_collision_objects(document.root()); }
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				named = message.find(path.filename().string()) != std::string::npos &&
					message.find("[0].drift") != std::string::npos &&
					message.find(rejected[index].field) != std::string::npos;
			}
			require(named, "malformed cloud drift must fail naming its file, object and " +
				std::string(rejected[index].field));
		}
		// Boundary values are useful authoring choices, not just refusal cases.
		const auto path = fixture("{\"distance\":256,\"period\":3600,\"phase\":0.999}", std::size(rejected));
		const JsonDocument document = read_json_file(path.string().c_str());
		const auto accepted = builder.build_non_collision_objects(document.root());
		require(accepted.size() == 1 && dynamic_cast<const DriftingVisual*>(accepted[0].get()) != nullptr,
			"valid cloud drift boundary values were rejected");
	}

	// Every quad lies inside the collision shape: the art draws no ledge, lip
	// or overhang that collision does not have.
	void check_drawing(const Structure& structure, const std::string& where)
	{
		const TerrainMaterial* material = structure.material();
		const auto& quads = material->quads();
		require(!quads.empty(), where + " draws nothing");
		require(quads.size() <= QUAD_BUDGET, where + " draws " + std::to_string(quads.size()) +
			" quads; the budget is " + std::to_string(QUAD_BUDGET));
		const Triangle* triangle = dynamic_cast<const Triangle*>(structure.shape());
		for (const TerrainQuad& quad : quads)
		{
			const RectangleF& r = quad.rectangle;
			require(r.width > 0.0f && r.height > 0.0f, where + " has an empty quad");
			require(inside(r, structure.shape()->bounding_box()),
				where + " draws outside its collision rectangle");
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

	void same_quads(const std::vector<TerrainQuad>& actual, const std::vector<TerrainQuad>& expected,
		const std::string& where)
	{
		require(actual.size() == expected.size(), where + ": quad count changed");
		for (size_t index = 0; index < actual.size(); ++index)
		{
			require(actual[index].role == expected[index].role &&
				actual[index].rectangle == expected[index].rectangle, where + ": quad order or geometry changed");
		}
	}

	TerrainRole pixel_role(const std::vector<TerrainQuad>& quads, int x, int y)
	{
		TerrainRole role = TerrainRole::count; // A clear pixel, outside all quads.
		for (const TerrainQuad& quad : quads)
		{
			const RectangleF& r = quad.rectangle;
			if (static_cast<float>(x) >= r.left() && static_cast<float>(x) < r.right() &&
				static_cast<float>(y) >= r.top() && static_cast<float>(y) < r.bottom())
			{
				role = quad.role;
			}
		}
		return role;
	}

	void check_miniature_projection(const TerrainMaterial& material, const Shape& shape,
		const std::string& where)
	{
		const auto authored = material.quads();
		const Camera cameras[]{ Camera(-13.25f, 9.75f, 0.04f),
			Camera(53.75f, -17.25f, 0.125f), Camera(-1.5f, 0.75f, 0.25f),
			Camera(7.25f, -4.5f, 0.7f), Camera(0.0f, 0.0f, 1.0f) };
		for (const Camera& camera : cameras)
		{
			const auto projected = material.projected_miniature_quads(camera);
			same_quads(projected, material.projected_miniature_quads(camera),
				where + ": repeated miniature projection");
			const RectangleF limit = ordinary_projection(shape.bounding_box(), camera);
			for (const TerrainQuad& quad : projected)
			{
				const RectangleF& r = quad.rectangle;
				require(std::isfinite(r.x) && std::isfinite(r.y) &&
					std::isfinite(r.width) && std::isfinite(r.height) &&
					r.width > 0.0f && r.height > 0.0f && inside(r, limit),
					where + ": miniature quad escaped its quantized collision bounds");
				require(r.x == std::trunc(r.x) && r.y == std::trunc(r.y) &&
					r.width == std::trunc(r.width) && r.height == std::trunc(r.height),
					where + ": miniature geometry is not in whole view pixels");
			}
			if (camera.scale >= 0.25f)
			{
				same_quads(projected, ordinary_projection(material, camera),
					where + ": native-size material changed");
			}
		}
		same_quads(material.quads(), authored, where + ": miniature projection mutated the authored art");
	}

	TerrainFaces all_faces(const RectangleF& rectangle)
	{
		return { { { rectangle.left(), rectangle.right() } },
			{ { rectangle.left(), rectangle.right() } },
			{ { rectangle.top(), rectangle.bottom() } },
			{ { rectangle.top(), rectangle.bottom() } } };
	}

	void check_miniature_rulers(World& world)
	{
		// Known 320-by-40 fixtures exercise both orientations. A one-pixel
		// ruler must stay light, and two pixels must retain a light body below
		// one edge. Blanket widening would turn these into solid black bars.
		for (bool vertical : { false, true })
		{
			const RectangleF box(0.0f, 0.0f, vertical ? 40.0f : 320.0f,
				vertical ? 320.0f : 40.0f);
			const RulerMaterial ruler(box, all_faces(box), &world.render);
			check_miniature_projection(ruler, box, "known miniature ruler");
			const auto at = [&](const std::vector<TerrainQuad>& quads, int along, int across)
			{
				return pixel_role(quads, vertical ? across : along, vertical ? along : across);
			};
			const auto thin = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.04f));
			for (int along = 0; along < 12; ++along)
			{
				require(at(thin, along, 0) == TerrainRole::ruler_fill,
					"one-pixel miniature ruler became a black bar or lost its light silhouette");
			}
			const auto two = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.05f));
			for (int along = 1; along < 15; ++along)
			{
				require(at(two, along, 0) == TerrainRole::keyline &&
					at(two, along, 1) == TerrainRole::ruler_fill,
					"two-pixel miniature ruler lost its leading edge or light body");
			}
			const auto marked = ruler.projected_miniature_quads(Camera(0.0f, 0.0f, 0.1f));
			for (int along : { 8, 16, 24 })
			{
				require(at(marked, along, 2) == TerrainRole::ruler_marks &&
					at(marked, along + 1, 2) == TerrainRole::ruler_fill,
					"resolvable ruler major tick collapsed or widened into its neighbour");
			}
			require(at(marked, 1, 2) == TerrainRole::ruler_fill &&
				at(marked, 3, 2) == TerrainRole::ruler_fill,
				"unresolved ruler minor ticks crowded out the light interior");
		}
	}

	void check_miniature_faces(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 96.0f, 32.0f);
		TerrainFaces partial;
		partial.top = { { 16.0f, 48.0f } };
		const GroundMaterial ledge(box, partial, false, &world.render);
		const auto projected = ledge.projected_miniature_quads(Camera(0.0f, 0.0f, 0.1f));
		for (int x = 0; x < 9; ++x)
		{
			require(pixel_role(projected, x, 0) ==
				(x >= 1 && x < 4 ? TerrainRole::keyline : TerrainRole::ground_fill),
				"miniature exposed edge disappeared or grew beyond its authored span");
			require(pixel_role(projected, x, 1) == TerrainRole::ground_fill,
				"miniature edge widened instead of remaining one inward pixel");
		}

		// Two touching pieces expose only their outer ends. Sampling immediately
		// on either side of the shared face must find fill, not a new seam.
		TerrainFaces left = all_faces(box);
		left.right.clear();
		const RectangleF neighbour(96.0f, 0.0f, 96.0f, 32.0f);
		TerrainFaces right = all_faces(neighbour);
		right.left.clear();
		const GroundMaterial first(box, left, false, &world.render);
		const GroundMaterial second(neighbour, right, false, &world.render);
		const Camera camera(0.0f, 0.0f, 0.125f);
		const auto a = first.projected_miniature_quads(camera);
		const auto b = second.projected_miniature_quads(camera);
		for (int y : { 1, 2 })
		{
			require(pixel_role(a, 11, y) == TerrainRole::ground_fill &&
				pixel_role(b, 12, y) == TerrainRole::ground_fill,
				"miniature outlined an unexposed shared face");
		}
	}

	void check_miniature_ramps(World& world)
	{
		// Both walking slopes, plus negative world coordinates and fractional
		// camera origins. The unchanged authored raster is an occupancy oracle;
		// comparing all pixels also catches holes hidden by a bounding-box test.
		const TriangleRightAxisAligned ramps[]{
			{ 0.0f, 128.0f, 256.0f, 128.0f, 256.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 128.0f, 256.0f, 128.0f },
			{ -256.0f, -64.0f, 0.0f, -64.0f, 0.0f, -192.0f } };
		for (const TriangleRightAxisAligned& triangle : ramps)
		{
			const SetSquareMaterial ramp(triangle, all_faces(triangle.bounding_box()), &world.render);
			check_miniature_projection(ramp, triangle, "known miniature set square");
			for (const Camera& camera : { Camera(0.0f, 0.0f, 0.1f),
				Camera(13.25f, -7.75f, 0.04f), Camera(-7.5f, 5.25f, 0.125f) })
			{
				const auto projected = ramp.projected_miniature_quads(camera);
				const auto original = ordinary_projection(ramp, camera);
				const RectangleF limit = ordinary_projection(triangle.bounding_box(), camera);
				size_t light = 0;
				size_t ink = 0;
				for (int y = static_cast<int>(limit.top()) - 1;
					y <= static_cast<int>(limit.bottom()); ++y)
				{
					for (int x = static_cast<int>(limit.left()) - 1;
						x <= static_cast<int>(limit.right()); ++x)
					{
						const TerrainRole actual = pixel_role(projected, x, y);
						require((actual == TerrainRole::count) ==
							(pixel_role(original, x, y) == TerrainRole::count),
							"miniature set square grew beyond its stepped triangle or lost filled pixels");
						light += actual == TerrainRole::metal_light ? 1 : 0;
						ink += actual == TerrainRole::keyline ? 1 : 0;
					}
				}
				require(light > 0 && ink > 0,
					"miniature set square lost its light body or resolvable walking edge");
			}
		}
	}

	void check_miniature_rejections(World& world)
	{
		const RectangleF box(0.0f, 0.0f, 320.0f, 40.0f);
		const RulerMaterial ruler(box, all_faces(box), &world.render);
		const auto authored = ruler.quads();
		const float infinity = std::numeric_limits<float>::infinity();
		const float nan = std::numeric_limits<float>::quiet_NaN();
		const Camera invalid[]{ Camera(0.0f, 0.0f, 0.0f), Camera(0.0f, 0.0f, -0.1f),
			Camera(0.0f, 0.0f, infinity), Camera(0.0f, 0.0f, nan),
			Camera(infinity, 0.0f, 0.1f), Camera(0.0f, -infinity, 0.1f),
			Camera(nan, 0.0f, 1.0f), Camera(0.0f, nan, 1.0f) };
		for (const Camera& camera : invalid)
		{
			bool refused = false;
			try { ruler.projected_miniature_quads(camera); }
			catch (const std::invalid_argument&) { refused = true; }
			require(refused, "miniature accepted a nonpositive/nonfinite camera");
		}
		same_quads(ruler.quads(), authored, "invalid miniature camera mutated the authored art");
	}

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
		for (size_t index = 0; index < first.size(); ++index)
		{
			const JsonValue json = collision_json.at(index);
			const std::string where = stage + " " + (json.has("name") ? json.string("name") : std::to_string(index));
			const auto* structure = dynamic_cast<const Structure*>(first[index].get());
			require(structure != nullptr, where + " is not a structure");
			require(structure->material() != nullptr, where + " has no material");
			require(structure->material()->name() == json.string("material"),
				where + " built a different material than it names");
			check_drawing(*structure, where);
			check_miniature_projection(*structure->material(), *structure->shape(), where);

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
		for (const auto& object : shadows)
		{
			const auto* shadow = dynamic_cast<const AirbrushShadow*>(object.get());
			require(shadow != nullptr, stage + ": a shadow is not an AirbrushShadow");
			const RectangleF bounds = shadow->bounds();
			require(bounds.width > 0.0f && bounds.height > 0.0f, stage + ": empty shadow");
			require(inside(bounds, shadow->cull_bounds(1.0f)), stage + ": shadow escapes its cull bounds");
			bool under = false;
			for (const auto& candidate : first)
			{
				const RectangleF caster = candidate->shape()->bounding_box();
				under |= std::abs(caster.bottom() - bounds.top()) < EPSILON &&
					bounds.left() >= caster.left() - EPSILON && bounds.right() <= caster.right() + EPSILON;
			}
			require(under, stage + ": a shadow is not under any underside");
		}

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
		const GameTuning tuning((content / "tuning.json").string().c_str());
		size_t stages = 0;
		for (const std::filesystem::path& level : shipped_levels(content))
		{
			check_stage(world, tuning, level);
			++stages;
		}
		check_rejections(world);
		check_miniature_rulers(world);
		check_miniature_faces(world);
		check_miniature_ramps(world);
		check_miniature_rejections(world);
		check_cloud_clock(world);
		check_cloud_rejections(world, tuning,
			std::filesystem::absolute(argv[0]).parent_path() / "terrain-fixtures");
		std::cout << "PASS terrain: " << stages << " stages fully materialled, every drawing inside "
			"its collision shape and deterministic, shadows under real undersides, untinted "
			"backdrops on paper, quantized miniature silhouettes with visible edges and ruler ticks, "
			"bounded deterministic cloud drift with live reduction, and material/drift/camera refusals.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL terrain: " << error.what() << '\n';
		return 1;
	}
}
