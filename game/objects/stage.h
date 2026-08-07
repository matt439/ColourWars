#pragma once

#include <string>
#include <vector>

// One pickable stage, as the stage-select menu sees it.
//
// There is no LevelStage enum and no table turning one into a filename: a
// stage *is* a level the manifest named. Adding a fourth is a .json in
// content/levels/ and one more line in content/manifest.json, and rebuilds
// nothing (T7). The name is the manifest's name for the level, which is also
// the key its LevelLoadedInfo sits under, so nothing has to keep two spellings
// in step.
//
// display_name and icon_frame are copied out of the level file at load rather
// than read back through the registry each time the menu redraws - the menu
// holds no handle and does no lookup for its own text (T8).
struct Stage
{
	std::string name;
	std::string display_name;
	std::string icon_frame;
};

// The stages a game offers, in the order its manifest names them.
using StageList = std::vector<Stage>;
