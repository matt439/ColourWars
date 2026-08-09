#pragma once

#include <string>

// The three paths the game knows before it has read anything. Everything else -
// which textures, fonts, levels and sounds exist and which folders they live in
// - is in the manifest, because it is data (T7).
//
// The tuning file is not a manifest entry. The manifest lists assets a level
// or a menu names by string and the loader resolves to handles; the tuning
// file is read once, at startup, before any of them, and is the thing those
// assets are used *by*. Putting it in the manifest would mean an asset kind
// whose loader has to run before every other kind, expressed as a line
// ordering rather than as a call.
namespace directory_consts
{
	const static std::string MANIFEST_PATH = "./manifest.json";
	const static std::string SAVE_FILE_PATH = "./save/save_data.json";
	const static std::string TUNING_PATH = "./tuning.json";
}
