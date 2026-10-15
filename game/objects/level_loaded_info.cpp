#include "game/pch.h"
#include "game/objects/level_loaded_info.h"
#include "game/level_format.h"
#include "engine/math/intersects.h"
#include "game/objects/level_object_builder.h"
#include <cmath>

using namespace mattmath;
using namespace labrador;

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	json_doc_(read_json_file(json_path))
{
	// An untagged legacy level has exactly the format-1 meaning. Reject a
	// future or malformed tag before interpreting any of its content fields.
	const JsonValue root = this->json_doc_.root();
	if (root.has("format") && root.integer("format") != level_format::current)
	{
		throw std::runtime_error(root.where() + ": unsupported 'format'; expected " +
			std::to_string(level_format::current) + " (or an untagged legacy level).");
	}
}

void LevelLoadedInfo::validate() const
{
	// Validate all exposed keys at startup. JsonValue errors identify the file and key.
	this->display_name();
	this->icon_frame();
	this->include_in_random();

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
	this->end_sting_name();
	this->end_sting_volume();

	// Require enough spawn points for a full roster on either team.
	const JsonValue root = this->json_doc_.root();

	if (team_a.size() < labrador::Gamepads::max_count ||
		team_b.size() < labrador::Gamepads::max_count)
	{
		throw std::runtime_error(root.where() +
			" lists " + std::to_string(team_a.size()) + " spawn(s) for team A "
			"and " + std::to_string(team_b.size()) + " for team B. Every team "
			"needs at least " + std::to_string(labrador::Gamepads::max_count) +
			", because a match can be that many players a side.");
	}

	// Reject spawns outside the arena before a match can start.
	for (const Vector2F& spawn : team_a)
	{
		validate_position(root, spawn, out_of_bounds, "a team_a spawn");
	}
	for (const Vector2F& spawn : team_b)
	{
		validate_position(root, spawn, out_of_bounds, "a team_b spawn");
	}

	// The opening previews team A's colour over the left half of the picture
	// and team B's over the right, so each team must start on its own half.
	const float middle = out_of_bounds.x + out_of_bounds.width / 2.0f;
	for (const Vector2F& spawn : team_a)
	{
		if (spawn.x >= middle)
		{
			throw std::runtime_error(root.where() + ": a team_a spawn is on the right "
				"half of the stage; team A starts on the left, team B on the right.");
		}
	}
	for (const Vector2F& spawn : team_b)
	{
		if (spawn.x < middle)
		{
			throw std::runtime_error(root.where() + ": a team_b spawn is on the left "
				"half of the stage; team A starts on the left, team B on the right.");
		}
	}

	this->validate_stations(out_of_bounds);
}

void LevelLoadedInfo::validate_position(const JsonValue& root,
	const Vector2F& position,
	const RectangleF& out_of_bounds,
	const std::string& what)
{
	if (rectangle_point_intersect(out_of_bounds, position))
	{
		return;
	}

	throw std::runtime_error(root.where() + " puts " + what + " at (" +
		std::to_string(position.x) + ", " +
		std::to_string(position.y) + "), outside its own "
		"out_of_bounds rectangle - a player built there is retired on the "
		"first tick of the match.");
}

void LevelLoadedInfo::validate_stations(const RectangleF& out_of_bounds) const
{
	const JsonValue root = this->json_doc_.root();
	if (!root.has("stations"))
	{
		return;
	}
	const JsonValue stations = root.array("stations");
	std::vector<std::string> seen;
	for (size_t index = 0; index < stations.size(); ++index)
	{
		const JsonValue station = stations.at(index);
		const std::string name = station.identifier("name");
		const bool spelled = !name.empty() && std::all_of(name.begin(), name.end(),
			[](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; });
		if (!spelled)
		{
			throw std::runtime_error(station.where() + " names a station '" + name +
				"'; station names are lower-case letters, digits and '_', so they can be typed.");
		}
		if (name == "spawn_a" || name == "spawn_b")
		{
			throw std::runtime_error(station.where() + " names a station '" + name +
				"', which every level already has (its team's first spawn).");
		}
		if (std::find(seen.begin(), seen.end(), name) != seen.end())
		{
			throw std::runtime_error(station.where() + " repeats the station name '" +
				name + "'.");
		}
		seen.push_back(name);
		validate_position(root, Vector2F(station.number("x"), station.number("y")),
			out_of_bounds, "a station '" + name + "'");
	}
}

