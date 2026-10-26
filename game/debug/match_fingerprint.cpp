#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/tune_document.h"
#include "game/objects/level.h"
#include <bit>
#include <cstdio>

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
