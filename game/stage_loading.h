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
#include <set>
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

// ROAD#16: the player's stage folder for a whole session. wWinMain owns one
// above the Application and attaches it once the manifest has loaded; scan()
// is the startup read. rescan() rereads the folder in place of the user stages
// already loaded, so a stage saved or edited while the game runs plays without
// a restart. Its caller guarantees that no match holds a definition: GameMenu
// lends it to Stage Select, which calls it as it opens from the main menu and
// never between series rounds. Manifest stages stay first and untouched; user
// stages follow in file-name order. A file that vanished, or now fails its
// checks, leaves the list; its old definition stays in the registry,
// unreachable from the list, because the registry has no remove.
// Unattached, both do nothing.
class UserStageFolder
{
public:
	UserStageFolder() = default;
	UserStageFolder(const UserStageFolder&) = delete;
	UserStageFolder& operator=(const UserStageFolder&) = delete;

	// All borrowed, and outliving this. `audio` is the checks' audio, mutable
	// so a rescan can regroup the sounds.
	void attach(std::filesystem::path directory, const UserStageChecks& checks,
		labrador::AudioResources* audio, labrador::Registry<LevelLoadedInfo>* level_infos,
		StageList* stages);
	bool attached() const;

	// Startup: before validate_game_content and sound_groups::assign, which
	// wWinMain then runs over the whole list. Prints every refusal on stderr.
	UserStageScan scan();

	// Later: replaces the user stages and regroups the sounds. Prints only the
	// refusals the previous scan did not, so reopening Stage Select is quiet.
	UserStageScan rescan();
	// Latest completed startup/rescan result, including refusals already reported
	// on stderr. An empty successful scan clears earlier refusals.
	const UserStageScan& latest_scan() const;

private:
	std::filesystem::path directory_;
	UserStageChecks checks_;
	labrador::AudioResources* audio_ = nullptr;
	labrador::Registry<LevelLoadedInfo>* level_infos_ = nullptr;
	StageList* stages_ = nullptr;
	std::set<std::string> reported_;
	UserStageScan latest_scan_;

	UserStageScan read_into(StageList& stages) const;
	void report(const UserStageScan& scan);
};
