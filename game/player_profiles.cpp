#include "game/pch.h"
#include "game/player_profiles.h"
#include "game/player_control.h"
#include "game/save.h"
#include "engine/assets/json.h"
#include <algorithm>
#include <stdexcept>

using namespace labrador;

std::string_view player_profiles::name_characters()
{
	return " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
}

bool player_profiles::name_character_supported(char value)
{
	return name_characters().find(value) != std::string_view::npos;
}

std::string player_profiles::normalized_name(std::string_view name)
{
	const std::size_t first = name.find_first_not_of(' ');
	if (first == std::string_view::npos) { throw std::invalid_argument("A profile name needs a letter or digit"); }
	const std::string_view trimmed = name.substr(first, name.find_last_not_of(' ') - first + 1);
	if (trimmed.size() > max_name_length)
	{
		throw std::invalid_argument("A profile name has at most " + std::to_string(max_name_length) + " characters");
	}
	if (!std::all_of(trimmed.begin(), trimmed.end(), name_character_supported))
	{
		throw std::invalid_argument("A profile name holds only A-Z, 0-9 and spaces");
	}
	return std::string(trimmed);
}

void PlayerProfiles::validate() const
{
	if (this->list.size() > player_profiles::max_count)
	{
		throw std::invalid_argument("At most " + std::to_string(player_profiles::max_count) + " profiles");
	}
	for (std::size_t index = 0; index < this->list.size(); ++index)
	{
		const PlayerProfile& profile = this->list[index];
		if (player_profiles::normalized_name(profile.name) != profile.name)
		{
			throw std::invalid_argument("Profile name \"" + profile.name + "\" has surrounding spaces");
		}
		if (this->find(profile.name) != static_cast<int>(index))
		{
			throw std::invalid_argument("Two profiles are named \"" + profile.name + "\"");
		}
		if (StickSettings::normalized_deadzone(profile.sticks.movement_deadzone) != profile.sticks.movement_deadzone ||
			StickSettings::normalized_deadzone(profile.sticks.aim_deadzone) != profile.sticks.aim_deadzone)
		{
			throw std::invalid_argument("Profile \"" + profile.name + "\" has a deadzone outside -1 to 90");
		}
		profile.buttons.validate();
		if (!handicaps::valid(profile.boost))
		{
			throw std::invalid_argument("Profile \"" + profile.name + "\" has an unknown Boost");
		}
	}
}

int PlayerProfiles::find(std::string_view name) const
{
	for (std::size_t index = 0; index < this->list.size(); ++index)
	{
		if (this->list[index].name == name) { return static_cast<int>(index); }
	}
	return -1;
}

std::string PlayerProfiles::next_free_name() const
{
	for (std::size_t number = 1; number <= player_profiles::max_count; ++number)
	{
		std::string name = "PLAYER " + std::to_string(number);
		if (this->find(name) < 0) { return name; }
	}
	// Only reachable when full, which callers refuse first.
	throw std::logic_error("Every default profile name is taken");
}

void PlayerProfilesSection::reset(SaveData& data) const
{
	data.profiles = PlayerProfiles();
}

void PlayerProfilesSection::read(const JsonValue& root, SaveData& data) const
{
	data.profiles = PlayerProfiles();
	if (!root.has(this->key())) { return; }
	const JsonValue entries = root.array(this->key());
	for (std::size_t index = 0; index < entries.size() && !data.profiles.full(); ++index)
	{
		PlayerProfile profile;
		std::optional<JsonValue> entry;
		try
		{
			entry = entries.at(index);
			profile.name = player_profiles::normalized_name(entry->string("name"));
		}
		catch (const std::runtime_error&) { continue; }
		catch (const std::invalid_argument&) { continue; }
		if (data.profiles.find(profile.name) >= 0) { continue; }
		const auto read_deadzone = [&entry](const char* key, int& destination)
		{
			try
			{
				const int value = entry->integer(key);
				if (value >= -1 && value <= 90) { destination = value; }
			}
			catch (const std::runtime_error&) {}
		};
		read_deadzone("movement_deadzone", profile.sticks.movement_deadzone);
		read_deadzone("aim_deadzone", profile.sticks.aim_deadzone);
		try { profile.rumble = entry->boolean("rumble"); }
		catch (const std::runtime_error&) {}
		const auto read_seat_switch = [&entry](const char* key, SeatSwitch& destination)
		{
			try
			{
				const std::optional<SeatSwitch> value = seat_switch_from_key(entry->string(key));
				if (value) { destination = *value; }
			}
			catch (const std::runtime_error&) {}
		};
		read_seat_switch("view_shake", profile.comfort.view_shake);
		read_seat_switch("aim_marker", profile.comfort.aim_marker);
		read_seat_switch("reduce_effects", profile.comfort.reduce_effects);
		try { profile.buttons = read_pad_profile(entry->object("buttons")); }
		catch (const std::runtime_error&) {}
		catch (const std::invalid_argument&) {}
		try
		{
			const int boost = entry->integer("boost");
			if (handicaps::valid(boost)) { profile.boost = boost; }
		}
		catch (const std::runtime_error&) {}
		data.profiles.list.push_back(std::move(profile));
	}
}

void PlayerProfilesSection::write(SaveWriter& writer, const SaveData& data) const
{
	writer.key(this->key());
	writer.begin_array();
	for (const PlayerProfile& profile : data.profiles.list)
	{
		writer.begin_object();
		writer.key("name");
		writer.string(profile.name);
		writer.key("movement_deadzone");
		writer.integer(profile.sticks.movement_deadzone);
		writer.key("aim_deadzone");
		writer.integer(profile.sticks.aim_deadzone);
		writer.key("rumble");
		writer.boolean(profile.rumble);
		writer.key("view_shake");
		writer.string(seat_switch_key(profile.comfort.view_shake));
		writer.key("aim_marker");
		writer.string(seat_switch_key(profile.comfort.aim_marker));
		writer.key("reduce_effects");
		writer.string(seat_switch_key(profile.comfort.reduce_effects));
		writer.key("buttons");
		write_pad_profile(writer, profile.buttons);
		writer.key("boost");
		writer.integer(profile.boost);
		writer.end_object();
	}
	writer.end_array();
}
