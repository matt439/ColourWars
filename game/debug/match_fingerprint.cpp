#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/tune_document.h"
#include "game/objects/level.h"
#include "game/objects/horizontal_motion.h"
#include <bit>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace
{
	std::uint64_t mix(std::uint64_t hash, std::uint32_t value)
	{
		const char bytes[4] = { static_cast<char>(value & 0xff), static_cast<char>((value >> 8) & 0xff),
			static_cast<char>((value >> 16) & 0xff), static_cast<char>((value >> 24) & 0xff) };
		return fnv1a(std::string_view(bytes, 4), hash);
	}

	std::string number(std::uint32_t bits)
	{
		char buffer[48] = {};
		std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(float_from_bits(bits)));
		return buffer;
	}
    std::uint32_t motion_count(std::size_t count)
    {
        if (count > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        { throw std::length_error("Horizontal fingerprint count exceeds schema1 int32 range"); }
        return static_cast<std::uint32_t>(count);
    }

    void append_motion(MatchFingerprint& print, const Level& level, std::uint64_t& hash)
    {
        if (!level.has_horizontal_motion()) { return; }
        MatchFingerprint::HorizontalTerrainPrint terrain;
        const auto states = level.horizontal_structure_states();
        if (states.empty()) { throw std::logic_error("Admitted horizontal motion has no structures"); }
        for (const HorizontalStructureState& state : states)
        {
            MatchFingerprint::HorizontalStructurePrint structure;
            structure.id = state.id;
            const auto& spec = state.motion.spec;
            structure.spec = { bits_from_float(spec.anchor.x), bits_from_float(spec.anchor.y),
                bits_from_float(spec.anchor.width), bits_from_float(spec.anchor.height),
                bits_from_float(spec.travel_x), bits_from_float(spec.period_seconds),
                bits_from_float(spec.initial_phase) };
            structure.phase = std::bit_cast<std::uint64_t>(state.motion.phase);
            structure.direction = state.motion.direction;
            structure.offsets = { bits_from_float(state.motion.previous_offset.x),
                bits_from_float(state.motion.previous_offset.y), bits_from_float(state.motion.current_offset.x),
                bits_from_float(state.motion.current_offset.y) };
            structure.pose = { bits_from_float(state.previous_bounds.x), bits_from_float(state.previous_bounds.y),
                bits_from_float(state.previous_bounds.width), bits_from_float(state.previous_bounds.height),
                bits_from_float(state.current_bounds.x), bits_from_float(state.current_bounds.y),
                bits_from_float(state.current_bounds.width), bits_from_float(state.current_bounds.height) };
            for (const PaintSurfaceSample& sample : state.local_paint)
            {
                MatchFingerprint::HorizontalTilePrint tile;
                tile.face = static_cast<int>(sample.face); tile.team = static_cast<int>(sample.team);
                tile.owner = sample.paint_owner; tile.area = bits_from_float(sample.scored_area);
                for (std::size_t corner = 0; corner < sample.corners.size(); ++corner)
                {
                    tile.corners[corner * 2] = bits_from_float(sample.corners[corner].x);
                    tile.corners[corner * 2 + 1] = bits_from_float(sample.corners[corner].y);
                }
                structure.tiles.push_back(tile);
            }
            terrain.structures.push_back(std::move(structure));
        }
        for (int view = 0; view < level.player_count(); ++view)
        { terrain.carriers.push_back(level.player(view).carrier_id()); }
        hash = fnv1a("colourwars-horizontal-terrain-1", hash);
        hash = mix(hash, motion_count(terrain.structures.size()));
        for (const auto& structure : terrain.structures)
        {
            hash = mix(hash, structure.id);
            for (const auto value : structure.spec) { hash = mix(hash, value); }
            hash = mix(hash, static_cast<std::uint32_t>(structure.phase));
            hash = mix(hash, static_cast<std::uint32_t>(structure.phase >> 32));
            hash = mix(hash, static_cast<std::uint32_t>(structure.direction));
            for (const auto value : structure.offsets) { hash = mix(hash, value); }
            for (const auto value : structure.pose) { hash = mix(hash, value); }
            hash = mix(hash, motion_count(structure.tiles.size()));
            for (const auto& tile : structure.tiles)
            {
                hash = mix(hash, static_cast<std::uint32_t>(tile.face));
                hash = mix(hash, static_cast<std::uint32_t>(tile.team));
                hash = mix(hash, static_cast<std::uint32_t>(tile.owner));
                hash = mix(hash, tile.area);
                for (const auto value : tile.corners) { hash = mix(hash, value); }
            }
        }
        hash = mix(hash, motion_count(terrain.carriers.size()));
        for (const auto id : terrain.carriers) { hash = mix(hash, id); }
        print.horizontal_motion = std::move(terrain);
    }

    std::string motion_difference(const MatchFingerprint::HorizontalTerrainPrint& expected,
        const MatchFingerprint::HorizontalTerrainPrint& actual)
    {
        if (expected.structures.size() != actual.structures.size())
        { return "horizontal motion structure count differs"; }
        for (std::size_t index = 0; index < expected.structures.size(); ++index)
        {
            const auto& e = expected.structures[index]; const auto& a = actual.structures[index];
            const std::string prefix = "horizontal motion " + std::to_string(index + 1) + " ";
            if (e.id != a.id) { return prefix + "id differs"; }
            const char* specs[] = { "anchor x", "anchor y", "width", "height", "travel", "period", "initial phase" };
            for (std::size_t field = 0; field < e.spec.size(); ++field)
            { if (e.spec[field] != a.spec[field]) { return prefix + "spec " + specs[field] + " differs"; } }
            if (e.phase != a.phase) { return prefix + "phase differs"; }
            if (e.direction != a.direction) { return prefix + "direction differs"; }
            const char* offsets[] = { "previous x", "previous y", "current x", "current y" };
            for (std::size_t field = 0; field < e.offsets.size(); ++field)
            { if (e.offsets[field] != a.offsets[field]) { return prefix + "offset " + offsets[field] + " differs"; } }
            for (std::size_t field = 0; field < e.pose.size(); ++field)
            { if (e.pose[field] != a.pose[field]) { return prefix + "pose field " + std::to_string(field) + " differs"; } }
            if (e.tiles.size() != a.tiles.size()) { return prefix + "local tile count differs"; }
            for (std::size_t tile_index = 0; tile_index < e.tiles.size(); ++tile_index)
            {
                const auto& et = e.tiles[tile_index]; const auto& at = a.tiles[tile_index];
                const std::string tile = prefix + "local tile " + std::to_string(tile_index) + " ";
                if (et.face != at.face) { return tile + "face differs"; }
                if (et.team != at.team) { return tile + "team differs"; }
                if (et.owner != at.owner) { return tile + "raw owner differs"; }
                if (et.area != at.area) { return tile + "area differs"; }
                for (std::size_t corner = 0; corner < et.corners.size(); ++corner)
                { if (et.corners[corner] != at.corners[corner]) { return tile + "corner " + std::to_string(corner) + " differs"; } }
            }
        }
        if (expected.carriers != actual.carriers) { return "horizontal motion seat carrier differs"; }
        return {};
    }

}

