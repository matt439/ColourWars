#include "game/pch.h"
#include "game/objects/level_loaded_info.h"
#include "game/content_value.h"
#include "game/level_format.h"
#include "engine/math/intersects.h"
#include "game/objects/level_object_builder.h"
#include "game/drawable_text.h"
#include "game/station_contract.h"
#include "engine/core/file_path.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <string_view>

using namespace mattmath;
using namespace labrador;
namespace key = level_format::key;

namespace
{
	std::string read_stage_snapshot(const char* path)
	{
		if (path == nullptr) { throw std::invalid_argument("LevelLoadedInfo - null path"); }
		std::ifstream file(path_from_utf8(path), std::ios::binary);
		if (!file) { throw std::runtime_error(std::string("LevelLoadedInfo - cannot open '") + path + "'"); }
		std::string text;
		char chunk[64 * 1024];
		while (file.read(chunk, sizeof(chunk)) || file.gcount() > 0)
		{ text.append(chunk, static_cast<std::size_t>(file.gcount())); }
		if (file.bad()) { throw std::runtime_error(std::string("LevelLoadedInfo - read error in '") + path + "'"); }
		return text;
	}

	class StageSemanticHash
	{
	public:
		std::uint64_t of(const rapidjson::Value& value)
		{
			this->text("colourwars-loaded-stage-1");
			this->append(value);
			return this->hash_;
		}

	private:
		std::uint64_t hash_ = 0xcbf29ce484222325ull;

		void byte(unsigned char value)
		{
			this->hash_ ^= value;
			this->hash_ *= 0x100000001b3ull;
		}

		void integer(std::uint64_t value)
		{
			for (int index = 0; index < 8; ++index)
			{
				this->byte(static_cast<unsigned char>(value & 0xff));
				value >>= 8;
			}
		}

		void text(std::string_view value)
		{
			this->integer(value.size());
			for (const char character : value) { this->byte(static_cast<unsigned char>(character)); }
		}

		void magnitude(std::int64_t value)
		{
			this->byte('-');
			// Unsigned subtraction also represents INT64_MIN without overflow.
			this->integer(std::uint64_t{0} - static_cast<std::uint64_t>(value));
		}

		void number(const rapidjson::Value& value)
		{
			if (value.IsInt64() && value.GetInt64() < 0)
			{ this->magnitude(value.GetInt64()); return; }
			if (value.IsUint64())
			{ this->byte('+'); this->integer(value.GetUint64()); return; }

			const double numeric = value.GetDouble();
			// Equal numeric spellings share a representation, without rounding
			// exact JSON integers above 2^53 through double. Floating -0 retains
			// its sign, as the loaded number does; an integer -0 parses as +0.
			if (!(numeric == 0.0 && std::signbit(numeric)) && std::trunc(numeric) == numeric)
			{
				if (numeric >= 0.0 && numeric < 18446744073709551616.0)
				{ this->byte('+'); this->integer(static_cast<std::uint64_t>(numeric)); return; }
				if (numeric < 0.0 && numeric >= static_cast<double>((std::numeric_limits<std::int64_t>::min)()))
				{ this->magnitude(static_cast<std::int64_t>(numeric)); return; }
			}
			this->byte('d');
			this->integer(std::bit_cast<std::uint64_t>(numeric));
		}

		void append(const rapidjson::Value& value)
		{
			switch (value.GetType())
			{
			case rapidjson::kNullType: this->byte('n'); break;
			case rapidjson::kFalseType: this->byte('f'); break;
			case rapidjson::kTrueType: this->byte('t'); break;
			case rapidjson::kNumberType: this->number(value); break;
			case rapidjson::kStringType:
				this->byte('s'); this->text({ value.GetString(), value.GetStringLength() }); break;
			case rapidjson::kArrayType:
				this->byte('['); this->integer(value.Size());
				for (const auto& item : value.GetArray()) { this->append(item); }
				break;
			case rapidjson::kObjectType:
				this->byte('{'); this->integer(value.MemberCount());
				// Every authored field is retained, including currently unknown
				// metadata. Sorting or selecting known fields could hide a change.
				for (const auto& member : value.GetObject())
				{
					this->text({ member.name.GetString(), member.name.GetStringLength() });
					this->append(member.value);
				}
				break;
			}
		}
	};

