#include "game/pch.h"
#include "game/directory_consts.h"
#include "engine/app/content_root.h"
#include "engine/core/file_path.h"
#include "game/contracts/user_stage_contract.h"

#include <cstdlib>
#include <memory>

namespace
{
	// Keep this directory name stable so existing per-user saves remain
	// reachable. The level editor's Save to My Stages shares it.
	constexpr const char* game_folder_name = user_stage_contract::game_folder_name;

	constexpr char save_file_name[] = "save_data.json";

	constexpr char screenshot_folder_name[] = "screens";

	constexpr char replay_folder_name[] = "replays";

	constexpr char debug_folder_name[] = "debug";

	// %LOCALAPPDATA%, read wide and without MAX_PATH; empty when unset.
	std::filesystem::path local_app_data()
	{
		size_t length = 0;
		wchar_t* raw_path = nullptr;
		const errno_t result = _wdupenv_s(&raw_path, &length, L"LOCALAPPDATA");
		const std::unique_ptr<wchar_t, decltype(&std::free)> buffer(raw_path, &std::free);
		if (result != 0 || length <= 1 || buffer == nullptr)
		{
			return {};
		}
		return std::filesystem::path(buffer.get());
	}
}

namespace directory_consts
{
	std::string save_file_path()
	{
		// The save is needed before creating the window. Read the native wide path
		// without imposing MAX_PATH, then convert to the engine's UTF-8 convention.
		const std::filesystem::path root = local_app_data();
		if (root.empty())
		{
			// Fall back to the executable-relative location if LOCALAPPDATA is unavailable.
			return legacy_save_file_path();
		}

		return labrador::path_to_utf8(root / game_folder_name / save_file_name);
	}

	std::string screenshot_directory()
	{
		const std::filesystem::path root = local_app_data();
		if (root.empty())
		{
			return labrador::resolved_under(labrador::executable_directory(),
				std::string("./save/") + screenshot_folder_name);
		}
		return labrador::path_to_utf8(root / game_folder_name / screenshot_folder_name);
	}

	std::string replay_directory()
	{
		const std::filesystem::path root = local_app_data();
		if (root.empty())
		{
			return labrador::resolved_under(labrador::executable_directory(),
				std::string("./save/") + replay_folder_name);
		}
		return labrador::path_to_utf8(root / game_folder_name / replay_folder_name);
	}

	std::string debug_directory()
	{
		const std::filesystem::path root = local_app_data();
		if (root.empty())
		{
			return labrador::resolved_under(labrador::executable_directory(),
				std::string("./save/") + debug_folder_name);
		}
		return labrador::path_to_utf8(root / game_folder_name / debug_folder_name);
	}

	std::string user_stage_directory()
	{
		const std::filesystem::path root = local_app_data();
		if (root.empty())
		{
			return labrador::resolved_under(labrador::executable_directory(),
				std::string("./save/") + user_stage_contract::stage_folder_name);
		}
		return labrador::path_to_utf8(user_stage_contract::folder_under(root));
	}

	std::string legacy_save_file_path()
	{
		// Resolve independently of the working directory for migration and fallback.
		return labrador::resolved_under(labrador::executable_directory(),
			std::string("./save/") + save_file_name);
	}

	std::string fatal_report_path()
	{
		const std::filesystem::path root = local_app_data();
		return root.empty() ? std::string() :
			labrador::path_to_utf8(root / game_folder_name / "last-error.log");
	}
}
