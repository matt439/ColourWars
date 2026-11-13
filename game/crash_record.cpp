#include "game/pch.h"
#include "game/crash_record.h"
#include "game/directory_consts.h"
#include "engine/core/file_path.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>

namespace
{
	// Everything the handlers read is computed by install_crash_handlers, so
	// the structured-exception path formats into static storage and writes
	// with raw Win32: a crashed process may have a corrupt heap.
	std::atomic<FailurePhase> crash_phase{ FailurePhase::preferences };
	std::atomic_flag crash_claimed;
	bool installed = false;
	HMODULE main_image = nullptr;
	HMODULE ntdll_image = nullptr;
	LPTOP_LEVEL_EXCEPTION_FILTER previous_filter = nullptr;
	std::wstring record_path;
	std::wstring temporary_path;
	std::wstring record_folder;
	std::wstring executable_path;
	std::wstring windows_directory;
	wchar_t module_path[MAX_PATH * 4]{};
	char record[4096]{};

	bool same_text(const wchar_t* left, std::size_t left_length,
		const wchar_t* right, std::size_t right_length) noexcept
	{
		return CompareStringOrdinal(left, static_cast<int>(left_length),
			right, static_cast<int>(right_length), TRUE) == CSTR_EQUAL;
	}

	bool same_text(const wchar_t* left, std::size_t left_length, const wchar_t* right) noexcept
	{
		return same_text(left, left_length, right, std::wcslen(right));
	}

	bool is_separator(wchar_t character) noexcept { return character == L'\\' || character == L'/'; }

	// The length of the directory part, separator included; 0 for a bare name.
	std::size_t directory_length(const wchar_t* path, std::size_t length) noexcept
	{
		while (length > 0 && !is_separator(path[length - 1])) { --length; }
		return length;
	}

	// The prefix-and-suffix match the runtime allowlist needs: "msvcp140*.dll".
	bool name_matches(const wchar_t* name, std::size_t length, const wchar_t* prefix) noexcept
	{
		const std::size_t prefix_length = std::wcslen(prefix);
		constexpr std::size_t suffix_length = 4;
		return length >= prefix_length + suffix_length &&
			same_text(name, prefix_length, prefix) &&
			same_text(name + length - suffix_length, suffix_length, L".dll");
	}

	// What the engine brings beside the executable ($<TARGET_RUNTIME_DLLS>).
	bool engine_runtime(const wchar_t* name, std::size_t length) noexcept
	{
		return same_text(name, length, L"DirectXTK.dll") || same_text(name, length, L"XAudio2_9Redist.dll");
	}

	// The app-local Microsoft C++ runtime the release ZIP carries.
	bool microsoft_runtime(const wchar_t* name, std::size_t length) noexcept
	{
		return name_matches(name, length, L"msvcp140") || name_matches(name, length, L"vcruntime140") ||
			same_text(name, length, L"concrt140.dll") || same_text(name, length, L"ucrtbase.dll") ||
			same_text(name, length, L"ucrtbased.dll");
	}

