#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include "game/debug/debug_file_name.h"
#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include "game/debug/match_plan.h"
#include "game/debug/tune_document.h"
#include "game/input_slots.h"
#include "game/objects/horizontal_motion.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
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

	const char* difficulty_name(BotDifficulty difficulty, const std::filesystem::path& path)
	{
		switch (difficulty)
		{
		case BotDifficulty::easy: return "easy";
		case BotDifficulty::normal: return "normal";
		case BotDifficulty::hard: return "hard";
		default: fail(path, "bot difficulty must be easy, normal or hard");
		}
	}

	BotDifficulty read_difficulty(const labrador::JsonValue& seat,
		const std::filesystem::path& path)
	{
		const std::string difficulty = seat.identifier("difficulty");
		if (difficulty == "easy") { return BotDifficulty::easy; }
		if (difficulty == "normal") { return BotDifficulty::normal; }
		if (difficulty == "hard") { return BotDifficulty::hard; }
		fail(path, "bot difficulty must be easy, normal or hard");
	}

	void validate_roster(const MatchSetup& settings, const std::filesystem::path& path)
	{
		const std::vector<SeatSetup>& seats = settings.player_settings();
		if (seats.empty() || seats.size() > 4) { fail(path, "settings must have 1-4 seats"); }
		if (seats.front().controller != PlayerController::human || seats.front().num < 0)
		{ fail(path, "seat 1 must be a human with a device"); }
		std::array<bool, input_slots::count> used{};
		for (const SeatSetup& seat : seats)
		{
			if (seat.controller == PlayerController::bot)
			{
				if (seat.num != -1) { fail(path, "a bot seat cannot have a device"); }
				difficulty_name(seat.difficulty, path);
			}
			else if (seat.controller == PlayerController::human)
			{
				if (seat.num < -1 || seat.num >= input_slots::count)
				{ fail(path, "a human device slot is out of range"); }
				if (seat.num >= 0)
				{
					if (used[static_cast<std::size_t>(seat.num)]) { fail(path, "human device slots must be unique"); }
					used[static_cast<std::size_t>(seat.num)] = true;
				}
			}
			else { fail(path, "controller must be human or bot"); }
		}
		const int humans = settings.human_count();
		const int layout = static_cast<int>(settings.screen_layout());
		if (layout != static_cast<int>(MatchPlanner::layout_for(humans)))
		{
			fail(path, "layout " + std::to_string(layout) + " is not the " + std::to_string(humans) +
				"-player layout (" + std::to_string(static_cast<int>(MatchPlanner::layout_for(humans))) + ")");
		}
	}
    // Strict schema1 helpers are separate from the unchanged legacy codec.
    std::string motion_hex32(std::uint32_t value)
    {
        char buffer[9] = {}; std::snprintf(buffer, sizeof(buffer), "%08x", value); return buffer;
    }
    std::string motion_hex64(std::uint64_t value)
    {
        char buffer[17] = {};
        std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value)); return buffer;
    }
    template<class Integer> Integer motion_hex(const std::string& text, std::size_t width,
        const std::filesystem::path& path)
    {
        Integer value = 0;
        const bool digits = std::all_of(text.begin(), text.end(), [](char c)
        { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); });
        if (text.size() != width || !digits || !parse_hex(text, value))
        { fail(path, "horizontal_motion requires exact " + std::to_string(width) + "-hex-digit fields"); }
        return value;
    }
    template<std::size_t N> std::array<std::uint32_t, N> motion_array(
        const labrador::JsonValue& entry, const char* key, const std::filesystem::path& path)
    {
        const auto values = entry.array(key);
        if (values.size() != N) { fail(path, std::string("horizontal_motion '") + key + "' cardinality"); }
        std::array<std::uint32_t, N> result{};
        for (std::size_t index = 0; index < N; ++index)
        { result[index] = motion_hex<std::uint32_t>(values.at(index).as_string(), 8, path); }
        return result;
    }
    template<std::size_t N> void write_motion_array(std::ostream& out, const std::array<std::uint32_t, N>& values)
    {
        out << "[";
        for (std::size_t index = 0; index < N; ++index)
        { out << (index == 0 ? " " : ", ") << quoted(motion_hex32(values[index])); }
        out << " ]";
    }
    void validate_motion(const MatchFingerprint::HorizontalTerrainPrint& terrain,
        std::size_t roster, std::size_t final_seats, const std::filesystem::path& path)
    {
        const auto bounded = [&](std::size_t count)
        {
            if (count > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
            { fail(path, "horizontal_motion count exceeds int32 range"); }
        };
        bounded(terrain.structures.size()); bounded(terrain.carriers.size());
        if (terrain.structures.empty()) { fail(path, "horizontal_motion structures must be nonempty"); }
        if (roster < 1 || roster > 4 || terrain.carriers.size() != roster || terrain.carriers.size() != final_seats)
        { fail(path, "horizontal_motion carrier count must match setup and final seats"); }
        for (std::size_t index = 0; index < terrain.structures.size(); ++index)
        {
            const auto& value = terrain.structures[index];
            if (value.id != index + 1) { fail(path, "horizontal_motion IDs must be canonical 1..N order"); }
            const auto finite = [&](std::uint32_t field)
            { if (!std::isfinite(float_from_bits(field))) { fail(path, "horizontal_motion float must be finite"); } };
            for (const auto field : value.spec) { finite(field); }
            for (const auto field : value.offsets) { finite(field); }
            for (const auto field : value.pose) { finite(field); }
            HorizontalMotionSpec spec;
            spec.anchor = { float_from_bits(value.spec[0]), float_from_bits(value.spec[1]),
                float_from_bits(value.spec[2]), float_from_bits(value.spec[3]) };
            spec.travel_x = float_from_bits(value.spec[4]);
            spec.period_seconds = float_from_bits(value.spec[5]); spec.initial_phase = float_from_bits(value.spec[6]);
            try { HorizontalMotion::validate_spec(spec); }
            catch (const std::exception& error) { fail(path, "horizontal_motion spec: " + std::string(error.what())); }
            const double phase = std::bit_cast<double>(value.phase);
            if (!std::isfinite(phase) || phase < 0.0 || phase >= 1.0)
            { fail(path, "horizontal_motion phase must be finite in [0,1)"); }
            if (value.direction != (phase < 0.5 ? 1 : -1))
            { fail(path, "horizontal_motion direction disagrees with phase"); }
            for (std::size_t frame = 0; frame < 2; ++frame)
            {
                const float offset = float_from_bits(value.offsets[frame * 2]);
                if (offset < 0.0f || offset > spec.travel_x || float_from_bits(value.offsets[frame * 2 + 1]) != 0.0f)
                { fail(path, "horizontal_motion offset outside horizontal travel"); }
                mattmath::RectangleF pose = spec.anchor;
                if (offset != 0.0f) { pose.x += offset; }
                const std::uint32_t expected[] = { bits_from_float(pose.x), bits_from_float(pose.y),
                    bits_from_float(pose.width), bits_from_float(pose.height) };
                for (std::size_t field = 0; field < 4; ++field)
                { if (value.pose[frame * 4 + field] != expected[field])
                  { fail(path, "horizontal_motion pose disagrees with exact offset/spec"); } }
                if (!std::isfinite(pose.right()) || !(pose.right() > pose.left()) ||
                    !std::isfinite(pose.bottom()) || !(pose.bottom() > pose.top()))
                { fail(path, "horizontal_motion pose has unrepresentable edges"); }
            }
            const double ratio = phase <= 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
            const float current = static_cast<float>(static_cast<double>(spec.travel_x) * ratio);
            if (value.offsets[2] != bits_from_float(current))
            { fail(path, "horizontal_motion current offset disagrees with phase"); }
            bounded(value.tiles.size());
            int previous_face = -1;
            float previous_start = 0.0f;
            for (const auto& tile : value.tiles)
            {
                if (tile.face < 0 || tile.face > 3 || tile.team < 0 || tile.team > 2)
                { fail(path, "horizontal_motion tile enum out of range"); }
                finite(tile.area);
                if (float_from_bits(tile.area) < 0.0f) { fail(path, "horizontal_motion tile area must be nonnegative"); }
                for (const auto field : tile.corners) { finite(field); }
                const float start = float_from_bits(tile.corners[tile.face < 2 ? 0 : 1]);
                if (tile.face < previous_face || (tile.face == previous_face && start < previous_start))
                { fail(path, "horizontal_motion local tiles must retain face/tile source order"); }
                previous_face = tile.face; previous_start = start;
            }
        }
        for (const auto id : terrain.carriers)
        { if (id > terrain.structures.size()) { fail(path, "horizontal_motion carrier references unknown ID"); } }
    }
    void write_motion(std::ostream& out, const MatchFingerprint::HorizontalTerrainPrint& terrain)
    {
        out << ",\n    \"horizontal_motion\": { \"schema\": 1, \"structures\": [";
        for (std::size_t index = 0; index < terrain.structures.size(); ++index)
        {
            const auto& value = terrain.structures[index];
            out << (index == 0 ? "\n" : ",\n") << "      { \"id\": " << value.id << ", \"spec\": ";
            write_motion_array(out, value.spec);
            out << ", \"phase\": " << quoted(motion_hex64(value.phase)) << ", \"direction\": " << value.direction;
            out << ", \"offsets\": "; write_motion_array(out, value.offsets);
            out << ", \"pose\": "; write_motion_array(out, value.pose); out << ", \"tiles\": [";
            for (std::size_t tile_index = 0; tile_index < value.tiles.size(); ++tile_index)
            {
                const auto& tile = value.tiles[tile_index];
                out << (tile_index == 0 ? "\n" : ",\n") << "        { \"face\": " << tile.face <<
                    ", \"team\": " << tile.team << ", \"owner\": " << tile.owner <<
                    ", \"area\": " << quoted(motion_hex32(tile.area)) << ", \"corners\": ";
                write_motion_array(out, tile.corners); out << " }";
            }
            out << (value.tiles.empty() ? "] }" : "\n      ] }");
        }
        out << "\n    ], \"carriers\": [";
        for (std::size_t seat = 0; seat < terrain.carriers.size(); ++seat)
        { out << (seat == 0 ? " " : ", ") << "{ \"motion_id\": " << terrain.carriers[seat] << " }"; }
        out << " ] }";
    }
    MatchFingerprint::HorizontalTerrainPrint read_motion(const labrador::JsonValue& entry,
        std::size_t roster, std::size_t final_seats, const std::filesystem::path& path)
    {
        if (entry.integer("schema") != 1) { fail(path, "horizontal_motion schema must be 1"); }
        MatchFingerprint::HorizontalTerrainPrint terrain;
        const auto structures = entry.array("structures");
        for (std::size_t index = 0; index < structures.size(); ++index)
        {
            const auto source = structures.at(index);
            MatchFingerprint::HorizontalStructurePrint value;
            const int id = source.integer("id");
            if (id <= 0) { fail(path, "horizontal_motion ID must be positive int32"); }
            value.id = static_cast<std::uint32_t>(id);
            value.spec = motion_array<7>(source, "spec", path);
            value.phase = motion_hex<std::uint64_t>(source.string("phase"), 16, path);
            value.direction = source.integer("direction");
            value.offsets = motion_array<4>(source, "offsets", path); value.pose = motion_array<8>(source, "pose", path);
            const auto tiles = source.array("tiles");
            for (std::size_t tile_index = 0; tile_index < tiles.size(); ++tile_index)
            {
                const auto sample = tiles.at(tile_index);
                MatchFingerprint::HorizontalTilePrint tile;
                tile.face = sample.integer("face"); tile.team = sample.integer("team"); tile.owner = sample.integer("owner");
                tile.area = motion_hex<std::uint32_t>(sample.string("area"), 8, path);
                tile.corners = motion_array<8>(sample, "corners", path); value.tiles.push_back(tile);
            }
            terrain.structures.push_back(std::move(value));
        }
        const auto carriers = entry.array("carriers");
        for (std::size_t seat = 0; seat < carriers.size(); ++seat)
        {
            const int id = carriers.at(seat).integer("motion_id");
            if (id < 0) { fail(path, "horizontal_motion carrier ID must be nonnegative int32"); }
            terrain.carriers.push_back(static_cast<std::uint32_t>(id));
        }
        validate_motion(terrain, roster, final_seats, path); return terrain;
    }

}

