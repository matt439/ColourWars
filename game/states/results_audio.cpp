#include "game/pch.h"
#include "game/states/results_audio.h"

ResultsAudio::ResultsAudio(const labrador::AudioResources* resources,
	const PlaySettings* preferences, const Presentation& presentation) :
	SoundBankObject(presentation.menu().sound_bank, resources), preferences_(preferences),
	music_(presentation.results_menu().music)
{
	this->effect_ = this->resolve_effect(this->music_.name);
	this->play_effect(this->effect_, false, this->music_.volume * this->preferences_->music_gain());
}

ResultsAudio::~ResultsAudio()
{
	this->stop_effect(this->effect_, true);
}

void ResultsAudio::suspend()
{
	this->pause_effect(this->effect_);
}

void ResultsAudio::resume()
{
	this->set_effect_volume(this->effect_, this->music_.volume * this->preferences_->music_gain());
	// A naturally finished coda stays finished when focus returns.
	if (this->effect_state(this->effect_) == labrador::SoundState::paused)
	{
		this->resume_effect(this->effect_);
	}
}
