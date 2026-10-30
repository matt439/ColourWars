#pragma once

#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "engine/render/render_resources.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/tuning.h"

// The startup checks that need a menu page, kept here so the content checks in
// game/content_validation.h never name one. Both build the Mode page's actual
// setup row (MainMenuModeSelect) for every value it can show and refuse a value
// the font cannot draw or a complete row that leaves main_menu.setup_box_size.
// Duration words depend on actual tuning, not a Presentation-only sample.
void validate_mode_durations(labrador::RenderResources* render,
	const GameTuning& tuning, const Presentation& presentation);
// The Series stage row (ROAD#20), checked like the duration row for every rule.
void validate_mode_stage_rules(labrador::RenderResources* render, const Presentation& presentation);

// Everything startup checks before the first menu: validate_game_content, then
// the two Mode row checks above, after all of its diagnostics. Shared by
// wWinMain and the Debug `reload` and level-checking `tune` commands.
void validate_shell_content(const GameTuning& tuning, const Presentation& presentation,
	labrador::RenderResources* render, const labrador::AudioResources* audio,
	const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings);