// ---------------------------------------------------------------------------

namespace replay_codec
{
	std::string encode(const PlayerInputData& input)
	{
		// 64 and 256 were a seat's pause and intro-skip signals, which a seat
		// no longer carries (SlotInput): never written, ignored when read.
		const unsigned flags = (input.jump_pressed ? 1u : 0u) | (input.jump_held ? 2u : 0u) |
			(input.shoot_direction_requested ? 4u : 0u) | (input.primary_shoot ? 8u : 0u) |
			(input.drop_through ? 16u : 0u) | (input.toggle_debug ? 32u : 0u) |
			(input.connected ? 128u : 0u);
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
		if (!parse_hex(fields[0], flags) || flags > 0x1ff)
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
		if (debug_file_name::reserved_stem(name))
		{
			name.insert(name.begin(), '_');
		}
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

std::vector<std::vector<PlayerInputData>> ReplayFile::inputs_by_seat() const
{
	std::vector<std::vector<PlayerInputData>> by_seat(this->tape.settings().player_settings().size());
	for (int step = 0; step < this->tape.steps(); ++step)
	{
		const std::vector<PlayerInputData>& seats = this->tape.seats(step);
		if (by_seat.size() < seats.size()) { by_seat.resize(seats.size()); }
		for (std::size_t seat = 0; seat < seats.size(); ++seat)
		{
			by_seat[seat].push_back(seats[seat]);
		}
	}
	return by_seat;
}

bool ReplayFile::suspended_at(int update) const
{
	const std::vector<RoundTape::Gap>& gaps = this->tape.gaps();
	for (std::size_t i = 0; i < gaps.size() && i < this->gap_updates.size(); ++i)
	{
		if (!gaps[i].resume && this->gap_updates[i] == update) { return true; }
	}
	return false;
}

void ReplayFile::write(const std::filesystem::path& path) const
{
	const MatchSetup& roster = this->tape.settings();
	validate_roster(roster, path);
    if (this->final_print.horizontal_motion)
    { validate_motion(*this->final_print.horizontal_motion, roster.player_settings().size(),
        this->final_print.seats.size(), path); }
	const std::size_t participants = roster.player_settings().size();
	for (int step = 0; step < this->tape.steps(); ++step)
	{
		const std::uint32_t bits = std::bit_cast<std::uint32_t>(this->tape.dt(step));
		if (bits != this->dt_bits)
		{
			fail(path, "step " + std::to_string(step) + " was taken at dt " + hex32(bits) +
				", not " + hex32(this->dt_bits));
		}
		if (this->tape.seats(step).size() != participants)
		{
			fail(path, "step " + std::to_string(step) + " has " +
				std::to_string(this->tape.seats(step).size()) + " seats, not the roster's " +
				std::to_string(participants));
		}
	}

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
	if (this->stage_hash)
	{
		out << "  \"stage_hash\": " << quoted(hex64(*this->stage_hash)) << ",\n";
	}
	out << "  \"tune_diff\": [";
	for (std::size_t i = 0; i < this->tune_diff.size(); ++i)
	{
		out << (i == 0 ? " " : ", ") << quoted(this->tune_diff[i]);
	}
	out << (this->tune_diff.empty() ? "],\n" : " ],\n");
	out << "  \"cheats\": { \"god\": " << (this->tape.damage_immune() ? "true" : "false") <<
		", \"infinite_ammo\": " << (this->tape.infinite_ammo() ? "true" : "false") << " },\n";

	out << "  \"settings\": {\n";
	out << "    \"stage\": " << quoted(roster.stage()) << ",\n";
	out << "    \"best_of\": " << roster.best_of() << ",\n";
	if (roster.duration())
	{
		out << "    \"duration\": " << std::setprecision(std::numeric_limits<float>::max_digits10)
			<< *roster.duration() << ",\n";
	}
	out << "    \"layout\": " << static_cast<int>(roster.screen_layout()) << ",\n";
	out << "    \"seats\": [\n";
	const std::vector<SeatSetup> seats = roster.player_settings();
	for (std::size_t i = 0; i < seats.size(); ++i)
	{
		out << "      { \"slot\": " << seats[i].num << ", \"team\": " <<
			quoted(seats[i].team == PlayerTeam::a ? "a" : seats[i].team == PlayerTeam::b ? "b" : "none") <<
			", \"weapon\": " << quoted(MatchPlanner::weapon_name(seats[i].weapon)) <<
			", \"controller\": " << quoted(seats[i].controller == PlayerController::bot ? "bot" : "human") <<
			", \"difficulty\": " << quoted(difficulty_name(seats[i].controller == PlayerController::bot ?
				seats[i].difficulty : BotDifficulty::normal, path)) <<
			(seats[i].handicap != 0 ? ", \"handicap\": " + std::to_string(seats[i].handicap) : std::string()) << " }" <<
			(i + 1 < seats.size() ? ",\n" : "\n");
	}
	out << "    ]\n  },\n";

	out << "  \"updates\": " << this->updates << ",\n";
	const std::vector<RoundTape::Gap>& gaps = this->tape.gaps();
	if (this->gap_updates.size() != gaps.size())
	{
		fail(path, std::to_string(gaps.size()) + " gaps but " + std::to_string(this->gap_updates.size()) +
			" gap updates");
	}
	for (std::size_t i = 0; i < gaps.size(); ++i)
	{
		if (this->gap_updates[i] < (i == 0 ? 0 : this->gap_updates[i - 1]))
		{
			fail(path, "gap " + std::to_string(i + 1) + "'s update " + std::to_string(this->gap_updates[i]) +
				" is negative or before the gap ahead of it");
		}
	}
	out << "  \"gaps\": [";
	for (std::size_t i = 0; i < gaps.size(); ++i)
	{
		out << (i == 0 ? "\n" : ",\n") << "    { \"step\": " << gaps[i].step <<
			", \"update\": " << this->gap_updates[i];
		if (gaps[i].resume)
		{
			out << ", \"resume\": [";
			for (std::size_t seat = 0; seat < gaps[i].seats.size(); ++seat)
			{
				out << (seat == 0 ? " " : ", ") << quoted(replay_codec::encode(gaps[i].seats[seat]));
			}
			out << (gaps[i].seats.empty() ? "]" : " ]");
		}
		out << " }";
	}
	out << (gaps.empty() ? "],\n" : "\n  ],\n");

	const std::vector<std::vector<PlayerInputData>> inputs = this->inputs_by_seat();
	out << "  \"inputs\": [\n";
	for (std::size_t seat = 0; seat < inputs.size(); ++seat)
	{
		const std::vector<std::string> entries = replay_codec::run_length(inputs[seat]);
		out << "    [";
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			out << (i == 0 ? "\n      " : ",\n      ") << quoted(entries[i]);
		}
		out << (entries.empty() ? "]" : "\n    ]") << (seat + 1 < inputs.size() ? ",\n" : "\n");
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
			", \"alive\": " << seat.alive;
        if (seat.paint_underfoot)
        {
            out << ", \"paint_underfoot\": { \"support_team\": " << seat.support_paint_team <<
                ", \"step_team\": " << seat.step_paint_team << " }";
        }
        out << " }";
	}
    if (print.horizontal_motion)
    {
        out << (print.seats.empty() ? "]" : "\n    ]");
        write_motion(out, *print.horizontal_motion);
        out << "\n";
    }
    else
    {
	out << (print.seats.empty() ? "]\n" : "\n    ]\n");
    }
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
		fail(path, "format version " + std::to_string(version) + "; this build reads only " +
			std::to_string(format_version) + " (a replay holds only on the build that recorded it)");
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
	if (root.has("stage_hash"))
	{
		std::uint64_t hash = 0;
		if (!parse_hex(root.string("stage_hash"), hash))
		{
			fail(path, "'stage_hash' is not hex");
		}
		file.stage_hash = hash;
	}
	const labrador::JsonValue diff = root.array("tune_diff");
	for (std::size_t i = 0; i < diff.size(); ++i)
	{
		file.tune_diff.push_back(diff.at(i).as_string());
	}
	const labrador::JsonValue cheats = root.object("cheats");
	const bool god = cheats.boolean("god");
	const bool infinite_ammo = cheats.boolean("infinite_ammo");

	// Controller kind is explicit: an unclaimed human or Debug dummy (-1)
	// must never become a bot.
	const labrador::JsonValue setup = root.object("settings");
	const labrador::JsonValue seats = setup.array("seats");
	const int count = static_cast<int>(seats.size());
	if (count < 1 || count > 4)
	{
		fail(path, "it has " + std::to_string(count) + " seats; 1-4 expected");
	}
	MatchSetup rebuilt;
	rebuilt.set_player_count(count);
	std::vector<int> slots;
	for (int seat = 0; seat < count; ++seat)
	{
		const labrador::JsonValue entry = seats.at(static_cast<std::size_t>(seat));
		const int slot = entry.integer("slot");
		slots.push_back(slot);
		const std::string controller = entry.identifier("controller");
		const BotDifficulty difficulty = read_difficulty(entry, path);
		if (controller == "bot")
		{
			if (seat == 0) { fail(path, "seat 1 must be human"); }
			if (slot != -1) { fail(path, "a bot seat cannot have a device"); }
			rebuilt.set_player_bot(seat, difficulty);
		}
		else if (controller != "human") { fail(path, "controller must be human or bot"); }
		else if (difficulty != BotDifficulty::normal)
		{ fail(path, "a human seat's difficulty must be normal"); }
		if (slot < -1 || slot >= input_slots::count) { fail(path, "device slot is out of range"); }
	}
	if (slots.front() < 0)
	{
		fail(path, "seat 1 has no device");
	}
	rebuilt.begin_input_claims(slots.front());
	// Claim through the public roster operations without compressing a sparse
	// human gap. Temporary unused device claims are released after later humans
	// have reached their original seats; bots are skipped by claim_input_slot.
	std::array<bool, input_slots::count> occupied{};
	for (const int slot : slots)
	{
		if (slot >= 0) { occupied[static_cast<std::size_t>(slot)] = true; }
	}
	std::vector<int> temporary_claims;
	for (int seat = 1; seat < count; ++seat)
	{
		if (rebuilt.player_settings()[static_cast<std::size_t>(seat)].controller == PlayerController::bot)
		{ continue; }
		int slot = slots[static_cast<std::size_t>(seat)];
		if (slot < 0)
		{
			const auto unused = std::find(occupied.begin(), occupied.end(), false);
			if (unused == occupied.end()) { fail(path, "no temporary slot available for a human gap"); }
			slot = static_cast<int>(unused - occupied.begin());
			occupied[static_cast<std::size_t>(slot)] = true;
			temporary_claims.push_back(slot);
		}
		if (!rebuilt.claim_input_slot(slot))
		{
			fail(path, "seat " + std::to_string(seat + 1) + "'s slot cannot be claimed");
		}
	}
	for (const int slot : temporary_claims) { rebuilt.release_input_slot(slot); }
	const std::vector<SeatSetup> claimed = rebuilt.player_settings();
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
		// Absent is none.
		if (entry.has("handicap"))
		{
			const int handicap = entry.integer("handicap");
			if (!handicaps::valid(handicap)) { fail(path, "seat " + std::to_string(seat + 1) + " has an unknown handicap"); }
			rebuilt.set_player_handicap(seat, handicap);
		}
	}
	rebuilt.set_stage(setup.string("stage"));
	rebuilt.set_best_of(setup.integer("best_of"));
	if (setup.has("duration")) { rebuilt.set_duration(setup.number("duration")); }
	// Bots remain participants but have no split-screen pane.
	rebuilt.set_screen_layout(static_cast<labrador::ScreenLayout>(setup.integer("layout")));
	validate_roster(rebuilt, path);

