#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include "game/debug/match_plan.h"
#include "game/debug/tune_document.h"
#include <charconv>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

namespace
{
	std::string hex32(std::uint32_t value)
	{
		if (value == 0)
		{
			return "0";
		}
		char buffer[12] = {};
		std::snprintf(buffer, sizeof(buffer), "%08x", value);
		return buffer;
	}

	template <typename Integer>
	bool parse_hex(std::string_view text, Integer& value)
	{
		const char* end = text.data() + text.size();
		const auto result = std::from_chars(text.data(), end, value, 16);
		return !text.empty() && result.ec == std::errc() && result.ptr == end;
	}

	std::string quoted(std::string_view text)
	{
		std::string result = "\"";
		for (const char c : text)
		{
			switch (c)
			{
			case '"': result += "\\\""; break;
			case '\\': result += "\\\\"; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '\t': result += "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) < 0x20)
				{
					char buffer[8] = {};
					std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(c));
					result += buffer;
				}
				else
				{
					result += c;
				}
			}
		}
		return result + "\"";
	}

	const char* visual_word(PlayerVisualMode mode)
	{
		return MatchPlanner::visual_name(mode);
	}

	bool visual_from_word(const std::string& word, PlayerVisualMode& mode)
	{
		if (word == "original") { mode = PlayerVisualMode::original; return true; }
		if (word == "rotating") { mode = PlayerVisualMode::continuous; return true; }
		if (word == "posed") { mode = PlayerVisualMode::authored; return true; }
		return false;
	}

	[[noreturn]] void fail(const std::filesystem::path& path, const std::string& what)
	{
		throw std::runtime_error("replay '" + labrador::path_to_utf8(path) + "': " + what);
	}

	std::uint32_t hex_field(const labrador::JsonValue& value, const char* key,
		const std::filesystem::path& path)
	{
		std::uint32_t bits = 0;
		if (!parse_hex(value.string(key), bits))
		{
			fail(path, std::string("'") + key + "' is not a hex bit pattern");
		}
		return bits;
	}
}

// ---------------------------------------------------------------------------

namespace replay_codec
{
	std::string encode(const PlayerInputData& input)
	{
		const unsigned flags = (input.jump_pressed ? 1u : 0u) | (input.jump_held ? 2u : 0u) |
			(input.shoot_direction_requested ? 4u : 0u) | (input.primary_shoot ? 8u : 0u) |
			(input.drop_through ? 16u : 0u) | (input.toggle_debug ? 32u : 0u) |
			(input.toggle_pause_menu ? 64u : 0u) | (input.connected ? 128u : 0u);
		char buffer[8] = {};
		std::snprintf(buffer, sizeof(buffer), "%02x", flags);
		std::string text = buffer;
		for (const float value : { input.x_movement, input.shoot_direction.x,
			input.shoot_direction.y, input.shoot_angle, input.left_analog_stick.x,
			input.left_analog_stick.y, input.right_analog_stick.x, input.right_analog_stick.y })
		{
			text += "|" + hex32(bits_from_float(value));
		}
		return text;
	}

	bool decode(std::string_view text, PlayerInputData& input)
	{
		std::vector<std::string_view> fields;
		std::size_t start = 0;
		while (true)
		{
			const std::size_t bar = text.find('|', start);
			fields.push_back(text.substr(start, bar == std::string_view::npos ?
				std::string_view::npos : bar - start));
			if (bar == std::string_view::npos)
			{
				break;
			}
			start = bar + 1;
		}
		if (fields.size() != 9)
		{
			return false;
		}
		unsigned flags = 0;
		if (!parse_hex(fields[0], flags) || flags > 0xff)
		{
			return false;
		}
		float values[8] = {};
		for (std::size_t i = 0; i < 8; ++i)
		{
			std::uint32_t bits = 0;
			if (!parse_hex(fields[i + 1], bits))
			{
				return false;
			}
			values[i] = float_from_bits(bits);
		}
		input = PlayerInputData();
		input.jump_pressed = (flags & 1u) != 0;
		input.jump_held = (flags & 2u) != 0;
		input.shoot_direction_requested = (flags & 4u) != 0;
		input.primary_shoot = (flags & 8u) != 0;
		input.drop_through = (flags & 16u) != 0;
		input.toggle_debug = (flags & 32u) != 0;
		input.toggle_pause_menu = (flags & 64u) != 0;
		input.connected = (flags & 128u) != 0;
		input.x_movement = values[0];
		input.shoot_direction = { values[1], values[2] };
		input.shoot_angle = values[3];
		input.left_analog_stick = { values[4], values[5] };
		input.right_analog_stick = { values[6], values[7] };
		return true;
	}

