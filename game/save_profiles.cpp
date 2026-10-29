#include "game/pch.h"
#include "game/save.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace
{
    void copy_slot(PlaySettings& to, const PlaySettings& from, size_t slot)
    {
        to.sticks[slot] = from.sticks[slot];
        to.pad_rumble[slot] = from.pad_rumble[slot];
        to.pad_comfort[slot] = from.pad_comfort[slot];
    }
}

const PlaySettings& Save::play_settings() const
{
    return this->play_;
}

bool Save::set_play_settings_and_save(const PlaySettings& settings)
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
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        PlayerProfile& profile = this->save_data_.profiles.list[static_cast<size_t>(this->worn_[slot])];
        profile.sticks = normalized.sticks[slot];
        profile.rumble = normalized.pad_rumble[slot];
        profile.comfort = normalized.pad_comfort[slot];
        copy_slot(normalized, this->save_data_.play, slot);
    }
    this->save_data_.play = normalized;
    this->refresh_effective();
    return this->save_to_file();
}

const InputBindings& Save::input_bindings() const
{
    return this->bindings_;
}

bool Save::set_input_bindings_and_save(const InputBindings& bindings)
{
    bindings.validate();
    InputBindings own = bindings;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        this->save_data_.profiles.list[static_cast<size_t>(this->worn_[slot])].buttons = bindings.pads[slot];
        own.pads[slot] = this->save_data_.input_bindings.pads[slot];
    }
    this->save_data_.input_bindings = own;
    this->refresh_effective();
    return this->save_to_file();
}

void Save::refresh_effective()
{
    this->play_ = this->save_data_.play;
    this->bindings_ = this->save_data_.input_bindings;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        const PlayerProfile& profile = this->save_data_.profiles.list[static_cast<size_t>(this->worn_[slot])];
        this->play_.sticks[slot] = profile.sticks;
        this->play_.pad_rumble[slot] = profile.rumble;
        this->play_.pad_comfort[slot] = profile.comfort;
        this->bindings_.pads[slot] = profile.buttons;
    }
}

const PlayerProfiles& Save::profiles() const
{
    return this->save_data_.profiles;
}

int Save::worn_profile(int slot) const
{
    return input_slots::is_pad(slot) ? this->worn_[static_cast<size_t>(slot)] : -1;
}

int Save::profile_wearer(int profile) const
{
    if (profile < 0) { return -1; }
    const auto found = std::find(this->worn_.begin(), this->worn_.end(), profile);
    return found == this->worn_.end() ? -1 : static_cast<int>(std::distance(this->worn_.begin(), found));
}

bool Save::wear_profile(int slot, int profile)
{
    if (!input_slots::is_pad(slot) || profile < -1 ||
        profile >= static_cast<int>(this->save_data_.profiles.list.size()))
    {
        return false;
    }
    const int wearer = this->profile_wearer(profile);
    if (wearer >= 0 && wearer != slot) { return false; }
    this->worn_[static_cast<size_t>(slot)] = profile;
    this->refresh_effective();
    return true;
}

int Save::next_wearable_profile(int slot, int step) const
{
    const int count = static_cast<int>(this->save_data_.profiles.list.size());
    if (!input_slots::is_pad(slot) || count == 0) { return -1; }
    // Positions 0..count-1 are profiles and position count is "none".
    const int current = this->worn_profile(slot);
    int position = current < 0 ? count : current;
    const int direction = step < 0 ? -1 : 1;
    for (int tried = 0; tried <= count; ++tried)
    {
        position = (position + direction + count + 1) % (count + 1);
        if (position == count) { return -1; }
        const int wearer = this->profile_wearer(position);
        if (wearer < 0 || wearer == slot) { return position; }
    }
    return -1;
}

int Save::add_profile_and_save(const std::string& name, int slot, bool& saved)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profiles.full()) { throw std::invalid_argument("The profile list is full"); }
    PlayerProfile profile;
    profile.name = player_profiles::normalized_name(name);
    if (profiles.find(profile.name) >= 0) { throw std::invalid_argument("Another profile has that name"); }
    if (input_slots::is_pad(slot))
    {
        const size_t pad = static_cast<size_t>(slot);
        profile.sticks = this->play_.sticks[pad];
        profile.rumble = this->play_.pad_rumble[pad];
        profile.comfort = this->play_.pad_comfort[pad];
        profile.buttons = this->bindings_.pads[pad];
    }
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
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profile < 0 || profile >= static_cast<int>(profiles.list.size()) || !handicaps::valid(boost))
    {
        throw std::invalid_argument("Unknown profile or Boost");
    }
    profiles.list[static_cast<size_t>(profile)].boost = boost;
    return this->save_to_file();
}

bool Save::delete_profile_and_save(int profile)
{
    PlayerProfiles& profiles = this->save_data_.profiles;
    if (profile < 0 || profile >= static_cast<int>(profiles.list.size()))
    {
        throw std::invalid_argument("Unknown profile");
    }
    profiles.list.erase(profiles.list.begin() + profile);
    for (int& worn : this->worn_)
    {
        if (worn == profile) { worn = -1; }
        else if (worn > profile) { --worn; }
    }
    this->refresh_effective();
    return this->save_to_file();
}

bool Save::remember_profile_boosts_and_save(const std::array<int, input_slots::pad_count>& boosts)
{
    bool changed = false;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0 || !handicaps::valid(boosts[slot])) { continue; }
        int& boost = this->save_data_.profiles.list[static_cast<size_t>(this->worn_[slot])].boost;
        changed = changed || boost != boosts[slot];
        boost = boosts[slot];
    }
    return !changed || this->save_to_file();
}
