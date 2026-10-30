#include "game/pch.h"
#include "game/save.h"
#include <stdexcept>
#include <string>
#include <utility>

const PlaySettings& Save::saved_play_settings() const
{
    return this->save_data_.play;
}

const InputBindings& Save::saved_input_bindings() const
{
    return this->save_data_.input_bindings;
}

PlaySettings Save::checked_play_settings(const PlaySettings& settings)
{
    if (settings.aim_sensitivity < 1 || settings.aim_sensitivity > 5 ||
        settings.music_volume < 0 || settings.music_volume > 100 ||
        settings.sfx_volume < 0 || settings.sfx_volume > 100)
    {
        throw std::invalid_argument("Play settings are outside their supported range");
    }
    PlaySettings normalized = settings;
    for (StickSettings& profile : normalized.sticks)
    {
        profile.movement_deadzone = StickSettings::normalized_deadzone(profile.movement_deadzone);
        profile.aim_deadzone = StickSettings::normalized_deadzone(profile.aim_deadzone);
    }
    return normalized;
}

void Save::set_play_settings(const PlaySettings& settings)
{
    this->save_data_.play = checked_play_settings(settings);
}

void Save::set_input_bindings(const InputBindings& bindings)
{
    bindings.validate();
    this->save_data_.input_bindings = bindings;
}

const PlayerProfiles& Save::profiles() const
{
    return this->save_data_.profiles;
}

PlayerProfile& Save::profile_at(int profile)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profile < 0 || profile >= static_cast<int>(profiles.list.size()))
    {
        throw std::invalid_argument("Unknown profile");
    }
    return profiles.list[static_cast<size_t>(profile)];
}

int Save::add_profile_and_save(PlayerProfile profile, bool& saved)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profiles.full()) { throw std::invalid_argument("The profile list is full"); }
    profile.name = player_profiles::normalized_name(profile.name);
    if (profiles.find(profile.name) >= 0) { throw std::invalid_argument("Another profile has that name"); }
    profiles.list.push_back(std::move(profile));
    saved = this->save_to_file();
    return static_cast<int>(profiles.list.size()) - 1;
}

bool Save::rename_profile_and_save(int profile, const std::string& name)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profile < 0 || profile >= static_cast<int>(profiles.list.size()))
    {
        throw std::invalid_argument("Unknown profile");
    }
    const std::string normalized = player_profiles::normalized_name(name);
    const int existing = profiles.find(normalized);
    if (existing >= 0 && existing != profile) { throw std::invalid_argument("Another profile has that name"); }
    profiles.list[static_cast<size_t>(profile)].name = normalized;
    return this->save_to_file();
}

bool Save::set_profile_boost_and_save(int profile, int boost)
{
    this->set_profile_boost(profile, boost);
    return this->save_to_file();
}

void Save::set_profile_boost(int profile, int boost)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profile < 0 || profile >= static_cast<int>(profiles.list.size()) || !handicaps::valid(boost))
    {
        throw std::invalid_argument("Unknown profile or Boost");
    }
    profiles.list[static_cast<size_t>(profile)].boost = boost;
}

void Save::set_profile_controls(int profile, const StickSettings& sticks, bool rumble,
    const SeatComfort& comfort)
{
    PlayerProfile& worn = this->profile_at(profile);
    worn.sticks = sticks;
    worn.rumble = rumble;
    worn.comfort = comfort;
}

void Save::set_profile_buttons(int profile, const PadProfile& buttons)
{
    this->profile_at(profile).buttons = buttons;
}

bool Save::delete_profile_and_save(int profile)
{
    this->profile_at(profile);
    PlayerProfiles& profiles = this->save_data_.profiles;
    profiles.list.erase(profiles.list.begin() + profile);
    return this->save_to_file();
}
