#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/tuning.h"

// The Music and SFX volumes of the Controls page, as two engine sound groups
// (engine/audio/sound_group.h). The engine multiplies a sound's group volume
// into every level it sends, so an emitter plays at its authored level and
// never reads PlaySettings, and a paused effect takes a new volume without
// anyone re-pushing it on resume.
namespace sound_groups
{
	// Load-time, after the manifest and validate_game_content. Adds the two
	// groups to `audio` the first time and puts every sound the content names
	// in one: the stages' music and end stings and the menu and results music
	// in music, everything else in sfx (SoundReference lists them), except the
	// Controls volume preview, which stays in neither. Safe to call again on
	// the same AudioResources: an assignment moves a sound, never doubles it.
	void assign(labrador::AudioResources& audio, const GameTuning& tuning,
		const Presentation& presentation,
		const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages);

	// Sets both groups from the player's settings. Throws std::out_of_range
	// before assign has added them.
	void apply(labrador::AudioResources& audio, const PlaySettings& settings);
}
