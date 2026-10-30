#pragma once

#include "game/paint_owners.h"
#include "game/tuning.h"
#include "game/carried/sound_bank_object.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"

// One match's paint feedback. All surfaces and projectile kinds share each
// paint owner's limits; variants never consume the gameplay random stream.
class PaintAudio : public colourwars::SoundBankObject
{
public:
    PaintAudio(const PaintTuning* tuning, const labrador::AudioResources* audio);

    // Called once before the match's contact phases. Paused/nonpositive steps
    // neither age the limits nor permit cues, and suppressed hits are discarded.
    void update(float dt);
    void contact(ProjectileType type, int owner, bool painted, bool captured);

private:
    struct Owner
    {
        float impact_remaining = 0.0f;
        float capture_remaining = 0.0f;
        size_t next_splash = 0;
    };

    const PaintTuning* tuning_;
    std::vector<labrador::SoundBank::WaveHandle> splashes_;
    std::array<labrador::SoundBank::WaveHandle,
        static_cast<size_t>(ProjectileType::count)> impacts_;
    labrador::SoundBank::WaveHandle capture_;
    labrador::SoundBank::WaveHandle dud_;
    // Invalid/uncredited owners share a bounded fallback rather than indexing
    // outside the roster. Device-less players have their own paint owner IDs.
    std::array<Owner, paint_owners::count + 1> owners_{};
    bool emitting_ = false;
};