	void describe_address(const void* address, CrashDetails& details) noexcept
	{
		details.module = CrashModule::other;
		HMODULE image = nullptr;
		if (address == nullptr || GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCWSTR>(address), &image) == FALSE)
		{
			return;
		}
		if (image == main_image) { details.module = CrashModule::game; }
		else
		{
			const DWORD length = GetModuleFileNameW(image, module_path, static_cast<DWORD>(std::size(module_path)));
			if (length == 0 || length >= std::size(module_path)) { return; }
			details.module = classify_crash_module(module_path, executable_path.c_str(), windows_directory.c_str());
			if (details.module == CrashModule::other) { return; }
			const std::size_t name = directory_length(module_path, length);
			std::size_t out = 0;
			for (std::size_t index = name; index < length && out + 1 < sizeof(details.module_name); ++index)
			{
				const wchar_t character = module_path[index];
				details.module_name[out++] = character >= L'A' && character <= L'Z' ?
					static_cast<char>(character - L'A' + L'a') :
					character < 0x80 ? static_cast<char>(character) : '?';
			}
			details.module_name[out] = '\0';
		}
		details.has_rva = true;
		details.rva = static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(address) -
			reinterpret_cast<std::uintptr_t>(image));
	}

	// The temporary sibling is flushed and moved over the record, as the caught
	// path's atomic replacement does; a failure leaves the previous record.
	void write_crash_record(const char* text, int length) noexcept
	{
		if (record_path.empty() || length <= 0) { return; }
		CreateDirectoryW(record_folder.c_str(), nullptr);
		const HANDLE file = CreateFileW(temporary_path.c_str(), GENERIC_WRITE, 0, nullptr,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) { return; }
		DWORD written = 0;
		bool complete = WriteFile(file, text, static_cast<DWORD>(length), &written, nullptr) != FALSE &&
			written == static_cast<DWORD>(length) && FlushFileBuffers(file) != FALSE;
		complete = CloseHandle(file) != FALSE && complete;
		if (!complete || MoveFileExW(temporary_path.c_str(), record_path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
		{
			DeleteFileW(temporary_path.c_str());
		}
	}

	LONG WINAPI crash_filter(EXCEPTION_POINTERS* pointers)
	{
		if (!crash_claimed.test_and_set())
		{
			CrashDetails details;
			details.kind = "structured_exception";
			if (pointers != nullptr && pointers->ExceptionRecord != nullptr)
			{
				details.has_code = true;
				details.code = pointers->ExceptionRecord->ExceptionCode;
				describe_address(pointers->ExceptionRecord->ExceptionAddress, details);
			}
			write_crash_record(record, format_crash_record(record, sizeof(record),
				crash_phase.load(), details, nullptr));
		}
		// The CRT's own filter turns an unhandled C++ exception into terminate;
		// everything else continues to Windows Error Reporting, as before.
		return previous_filter != nullptr ? previous_filter(pointers) : EXCEPTION_CONTINUE_SEARCH;
	}

	HMODULE image_of(const void* address) noexcept
	{
		HMODULE image = nullptr;
		return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCWSTR>(address), &image) != FALSE ?
			image : nullptr;
	}

	// Frame 0 is this handler and terminate itself lives in the runtime DLL,
	// so the caller is the first main-image return address after a frame
	// outside it: the call to std::terminate, or the throw that left a
	// noexcept function. A main-image frame that ntdll called is skipped: it
	// is a frame handler the exception dispatcher ran (__GSHandlerCheck_EH4,
	// statically linked for every /GS function with unwinding), not the
	// throw. With no such frame, the handler's own caller.
	const void* terminate_caller(void* const* frames, unsigned count) noexcept
	{
		bool left_main_image = false;
		for (unsigned index = 1; index < count; ++index)
		{
			const bool inside = image_of(frames[index]) == main_image;
			const bool dispatched = inside && index + 1 < count && ntdll_image != nullptr &&
				image_of(frames[index + 1]) == ntdll_image;
			if (inside && left_main_image && !dispatched) { return frames[index]; }
			left_main_image = left_main_image || !inside;
		}
		return count > 1 ? frames[1] : nullptr;
	}

	[[noreturn]] void crash_terminate() noexcept
	{
		if (!crash_claimed.test_and_set())
		{
			CrashDetails details;
			details.kind = "terminate";
			void* frames[62]{};
			const unsigned count = CaptureStackBackTrace(0, 62, frames, nullptr);
			describe_address(terminate_caller(frames, count), details);
			const FailurePhase phase = crash_phase.load();
			// terminate is not heap corruption, so the exception being handled,
			// if any, is classified as the caught path classifies it.
			std::string causes;
			try
			{
				if (const std::exception_ptr current = std::current_exception())
				{
					causes = FatalReport(phase, current).cause_fields();
				}
			}
			catch (...) { causes.clear(); } // The record goes without causes.
			write_crash_record(record, format_crash_record(record, sizeof(record), phase, details,
				causes.empty() ? nullptr : causes.c_str()));
		}
		// The default terminate behaviour, so the exit is what it always was.
		std::abort();
	}
}

const char* crash_module_label(CrashModule module) noexcept
{
	switch (module)
	{
	case CrashModule::game: return "game";
	case CrashModule::engine: return "engine";
	case CrashModule::system: return "system";
	case CrashModule::other: return "other";
	}
	return "other";
}

