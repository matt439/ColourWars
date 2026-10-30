#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "engine/render/render_resources.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/tuning.h"

class LevelObjectBuilder;

// Every startup check that needs the loaded assets: tuning and presentation
// resource names, each stage's object construction, match sounds and
// anti-tunnelling bound (game/tunnelling_limits.h), and the authored text
// against its fonts. Throws with source context on the first
// problem. Nothing here names a menu page: validate_shell_content
// (game/states/menu_content_validation.h) runs this and then the Mode page's
// row checks, and is what wWinMain and the Debug `reload` command run, the
// latter on candidate content before swapping it in.
void validate_game_content(const GameTuning& tuning, const Presentation& presentation,
	labrador::RenderResources* render, const labrador::AudioResources* audio,
	const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings);

// validate_game_content's two per-stage halves, in the order it runs them: the
// built objects, their anti-tunnelling bound, team gates clear of every spawn
// body and the stage's match sounds; then
// the display name and practice lessons against their fonts. A user stage
// (game/stage_loading.h) passes both before it joins the stage list, so the
// whole-list check above cannot then fail on it.
void validate_stage_objects(const GameTuning& tuning, const LevelObjectBuilder& builder,
	labrador::RenderResources& render, const labrador::AudioResources& audio,
	const LevelLoadedInfo& info);
void validate_stage_text(const GameTuning& tuning, const Presentation& presentation,
	labrador::RenderResources* render, const LevelLoadedInfo& info, const Stage& stage);
