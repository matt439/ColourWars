#include "game/pch.h"
#include "game/objects/match_voices.h"

using namespace labrador;

MatchVoice::MatchVoice(const std::string& sound_bank_name,
	const AudioResources* audio_resources, const std::string& effect_name) :
	colourwars::SoundBankObject(sound_bank_name, audio_resources)
{
	if (!effect_name.empty())
	{
		this->effect_ = this->resolve_effect(effect_name);
	}
}

bool MatchVoice::valid() const
{
	return this->effect_.valid();
}

bool MatchVoice::audible() const
{
	return this->sound_bank()->audible();
}

void MatchVoice::play(bool loop, float volume)
{
	if (this->valid()) { this->play_effect(this->effect_, loop, volume); }
}

void MatchVoice::set_volume(float volume)
{
	if (this->valid()) { this->set_effect_volume(this->effect_, volume); }
}

SoundState MatchVoice::state() const
{
	return this->valid() ? this->effect_state(this->effect_) : SoundState::stopped;
}

void MatchVoice::pause()
{
	if (this->valid()) { this->pause_effect(this->effect_); }
}

void MatchVoice::resume()
{
	if (this->valid()) { this->resume_effect(this->effect_); }
}

void MatchVoice::stop()
{
	if (this->valid()) { this->stop_effect(this->effect_, true); }
}

void MatchVoices::add(MatchVoice& voice)
{
	this->voices_.push_back(&voice);
}

void MatchVoices::pause()
{
	for (MatchVoice* voice : this->voices_) { voice->pause(); }
}

void MatchVoices::resume()
{
	for (MatchVoice* voice : this->voices_) { voice->resume(); }
}

void MatchVoices::stop()
{
	for (MatchVoice* voice : this->voices_) { voice->stop(); }
}
