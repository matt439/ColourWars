#include "game/pch.h"
#include "game/simulation.h"
#include "game/tunnelling_limits.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/projectile_builder.h"
#include "game/objects/structure.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/collision/tunnelling.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

// The engine's anti-tunnelling bound, evaluated by the game against the real
// tuning and every shipped stage, and the one place it says "unsafe" and the
// game is right to ignore it: a swept projectile.
namespace
{
	void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	struct World
	{
		RenderResources render;
		AudioResources audio;
		PlaySettings preferences;

		explicit World(const std::filesystem::path& content)
		{
			for (const char* sheet : { "sprite_sheet_1", "paint_fx" })
			{
				render.add_sprite_sheet(sheet, read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
					TextureHandle{}));
			}
			audio.add_sound_bank("paint_exe", SoundBank::silent());
		}

		LevelObjectBuilder builder(const GameTuning& tuning)
		{
			return LevelObjectBuilder(&render, &audio, &preferences, &tuning.paint());
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

	// Copies of the authored tuning with one edit, written beside this case.
	struct Fixtures
	{
		rapidjson::Document source;
		std::filesystem::path scratch;
		size_t next = 0;

		Fixtures(const std::filesystem::path& content, const std::filesystem::path& folder) :
			scratch(folder)
		{
			std::ifstream input(content / "tuning.json", std::ios::binary);
			require(input.good(), "Cannot read authored tuning");
			const std::string text{ std::istreambuf_iterator<char>(input), {} };
			source.Parse(text.c_str());
			require(!source.HasParseError(), "Cannot parse authored tuning");
			std::filesystem::create_directories(scratch);
		}

		std::filesystem::path write_text(const std::string& name, const std::string& text) const
		{
			const auto path = scratch / name;
			std::ofstream output(path, std::ios::binary | std::ios::trunc);
			output << text;
			output.close();
			require(output.good(), "Cannot write an isolated tunnelling fixture");
			return path;
		}

		template<class Edit>
		std::filesystem::path write(Edit edit)
		{
			rapidjson::Document document;
			document.CopyFrom(source, document.GetAllocator());
			edit(document);
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			require(document.Accept(writer), "Cannot encode isolated tuning");
			return write_text("tunnelling-tuning-" + std::to_string(next++) + ".json",
				std::string(buffer.GetString(), buffer.GetSize()));
		}

		template<class Edit>
		GameTuning load(Edit edit)
		{
			return GameTuning(write(edit).string().c_str());
		}
	};

	// The message from check_stage_tunnelling, or empty when it passes.
	std::string rejection(const GameTuning& tuning, const LevelObjectBuilder& builder,
		const JsonValue& objects, float seconds_per_step = SECONDS_PER_STEP)
	{
		try
		{
			check_stage_tunnelling(tuning, builder, objects, seconds_per_step);
		}
		catch (const std::runtime_error& error)
		{
			return error.what();
		}
		return {};
	}

	bool mentions(const std::string& message, std::initializer_list<const char*> parts)
	{
		return std::all_of(parts.begin(), parts.end(),
			[&](const char* part) { return message.find(part) != std::string::npos; });
	}

	void check_shipped(World& world, const std::filesystem::path& content)
	{
		const GameTuning tuning((content / "tuning.json").string().c_str());
		const LevelObjectBuilder builder = world.builder(tuning);
		const PlayerTuning& player = tuning.player();
		// The farthest step Player::update can take: x_initial_velocity is
		// unclamped on the first step of input, drop_down_velocity is added
		// after the y clamp.
		const Vector2F travel = Vector2F(
			std::max(player.max_velocity.x, std::abs(player.x_initial_velocity)),
			player.max_velocity.y + std::abs(player.drop_down_velocity)) * SECONDS_PER_STEP;
		for (const auto& path : shipped_levels(content))
		{
			const LevelLoadedInfo info(path.string().c_str());
			info.validate();
			const JsonValue objects = info.collision_objects_json();
			const std::string failure = rejection(tuning, builder, objects);
			require(failure.empty(), "A shipped stage failed the tunnelling bound: " + failure);

			float thinnest_width = std::numeric_limits<float>::max();
			float thinnest_height = std::numeric_limits<float>::max();
			for (const auto& object : builder.build_collision_objects(objects, TeamColour{}))
			{
				const RectangleF box = object->shape()->bounding_box();
				thinnest_width = std::min(thinnest_width, box.width);
				thinnest_height = std::min(thinnest_height, box.height);
			}
			std::cout << path.filename().string() << ": " << objects.size()
				<< " collision objects; thinnest " << thinnest_width << " wide, "
				<< thinnest_height << " high; player travels " << travel.x << " x "
				<< travel.y << " per step against bounds of "
				<< max_safe_displacement(player.size.x, thinnest_width) << " x "
				<< max_safe_displacement(player.size.y, thinnest_height) << "\n";
		}
	}

	void check_rejections(World& world, const std::filesystem::path& content,
		Fixtures& fixtures)
	{
		const auto course = (content / "levels/movement_course.json").string();
		const LevelLoadedInfo info(course.c_str());
		const JsonValue objects = info.collision_objects_json();

		const GameTuning falling = fixtures.load([](rapidjson::Document& d)
			{ d["player"]["max_velocity"]["y"].SetFloat(1.0e5f); });
		const std::string vertical = rejection(falling, world.builder(falling), objects);
		require(mentions(vertical, { "movement_course.json", "collision_objects", "a player",
			"vertically", "height", "max_safe_displacement", "player.max_velocity.y" }),
			"A player fast enough to fall through a floor was not rejected with its object, "
			"axis and key: " + vertical);

		const GameTuning running = fixtures.load([](rapidjson::Document& d)
			{ d["player"]["max_velocity"]["x"].SetFloat(1.0e5f); });
		const std::string horizontal = rejection(running, world.builder(running), objects);
		require(mentions(horizontal, { "movement_course.json", "horizontally", "width",
			"player.max_velocity.x" }),
			"A player fast enough to run through a wall was not rejected: " + horizontal);

		// The two speeds Player::update sets past its clamp count too.
		const GameTuning starting = fixtures.load([](rapidjson::Document& d)
			{ d["player"]["x_initial_velocity"].SetFloat(1.0e5f); });
		const std::string first_step = rejection(starting, world.builder(starting), objects);
		require(mentions(first_step, { "horizontally", "player.x_initial_velocity" }),
			"A first step of input fast enough to cross a wall was not rejected: " + first_step);

		const GameTuning dropping = fixtures.load([](rapidjson::Document& d)
			{ d["player"]["drop_down_velocity"].SetFloat(1.0e5f); });
		const std::string drop = rejection(dropping, world.builder(dropping), objects);
		require(mentions(drop, { "vertically", "player.drop_down_velocity" }),
			"A drop-through fast enough to cross a floor was not rejected: " + drop);

		// A non-positive extent or cap would make the bound meaningless; the
		// tuning reader refuses one at load, before any stage is checked.
		for (const char* field : { "size", "max_velocity" })
		{
			const auto path = fixtures.write([field](rapidjson::Document& d)
				{ d["player"][field]["y"].SetFloat(0.0f); });
			bool refused = false;
			try { const GameTuning invalid(path.string().c_str()); }
			catch (const std::runtime_error& error)
			{
				refused = mentions(error.what(), { "player", field });
			}
			require(refused, std::string("A zero player.") + field + ".y loaded");
		}

		// The engine's boundary is unsafe: touching endpoints are no contact.
		// A half-second step makes the travel exact: 120 + 10 is the bound.
		// The drop-through speed is zeroed so max_velocity.y alone is the travel.
		const auto slab = fixtures.write_text("tunnelling-slab.json", R"({ "collision_objects": [
			{ "type": "Structure", "collision_type": "STRUCTURE",
			  "sheet_name": "sprite_sheet_1", "frame_name": "square_white_4", "colour": "808080",
			  "rectangle": { "x": 0, "y": 0, "width": 1000, "height": 10 } } ] })");
		const JsonDocument slab_document = read_json_file(slab.string().c_str());
		const JsonValue slab_objects = slab_document.root().array("collision_objects");
		require(fixtures.load([](rapidjson::Document&) {}).player().size.y == 120.0f,
			"The boundary fixture assumes the authored 120-unit player height");
		for (const float speed : { 259.0f, 260.0f })
		{
			const GameTuning edge = fixtures.load([speed](rapidjson::Document& d)
				{
					d["player"]["max_velocity"]["y"].SetFloat(speed);
					d["player"]["drop_down_velocity"].SetFloat(0.0f);
				});
			const std::string message = rejection(edge, world.builder(edge), slab_objects, 0.5f);
			require(speed == 260.0f ? mentions(message, { "vertically", "is 130," }) : message.empty(),
				"The stage check disagreed with labrador::can_tunnel at its boundary: " + message);
		}
		std::cout << "Rejected: falling, running, a first step and a drop-through past the "
			"course's geometry, a zero size or cap at load, and a step landing exactly on "
			"the engine's boundary.\n";
	}

	// The row this case closes said "the sniper's 2000 u/s already exceeds
	// it". By the engine's arithmetic a jet at the projectile cap is unsafe
	// against a thin slab; it is exempt because Projectile::resolve_sweep tests
	// its trajectory, and this is that trajectory.
	void check_swept_exemption(World& world, const std::filesystem::path& content)
	{
		const GameTuning tuning((content / "tuning.json").string().c_str());
		const ProjectileTuning& jet = tuning.projectile(ProjectileType::jet);
		const float speed = jet.max_velocity.x;
		const RectangleF slab(0.0f, 0.0f, 10.0f, 100.0f);
		require(is_swept(ProjectileType::jet) && !is_swept(ProjectileType::rolling),
			"The swept exemption no longer matches the kinds the sweep serves");
		require(can_tunnel(speed * SECONDS_PER_STEP, jet.details.col_rect_size.x, slab.width),
			"The demonstration needs a jet the endpoint test alone could lose");

		auto cover = std::make_unique<Structure>("sprite_sheet_1", "square_white_4", slab,
			std::make_unique<RectangleF>(slab), &world.render, CollisionObjectType::structure);
		CollisionObject* targets[]{ cover.get() };
		auto shots = ProjectileBuilder(&tuning).build_projectiles({ -40.0f, 50.0f },
			{ speed, 0.0f }, PlayerTeam::a, 0, Colour::blue, ProjectileType::jet, &world.render);
		Projectile& shot = *shots.at(0);
		shot.update(SECONDS_PER_STEP);
		require(shot.shape()->bounding_box().left() > slab.right(),
			"The jet's endpoint should already be past the slab, as the bound predicts");
		shot.resolve_movement_contacts(targets);
		const auto contact = shot.take_blocked_contact();
		require(shot.for_deletion() && contact.has_value() &&
			std::abs(contact->position.x) < 0.001f,
			"The swept jet passed through a slab the engine's bound calls unsafe");
		std::cout << "Swept: a jet at " << speed << " u/s travels " << speed * SECONDS_PER_STEP
			<< " per step past a 10-unit slab (bound " << max_safe_displacement(
				jet.details.col_rect_size.x, slab.width) << ") and still stops on its face.\n";
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: tunnelling.exe <game/content directory>");
		const std::filesystem::path content = argv[1];
		World world(content);
		Fixtures fixtures(content,
			std::filesystem::absolute(argv[0]).parent_path() / "tunnelling-fixtures");
		check_shipped(world, content);
		check_rejections(world, content, fixtures);
		check_swept_exemption(world, content);
		std::cout << "PASS tunnelling: shipped stages within the engine's bound, "
			"rejections at load and per stage, and the swept projectile exemption.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL tunnelling: " << error.what() << '\n';
		return 1;
	}
}