	std::uint64_t hash_stage_snapshot(std::string_view snapshot, const char* source_path)
	{
		rapidjson::Document parsed;
		parsed.Parse<rapidjson::kParseDefaultFlags | rapidjson::kParseValidateEncodingFlag>(snapshot.data(), snapshot.size());
		if (parsed.HasParseError())
		{
			throw std::runtime_error(std::string("LevelLoadedInfo - '") + source_path +
				"' is not valid JSON: " + rapidjson::GetParseError_En(parsed.GetParseError()) +
				" (offset " + std::to_string(parsed.GetErrorOffset()) + ")");
		}
		// An explicit-length parse can still stop at raw NUL; inspect the entire snapshot.
		const std::size_t nul = snapshot.find('\0');
		if (nul != std::string_view::npos)
		{
			throw std::runtime_error(std::string("LevelLoadedInfo - '") + source_path +
				"' is not valid JSON: Unescaped NUL byte. (offset " + std::to_string(nul) + ")");
		}
		return StageSemanticHash{}.of(parsed);
	}
}

LevelLoadedInfo::LevelLoadedInfo(const char* json_path) :
	LevelLoadedInfo(json_path, read_stage_snapshot(json_path))
{
}

LevelLoadedInfo::LevelLoadedInfo(const char* json_path, std::string snapshot) :
	json_doc_(parse_json(snapshot, json_path))
{
	// An untagged legacy level has exactly the format-1 meaning. Reject a
	// future or malformed tag before interpreting any of its content fields.
	const JsonValue root = this->json_doc_.root();
	if (root.has(key::format) && root.integer(key::format) != level_format::current)
	{
		throw std::runtime_error(root.where() + ": unsupported 'format'; expected " +
			std::to_string(level_format::current) + " (or an untagged legacy level).");
	}
	this->semantic_hash_ = hash_stage_snapshot(snapshot, json_path);
}