	bool same(const PlayerInputData& a, const PlayerInputData& b)
	{
		return encode(a) == encode(b);
	}

	std::vector<std::string> run_length(const std::vector<PlayerInputData>& inputs)
	{
		std::vector<std::string> entries;
		std::size_t i = 0;
		while (i < inputs.size())
		{
			const std::string text = encode(inputs[i]);
			std::size_t run = 1;
			while (i + run < inputs.size() && encode(inputs[i + run]) == text)
			{
				++run;
			}
			entries.push_back(run > 1 ? std::to_string(run) + "*" + text : text);
			i += run;
		}
		return entries;
	}

	std::vector<PlayerInputData> expand(const std::vector<std::string>& entries)
	{
		std::vector<PlayerInputData> inputs;
		for (const std::string& entry : entries)
		{
			std::string_view text = entry;
			std::size_t count = 1;
			const std::size_t star = text.find('*');
			if (star != std::string_view::npos)
			{
				const char* end = text.data() + star;
				const auto result = std::from_chars(text.data(), end, count);
				if (result.ec != std::errc() || result.ptr != end || count == 0 || count > 10000000)
				{
					throw std::runtime_error("bad run count in input '" + entry + "'");
				}
				text.remove_prefix(star + 1);
			}
			PlayerInputData input;
			if (!decode(text, input))
			{
				throw std::runtime_error("malformed input '" + entry + "'");
			}
			inputs.insert(inputs.end(), count, input);
		}
		return inputs;
	}
}

std::string replay_name(std::string_view requested)
{
	std::string name;
	for (const char c : requested)
	{
		const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '_' || c == '-';
		name += safe ? c : '_';
	}
	if (name.size() > 5 && name.compare(name.size() - 5, 5, "_json") == 0)
	{
		name.resize(name.size() - 5);
	}
	if (!name.empty())
	{
		return name;
	}
	const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm local = {};
	localtime_s(&local, &now);
	char buffer[40] = {};
	std::strftime(buffer, sizeof(buffer), "replay-%Y%m%d-%H%M%S", &local);
	return buffer;
}

// ---------------------------------------------------------------------------

