#include "game/pch.h"
#include "game/save/play_settings_json.h"
#include "game/save/save_section.h"
#include "engine/assets/json.h"
#include <optional>
#include <stdexcept>

using namespace labrador;

namespace
{
	void read_deadzone(const JsonValue& entry, const char* key, int& destination)
	{
		try
		{
			destination = StickSettings::normalized_deadzone(entry.integer(key));
		}
		catch (const std::runtime_error&) {}
	}

	void read_seat_switch(const JsonValue& entry, const char* key, SeatSwitch& destination)
	{
		try
		{
			const std::optional<SeatSwitch> value = seat_switch_from_key(entry.string(key));
			if (value) { destination = *value; }
		}
		catch (const std::runtime_error&) {}
	}
}

void read_stick_settings(const JsonValue& entry, StickSettings& destination)
{
	read_deadzone(entry, "movement_deadzone", destination.movement_deadzone);
	read_deadzone(entry, "aim_deadzone", destination.aim_deadzone);
}

void write_stick_settings(SaveWriter& writer, const StickSettings& sticks)
{
	writer.key("movement_deadzone");
	writer.integer(sticks.movement_deadzone);
	writer.key("aim_deadzone");
	writer.integer(sticks.aim_deadzone);
}

void read_seat_comfort(const JsonValue& entry, SeatComfort& destination)
{
	read_seat_switch(entry, "view_shake", destination.view_shake);
	read_seat_switch(entry, "aim_marker", destination.aim_marker);
	read_seat_switch(entry, "reduce_effects", destination.reduce_effects);
}

void write_seat_comfort(SaveWriter& writer, const SeatComfort& comfort)
{
	writer.key("view_shake");
	writer.string(seat_switch_key(comfort.view_shake));
	writer.key("aim_marker");
	writer.string(seat_switch_key(comfort.aim_marker));
	writer.key("reduce_effects");
	writer.string(seat_switch_key(comfort.reduce_effects));
}
