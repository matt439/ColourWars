#include "game/pch.h"
#include "game/save/atomic_file.h"
#include <atomic>
#include <limits>
#include <string>

namespace
{
	// The temporary beside `path`, created exclusively. A name left behind by a
	// process that died mid-write (whose id was later reused) is skipped.
	HANDLE create_temporary(const std::filesystem::path& path, std::filesystem::path& temporary)
	{
		static std::atomic<unsigned long long> sequence{ 0 };
		for (int attempt = 0; attempt < 16; ++attempt)
		{
			temporary = path;
			temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L"-" +
				std::to_wstring(sequence.fetch_add(1, std::memory_order_relaxed)) + L".tmp";
			const HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
				CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_EXISTS) { return file; }
		}
		return INVALID_HANDLE_VALUE;
	}

	bool write_all(HANDLE file, std::string_view bytes)
	{
		std::size_t offset = 0;
		while (offset < bytes.size())
		{
			const DWORD count = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset,
				(std::numeric_limits<DWORD>::max)()));
			DWORD written = 0;
			if (!WriteFile(file, bytes.data() + offset, count, &written, nullptr) || written == 0)
			{
				return false;
			}
			offset += written;
		}
		return true;
	}
}

bool replace_file_atomically(const std::filesystem::path& path, std::string_view bytes) noexcept
{
	try
	{
		if (path.empty() || !path.has_filename()) { return false; }
		if (path.has_parent_path())
		{
			std::error_code error;
			std::filesystem::create_directories(path.parent_path(), error);
			if (error) { return false; }
		}
		std::filesystem::path temporary;
		const HANDLE file = create_temporary(path, temporary);
		if (file == INVALID_HANDLE_VALUE) { return false; }
		const bool written = write_all(file, bytes) && FlushFileBuffers(file) != FALSE;
		const bool closed = CloseHandle(file) != FALSE;
		if (written && closed && MoveFileExW(temporary.c_str(), path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE)
		{
			return true;
		}
		DeleteFileW(temporary.c_str());
	}
	catch (...) { }
	return false;
}
