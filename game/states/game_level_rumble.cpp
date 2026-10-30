#include "game/pch.h"
#include "game/states/game_states.h"
#include "game/rumble_events.h"
#include <cmath>

void GameLevel::stop_rumble() const noexcept
{
    if (this->rumble_ != nullptr)
    {
        this->rumble_->set_session({ false, false, true });
    }
    if (this->level_ != nullptr) { this->level_->clear_rumble_events(); }
}

void GameLevel::sync_rumble()
{
    if (this->rumble_ == nullptr) { return; }
    // An instant replay's hits were felt when they happened.
    const bool playing = this->level_ != nullptr && this->level_->state() == LevelState::active &&
        this->replay_phase_ == ReplayPhase::none;
    const bool paused = this->covered_ || this->pause_menu_open_ || this->resume_countdown_.active() ||
        this->data()->debug().match_audio_paused();
    const bool focused = this->context()->active();
    this->rumble_->set_session({ playing, focused, paused });
    for (int device = 0; device < input_slots::pad_count; ++device)
    {
        // Motors are physical; the seat a pad drives may have started on
        // another slot (ROAD#4), and its preference and driver stay with it.
        const int player = this->settings_.player_index_for_device(device);
        const int slot = player == -1 ? device : this->settings_.player_settings()[player].num;
        // The public driver gate covers both bots and active/armed replays.
        // No private Debug state or replay format leaks into the match owner.
        const bool automated = this->data()->debug().drives_slot(slot);
        this->rumble_->set_slot(device, { player != -1 && this->data()->gamepads()->connected(device),
            this->data()->settings_session()->play_settings().rumble_enabled(slot), automated, false });
    }
    if (this->level_ != nullptr && (!playing || !focused || paused))
    {
        this->level_->clear_rumble_events();
    }
}

void GameLevel::drain_rumble(float dt)
{
    if (this->rumble_ == nullptr || this->level_ == nullptr) { return; }
    this->sync_rumble();
    // Consume every source, including blocked, keyboard and unclaimed seats.
    // A zero step cannot release a cue accumulated outside live simulation.
    const auto records = this->level_->take_rumble_events();
    if (!(dt > 0.0f) || !std::isfinite(dt)) { return; }
    for (const SlotRumbleEvents& record : records)
    {
        if (!input_slots::is_pad(record.input_slot)) { continue; }
        for (const RumbleEvent event : { RumbleEvent::hit, RumbleEvent::elimination,
            RumbleEvent::hard_landing, RumbleEvent::dry_fire })
        {
            if (record.events.contains(event))
            {
                this->rumble_->trigger(this->settings_.device_slot(record.input_slot), event);
            }
        }
    }
}