const std::string& LevelLoadedInfo::source_path() const
{
	return this->json_doc_.source_path();
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

	static_assert(level_format::min_team_spawns == labrador::Gamepads::max_count,
		"the level format's spawn minimum is a full roster a side");
	if (team_a.size() < level_format::min_team_spawns ||
		team_b.size() < level_format::min_team_spawns)
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
	for (const Vector2F& spawn : team_a)
	{
		if (!level_format::on_own_half(true, spawn.x, out_of_bounds.x, out_of_bounds.width))
		{
			throw std::runtime_error(root.where() + ": a team_a spawn is on the right "
				"half of the stage; team A starts on the left, team B on the right.");
		}
	}
	for (const Vector2F& spawn : team_b)
	{
		if (!level_format::on_own_half(false, spawn.x, out_of_bounds.x, out_of_bounds.width))
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
	if (!root.has(key::stations))
	{
		return;
	}
	const JsonValue stations = root.array(key::stations);
	const station_contract::Rectangle arena{ out_of_bounds.x, out_of_bounds.y,
		out_of_bounds.width, out_of_bounds.height };
	std::vector<std::string> seen;
	for (size_t index = 0; index < stations.size(); ++index)
	{
		const JsonValue station = stations.at(index);
		const std::string name = station.identifier(key::name);
		try { station_contract::validate_name(name, seen); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(station.where() + " " + error.what());
		}
		const Vector2F position(station.number("x"), station.number("y"));
		try { station_contract::validate_position(name, { position.x, position.y }, arena); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(root.where() + " " + error.what());
		}
		if (station.has(key::lesson))
		{
			validate_lesson(station.object(key::lesson), position, name);
		}
	}
}

void LevelLoadedInfo::validate_lesson(const JsonValue& lesson,
	const Vector2F& station, const std::string& name) const
{
	const RectangleF area = lesson_area(lesson);
	const RectangleF camera = this->camera_bounds_rectangle();
	try
	{
		station_contract::validate_lesson_area(name, { station.x, station.y },
			{ area.x, area.y, area.width, area.height },
			{ camera.x, camera.y, camera.width, camera.height });
	}
	catch (const std::invalid_argument& error)
	{
		throw std::runtime_error(lesson.object("area").where() + " " + error.what());
	}
	for (const char* field : { "title", "text" })
	{
		const std::string words = lesson.string(field);
		const std::string where = lesson.where() + "." + field;
		try { station_contract::validate_lesson_words(words); }
		catch (const std::invalid_argument& error)
		{
			throw std::runtime_error(where + " " + error.what());
		}
		std::ignore = drawable_text::widen_drawable(words, where);
	}
}

RectangleF LevelLoadedInfo::lesson_area(const JsonValue& lesson)
{
	const JsonValue area = lesson.object("area");
	return RectangleF(area.number("x"), area.number("y"),
		area.number("width"), area.number("height"));
}

std::vector<LevelLesson> LevelLoadedInfo::lessons() const
{
	std::vector<LevelLesson> result;
	const JsonValue root = this->json_doc_.root();
	if (!root.has(key::stations)) { return result; }
	const JsonValue stations = root.array(key::stations);
	for (size_t index = 0; index < stations.size(); ++index)
	{
		const JsonValue station = stations.at(index);
		if (!station.has(key::lesson)) { continue; }
		const JsonValue lesson = station.object(key::lesson);
		result.push_back({ station.identifier(key::name), lesson_area(lesson),
			lesson.string("title"), lesson.string("text") });
	}
	return result;
}

std::vector<LevelStation> LevelLoadedInfo::stations() const
{
	std::vector<LevelStation> result;
	const JsonValue root = this->json_doc_.root();
	if (root.has(key::stations))
	{
		const JsonValue stations = root.array(key::stations);
		for (size_t index = 0; index < stations.size(); ++index)
		{
			const JsonValue station = stations.at(index);
			result.push_back({ station.identifier(key::name),
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
		// Legacy icon metadata still resolves against sprite_sheet_1; Stage Select
		// now draws a live preview rather than this icon.
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
		throw std::runtime_error(root.object(key::sound).where() + ": " + error.what());
	}
	// Uses the builder itself, so the accepted vocabulary cannot drift from
	// the path that starts a match. Objects resolve their assets on construction.
	builder.validate_objects(this->collision_objects_json(),
		this->non_collision_objects_json());
}

std::string LevelLoadedInfo::display_name() const
{
	return this->json_doc_.root().string(key::display_name);
}
std::string LevelLoadedInfo::icon_frame() const
{
	return this->json_doc_.root().string(key::icon_frame);
}
bool LevelLoadedInfo::include_in_random() const
{
	const JsonValue root = this->json_doc_.root();
	return !root.has(key::include_in_random) || root.boolean(key::include_in_random);
}
std::vector<Vector2F> LevelLoadedInfo::team_a_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object(key::spawn_locations).array(key::team_a));
}
std::vector<Vector2F> LevelLoadedInfo::team_b_spawns() const
{
	return decode_team_spawns(
		this->json_doc_.root().object(key::spawn_locations).array(key::team_b));
}

RectangleF LevelLoadedInfo::bounds_rectangle(const char* bounds_key) const
{
	const JsonValue bounds =
		this->json_doc_.root().object(key::level_bounds).object(bounds_key);

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
	return this->bounds_rectangle(key::out_of_bounds);
}
RectangleF LevelLoadedInfo::camera_bounds_rectangle() const
{
	return this->bounds_rectangle(key::camera_bounds);
}
RectangleF LevelLoadedInfo::zoom_out_start_bounds_rectangle() const
{
	return this->bounds_rectangle(key::zoom_out_start_bounds);
}
RectangleF LevelLoadedInfo::zoom_out_finish_bounds_rectangle() const
{
	return this->bounds_rectangle(key::zoom_out_finish_bounds);
}

JsonValue LevelLoadedInfo::collision_objects_json() const
{
	return this->json_doc_.root().array(key::collision_objects);
}
JsonValue LevelLoadedInfo::non_collision_objects_json() const
{
	return this->json_doc_.root().array(key::non_collision_objects);
}
std::string LevelLoadedInfo::sound_bank_name() const
{
	return this->json_doc_.root().object(key::sound).string(key::sound_bank_name);
}
bool LevelLoadedInfo::has_music() const
{
	const JsonValue sound = this->json_doc_.root().object(key::sound);
	const bool named = sound.has(key::music_name);
	if (named != sound.has(key::music_volume))
	{
		throw std::runtime_error(sound.where() +
			": music_name and music_volume come together, or neither for a silent stage.");
	}
	return named;
}
std::string LevelLoadedInfo::music_name() const
{
	return this->has_music() ?
		this->json_doc_.root().object(key::sound).string(key::music_name) : std::string();
}
float LevelLoadedInfo::music_volume() const
{
	// Authored gain under the player's music preference, as the sting is.
	return this->has_music() ?
		content_value::read_unit(this->json_doc_.root().object(key::sound), key::music_volume) : 0.0f;
}

bool LevelLoadedInfo::has_end_sting() const
{
	const JsonValue sound = this->json_doc_.root().object(key::sound);
	const bool named = sound.has(key::end_sting_name);
	if (named != sound.has(key::end_sting_volume))
	{
		throw std::runtime_error(sound.where() +
			": end_sting_name and end_sting_volume come together, or neither.");
	}
	return named;
}

std::string LevelLoadedInfo::end_sting_name() const
{
	return this->has_end_sting() ?
		this->json_doc_.root().object(key::sound).identifier(key::end_sting_name) : std::string();
}

float LevelLoadedInfo::end_sting_volume() const
{
	if (!this->has_end_sting()) { return 0.0f; }
	return content_value::read_unit(this->json_doc_.root().object(key::sound), key::end_sting_volume);
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