void ReplayFile::write(const std::filesystem::path& path) const
{
	std::ostringstream out;
	out << "{\n";
	out << "  \"version\": " << format_version << ",\n";
	out << "  \"build\": " << quoted(this->build) << ",\n";
	out << "  \"dt\": " << quoted(hex32(this->dt_bits)) << ",\n";
	if (this->seed)
	{
		out << "  \"seed\": " << quoted(std::to_string(*this->seed)) << ",\n";
	}
	out << "  \"tuning_hash\": " << quoted(hex64(this->tuning_hash)) << ",\n";
	out << "  \"presentation_hash\": " << quoted(hex64(this->presentation_hash)) << ",\n";
	out << "  \"tune_diff\": [";
	for (std::size_t i = 0; i < this->tune_diff.size(); ++i)
	{
		out << (i == 0 ? " " : ", ") << quoted(this->tune_diff[i]);
	}
	out << (this->tune_diff.empty() ? "],\n" : " ],\n");
	out << "  \"cheats\": { \"god\": " << (this->god ? "true" : "false") <<
		", \"infinite_ammo\": " << (this->infinite_ammo ? "true" : "false") << " },\n";

	const MenuLevelSettings& roster = this->settings;
	out << "  \"settings\": {\n";
	out << "    \"stage\": " << quoted(roster.stage()) << ",\n";
	out << "    \"visual\": " << quoted(visual_word(roster.workshop_visual_mode())) << ",\n";
	out << "    \"best_of\": " << roster.best_of() << ",\n";
	out << "    \"layout\": " << static_cast<int>(roster.screen_layout()) << ",\n";
	out << "    \"seats\": [\n";
	const std::vector<MenuPlayerSettings> seats = roster.player_settings();
	for (std::size_t i = 0; i < seats.size(); ++i)
	{
		out << "      { \"slot\": " << seats[i].num << ", \"team\": " <<
			quoted(seats[i].team == PlayerTeam::a ? "a" : seats[i].team == PlayerTeam::b ? "b" : "none") <<
			", \"weapon\": " << quoted(MatchPlanner::weapon_name(seats[i].weapon)) << " }" <<
			(i + 1 < seats.size() ? ",\n" : "\n");
	}
	out << "    ]\n  },\n";

	out << "  \"updates\": " << this->updates << ",\n";
	out << "  \"suspends\": [";
	for (std::size_t i = 0; i < this->suspends.size(); ++i)
	{
		out << (i == 0 ? " " : ", ") << quoted(std::to_string(this->suspends[i]));
	}
	out << (this->suspends.empty() ? "],\n" : " ],\n");

	out << "  \"inputs\": [\n";
	for (std::size_t seat = 0; seat < this->inputs.size(); ++seat)
	{
		const std::vector<std::string> entries = replay_codec::run_length(this->inputs[seat]);
		out << "    [";
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			out << (i == 0 ? "\n      " : ",\n      ") << quoted(entries[i]);
		}
		out << (entries.empty() ? "]" : "\n    ]") << (seat + 1 < this->inputs.size() ? ",\n" : "\n");
	}
	out << "  ],\n";

	out << "  \"checkpoints\": [";
	for (std::size_t i = 0; i < this->checkpoints.size(); ++i)
	{
		out << (i == 0 ? "\n" : ",\n") << "    { \"update\": " << this->checkpoints[i].update <<
			", \"hash\": " << quoted(hex64(this->checkpoints[i].hash)) << " }";
	}
	out << (this->checkpoints.empty() ? "],\n" : "\n  ],\n");

	const MatchFingerprint& print = this->final_print;
	out << "  \"final\": {\n";
	out << "    \"updates\": " << print.updates << ",\n";
	out << "    \"hash\": " << quoted(hex64(print.hash)) << ",\n";
	out << "    \"paint\": " << quoted(print.paint_text()) << ",\n";
	out << "    \"team_a\": " << quoted(hex32(print.team_a)) << ",\n";
	out << "    \"team_b\": " << quoted(hex32(print.team_b)) << ",\n";
	out << "    \"total\": " << quoted(hex32(print.total)) << ",\n";
	out << "    \"level_state\": " << print.level_state << ",\n";
	out << "    \"seats\": [";
	for (std::size_t i = 0; i < print.seats.size(); ++i)
	{
		const MatchFingerprint::Seat& seat = print.seats[i];
		out << (i == 0 ? "\n" : ",\n") << "      { \"x\": " << quoted(hex32(seat.x)) <<
			", \"y\": " << quoted(hex32(seat.y)) << ", \"vx\": " << quoted(hex32(seat.vx)) <<
			", \"vy\": " << quoted(hex32(seat.vy)) << ", \"health\": " << quoted(hex32(seat.health)) <<
			", \"ammo\": " << quoted(hex32(seat.ammo)) << ", \"respawn\": " <<
			quoted(hex32(seat.respawn)) << ", \"move_state\": " << seat.move_state <<
			", \"alive\": " << seat.alive << " }";
	}
	out << (print.seats.empty() ? "]\n" : "\n    ]\n");
	out << "  }\n}\n";

	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);
	std::ofstream stream(path, std::ios::binary | std::ios::trunc);
	const std::string text = out.str();
	stream.write(text.data(), static_cast<std::streamsize>(text.size()));
	stream.close();
	if (!stream)
	{
		fail(path, "could not be written");
	}
}