float float_from_bits(std::uint32_t bits)
{
	return std::bit_cast<float>(bits);
}

std::uint32_t bits_from_float(float value)
{
	return std::bit_cast<std::uint32_t>(value);
}

MatchFingerprint MatchFingerprint::of(const Level& level, int updates)
{
	MatchFingerprint print;
	print.updates = updates;
	std::uint64_t hash = fnv1a("colourwars-fingerprint-1");
	hash = mix(hash, static_cast<std::uint32_t>(updates));
	for (int view = 0; view < level.player_count(); ++view)
	{
		const Player& player = level.player(view);
		Seat seat;
		seat.x = bits_from_float(player.center().x);
		seat.y = bits_from_float(player.center().y);
		seat.vx = bits_from_float(player.velocity().x);
		seat.vy = bits_from_float(player.velocity().y);
		seat.health = bits_from_float(player.health());
		seat.ammo = bits_from_float(player.weapon_ammo());
		seat.respawn = bits_from_float(player.respawn_timer());
		seat.move_state = static_cast<int>(player.move_state());
		seat.alive = player.state() == PlayerState::alive ? 1 : 0;
		for (const std::uint32_t value : { seat.x, seat.y, seat.vx, seat.vy, seat.health,
			seat.ammo, seat.respawn, static_cast<std::uint32_t>(seat.move_state),
			static_cast<std::uint32_t>(seat.alive) })
		{
			hash = mix(hash, value);
		}
        const PlayerPaintUnderfootState underfoot = player.paint_underfoot_state();
        seat.paint_underfoot = underfoot.enabled;
        if (underfoot.enabled)
        {
            seat.support_paint_team = static_cast<int>(underfoot.support_team);
            seat.step_paint_team = static_cast<int>(underfoot.step_team);
            hash = fnv1a("colourwars-underfoot-state-1", hash);
            hash = mix(hash, static_cast<std::uint32_t>(seat.support_paint_team));
            hash = mix(hash, static_cast<std::uint32_t>(seat.step_paint_team));
        }
		print.seats.push_back(seat);
	}
	const LevelEndInfo info = level.level_end_info();
	print.team_a = bits_from_float(info.team_a_area);
	print.team_b = bits_from_float(info.team_b_area);
	print.total = bits_from_float(info.total_area);
	print.level_state = static_cast<int>(level.state());
	hash = mix(hash, print.team_a);
	hash = mix(hash, print.team_b);
	hash = mix(hash, print.total);
	// Include device and device-less ownership so replay checks catch credit changes.
	for (const float area : info.player_areas)
	{
		hash = mix(hash, bits_from_float(area));
	}
	hash = mix(hash, static_cast<std::uint32_t>(print.level_state));
    // All static/POL36 hash operations above retain their original order. The
    // Level owns completed-boundary scratch overwrite/consume invariants.
    append_motion(print, level, hash);
	print.hash = hash;
	return print;
}

