#pragma once

#include "game/tuning.h"
#include "game/carried/sound_bank_object.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include <string>
#include <vector>

// The same completed-contact events drive sound and cosmetic contact flecks.
// Muting a sound changes its gain, never whether the event happened.
struct MovementContactCues
{
    bool step = false;
    bool skid = false;
};

// One player's movement cues. Completed contacts drive this once per tick;
// drawing and the number of views never advance its variation or rate limits.
class MovementAudio : public colourwars::SoundBankObject
{
public:
    MovementAudio(const PlayerMovementSounds* tuning,
        const labrador::AudioResources* audio_resources,
        const std::string& sound_bank_name, int player_num);

    MovementContactCues end_contacts(bool can_act, bool supported, bool grounded,
        float impact_speed, float distance, float previous_velocity, float input, float dt,
        bool painted = false);
    void reset();

private:
    const PlayerMovementSounds* tuning_;
    std::vector<labrador::SoundBank::WaveHandle> land_;
    std::vector<labrador::SoundBank::WaveHandle> step_;
    std::vector<labrador::SoundBank::WaveHandle> step_painted_;
    labrador::SoundBank::WaveHandle skid_;
    size_t land_index_ = 0;
    size_t step_index_ = 0;
    size_t painted_index_ = 0;
    bool established_support_ = false;
    bool had_support_ = false;
    bool braking_ = false;
    float distance_ = 0.0f;
    float step_delay_ = 0.0f;
    float skid_delay_ = 0.0f;

    void play(labrador::SoundBank::WaveHandle wave, const SoundTuning& tuning,
        float strength = 1.0f);
    void play_painted(float strength = 1.0f);
};