ReplayFile ReplayFile::read(const std::filesystem::path& path)
{
	const std::string utf8 = labrador::path_to_utf8(path);
	const labrador::JsonDocument document = labrador::read_json_file(utf8.c_str());
	const labrador::JsonValue root = document.root();
	const int version = root.integer("version");
	if (version != format_version)
	{
		fail(path, "format version " + std::to_string(version) + "; this build reads " +
			std::to_string(format_version));
	}

	ReplayFile file;
	file.build = root.string("build");
	file.dt_bits = hex_field(root, "dt", path);
	if (root.has("seed"))
	{
		unsigned int seed = 0;
		const std::string text = root.string("seed");
		const auto result = std::from_chars(text.data(), text.data() + text.size(), seed);
		if (result.ec != std::errc() || result.ptr != text.data() + text.size())
		{
			fail(path, "'seed' is not a number");
		}
		file.seed = seed;
	}
	if (!parse_hex(root.string("tuning_hash"), file.tuning_hash) ||
		!parse_hex(root.string("presentation_hash"), file.presentation_hash))
	{
		fail(path, "a content hash is not hex");
	}
	const labrador::JsonValue diff = root.array("tune_diff");
	for (std::size_t i = 0; i < diff.size(); ++i)
	{
		file.tune_diff.push_back(diff.at(i).as_string());
	}
	const labrador::JsonValue cheats = root.object("cheats");
	file.god = cheats.boolean("god");
	file.infinite_ammo = cheats.boolean("infinite_ammo");

	// The roster, claimed in seat order as MatchPlanner claims it: devices
	// first, idle dummies (-1) after.
	const labrador::JsonValue setup = root.object("settings");
	const labrador::JsonValue seats = setup.array("seats");
	const int count = static_cast<int>(seats.size());
	if (count < 1 || count > 4)
	{
		fail(path, "it has " + std::to_string(count) + " seats; 1-4 expected");
	}
	MenuLevelSettings& rebuilt = file.settings;
	rebuilt.set_player_count(count);
	std::vector<int> slots;
	for (int seat = 0; seat < count; ++seat)
	{
		slots.push_back(seats.at(static_cast<std::size_t>(seat)).integer("slot"));
	}
	if (slots.front() < 0)
	{
		fail(path, "seat 1 has no device");
	}
	rebuilt.begin_input_claims(slots.front());
	for (int seat = 1; seat < count; ++seat)
	{
		if (slots[static_cast<std::size_t>(seat)] >= 0 &&
			!rebuilt.claim_input_slot(slots[static_cast<std::size_t>(seat)]))
		{
			fail(path, "seat " + std::to_string(seat + 1) + "'s slot cannot be claimed");
		}
	}
	const std::vector<MenuPlayerSettings> claimed = rebuilt.player_settings();
	for (int seat = 0; seat < count; ++seat)
	{
		if (claimed[static_cast<std::size_t>(seat)].num != slots[static_cast<std::size_t>(seat)])
		{
			fail(path, "the roster's slots cannot be rebuilt in seat order");
		}
		const labrador::JsonValue entry = seats.at(static_cast<std::size_t>(seat));
		const std::string team = entry.string("team");
		rebuilt.set_player_team(seat, team == "a" ? PlayerTeam::a :
			team == "b" ? PlayerTeam::b : PlayerTeam::none);
		WeaponType weapon = WeaponType::sprayer;
		if (!MatchPlanner::weapon_from_name(entry.string("weapon"), weapon) ||
			weapon == WeaponType::random_primary)
		{
			fail(path, "seat " + std::to_string(seat + 1) + " has an unknown weapon");
		}
		rebuilt.set_player_weapon(seat, weapon);
	}
	rebuilt.set_stage(setup.string("stage"));
	PlayerVisualMode visual = PlayerVisualMode::original;
	if (!visual_from_word(setup.string("visual"), visual))
	{
		fail(path, "unknown visual '" + setup.string("visual") + "'");
	}
	rebuilt.set_workshop_visual_mode(visual);
	rebuilt.set_best_of(setup.integer("best_of"));
	// The layout a plan derives from the seat count, and no other: a build
	// throws on an unknown layout, and a wrong one overlaps the views.
	const int layout = setup.integer("layout");
	if (layout != static_cast<int>(MatchPlanner::layout_for(count)))
	{
		fail(path, "layout " + std::to_string(layout) + " is not the " + std::to_string(count) +
			"-player layout (" + std::to_string(static_cast<int>(MatchPlanner::layout_for(count))) + ")");
	}
	rebuilt.set_screen_layout(MatchPlanner::layout_for(count));

	file.updates = root.integer("updates");
	// Array elements read only as strings (JsonValue::as_string): the
	// update numbers are written as strings.
	const labrador::JsonValue suspends = root.array("suspends");
	for (std::size_t i = 0; i < suspends.size(); ++i)
	{
		const std::string text = suspends.at(i).as_string();
		int update = 0;
		const auto result = std::from_chars(text.data(), text.data() + text.size(), update);
		if (result.ec != std::errc() || result.ptr != text.data() + text.size() || update < 0)
		{
			fail(path, "'suspends' holds '" + text + "', not an update number");
		}
		file.suspends.push_back(update);
	}

	const labrador::JsonValue inputs = root.array("inputs");
	for (std::size_t seat = 0; seat < inputs.size(); ++seat)
	{
		const labrador::JsonValue entries = inputs.at(seat);
		std::vector<std::string> texts;
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			texts.push_back(entries.at(i).as_string());
		}
		try
		{
			file.inputs.push_back(replay_codec::expand(texts));
		}
		catch (const std::exception& e)
		{
			fail(path, "seat " + std::to_string(seat + 1) + ": " + e.what());
		}
	}

	const labrador::JsonValue checkpoints = root.array("checkpoints");
	for (std::size_t i = 0; i < checkpoints.size(); ++i)
	{
		const labrador::JsonValue entry = checkpoints.at(i);
		Checkpoint checkpoint;
		checkpoint.update = entry.integer("update");
		if (!parse_hex(entry.string("hash"), checkpoint.hash))
		{
			fail(path, "a checkpoint hash is not hex");
		}
		file.checkpoints.push_back(checkpoint);
	}

	const labrador::JsonValue last = root.object("final");
	MatchFingerprint& print = file.final_print;
	print.updates = last.integer("updates");
	if (!parse_hex(last.string("hash"), print.hash))
	{
		fail(path, "the final hash is not hex");
	}
	print.team_a = hex_field(last, "team_a", path);
	print.team_b = hex_field(last, "team_b", path);
	print.total = hex_field(last, "total", path);
	print.level_state = last.integer("level_state");
	const labrador::JsonValue final_seats = last.array("seats");
	for (std::size_t i = 0; i < final_seats.size(); ++i)
	{
		const labrador::JsonValue entry = final_seats.at(i);
		MatchFingerprint::Seat seat;
		seat.x = hex_field(entry, "x", path);
		seat.y = hex_field(entry, "y", path);
		seat.vx = hex_field(entry, "vx", path);
		seat.vy = hex_field(entry, "vy", path);
		seat.health = hex_field(entry, "health", path);
		seat.ammo = hex_field(entry, "ammo", path);
		seat.respawn = hex_field(entry, "respawn", path);
		seat.move_state = entry.integer("move_state");
		seat.alive = entry.integer("alive");
		print.seats.push_back(seat);
	}
	return file;
}

