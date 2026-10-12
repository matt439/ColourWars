#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_files.h"
#include <fstream>

namespace debug_files
{
	std::string read_file(const std::filesystem::path& path, std::size_t limit, bool& too_big)
	{
		too_big = false;
		std::error_code error;
		const std::uintmax_t size = std::filesystem::file_size(path, error);
		if (!error && size > limit)
		{
			too_big = true;
			return {};
		}
		std::ifstream stream(path, std::ios::binary);
		return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	}

	// Atomically: a temporary file beside the target, then a replacing move.
	bool write_file(const std::filesystem::path& path, const std::string& text)
	{
		std::error_code ignored;
		std::filesystem::create_directories(path.parent_path(), ignored);
		// Per process: two Debug instances on one profile never share a
		// half-written temporary (the last whole file wins).
		std::filesystem::path temporary = path;
		temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
		{
			std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
			if (!stream)
			{
				return false;
			}
			stream << text;
			if (!stream.flush())
			{
				return false;
			}
		}
		if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
		{
			std::filesystem::remove(temporary, ignored);
			return false;
		}
		return true;
	}
}
#endif
