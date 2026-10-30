#pragma once

#include "engine/core/registry.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/stage.h"
#include <memory>
#include <string>

// A level file read into what the catalog holds: the pickable entry and its
// definition. The manifest's "level" kind and the user-stage scan read stages
// the same way (read_stage_definition in game/stage_loading.h).
struct StageDefinition
{
	Stage stage;
	std::unique_ptr<LevelLoadedInfo> info;
};

// The stages this session offers and the level definitions behind them, kept
// in step by being their only writer. The list is the manifest's stages in
// manifest order, then the accepted user stages in file-name order; every
// listed stage's definition is registered under its name. wWinMain owns one
// above the Application; GameData lends it out read-only, and only the
// manifest's "level" kind and UserStageFolder write to it.
//
// A user stage that leaves the list keeps its definition registered,
// unreachable from the list, because the engine registry cannot forget a name
// (ENG#26). definition() answers from the list, so it never finds one.
class StageCatalog
{
public:
	StageCatalog();

	// Manifest stages in manifest order, then user stages in file-name order.
	const StageList& stages() const;

	// Every definition registered this session, including those of user
	// stages that have since left the list. For whole-registry readers
	// (LevelBuilder, the content and sound-group checks); a lookup by name
	// that must agree with the list uses definition().
	const labrador::Registry<LevelLoadedInfo>& level_infos() const;

	// The definition of the listed stage `name`, or nullptr when no listed
	// stage has that name or it has no definition.
	const LevelLoadedInfo* definition(const std::string& name) const;

	// Appends `definition.stage` to the list and registers its definition
	// under its name, replacing one already registered there. A null `info`
	// lists the stage with no definition: only a fixture that never builds
	// the stage does that.
	void add(StageDefinition definition);

	// Drops the user stages from the list, keeping the manifest's in order.
	// Their definitions stay registered.
	void remove_user_stages();

	// Replaces the list's user stages with the user stages in `previous`, an
	// earlier copy of stages(): how an interrupted rescan puts back the list
	// it started from. Their definitions were never removed.
	void restore_user_stages(const StageList& previous);

private:
	labrador::Registry<LevelLoadedInfo> level_infos_;
	StageList stages_;
};
