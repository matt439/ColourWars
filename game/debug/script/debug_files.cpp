#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/script/debug_files.h"
#include "game/save/atomic_file.h"
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

	std::string read_file(const std::filesystem::path& path, bool& ok)
	{
		std::ifstream stream(path, std::ios::binary);
		std::string text;
		if (stream)
		{
			text.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		ok = stream.is_open() && !stream.bad();
		return ok ? text : std::string();
	}

	bool write_file(const std::filesystem::path& path, const std::string& text)
	{
		return replace_file_atomically(path, text);
	}
}
#endif
