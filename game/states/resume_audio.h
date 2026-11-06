#pragma once

#include "game/carried/sound_bank_object.h"
#include "game/config/presentation/presentation.h"

// One owned voice for the shared pause/reconnect count-in. Its owner cancels
// it when the count-in is interrupted, so a later resume starts a fresh beat.
class ResumeAudio : public colourwars::SoundBankObject
{
public:
	ResumeAudio(const labrador::AudioResources* resources, const Presentation& presentation);
	~ResumeAudio();
	// Owns a playing effect: a copy would stop the same voice twice.
	ResumeAudio(const ResumeAudio&) = delete;
	ResumeAudio& operator=(const ResumeAudio&) = delete;
	void update(float remaining);
	void stop();
private:
	PresentationSound sound_;
	labrador::SoundBank::EffectHandle effect_;
	float beat_ = 0.0f;
};
