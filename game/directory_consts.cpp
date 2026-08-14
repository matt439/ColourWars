#include "game/pch.h"
#include "game/directory_consts.h"

#include <cstdlib>

namespace
{
	// The folder name under %LOCALAPPDATA%. The game's own name rather than
	// the executable's: ArtAttackGame.exe is a grandfathered identifier and
	// this is a directory a player will see in their profile.
	constexpr char GAME_FOLDER_NAME[] = "ColourWars";

	constexpr char SAVE_FILE_NAME[] = "save_data.json";
}

namespace directory_consts
{
	std::string save_file_path()
	{
		// Read rather than resolved through SHGetKnownFolderPath, because this
		// runs before the window, the renderer or COM exist - main() loads the
		// save to find out what resolution to create the window at - and the
		// environment variable is set for every interactive session on every
		// supported Windows.
		//
		// getenv rather than the wide form: every path in this game is narrow,
		// including the ones handed to fopen_s and std::filesystem in save.cpp,
		// and widening this one alone would only move the problem. A profile
		// path outside the system code page is a real limitation and is
		// exactly as real as it was when the save lived in the content mirror.
		size_t length = 0;
		char buffer[MAX_PATH] = {};
		if (getenv_s(&length, buffer, sizeof(buffer), "LOCALAPPDATA") != 0 ||
			length == 0)
		{
			// No LOCALAPPDATA at all, which means something unusual - a
			// service account, a stripped environment. Falling back to where
			// the save has always been is worse than the new location and
			// better than not saving.
			return legacy_save_file_path();
		}

		return std::string(buffer) + "\\" + GAME_FOLDER_NAME + "\\" +
			SAVE_FILE_NAME;
	}

	std::string legacy_save_file_path()
	{
		return std::string("./save/") + SAVE_FILE_NAME;
	}
}
