#pragma once

#include <string>

// Startup content paths, resolved relative to the executable.
// Application::load_manifest resolves its own path; main.cpp resolves tuning
// and presentation before loading the manifest.
namespace directory_consts
{
	const static std::string MANIFEST_PATH = "./manifest.json";
	const static std::string TUNING_PATH = "./tuning.json";
	const static std::string PRESENTATION_PATH = "./presentation.json";

	// Per-user settings at %LOCALAPPDATA%\ColourWars\save_data.json.
	// Returns a UTF-8 path, falling back to legacy_save_file_path() if the
	// environment variable is unavailable.
	std::string save_file_path();

	// Executable-relative save/save_data.json, checked for migration when the
	// per-user save cannot be opened. Also used as the fallback save location.
	std::string legacy_save_file_path();
}
