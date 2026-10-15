#include "game/pch.h"
#include "game/states/results_audio.h"

ResultsAudio::ResultsAudio(const labrador::AudioResources* resources,
	const PlaySettings* preferences, const Presentation& presentation) :
	ScreenMusic(presentation.menu().sound_bank, resources, preferences,
		presentation.results_menu().music, presentation.music_mix(), false)
{
}
