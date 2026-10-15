#pragma once

#include "engine/audio/sound_bank_object.h"
#include "game/play_settings.h"
#include "game/presentation.h"

// One owned voice for the shared pause/reconnect count-in. Its owner cancels
// it when the count-in is interrupted, so a later resume starts a fresh beat.
class ResumeAudio : public labrador::SoundBankObject
{
public:
	ResumeAudio(const labrador::AudioResources* resources, const PlaySettings* preferences,
		const Presentation& presentation);
	~ResumeAudio();
	void update(float remaining);
	void stop();
private:
	const PlaySettings* preferences_ = nullptr;
	PresentationSound sound_;
	labrador::SoundBank::EffectHandle effect_;
	float beat_ = 0.0f;
};
