#pragma once

#include <string>

// The two paths the game knows before it has read anything. Everything else -
// which textures, fonts, levels and sounds exist and which folders they live in
// - is in the manifest, because it is data (T7).
namespace directory_consts
{
	const static std::string MANIFEST_PATH = "./manifest.json";
	const static std::string SAVE_FILE_PATH = "./save/save_data.json";
}