	file.updates = root.integer("updates");

	const std::size_t participants = rebuilt.player_settings().size();
	const labrador::JsonValue inputs = root.array("inputs");
	if (inputs.size() != participants)
	{
		fail(path, "'inputs' has " + std::to_string(inputs.size()) + " seats, not the roster's " +
			std::to_string(participants));
	}
	std::vector<std::vector<PlayerInputData>> by_seat;
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
			by_seat.push_back(replay_codec::expand(texts));
		}
		catch (const std::exception& e)
		{
			fail(path, "seat " + std::to_string(seat + 1) + ": " + e.what());
		}
		if (by_seat.back().size() != by_seat.front().size())
		{
			fail(path, "seat " + std::to_string(seat + 1) + " has " + std::to_string(by_seat.back().size()) +
				" steps, seat 1 " + std::to_string(by_seat.front().size()));
		}
	}
	const int steps = static_cast<int>(by_seat.front().size());

	// The tape, rebuilt as it was recorded: each gap before the step it
	// preceded, in order.
	file.tape.start(0, rebuilt);
	file.tape.set_cheats(god, infinite_ammo);
	const float dt = std::bit_cast<float>(file.dt_bits);
	const labrador::JsonValue gaps = root.array("gaps");
	std::size_t gap = 0;
	int previous = 0;
	for (int step = 0; step <= steps; ++step)
	{
		for (; gap < gaps.size(); ++gap)
		{
			const labrador::JsonValue entry = gaps.at(gap);
			const int at = entry.integer("step");
			if (at < previous || at > steps)
			{
				fail(path, "gap " + std::to_string(gap + 1) + " is at step " + std::to_string(at) +
					", out of order or past the last step");
			}
			previous = at;
			if (at != step) { break; }
			const int update = entry.integer("update");
			if (update < (file.gap_updates.empty() ? 0 : file.gap_updates.back()))
			{
				fail(path, "gap " + std::to_string(gap + 1) + " is at update " + std::to_string(update) +
					", negative or before the gap ahead of it");
			}
			file.gap_updates.push_back(update);
			if (!entry.has("resume"))
			{
				file.tape.record_suspend();
				continue;
			}
			const labrador::JsonValue resume = entry.array("resume");
			if (resume.size() != participants)
			{
				fail(path, "gap " + std::to_string(gap + 1) + " resumes " + std::to_string(resume.size()) +
					" seats, not the roster's " + std::to_string(participants));
			}
			std::vector<PlayerInputData> resumed(participants);
			for (std::size_t seat = 0; seat < participants; ++seat)
			{
				const std::string text = resume.at(seat).as_string();
				if (!replay_codec::decode(text, resumed[seat]))
				{
					fail(path, "gap " + std::to_string(gap + 1) + " holds '" + text + "', not an input");
				}
			}
			file.tape.record_resume(resumed);
		}
		if (step == steps) { break; }
		std::vector<PlayerInputData> row(participants);
		for (std::size_t seat = 0; seat < participants; ++seat)
		{
			row[seat] = by_seat[seat][static_cast<std::size_t>(step)];
		}
		file.tape.record_step(row, dt);
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
        if (entry.has("paint_underfoot"))
        {
            const labrador::JsonValue effect = entry.object("paint_underfoot");
            seat.paint_underfoot = true;
            seat.support_paint_team = effect.integer("support_team");
            seat.step_paint_team = effect.integer("step_team");
            if (seat.support_paint_team < 0 || seat.support_paint_team > 2 ||
                seat.step_paint_team < 0 || seat.step_paint_team > 2)
            { fail(path, "paint_underfoot teams must be a, b or neutral enum values"); }
        }
		print.seats.push_back(seat);
	}
    if (last.has("horizontal_motion"))
    { print.horizontal_motion = read_motion(last.object("horizontal_motion"),
        seats.size(), print.seats.size(), path); }

	return file;
}

