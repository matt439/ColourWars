#pragma once

#include "game/states/screen_music.h"

// The accepted results coda is a one-shot. GameLevel owns it across both
// results pages so moving to the rematch menu preserves its natural tail.
class ResultsAudio : public ScreenMusic
{
public:
	ResultsAudio(const labrador::AudioResources* resources, const PlaySettings* preferences,
		const Presentation& presentation);
};
