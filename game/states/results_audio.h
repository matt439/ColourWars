#pragma once

#include "engine/audio/sound_bank_object.h"
#include "game/play_settings.h"
#include "game/presentation.h"

// The accepted results coda is a one-shot. GameLevel owns it across both
// results pages so moving to the rematch menu preserves its natural tail.
class ResultsAudio : public labrador::SoundBankObject
{
public:
	ResultsAudio(const labrador::AudioResources* resources, const PlaySettings* preferences,
		const Presentation& presentation);
	~ResultsAudio();
	void suspend();
	void resume();
private:
	const PlaySettings* preferences_ = nullptr;
	PresentationSound music_;
	labrador::SoundBank::EffectHandle effect_;
};
