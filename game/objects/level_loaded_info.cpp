#include "game/pch.h"
#include "game/objects/level_loaded_info.h"

using namespace mattmath;
using namespace artattack;

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	json_doc_(read_json_file(json_path))
{

}

void LevelLoadedInfo::validate() const
{
	// Every accessor, so every key is touched. JsonValue throws with the file
	// and the key on the first one that is absent or the wrong type, so this
	// needs no messages of its own for any of them.
	this->display_name();
	this->icon_frame();

	const std::vector<Vector2F> team_a = this->team_a_spawns();
	const std::vector<Vector2F> team_b = this->team_b_spawns();

	const RectangleF out_of_bounds = this->out_of_bounds_rectangle();
	this->camera_bounds_rectangle();
	this->zoom_out_start_bounds_rectangle();
	this->zoom_out_finish_bounds_rectangle();

	this->collision_objects_json();
	this->non_collision_objects_json();

	this->sound_bank_name();
	this->music_name();
	this->music_volume();

	// Then the two things no single key is wrong about, and which therefore
	// have to be said here.
	//
	// A roster is up to Gamepads::max_count per team and PlayerBuilder
	// subscripts the spawn array by position in it. A stage with three spawns
	// is a perfectly valid document that throws when the fourth player is
	// built, which is again inside GameLevel::init().
	const JsonValue root = this->json_doc_.root();

	if (team_a.size() < artattack::Gamepads::max_count ||
		team_b.size() < artattack::Gamepads::max_count)
	{
		throw std::runtime_error(root.where() +
			" lists " + std::to_string(team_a.size()) + " spawn(s) for team A "
			"and " + std::to_string(team_b.size()) + " for team B. Every team "
			"needs at least " + std::to_string(artattack::Gamepads::max_count) +
			", because a match can be that many players a side.");
	}

	// A spawn outside the arena is not a content mistake the loader can see:
	// the player is built there, falls out of the level on the first tick, and
	// Level::update_level_logic throws "Player out of bounds" mid-match.
	for (const Vector2F& spawn : team_a)
	{
		validate_spawn(root, spawn, out_of_bounds, "team_a");
	}
	for (const Vector2F& spawn : team_b)
	{
		validate_spawn(root, spawn, out_of_bounds, "team_b");
	}
}

void LevelLoadedInfo::validate_spawn(const JsonValue& root,
	const Vector2F& spawn,
	const RectangleF& out_of_bounds,
	const char* team)
{
	if (out_of_bounds.contains(spawn))
	{
		return;
	}

	// Whole numbers, because every coordinate in a level file is one and
	// to_string on a float spends six decimal places saying so.
	throw std::runtime_error(root.where() + " puts a " + team + " spawn at (" +
		std::to_string(static_cast<int>(spawn.x)) + ", " +
		std::to_string(static_cast<int>(spawn.y)) + "), outside its own "
		"out_of_bounds rectangle - a player built there is retired on the "
		"first tick of the match.");
}

std::string LevelLoadedInfo::display_name() const
{
	return this->json_doc_.root().string("display_name");
}
std::string LevelLoadedInfo::icon_frame() const
{
	return this->json_doc_.root().string("icon_frame");
}
std::vector<Vector2F> LevelLoadedInfo::team_a_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object("spawn_locations").array("team_a"));
}
std::vector<Vector2F> LevelLoadedInfo::team_b_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object("spawn_locations").array("team_b"));
}

RectangleF LevelLoadedInfo::bounds_rectangle(const char* key) const
{
	const JsonValue bounds =
		this->json_doc_.root().object("level_bounds").object(key);

	return RectangleF(
		bounds.number("x"),
		bounds.number("y"),
		bounds.number("width"),
		bounds.number("height"));
}

RectangleF LevelLoadedInfo::out_of_bounds_rectangle() const
{
	return this->bounds_rectangle("out_of_bounds");
}
RectangleF LevelLoadedInfo::camera_bounds_rectangle() const
{
	return this->bounds_rectangle("camera_bounds");
}
RectangleF LevelLoadedInfo::zoom_out_start_bounds_rectangle() const
{
	return this->bounds_rectangle("zoom_out_start_bounds");
}
RectangleF LevelLoadedInfo::zoom_out_finish_bounds_rectangle() const
{
	return this->bounds_rectangle("zoom_out_finish_bounds");
}

JsonValue LevelLoadedInfo::collision_objects_json() const
{
	return this->json_doc_.root().array("collision_objects");
}
JsonValue LevelLoadedInfo::non_collision_objects_json() const
{
	return this->json_doc_.root().array("non_collision_objects");
}
std::string LevelLoadedInfo::sound_bank_name() const
{
	return this->json_doc_.root().object("sound").string("sound_bank_name");
}
std::string LevelLoadedInfo::music_name() const
{
	return this->json_doc_.root().object("sound").string("music_name");
}
float LevelLoadedInfo::music_volume() const
{
	return this->json_doc_.root().object("sound").number("music_volume");
}

std::vector<Vector2F>
	LevelLoadedInfo::decode_team_spawns(const JsonValue& json)
{
	std::vector<Vector2F> spawns;
	spawns.reserve(json.size());
	for (size_t index = 0; index < json.size(); ++index)
	{
		const JsonValue spawn = json.at(index);
		spawns.push_back(Vector2F(spawn.number("x"), spawn.number("y")));
	}
	return spawns;
}
