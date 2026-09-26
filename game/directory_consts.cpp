#include "game/pch.h"
#include "game/directory_consts.h"
#include "engine/app/content_root.h"
#include "engine/core/file_path.h"

#include <cstdlib>
#include <memory>

namespace
{
	// The folder name under %LOCALAPPDATA%, and a directory a player will see
	// in their profile. It has read "ColourWars" since before the executable
	// did — the save path was named for the game while the exe was still
	// named ArtAttackGame — so nothing here moves now that the two agree, and
	// no existing save is stranded.
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
		// Windows supplies a native wide path. Keep every character while
		// converting to the engine's UTF-8 path convention; Save converts back
		// only at the filesystem boundary. Allocate for the actual environment
		// value rather than imposing MAX_PATH on a user's profile location.
		size_t length = 0;
		wchar_t* raw_path = nullptr;
		const errno_t result = _wdupenv_s(&raw_path, &length, L"LOCALAPPDATA");
		const std::unique_ptr<wchar_t, decltype(&std::free)> buffer(raw_path, &std::free);
		if (result != 0 || length <= 1 || buffer == nullptr)
		{
			// No LOCALAPPDATA at all, which means something unusual - a
			// service account, a stripped environment. Falling back to where
			// the save has always been is worse than the new location and
			// better than not saving.
			return legacy_save_file_path();
		}

		return labrador::path_to_utf8(std::filesystem::path(buffer.get()) /
			GAME_FOLDER_NAME / SAVE_FILE_NAME);
	}

	std::string legacy_save_file_path()
	{
		// Beside the executable, by the same rule as the content - and it
		// used to be inside the content mirror, which is why. Resolved here
		// rather than left relative so that the carry-over in save.cpp finds
		// an old save from any start directory, and so that the fallback
		// above writes somewhere that does not move with one.
		return labrador::resolved_under(labrador::executable_directory(),
			std::string("./save/") + SAVE_FILE_NAME);
	}
}
