#pragma once

#include <filesystem>
#include <string_view>

// The game's one whole-file writer: the save, the fatal-error record and every
// file the developer tools write go through it.
//
// Creates the parent directory, writes `bytes` to a sibling temporary whose
// name no other writer shares (`<file>.<process>-<n>.tmp`, opened CREATE_NEW),
// flushes it to disk, and moves it over `path` with one replacing, write-through
// MoveFileExW. Either `path` holds all of `bytes` or it is as it was; on failure
// the temporary is removed. Returns false on any failure and never throws.
bool replace_file_atomically(const std::filesystem::path& path, std::string_view bytes) noexcept;