// ---------------------------------------------------------------------------

ReplayRecorder::ReplayRecorder(std::string name, ReplayFile header, SeatInputFilter* inner,
	int limit) :
	name_(std::move(name)), file_(std::move(header)), inner_(inner),
	dt_(std::bit_cast<float>(file_.dt_bits)), limit_(limit)
{
	if (!this->file_.tape.started())
	{
		const MatchSetup settings = this->file_.tape.settings();
		this->file_.tape.start(0, settings);
	}
}

void ReplayRecorder::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	this->recorder_.prepare(this->inner_, this->dt_);
	this->recorder_.filter_seats(seats, level);
}

void ReplayRecorder::note_suspend(int update)
{
	this->file_.tape.record_suspend();
	this->file_.gap_updates.push_back(update);
}

void ReplayRecorder::note_resume(int update, const std::vector<PlayerInputData>& seats)
{
	this->file_.tape.record_resume(seats);
	this->file_.gap_updates.push_back(update);
}

void ReplayRecorder::add_checkpoint(const MatchFingerprint& print)
{
	this->file_.checkpoints.push_back({ print.updates, print.hash });
}

ReplayPlayer::ReplayPlayer(std::string name, ReplayFile file) :
	name_(std::move(name)), file_(std::move(file))
{
}

void ReplayPlayer::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	if (this->player_.finished())
	{
		PlayerInputData input;
		input.connected = true;
		std::fill(seats.begin(), seats.end(), input);
		return;
	}
	this->player_.filter_seats(seats, level);
}
#endif
