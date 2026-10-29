#include "game/pch.h"
#include "game/states/resume_audio.h"

ResumeAudio::ResumeAudio(const labrador::AudioResources* resources,
	const Presentation& presentation) :
	colourwars::SoundBankObject(presentation.menu().sound_bank, resources),
	sound_(presentation.pause_menu().resume_sound)
{
	this->effect_ = this->resolve_effect(this->sound_.name);
}

ResumeAudio::~ResumeAudio()
{
	this->stop();
}

void ResumeAudio::update(float remaining)
{
	if (!(remaining > 0.0f))
	{
		this->stop();
		return;
	}
	const float beat = std::ceil(remaining);
	if (beat == this->beat_) { return; }
	// Only the currently displayed beat sounds. A long update does not queue
	// missed numbers, and any prior tail is discarded before this fresh play.
	this->stop_effect(this->effect_, true);
	this->play_effect(this->effect_, false, this->sound_.volume);
	this->beat_ = beat;
}

void ResumeAudio::stop()
{
	if (this->beat_ > 0.0f) { this->stop_effect(this->effect_, true); }
	this->beat_ = 0.0f;
}