std::vector<LevelStation> LevelLoadedInfo::stations() const
{
	std::vector<LevelStation> result;
	const JsonValue root = this->json_doc_.root();
	if (root.has("stations"))
	{
		const JsonValue stations = root.array("stations");
		for (size_t index = 0; index < stations.size(); ++index)
		{
			const JsonValue station = stations.at(index);
			result.push_back({ station.identifier("name"),
				Vector2F(station.number("x"), station.number("y")) });
		}
	}
	result.push_back({ "spawn_a", this->team_a_spawns().at(0) });
	result.push_back({ "spawn_b", this->team_b_spawns().at(0) });
	return result;
}

void LevelLoadedInfo::validate_content(const LevelObjectBuilder& builder,
	const RenderResources& render_resources,
	const AudioResources& audio_resources) const
{
	const JsonValue root = this->json_doc_.root();
	try
	{
		// Stage icons use the same sheet as the stage-select page.
		render_resources.sprite_sheet("sprite_sheet_1")->
			resolve_sprite_frame(this->icon_frame());
	}
	catch (const std::exception& error)
	{
		throw std::runtime_error(root.where() + ": icon_frame: " + error.what());
	}
	try
	{
		const SoundBank* bank = audio_resources.sound_bank(this->sound_bank_name());
		if (this->has_music())
		{
			bank->resolve_effect(this->music_name());
		}
		if (this->has_end_sting())
		{
			bank->resolve_effect(this->end_sting_name());
		}
	}
	catch (const std::exception& error)
	{
		throw std::runtime_error(root.object("sound").where() + ": " + error.what());
	}
	// Uses the builder itself, so the accepted vocabulary cannot drift from
	// the path that starts a match. Objects resolve their assets on construction.
	builder.validate_objects(this->collision_objects_json(),
		this->non_collision_objects_json());
}

std::string LevelLoadedInfo::display_name() const
{
	return this->json_doc_.root().string("display_name");
}
std::string LevelLoadedInfo::icon_frame() const
{
	return this->json_doc_.root().string("icon_frame");
}
bool LevelLoadedInfo::include_in_random() const
{
	const JsonValue root = this->json_doc_.root();
	return !root.has("include_in_random") || root.boolean("include_in_random");
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

	const RectangleF rectangle(
		bounds.number("x"),
		bounds.number("y"),
		bounds.number("width"),
		bounds.number("height"));
	if (!std::isfinite(rectangle.x) || !std::isfinite(rectangle.y) ||
		!std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
		rectangle.width <= 0.0f || rectangle.height <= 0.0f)
	{
		throw std::runtime_error(bounds.where() +
			" needs finite coordinates and positive width and height");
	}
	return rectangle;
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
bool LevelLoadedInfo::has_music() const
{
	const JsonValue sound = this->json_doc_.root().object("sound");
	const bool named = sound.has("music_name");
	if (named != sound.has("music_volume"))
	{
		throw std::runtime_error(sound.where() +
			": music_name and music_volume come together, or neither for a silent stage.");
	}
	return named;
}
std::string LevelLoadedInfo::music_name() const
{
	return this->has_music() ?
		this->json_doc_.root().object("sound").string("music_name") : std::string();
}
float LevelLoadedInfo::music_volume() const
{
	return this->has_music() ?
		this->json_doc_.root().object("sound").number("music_volume") : 0.0f;
}

bool LevelLoadedInfo::has_end_sting() const
{
	const JsonValue sound = this->json_doc_.root().object("sound");
	const bool named = sound.has("end_sting_name");
	if (named != sound.has("end_sting_volume"))
	{
		throw std::runtime_error(sound.where() +
			": end_sting_name and end_sting_volume come together, or neither.");
	}
	return named;
}

std::string LevelLoadedInfo::end_sting_name() const
{
	return this->has_end_sting() ?
		this->json_doc_.root().object("sound").identifier("end_sting_name") : std::string();
}

float LevelLoadedInfo::end_sting_volume() const
{
	if (!this->has_end_sting()) { return 0.0f; }
	const JsonValue sound = this->json_doc_.root().object("sound");
	const float volume = sound.number("end_sting_volume");
	if (!std::isfinite(volume) || volume < 0.0f || volume > 1.0f)
	{
		throw std::runtime_error(sound.where() + ": end_sting_volume must be 0..1.");
	}
	return volume;
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
