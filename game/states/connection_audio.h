#pragma once

#include "engine/audio/sound_bank_object.h"
#include "game/input_slots.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include <array>

// Shared by the match and all of its menu pages, so replacing a page cannot
// repeat a lost-pad warning or forget a pad still waiting to return.
class ConnectionAudio : public labrador::SoundBankObject
{
public:
	using Pads = std::array<bool, input_slots::pad_count>;
	ConnectionAudio(const labrador::AudioResources* resources, const PlaySettings* preferences,
		const Presentation& presentation);
	// Only participating, human-driven pads are eligible. Unclaimed devices,
	// keyboard/mouse and debug-driven seats never produce a connection cue.
	void update(const Pads& eligible, const Pads& connected);
private:
	Pads missing_{};
	const PlaySettings* preferences_ = nullptr;
	PresentationSound lost_;
	PresentationSound back_;
	labrador::SoundBank::WaveHandle lost_wave_;
	labrador::SoundBank::WaveHandle back_wave_;
};
