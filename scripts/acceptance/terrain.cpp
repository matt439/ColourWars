#include "game/pch.h"
#include "game/tuning.h"
#include "game/objects/airbrush_shadow.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/structure.h"
#include "game/objects/terrain_material.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/math/triangle.h"

#include <filesystem>
#include <iostream>

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
		builder.build_non_collision_objects(decorations);
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
		std::cout << "PASS terrain: " << stages << " stages fully materialled, every drawing inside "
			"its collision shape and deterministic, shadows under real undersides, untinted "
			"backdrops on paper, and the material table's refusals.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL terrain: " << error.what() << '\n';
		return 1;
	}
}
