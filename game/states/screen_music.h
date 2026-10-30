#pragma once

#include "game/carried/sound_bank_object.h"
#include "game/objects/music_envelope.h"
#include "game/play_settings.h"
#include "game/presentation.h"

// One screen owner's music voice. Page navigation borrows its clock instead
// of replaying the shared bank instance; an exit waits for silence.
class ScreenMusic : public colourwars::SoundBankObject
{
public:
	ScreenMusic(const std::string& bank, const labrador::AudioResources* resources,
		const PlaySettings* preferences, const PresentationSound& music,
		const MusicMixPresentation& mix, bool loop);
	~ScreenMusic();
	void update(float dt);
	void suspend();
	void resume();
	void fade_out();
	bool finished() const;
private:
	const PlaySettings* preferences_ = nullptr;
	PresentationSound music_;
	MusicMixPresentation mix_;
	labrador::SoundBank::EffectHandle effect_;
	MusicEnvelope envelope_;
	bool loop_ = false;
	bool leaving_ = false;
	void apply_gain();
	bool restart_lost_loop();
};
