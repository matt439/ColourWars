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
    const bool playing = this->level_ != nullptr && this->level_->state() == LevelState::active;
    const bool paused = this->covered_ || this->pause_menu_open_ || this->resume_countdown_.active() ||
        this->data()->debug().match_audio_paused();
    const bool focused = this->context()->active();
    this->rumble_->set_session({ playing, focused, paused });
    for (int slot = 0; slot < input_slots::pad_count; ++slot)
    {
        const bool claimed = this->settings_.player_index_for_slot(slot) != -1;
        // The public driver gate covers both bots and active/armed replays.
        // No private Debug state or replay format leaks into the match owner.
        const bool automated = this->data()->debug().drives_slot(slot);
        this->rumble_->set_slot(slot, { claimed && this->data()->gamepads()->connected(slot),
            this->data()->save()->play_settings().rumble_enabled(slot), automated, false });
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
            if (record.events.contains(event)) { this->rumble_->trigger(record.input_slot, event); }
        }
    }
}