CrashModule classify_crash_module(const wchar_t* path, const wchar_t* executable,
	const wchar_t* windows) noexcept
{
	if (path == nullptr || *path == L'\0') { return CrashModule::other; }
	const std::size_t module_length = std::wcslen(path);
	const std::size_t executable_length = executable != nullptr ? std::wcslen(executable) : 0;
	if (executable_length > 0 && same_text(path, module_length, executable, executable_length))
	{
		return CrashModule::game;
	}
	const std::size_t module_directory = directory_length(path, module_length);
	const std::size_t executable_directory = executable_length > 0 ?
		directory_length(executable, executable_length) : 0;
	const wchar_t* name = path + module_directory;
	const std::size_t name_length = module_length - module_directory;
	if (executable_directory > 0 && module_directory == executable_directory &&
		same_text(path, module_directory, executable, executable_directory))
	{
		if (engine_runtime(name, name_length)) { return CrashModule::engine; }
		if (microsoft_runtime(name, name_length)) { return CrashModule::system; }
		return CrashModule::other;
	}
	std::size_t windows_length = windows != nullptr ? std::wcslen(windows) : 0;
	while (windows_length > 0 && is_separator(windows[windows_length - 1])) { --windows_length; }
	if (windows_length > 0 && module_length > windows_length + 1 && is_separator(path[windows_length]) &&
		same_text(path, windows_length, windows, windows_length))
	{
		return CrashModule::system;
	}
	return CrashModule::other;
}

int format_crash_record(char* buffer, std::size_t size, FailurePhase phase,
	const CrashDetails& details, const char* cause_fields) noexcept
{
	int length = format_record_header(buffer, size, phase);
	const auto append = [&](const char* format, auto... values) noexcept
	{
		if (length < 0) { return; }
		const std::size_t used = static_cast<std::size_t>(length);
		const int added = std::snprintf(buffer + used, size - used, format, values...);
		length = added >= 0 && static_cast<std::size_t>(added) < size - used ? length + added : -1;
	};
	append("\ncrash_kind=%s", details.kind);
	if (details.has_code) { append("\nexception_code=0x%08lX", details.code); }
	append("\ncrash_module=%s", crash_module_label(details.module));
	const bool named = details.module == CrashModule::engine || details.module == CrashModule::system;
	if (named && details.module_name[0] != '\0') { append("\ncrash_module_name=%s", details.module_name); }
	if (details.has_rva && details.module != CrashModule::other) { append("\ncrash_rva=0x%08llX", details.rva); }
	append("%s", cause_fields != nullptr ? cause_fields : "\ncauses_truncated=false");
	append("%s", "\nmessage_truncated=false\n");
	return length;
}

std::string crash_record_text(FailurePhase phase, const CrashDetails& details, const std::string& cause_fields)
{
	std::string text(16384, '\0');
	const int length = format_crash_record(text.data(), text.size(), phase, details,
		cause_fields.empty() ? nullptr : cause_fields.c_str());
	if (length < 0) { throw std::length_error("Crash record does not fit its buffer"); }
	text.resize(static_cast<std::size_t>(length));
	return text;
}

void set_crash_phase(FailurePhase phase) noexcept { crash_phase.store(phase); }

void install_crash_handlers() noexcept
{
	if (installed) { return; }
	installed = true;
	main_image = GetModuleHandleW(nullptr);
	ntdll_image = GetModuleHandleW(L"ntdll.dll");
	try
	{
		wchar_t buffer[MAX_PATH * 4]{};
		const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
		if (length > 0 && length < std::size(buffer)) { executable_path.assign(buffer, length); }
		const UINT windows = GetSystemWindowsDirectoryW(buffer, static_cast<UINT>(std::size(buffer)));
		if (windows > 0 && windows < std::size(buffer)) { windows_directory.assign(buffer, windows); }
		// No LOCALAPPDATA, no record: as the caught path, never beside the game.
		const std::string path = directory_consts::fatal_report_path();
		if (!path.empty())
		{
			const std::filesystem::path log = labrador::path_from_utf8(path);
			record_folder = log.parent_path().wstring();
			temporary_path = log.wstring() + L".crash.tmp";
			record_path = log.wstring();
		}
	}
	catch (...)
	{
		// Without its paths the handlers still run; they write nothing.
		record_path.clear();
	}
	// A stack overflow on the main thread keeps this much stack for the filter.
	ULONG guarantee = 32 * 1024;
	SetThreadStackGuarantee(&guarantee);
	previous_filter = SetUnhandledExceptionFilter(crash_filter);
	std::set_terminate(crash_terminate);
}
