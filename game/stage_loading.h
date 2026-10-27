#pragma once

#include "engine/core/registry.h"
#include "engine/audio/audio_resources.h"
#include "engine/render/render_resources.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// A level file read into what the stage list and the level registry hold. The
// manifest's "level" kind and the user-stage scan read stages the same way.
struct StageDefinition
{
	Stage stage;
	std::unique_ptr<LevelLoadedInfo> info;
};

// Reads and structurally validates a level file (LevelLoadedInfo::validate) and
// checks its display_name converts to drawable text. Throws naming the file.
// `path` is UTF-8.
StageDefinition read_stage_definition(const std::string& path, const std::string& name);

// What a user stage is checked against: the content every shipped stage is
// checked against in validate_game_content. All borrowed.
struct UserStageChecks
{
	const GameTuning* tuning = nullptr;
	const Presentation* presentation = nullptr;
	labrador::RenderResources* render = nullptr;
	const labrador::AudioResources* audio = nullptr;
	const PlaySettings* play_settings = nullptr;
};

// One scan's outcome. `accepted` holds the stage names added, in the order
// they joined; `refused` one line per file left out, for stderr.
struct UserStageScan
{
	std::vector<std::string> accepted;
	std::vector<std::string> refused;
};

// ROAD#16: adds every valid *.json directly in `directory` to `stages` and
// `level_infos`, after the manifest's stages, in file-name order. Each file is
// checked on its own with what a shipped stage is checked with (the level
// format, its built objects, sounds, tunnelling, fonts and the arena counter)
// plus a file name the registry can key ([A-Za-z0-9_-], no leading hyphen, at
// most 64 characters) that no stage already has, compared without case. A
// file that fails is refused with one line and the scan goes on; a missing
// folder is no stages and no line. Never throws for what is in the folder.
// User stages are marked `user` and are never Random candidates.
//
// Call after the manifest has loaded and before validate_game_content and
// sound_groups::assign, which then see the accepted stages like any other.
UserStageScan load_user_stages(const std::filesystem::path& directory,
	const UserStageChecks& checks, labrador::Registry<LevelLoadedInfo>& level_infos,
	StageList& stages);
