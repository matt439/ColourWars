#include "game/pch.h"
#include "game/objects/movement_audio.h"

using namespace labrador;

MovementAudio::MovementAudio(const PlayerMovementSounds* tuning, SoundBank* bank,
    const PlaySettings* preferences, int player_num) :
    tuning_(tuning), bank_(bank), preferences_(preferences)
{
    for (const SoundTuning& sound : tuning->land)
    {
        this->land_.push_back(bank->resolve_wave(sound.name));
    }
    for (const SoundTuning& sound : tuning->step)
    {
        this->step_.push_back(bank->resolve_wave(sound.name));
    }
    for (const SoundTuning& sound : tuning->step_painted)
    {
        this->step_painted_.push_back(bank->resolve_wave(sound.name));
    }
    this->skid_ = bank->resolve_wave(tuning->skid.name);
    this->land_index_ = static_cast<size_t>(std::max(player_num, 0)) % this->land_.size();
    this->step_index_ = static_cast<size_t>(std::max(player_num, 0)) % this->step_.size();
    this->painted_index_ = static_cast<size_t>(std::max(player_num, 0)) % this->step_painted_.size();
}

void MovementAudio::end_contacts(bool can_act, bool supported, bool grounded,
    float impact_speed, float distance, float previous_velocity, float input, float dt, bool painted)
{
    if (dt <= 0.0f)
    {
        return;
    }
    if (!can_act)
    {
        this->reset();
        return;
    }

    this->step_delay_ = std::max(0.0f, this->step_delay_ - dt);
    this->skid_delay_ = std::max(0.0f, this->skid_delay_ - dt);
    if (supported && !this->had_support_ && this->established_support_)
    {
        const float strength = std::clamp(impact_speed / this->tuning_->landing_full_speed,
            this->tuning_->landing_min_gain, 1.0f);
        if (painted)
        {
            this->play_painted(strength);
        }
        else
        {
            this->play(this->land_[this->land_index_], this->tuning_->land[this->land_index_], strength);
            this->land_index_ = (this->land_index_ + 1) % this->land_.size();
        }
    }

    // No step on the landing or takeoff tick. In particular a buffered jump
    // can land and launch during one contact phase, while never walking.
    const bool walking = this->had_support_ && supported && grounded && distance > 0.0f;
    if (walking)
    {
        this->distance_ += distance;
        const bool opposed = previous_velocity * input < 0.0f;
        if (opposed && !this->braking_ && this->skid_delay_ <= 0.0f &&
            std::abs(previous_velocity) >= this->tuning_->skid_min_speed)
        {
            this->play(this->skid_, this->tuning_->skid);
            this->skid_delay_ = this->tuning_->skid_repeat_delay;
        }
        this->braking_ = opposed;
        if (this->distance_ >= this->tuning_->stride_distance && this->step_delay_ <= 0.0f)
        {
            if (painted)
            {
                this->play_painted();
            }
            else
            {
                this->play(this->step_[this->step_index_], this->tuning_->step[this->step_index_]);
                this->step_index_ = (this->step_index_ + 1) % this->step_.size();
            }
            // A long tick can request one step, never a backlog of missed ones.
            this->distance_ = 0.0f;
            this->step_delay_ = this->tuning_->min_step_interval;
        }
    }
    else
    {
        this->distance_ = 0.0f;
        this->braking_ = false;
    }
    this->established_support_ = this->established_support_ || supported;
    this->had_support_ = supported && grounded;
}

void MovementAudio::reset()
{
    this->established_support_ = false;
    this->had_support_ = false;
    this->braking_ = false;
    this->distance_ = 0.0f;
    this->step_delay_ = 0.0f;
    this->skid_delay_ = 0.0f;
}

void MovementAudio::play(SoundBank::WaveHandle wave, const SoundTuning& tuning,
    float strength) const
{
    this->bank_->play_wave(wave, tuning.volume * strength * this->preferences_->sfx_gain());
}

void MovementAudio::play_painted(float strength)
{
    this->play(this->step_painted_[this->painted_index_],
        this->tuning_->step_painted[this->painted_index_], strength);
    this->painted_index_ = (this->painted_index_ + 1) % this->step_painted_.size();
}
