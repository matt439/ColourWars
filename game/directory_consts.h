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
	const static std::string TUNING_PATH = "./tuning.json";
	const static std::string PRESENTATION_PATH = "./presentation.json";

	// The save is the odd one out and is not a constant, because it is the one
	// path that is not content.
	//
	// The three above are things the build puts beside the exe and the game
	// only reads. The save is the opposite on both counts: the player writes
	// it, and the game is the only thing that ever puts it there. Keeping it
	// with them - at "./save/save_data.json", inside the staged content mirror
	// under out/build - meant a clean rebuild deleted somebody's settings, two
	// Windows accounts shared one file, and a shortcut with a different "Start
	// in" read a different save.
	//
	// %LOCALAPPDATA%\ColourWars\save_data.json, which is where per-user
	// settings belong on this platform and is writable without asking.
	std::string save_file_path();

	// Where the save used to live, still read once if nothing is at the path
	// above - see save.cpp. Returns the same "./save/save_data.json" it always
	// was, relative to the working directory, which is what made it fragile.
	std::string legacy_save_file_path();
}
