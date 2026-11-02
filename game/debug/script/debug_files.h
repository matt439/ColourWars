#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

// Whole-file text I/O for the developer console's own files (binds.cfg,
// history.txt, scripts).
namespace debug_files
{
	// The whole file; empty with too_big set when it is larger than limit.
	std::string read_file(const std::filesystem::path& path, std::size_t limit, bool& too_big);

	// Atomically: a temporary file beside the target, then a replacing move.
	// Per process: two Debug instances on one profile never share a
	// half-written temporary (the last whole file wins).
	bool write_file(const std::filesystem::path& path, const std::string& text);
}
