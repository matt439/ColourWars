#include "game/pch.h"
#include "game/states/connection_audio.h"

ConnectionAudio::ConnectionAudio(const labrador::AudioResources* resources,
	const Presentation& presentation) :
	colourwars::SoundBankObject(presentation.menu().sound_bank, resources),
	lost_(presentation.pause_menu().pad_lost_sound), back_(presentation.pause_menu().pad_back_sound)
{
	this->lost_wave_ = this->resolve_wave(this->lost_.name);
	this->back_wave_ = this->resolve_wave(this->back_.name);
}

void ConnectionAudio::update(const Pads& eligible, const Pads& connected)
{
	bool lost = false;
	bool back = false;
	for (size_t slot = 0; slot < this->missing_.size(); ++slot)
	{
		const bool missing = eligible[slot] && !connected[slot];
		lost = lost || (missing && !this->missing_[slot]);
		back = back || (eligible[slot] && connected[slot] && this->missing_[slot]);
		this->missing_[slot] = missing;
	}
	// One of each kind per sample, even if several pads change together.
	if (lost)
	{
		this->play_wave(this->lost_wave_, this->lost_.volume);
	}
	if (back)
	{
		this->play_wave(this->back_wave_, this->back_.volume);
	}
}
