#pragma once

// The level file format's one owner: its version, its key names, its object
// and collision vocabulary and the material table. Engine-free, so the game's
// loader (LevelLoadedInfo, LevelObjectBuilder, TerrainMaterial) and the level
// editor (editor/level_json.cpp and the files beside it) read the same words
// and refuse the same records; each caller keeps its own messages and adds its
// own location. The structural schema is level.schema.json beside this file,
// and docs/level-format.md is the prose.
//
// It also owns the format's size limits (namespace limit): how far a
// coordinate may lie from the origin, how large a rectangle, an object array,
// a stage's paint and a level file may be, and how many files the user stage
// folder is read for.
//
// The rules for the format's larger records live in their own contracts, which
// are part of this format: station_contract.h (stations and lessons),
// team_gate_contract.h (gates), horizontal_motion_contract.h (a mover's drive)
// and fixed_slab_paint_geometry.h with paint_polygon_geometry.h (slab shapes).
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace level_format
{
	// Missing version tags are the original spelling of this same format. Bump
	// only when a level's meaning changes, with an explicit compatibility policy.
	inline constexpr int current = 1;

	// Key names. Coordinate fields (x, y, width, height, left, top, right,
	// bottom, x1..y3) and a record's own small fields are written where they
	// are read; the schema lists every one.
	namespace key
	{
		inline constexpr const char* format = "format";
		inline constexpr const char* display_name = "display_name";
		inline constexpr const char* icon_frame = "icon_frame";
		inline constexpr const char* include_in_random = "include_in_random";

		inline constexpr const char* spawn_locations = "spawn_locations";
		inline constexpr const char* team_a = "team_a";
		inline constexpr const char* team_b = "team_b";

		inline constexpr const char* level_bounds = "level_bounds";
		inline constexpr const char* out_of_bounds = "out_of_bounds";
		inline constexpr const char* camera_bounds = "camera_bounds";
		inline constexpr const char* zoom_out_start_bounds = "zoom_out_start_bounds";
		inline constexpr const char* zoom_out_finish_bounds = "zoom_out_finish_bounds";

		inline constexpr const char* collision_objects = "collision_objects";
		inline constexpr const char* non_collision_objects = "non_collision_objects";

		inline constexpr const char* sound = "sound";
		inline constexpr const char* sound_bank_name = "sound_bank_name";
		inline constexpr const char* music_name = "music_name";
		inline constexpr const char* music_volume = "music_volume";
		inline constexpr const char* end_sting_name = "end_sting_name";
		inline constexpr const char* end_sting_volume = "end_sting_volume";

		inline constexpr const char* stations = "stations";
		inline constexpr const char* lesson = "lesson";

		// An object record.
		inline constexpr const char* name = "name";
		inline constexpr const char* type = "type";
		inline constexpr const char* collision_type = "collision_type";
		inline constexpr const char* material = "material";
		inline constexpr const char* sheet_name = "sheet_name";
		inline constexpr const char* frame_name = "frame_name";
		inline constexpr const char* colour = "colour";
		inline constexpr const char* rectangle = "rectangle";
		inline constexpr const char* rectangle_tlbr = "rectangle_tlbr";
		inline constexpr const char* triangle = "triangle";
		inline constexpr const char* paintable_faces = "paintable_faces";
		inline constexpr const char* motion = "motion";
		inline constexpr const char* drift = "drift";
		inline constexpr const char* gate = "gate";
		inline constexpr const char* slab = "slab";
	}

	// The four keys under "paintable_faces", each true or false.
	namespace face
	{
		inline constexpr const char* left = "left";
		inline constexpr const char* top = "top";
		inline constexpr const char* right = "right";
		inline constexpr const char* bottom = "bottom";
	}

	inline constexpr std::array<const char*, 4> paintable_face_names = {
		face::left, face::top, face::right, face::bottom };

	// Scenery is grey: nothing in the picture has been coloured yet, so team
	// hues never appear in it. A "colour" is RRGGBB with three equal pairs or
	// one of these names, the engine's colour names (labrador::colour_from_name)
	// whose channels are equal. The game resolves the name and checks the
	// colour; the editor, which has no engine, reads this list, and the
	// level_format acceptance case holds it to the engine's table.
	inline constexpr std::array<const char*, 13> scenery_grey_names = {
		"BLACK", "DARK_GRAY", "DARK_GREY", "DIM_GRAY", "DIM_GREY", "GAINSBORO", "GRAY",
		"GREY", "LIGHT_GRAY", "LIGHT_GREY", "SILVER", "WHITE", "WHITE_SMOKE" };

	inline bool is_scenery_grey(std::string_view colour)
	{
		if (colour.size() == 6 && colour.find_first_not_of("0123456789abcdefABCDEF") == std::string_view::npos)
		{
			const auto digit = [](char c) { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; };
			const auto pair = [&](std::size_t at) { return digit(colour[at]) * 16 + digit(colour[at + 1]); };
			return pair(0) == pair(2) && pair(2) == pair(4);
		}
		for (const char* name : scenery_grey_names)
		{
			if (colour == name) { return true; }
		}
		return false;
	}

	// The four rectangles under level_bounds, in the order the editor lists them.
	inline constexpr std::array<const char*, 4> bounds_keys = {
		key::out_of_bounds, key::camera_bounds, key::zoom_out_start_bounds, key::zoom_out_finish_bounds };

	// Every team lists at least this many spawns: a match can be that many
	// players a side. The game checks it against its pad count.
	inline constexpr std::size_t min_team_spawns = 4;

	// The opening previews team A's colour over the left half of the picture
	// and team B's over the right, so each team starts on its own half. Each
	// caller passes its own number type, so its arithmetic is unchanged.
	template <typename Number>
	bool on_own_half(bool team_a, Number x, Number arena_x, Number arena_width)
	{
		const Number middle = arena_x + arena_width / Number(2);
		return team_a ? x < middle : !(x < middle);
	}

	// A record's "type".
	namespace object_type
	{
		inline constexpr const char* structure = "Structure";
		inline constexpr const char* structure_paintable = "StructurePaintable";
		inline constexpr const char* structure_ramp = "StructureRamp";
		inline constexpr const char* fixed_slab_paintable = "FixedSlabPaintable";
		// The one decoration type, in non_collision_objects.
		inline constexpr const char* visual = "Visual";
	}

	// A record's "collision_type".
	enum class Collision
	{
		structure,
		structure_jump_through,
		structure_paintable,
		structure_fixed_slab,
		structure_ramp_right,
		structure_ramp_left,
	};

	struct CollisionName
	{
		Collision collision;
		const char* name;
	};

	inline constexpr std::array<CollisionName, 6> collision_names = { {
		{ Collision::structure, "STRUCTURE" },
		{ Collision::structure_jump_through, "STRUCTURE_JUMP_THROUGH" },
		{ Collision::structure_paintable, "STRUCTURE_PAINTABLE" },
		{ Collision::structure_fixed_slab, "STRUCTURE_FIXED_SLAB" },
		{ Collision::structure_ramp_right, "STRUCTURE_RAMP_RIGHT" },
		{ Collision::structure_ramp_left, "STRUCTURE_RAMP_LEFT" } } };

	inline std::optional<Collision> collision_named(std::string_view name)
	{
		for (const CollisionName& entry : collision_names)
		{
			if (name == entry.name) { return entry.collision; }
		}
		return std::nullopt;
	}

	inline const char* collision_name(Collision collision)
	{
		for (const CollisionName& entry : collision_names)
		{
			if (entry.collision == collision) { return entry.name; }
		}
		return "";
	}

	// The collision tags each collision object type may carry. Visual carries
	// none; an unknown type carries none.
	inline bool may_carry(std::string_view type, Collision collision)
	{
		if (type == object_type::structure)
		{
			return collision == Collision::structure || collision == Collision::structure_jump_through;
		}
		if (type == object_type::structure_paintable)
		{
			return collision == Collision::structure_paintable || collision == Collision::structure_jump_through;
		}
		if (type == object_type::structure_ramp)
		{
			return collision == Collision::structure_ramp_right || collision == Collision::structure_ramp_left;
		}
		if (type == object_type::fixed_slab_paintable)
		{
			return collision == Collision::structure_fixed_slab;
		}
		return false;
	}

	// The optional "material": which terrain-atlas look a record draws as, and
	// the only collision tags it may sit on. A jump-through is a ruler, a ramp
	// a set square. A fixed slab is always eraser, by its own rule below.
	struct Material
	{
		const char* name;
		std::array<std::optional<Collision>, 2> sits_on;
	};

	namespace material_name
	{
		inline constexpr const char* ground = "ground";
		inline constexpr const char* pencil = "pencil";
		inline constexpr const char* eraser = "eraser";
		inline constexpr const char* paint_tin = "paint_tin";
		inline constexpr const char* ruler = "ruler";
		inline constexpr const char* set_square = "set_square";
	}

	inline constexpr std::array<Material, 6> materials = { {
		{ material_name::ground, { Collision::structure, Collision::structure_paintable } },
		{ material_name::pencil, { Collision::structure_paintable, std::nullopt } },
		{ material_name::eraser, { Collision::structure_paintable, std::nullopt } },
		{ material_name::paint_tin, { Collision::structure_paintable, std::nullopt } },
		{ material_name::ruler, { Collision::structure_jump_through, std::nullopt } },
		{ material_name::set_square, { Collision::structure_ramp_left, Collision::structure_ramp_right } } } };

	inline constexpr const char* fixed_slab_material = material_name::eraser;

	inline const Material* material_named(std::string_view name)
	{
		for (const Material& material : materials)
		{
			if (name == material.name) { return &material; }
		}
		return nullptr;
	}

	inline bool sits_on(const Material& material, Collision collision)
	{
		return material.sits_on[0] == collision || material.sits_on[1] == collision;
	}

	// The materials a collision tag may carry, comma-separated in table order,
	// empty for none or for a name that is not a collision tag.
	inline std::string allowed_materials(std::string_view collision_type)
	{
		std::string result;
		const std::optional<Collision> collision = collision_named(collision_type);
		if (!collision) { return result; }
		for (const Material& material : materials)
		{
			if (!sits_on(material, *collision)) { continue; }
			if (!result.empty()) { result += ", "; }
			result += material.name;
		}
		return result;
	}

	// Fields a fixed slab record refuses, and fields its "slab" object refuses,
	// in the order they are checked.
	inline constexpr std::array<const char*, 6> fixed_slab_refused_keys = {
		key::motion, key::drift, "rotation", key::rectangle, key::rectangle_tlbr, key::triangle };
	inline constexpr std::array<const char*, 6> fixed_slab_refused_slab_keys = {
		"width", "height", "thickness", "angle", "rotation", "motion" };

	// Upper limits. The largest shipped stage is 35,076 bytes with bounds 6200
	// wide, an object 7000 tall, an edge 7388 from the origin, 41 collision and
	// 24 decorative objects and 26,560 units of paintable face; every cap sits
	// an order of magnitude or more above that, so only a mistake meets one
	// (docs/level-format.md, "Limits"). The check functions throw
	// std::invalid_argument carrying the rule's words; each caller adds its own
	// location. Comparisons are negated so a NaN fails.
	namespace limit
	{
		// Every edge of a bounds rectangle, an object rectangle or a ramp's
		// bounding box lies within +/- this many world units. At 2^20 a float
		// still resolves an eighth of a unit; far beyond it the game's 32-unit
		// steps stop advancing.
		inline constexpr double max_coordinate = 1048576.0;
		// Width and height of a level_bounds rectangle and of an object. The
		// schema has one rectangle definition, so the two are the same number.
		inline constexpr double max_bounds_extent = 1048576.0;
		inline constexpr double max_object_extent = 1048576.0;
		// Entries in collision_objects, and in non_collision_objects.
		inline constexpr std::size_t max_objects = 512;
		// The stage's StructurePaintable faces as authored: width for each
		// enabled top and bottom face, height for each enabled left and right.
		// Paint is built as tiles along them, so this bounds the tile count.
		inline constexpr double max_paint_length = 524288.0;
		// Tiles on one face strip, whatever tile size tuning.json gives. The
		// game checks it before the float-to-int conversion that counts them.
		inline constexpr int max_face_tiles = 1048576;
		// A level file, shipped or user.
		inline constexpr std::size_t max_file_bytes = 1048576;
		// .json files read from the user stage folder, first by file name.
		inline constexpr std::size_t max_user_stage_files = 256;

		inline std::string whole(double value)
		{
			return std::to_string(static_cast<long long>(value));
		}

		inline void check_edges(double x, double y, double width, double height)
		{
			for (const double edge : { x, y, x + width, y + height })
			{
				if (!(edge >= -max_coordinate && edge <= max_coordinate))
				{
					throw std::invalid_argument("has an edge outside the level format's coordinate limit of +/-" +
						whole(max_coordinate) + " world units");
				}
			}
		}

		inline void check_extent(double width, double height, double most)
		{
			if (!(width <= most && height <= most))
			{
				throw std::invalid_argument("is wider or taller than the level format's limit of " +
					whole(most) + " world units");
			}
		}

		// One of the four level_bounds rectangles.
		inline void check_bounds_rectangle(double x, double y, double width, double height)
		{
			check_edges(x, y, width, height);
			check_extent(width, height, max_bounds_extent);
		}

		// An object's rectangle, in either encoding, or a ramp's bounding box.
		inline void check_object_rectangle(double x, double y, double width, double height)
		{
			check_edges(x, y, width, height);
			check_extent(width, height, max_object_extent);
		}

		inline void check_object_count(std::size_t count)
		{
			if (count > max_objects)
			{
				throw std::invalid_argument("holds " + std::to_string(count) +
					" objects; the level format's limit is " + std::to_string(max_objects) + " in one array");
			}
		}

		// What one StructurePaintable record adds to the stage's paint length.
		inline double paint_length(double width, double height, bool left, bool top, bool right, bool bottom)
		{
			return width * ((top ? 1.0 : 0.0) + (bottom ? 1.0 : 0.0)) +
				height * ((left ? 1.0 : 0.0) + (right ? 1.0 : 0.0));
		}

		// The running total over the stage's StructurePaintable records.
		inline void check_paint_length(double total)
		{
			if (!(total <= max_paint_length))
			{
				throw std::invalid_argument("brings the stage's paintable faces past the level format's limit of " +
					whole(max_paint_length) + " world units of paint");
			}
		}

		inline void check_file_bytes(std::size_t bytes)
		{
			if (bytes > max_file_bytes)
			{
				throw std::invalid_argument("is larger than the level format's limit of " +
					std::to_string(max_file_bytes) + " bytes for one level file");
			}
		}
	}
}
