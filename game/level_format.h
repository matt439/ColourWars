#pragma once

// The level file format's one owner: its version, its key names, its object
// and collision vocabulary and the material table. Engine-free, so the game's
// loader (LevelLoadedInfo, LevelObjectBuilder, TerrainMaterial) and the level
// editor (editor/level_json.cpp and the files beside it) read the same words
// and refuse the same records; each caller keeps its own messages and adds its
// own location. The structural schema is level.schema.json beside this file,
// and docs/level-format.md is the prose.
//
// The rules for the format's larger records live in their own contracts, which
// are part of this format: station_contract.h (stations and lessons),
// team_gate_contract.h (gates), horizontal_motion_contract.h (a mover's drive)
// and fixed_slab_paint_geometry.h with paint_polygon_geometry.h (slab shapes).
#include <array>
#include <cstddef>
#include <optional>
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

	inline constexpr std::array<Material, 6> materials = { {
		{ "ground", { Collision::structure, Collision::structure_paintable } },
		{ "pencil", { Collision::structure_paintable, std::nullopt } },
		{ "eraser", { Collision::structure_paintable, std::nullopt } },
		{ "paint_tin", { Collision::structure_paintable, std::nullopt } },
		{ "ruler", { Collision::structure_jump_through, std::nullopt } },
		{ "set_square", { Collision::structure_ramp_left, Collision::structure_ramp_right } } } };

	inline constexpr const char* fixed_slab_material = "eraser";

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
}
