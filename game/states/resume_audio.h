#pragma once

#include "engine/audio/sound_bank_object.h"
#include "game/presentation.h"

// One owned voice for the shared pause/reconnect count-in. Its owner cancels
// it when the count-in is interrupted, so a later resume starts a fresh beat.
class ResumeAudio : public labrador::SoundBankObject
{
public:
	ResumeAudio(const labrador::AudioResources* resources, const Presentation& presentation);
	~ResumeAudio();
	void update(float remaining);
	void stop();
private:
	PresentationSound sound_;
	labrador::SoundBank::EffectHandle effect_;
	float beat_ = 0.0f;
};
