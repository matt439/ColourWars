#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "engine/render/render_resources.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/tuning.h"

// Every startup check that needs the loaded assets: tuning and presentation
// resource names, each stage's object construction and match sounds, and the
// authored text against its fonts. Throws with source context on the first
// problem. Shared by wWinMain and the Debug `reload` command, which runs it on
// candidate content before swapping it in.
void validate_game_content(const GameTuning& tuning, const Presentation& presentation,
	labrador::RenderResources* render, const labrador::AudioResources* audio,
	const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings);
