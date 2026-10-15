#pragma once

#include "game/play_settings.h"
#include "game/tuning.h"
#include "engine/audio/sound_bank.h"

// One player's movement cues. Completed contacts drive this once per tick;
// drawing and the number of views never advance its variation or rate limits.
class MovementAudio
{
public:
    MovementAudio(const PlayerMovementSounds* tuning, labrador::SoundBank* bank,
        const PlaySettings* preferences, int player_num);

    void end_contacts(bool can_act, bool supported, bool grounded,
        float impact_speed, float distance, float previous_velocity, float input, float dt);
    void reset();

private:
    const PlayerMovementSounds* tuning_;
    labrador::SoundBank* bank_;
    const PlaySettings* preferences_;
    std::vector<labrador::SoundBank::WaveHandle> land_;
    std::vector<labrador::SoundBank::WaveHandle> step_;
    labrador::SoundBank::WaveHandle skid_;
    size_t land_index_ = 0;
    size_t step_index_ = 0;
    bool established_support_ = false;
    bool had_support_ = false;
    bool braking_ = false;
    float distance_ = 0.0f;
    float step_delay_ = 0.0f;
    float skid_delay_ = 0.0f;

    void play(labrador::SoundBank::WaveHandle wave, const SoundTuning& tuning,
        float strength = 1.0f) const;
};
