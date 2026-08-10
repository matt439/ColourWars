#pragma once

#include <string>

// The four paths the game knows before it has read anything. Everything else -
// which textures, fonts, levels and sounds exist and which folders they live in
// - is in the manifest, because it is data (T7).
//
// The tuning and presentation files are not manifest entries. The manifest
// lists assets a level or a menu names by string and the loader resolves to
// handles; these two are read once, at startup, before any of them, and are the
// thing those assets are used *by* - presentation.json names fonts and sprite
// frames the manifest has to have loaded first. Putting them in the manifest
// would mean an asset kind whose loader has to run before every other kind,
// expressed as a line ordering rather than as a call.
namespace directory_consts
{
	const static std::string MANIFEST_PATH = "./manifest.json";
	const static std::string SAVE_FILE_PATH = "./save/save_data.json";
	const static std::string TUNING_PATH = "./tuning.json";
	const static std::string PRESENTATION_PATH = "./presentation.json";
}