std::string MatchFingerprint::difference(const MatchFingerprint& expected,
	const MatchFingerprint& actual)
{
	if (expected.updates != actual.updates)
	{
		return "updates " + std::to_string(actual.updates) + " != " +
			std::to_string(expected.updates);
	}
	if (expected.seats.size() != actual.seats.size())
	{
		return "seat count " + std::to_string(actual.seats.size()) + " != " +
			std::to_string(expected.seats.size());
	}
	for (std::size_t i = 0; i < expected.seats.size(); ++i)
	{
		const Seat& e = expected.seats[i];
		const Seat& a = actual.seats[i];
		const std::string seat = "P" + std::to_string(i + 1) + " ";
		const struct { const char* name; std::uint32_t want; std::uint32_t got; } fields[] = {
			{ "x", e.x, a.x }, { "y", e.y, a.y }, { "vx", e.vx, a.vx }, { "vy", e.vy, a.vy },
			{ "health", e.health, a.health }, { "ammo", e.ammo, a.ammo },
			{ "respawn", e.respawn, a.respawn } };
		for (const auto& field : fields)
		{
			if (field.want != field.got)
			{
				return seat + field.name + " " + number(field.got) + " != " + number(field.want);
			}
		}
		if (e.move_state != a.move_state)
		{
			return seat + "move state " + std::to_string(a.move_state) + " != " +
				std::to_string(e.move_state);
		}
        if (e.paint_underfoot != a.paint_underfoot ||
            (e.paint_underfoot && (e.support_paint_team != a.support_paint_team || e.step_paint_team != a.step_paint_team)))
        {
            return seat + "paint-underfoot enabled/support/step differs";
        }
		if (e.alive != a.alive)
		{
			return seat + (a.alive != 0 ? "alive" : "dead") + " (recorded " +
				(e.alive != 0 ? "alive" : "dead") + ")";
		}
	}
	if (expected.team_a != actual.team_a || expected.team_b != actual.team_b ||
		expected.total != actual.total)
	{
		return "paint " + actual.paint_text() + " != " + expected.paint_text();
	}
	if (expected.level_state != actual.level_state)
	{
		return "level state " + std::to_string(actual.level_state) + " != " +
			std::to_string(expected.level_state);
	}
    if (expected.horizontal_motion.has_value() != actual.horizontal_motion.has_value())
    { return "horizontal motion domain presence differs"; }
    if (expected.horizontal_motion)
    {
        const std::string difference = motion_difference(*expected.horizontal_motion, *actual.horizontal_motion);
        if (!difference.empty()) { return difference; }
    }

	if (expected.hash != actual.hash)
	{
		return "per-slot paint (hash " + hex64(actual.hash) + " != " + hex64(expected.hash) + ")";
	}
	return {};
}

std::string MatchFingerprint::paint_text() const
{
	return "A " + number(this->team_a) + " B " + number(this->team_b) + " of " +
		number(this->total);
}
#endif
