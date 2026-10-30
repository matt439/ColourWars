#include "game/pch.h"
#include "game/settings_session.h"
#include "game/save.h"
#include <algorithm>
#include <iterator>
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

SettingsSession::SettingsSession(Save& save)
    : save_(&save)
{
    this->refresh_effective();
}

void SettingsSession::load_save_file()
{
    this->save_->load_save_file();
    this->worn_.fill(-1);
    this->refresh_effective();
}

const PlaySettings& SettingsSession::play_settings() const
{
    return this->play_;
}

bool SettingsSession::set_play_settings_and_save(const PlaySettings& settings)
{
    // Checked before anything changes; the worn slots' values go to their
    // profiles and those slots keep their own.
    const PlaySettings checked = Save::checked_play_settings(settings);
    PlaySettings own = checked;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        copy_slot(own, this->save_->saved_play_settings(), slot);
    }
    this->save_->set_play_settings(own);
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        this->save_->set_profile_controls(this->worn_[slot], checked.sticks[slot],
            checked.pad_rumble[slot], checked.pad_comfort[slot]);
    }
    this->refresh_effective();
    return this->save_->save_to_file();
}

const InputBindings& SettingsSession::input_bindings() const
{
    return this->bindings_;
}

bool SettingsSession::set_input_bindings_and_save(const InputBindings& bindings)
{
    bindings.validate();
    InputBindings own = bindings;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        own.pads[slot] = this->save_->saved_input_bindings().pads[slot];
    }
    this->save_->set_input_bindings(own);
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0) { continue; }
        this->save_->set_profile_buttons(this->worn_[slot], bindings.pads[slot]);
    }
    this->refresh_effective();
    return this->save_->save_to_file();
}

void SettingsSession::refresh_effective()
{
    const PlayerProfiles& profiles = this->save_->profiles();
    this->play_ = this->save_->saved_play_settings();
    this->bindings_ = this->save_->saved_input_bindings();
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        // A profile deleted past the session goes back to the pad's own.
        if (this->worn_[slot] >= static_cast<int>(profiles.list.size())) { this->worn_[slot] = -1; }
        if (this->worn_[slot] < 0) { continue; }
        const PlayerProfile& profile = profiles.list[static_cast<size_t>(this->worn_[slot])];
        this->play_.sticks[slot] = profile.sticks;
        this->play_.pad_rumble[slot] = profile.rumble;
        this->play_.pad_comfort[slot] = profile.comfort;
        this->bindings_.pads[slot] = profile.buttons;
    }
}

int SettingsSession::worn_profile(int slot) const
{
    return input_slots::is_pad(slot) ? this->worn_[static_cast<size_t>(slot)] : -1;
}

int SettingsSession::profile_wearer(int profile) const
{
    if (profile < 0) { return -1; }
    const auto found = std::find(this->worn_.begin(), this->worn_.end(), profile);
    return found == this->worn_.end() ? -1 : static_cast<int>(std::distance(this->worn_.begin(), found));
}

bool SettingsSession::wear_profile(int slot, int profile)
{
    if (!input_slots::is_pad(slot) || profile < -1 ||
        profile >= static_cast<int>(this->save_->profiles().list.size()))
    {
        return false;
    }
    const int wearer = this->profile_wearer(profile);
    if (wearer >= 0 && wearer != slot) { return false; }
    this->worn_[static_cast<size_t>(slot)] = profile;
    this->refresh_effective();
    return true;
}

int SettingsSession::next_wearable_profile(int slot, int step) const
{
    const int count = static_cast<int>(this->save_->profiles().list.size());
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

int SettingsSession::add_profile_and_save(const std::string& name, int slot, bool& saved)
{
    PlayerProfile profile;
    profile.name = name;
    if (input_slots::is_pad(slot))
    {
        const size_t pad = static_cast<size_t>(slot);
        profile.sticks = this->play_.sticks[pad];
        profile.rumble = this->play_.pad_rumble[pad];
        profile.comfort = this->play_.pad_comfort[pad];
        profile.buttons = this->bindings_.pads[pad];
    }
    return this->save_->add_profile_and_save(std::move(profile), saved);
}

bool SettingsSession::delete_profile_and_save(int profile)
{
    // The Save refuses an unknown index before anything changes here.
    const bool saved = this->save_->delete_profile_and_save(profile);
    for (int& worn : this->worn_)
    {
        if (worn == profile) { worn = -1; }
        else if (worn > profile) { --worn; }
    }
    this->refresh_effective();
    return saved;
}

bool SettingsSession::remember_profile_boosts_and_save(const std::array<int, input_slots::pad_count>& boosts)
{
    bool changed = false;
    for (size_t slot = 0; slot < this->worn_.size(); ++slot)
    {
        if (this->worn_[slot] < 0 || !handicaps::valid(boosts[slot])) { continue; }
        const int profile = this->worn_[slot];
        const int boost = this->save_->profiles().list[static_cast<size_t>(profile)].boost;
        changed = changed || boost != boosts[slot];
        this->save_->set_profile_boost(profile, boosts[slot]);
    }
    return !changed || this->save_->save_to_file();
}
