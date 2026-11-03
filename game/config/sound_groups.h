#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/config/play_settings.h"
#include "game/config/presentation/presentation.h"
#include "game/config/tuning.h"

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

	// What assign would throw if `candidate` joined `stages`, without assigning
	// anything: a user stage (game/config/stage_loading.h) that names a wave as music
	// which the content plays as an effect is refused before it joins.
	void check_candidate(const GameTuning& tuning, const Presentation& presentation,
		const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
		const LevelLoadedInfo& candidate, const Stage& stage);

	// Sets both groups from the player's settings, or to zero while muted.
	// Throws std::out_of_range before assign has added them.
	void apply(labrador::AudioResources& audio, const PlaySettings& settings);

	// A process-wide quiet switch for test runs, off unless something turns it
	// on: never a player setting, never saved. While it is on, apply holds both
	// groups at zero whatever the settings say, and assign puts the sounds in
	// neither group (the volume preview) in a third group held at zero, so a
	// muted process makes no sound at all. Set it before assign.
	void set_muted(bool muted);
	bool muted();

	// Whether COLOURWARS_MUTE is set to anything but empty or "0". The game
	// shell and the probes that open a real audio device ask this once at
	// startup; the check scripts set it so their game processes stay silent.
	bool muted_by_environment();
}
