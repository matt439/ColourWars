#include "game/pch.h"
#include "game/objects/paint_audio.h"

PaintAudio::PaintAudio(const PaintTuning* tuning,
    const labrador::AudioResources* audio, const PlaySettings* preferences) :
    tuning_(tuning), preferences_(preferences), bank_(audio->sound_bank(tuning->sound_bank))
{
    for (const SoundTuning& sound : tuning->splash_sounds)
    {
        this->splashes_.push_back(this->bank_->resolve_wave(sound.name));
    }
    for (size_t i = 0; i < this->impacts_.size(); ++i)
    {
        this->impacts_[i] = this->bank_->resolve_wave(tuning->impact_sounds[i].name);
    }
    this->capture_ = this->bank_->resolve_wave(tuning->capture_sound.name);
    this->dud_ = this->bank_->resolve_wave(tuning->dud_sound.name);
    for (size_t i = 0; i < this->owners_.size(); ++i)
    {
        this->owners_[i].next_splash = i % this->splashes_.size();
    }
}

void PaintAudio::update(float dt)
{
    this->emitting_ = dt > 0.0f;
    if (!this->emitting_) { return; }
    for (Owner& owner : this->owners_)
    {
        owner.impact_remaining = std::max(0.0f, owner.impact_remaining - dt);
        owner.capture_remaining = std::max(0.0f, owner.capture_remaining - dt);
    }
}

void PaintAudio::contact(ProjectileType type, int owner, bool painted, bool captured)
{
    if (!this->emitting_) { return; }
    const size_t index = owner >= 0 && owner < paint_owners::count ?
        static_cast<size_t>(owner) : static_cast<size_t>(paint_owners::count);
    Owner& state = this->owners_[index];
    const float gain = this->preferences_->sfx_gain();
    if (state.impact_remaining <= 0.0f)
    {
        if (painted)
        {
            const size_t kind = static_cast<size_t>(type);
            this->bank_->play_wave(this->impacts_.at(kind),
                this->tuning_->impact_sounds.at(kind).volume * gain);
            const size_t variant = state.next_splash;
            this->bank_->play_wave(this->splashes_[variant],
                this->tuning_->splash_sounds[variant].volume * gain);
            state.next_splash = (variant + 1) % this->splashes_.size();
        }
        else
        {
            this->bank_->play_wave(this->dud_, this->tuning_->dud_sound.volume * gain);
        }
        state.impact_remaining = this->tuning_->min_impact_interval;
    }
    // A capture has its own, quieter budget: a friendly repaint cannot consume
    // it, and a prior dud/impact cannot hide a newly changed patch's cue.
    if (painted && captured && state.capture_remaining <= 0.0f)
    {
        this->bank_->play_wave(this->capture_, this->tuning_->capture_sound.volume * gain);
        state.capture_remaining = this->tuning_->min_capture_interval;
    }
}