// ---------------------------------------------------------------------------

ReplayRecorder::ReplayRecorder(std::string name, ReplayFile header, SeatInputFilter* inner,
	int limit) :
	name_(std::move(name)), file_(std::move(header)), inner_(inner), limit_(limit)
{
}

void ReplayRecorder::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	if (this->inner_ != nullptr)
	{
		this->inner_->filter_seats(seats, level);
	}
	if (this->file_.inputs.size() < seats.size())
	{
		this->file_.inputs.resize(seats.size());
	}
	for (std::size_t seat = 0; seat < seats.size(); ++seat)
	{
		this->file_.inputs[seat].push_back(seats[seat]);
	}
}

void ReplayRecorder::note_suspend(int update)
{
	if (this->file_.suspends.empty() || this->file_.suspends.back() != update)
	{
		this->file_.suspends.push_back(update);
	}
}

void ReplayRecorder::add_checkpoint(const MatchFingerprint& print)
{
	this->file_.checkpoints.push_back({ print.updates, print.hash });
}

ReplayPlayer::ReplayPlayer(std::string name, ReplayFile file) :
	name_(std::move(name)), file_(std::move(file))
{
}

void ReplayPlayer::filter_seats(std::vector<PlayerInputData>& seats, const Level&)
{
	for (std::size_t seat = 0; seat < seats.size(); ++seat)
	{
		const bool recorded = seat < this->file_.inputs.size() &&
			this->step_ < this->file_.inputs[seat].size();
		PlayerInputData input;
		input.connected = true;
		seats[seat] = recorded ? this->file_.inputs[seat][this->step_] : input;
	}
	++this->step_;
}

bool ReplayPlayer::recorded_suspend(int update) const
{
	return std::find(this->file_.suspends.begin(), this->file_.suspends.end(), update) !=
		this->file_.suspends.end();
}
#endif
