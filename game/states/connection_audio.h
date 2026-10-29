#pragma once

#include "game/carried/sound_bank_object.h"
#include "game/input_slots.h"
#include "game/presentation.h"
#include <array>

// Shared by the match and all of its menu pages, so replacing a page cannot
// repeat a lost-pad warning or forget a pad still waiting to return.
class ConnectionAudio : public colourwars::SoundBankObject
{
public:
	using Pads = std::array<bool, input_slots::pad_count>;
	ConnectionAudio(const labrador::AudioResources* resources, const Presentation& presentation);
	// Only participating, human-driven pads are eligible. Unclaimed devices,
	// keyboard/mouse and debug-driven seats never produce a connection cue.
	void update(const Pads& eligible, const Pads& connected);
private:
	Pads missing_{};
	PresentationSound lost_;
	PresentationSound back_;
	labrador::SoundBank::WaveHandle lost_wave_;
	labrador::SoundBank::WaveHandle back_wave_;
};
