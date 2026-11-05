#pragma once

#include "engine/assets/json.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/registry.h"
#include "engine/render/render_resources.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/config/play_settings.h"
#include "game/config/presentation/presentation.h"
#include "game/config/tuning.h"

class LevelObjectBuilder;

// Every startup check that needs the loaded assets: tuning and presentation
// resource names, each stage's object construction, match sounds and
// anti-tunnelling bound (game/config/tunnelling_limits.h), and the authored text
// against its fonts. Throws with source context on the first
// problem. Nothing here names a menu page: validate_shell_content
// (game/states/menu_content_validation.h) runs this and then the Mode page's
// row checks, and is what wWinMain and the Debug `reload` command run, the
// latter on candidate content before swapping it in.
void validate_game_content(const GameTuning& tuning, const Presentation& presentation,
	labrador::RenderResources* render, const labrador::AudioResources* audio,
	const labrador::Registry<LevelLoadedInfo>& level_infos, const StageList& stages,
	const PlaySettings& play_settings);

// presentation.json's menu_text.stage_names against the stage list: an
// entry for every shipped (non-user) stage and none for anything else. A
// shipped name may not start with '_', which the table would read as a
// comment. validate_game_content runs it on every Presentation, whatever its
// locale.
void validate_stage_names(const Presentation& presentation, const StageList& stages);

// That the base presentation.json's menu_text.stage_names entry for each
// shipped stage is its level file's display_name, byte for byte. Judged on
// the base document, never a composed locale, so it runs where that document
// is at hand: wWinMain and the Debug `reload` read the file at
// `presentation_path`, the Debug presentation `tune` target passes its
// candidate. A stage without an entry is validate_stage_names' to refuse.
void check_stage_names_agree(const labrador::JsonValue& base_presentation, const StageList& stages);
void check_stage_names_agree(const std::string& presentation_path, const StageList& stages);

// The in-match controls reminder: presentation.json's menu_text.controls
// copy, formatted for every keyboard and pad choice, drawn and measured in
// presentation.json's match.controls_text.font. validate_game_content runs it,
// and the Debug `tune` target runs it on a candidate presentation.
void validate_controls_reminders(const Presentation& presentation,
	const labrador::RenderResources& render);

// validate_game_content's two per-stage halves, in the order it runs them: the
// built objects, their anti-tunnelling bound, team gates clear of every spawn
// body and the stage's match sounds; then
// the display name and practice lessons against their fonts. A user stage
// (game/config/stage_loading.h) passes both before it joins the stage list, so the
// whole-list check above cannot then fail on it.
void validate_stage_objects(const GameTuning& tuning, const LevelObjectBuilder& builder,
	labrador::RenderResources& render, const labrador::AudioResources& audio,
	const LevelLoadedInfo& info);
void validate_stage_text(const Presentation& presentation,
	labrador::RenderResources* render, const LevelLoadedInfo& info, const Stage& stage);
