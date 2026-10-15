#include "game/pch.h"
#include "game/fatal_report.h"
#include "game/build_info.h"
#include "game/directory_consts.h"
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include <cstring>
#include <limits>
#include <new>
#include <system_error>

namespace
{
	const char* phase_name(FailurePhase phase)
	{
		switch (phase)
		{
		case FailurePhase::preferences: return "preferences";
		case FailurePhase::definitions: return "content_definitions";
		case FailurePhase::initialization: return "application_initialization";
		case FailurePhase::manifest: return "manifest";
		case FailurePhase::validation: return "content_validation";
		case FailurePhase::launch: return "launch_configuration";
		case FailurePhase::running: return "run_loop";
		}
		return "unknown";
	}

	const char* prefix(FailurePhase phase)
	{
		return phase == FailurePhase::running ? "runtime" : "startup";
	}

	const char* phase_description(FailurePhase phase)
	{
		switch (phase)
		{
		case FailurePhase::preferences: return "loading player settings";
		case FailurePhase::definitions: return "reading tuning and presentation";
		case FailurePhase::initialization: return "initializing the window, graphics and audio";
		case FailurePhase::manifest: return "loading game assets";
		case FailurePhase::validation: return "validating game content";
		case FailurePhase::launch: return "applying launch configuration";
		case FailurePhase::running: return "running the game";
		}
		return "processing an unknown phase";
	}

	void fallback(FailurePhase phase, const char* message) noexcept
	{
		const char* cause = message != nullptr ? message : "No exception details available.";
		std::fprintf(stderr, "%s failure: %s\n", prefix(phase), cause);
		std::fflush(stderr);
		MessageBoxA(nullptr, cause, phase == FailurePhase::running ?
			"ColourWars - runtime failure" : "ColourWars - startup failure", MB_OK | MB_ICONERROR);
	}
}

FatalReport::FatalReport(FailurePhase phase, const std::exception_ptr& failure) : phase_(phase)
{
	this->capture(failure);
}

void FatalReport::append_message(const char* text)
{
	if (this->message_truncated_) { return; }
	const char* message = text != nullptr ? text : "No exception details available.";
	const std::size_t remaining = max_message_bytes - this->message_.size();
	const std::size_t length = strnlen_s(message, remaining + 1);
	this->message_.append(message, std::min(length, remaining));
	this->message_truncated_ = length > remaining;
}

void FatalReport::capture(const std::exception_ptr& failure)
{
	if (this->causes_.size() == max_causes)
	{
		this->causes_truncated_ = true;
		this->append_message("\n[Further exception causes omitted]");
		return;
	}
	if (!this->causes_.empty()) { this->append_message("\nCaused by: "); }
	try
	{
		if (!failure)
		{
			this->append_message("No exception details available.");
			this->causes_.push_back(Cause{ "unavailable" });
			return;
		}
		std::rethrow_exception(failure);
	}
	catch (const std::exception& error)
	{
		const char* diagnostic = error.what();
		this->append_message(diagnostic);
		Cause cause;
		if (const auto* system = dynamic_cast<const std::system_error*>(&error))
		{
			cause.kind = "system_error";
			const std::error_code code = system->code();
			if (code.category() == std::system_category()) { cause.code_category = "system"; }
			else if (code.category() == std::generic_category()) { cause.code_category = "generic"; }
			if (cause.code_category != nullptr) { cause.code = code.value(); }
		}
		else if (dynamic_cast<const std::bad_alloc*>(&error)) { cause.kind = "bad_alloc"; }
		else if (dynamic_cast<const std::invalid_argument*>(&error)) { cause.kind = "invalid_argument"; }
		else if (dynamic_cast<const std::out_of_range*>(&error)) { cause.kind = "out_of_range"; }
		else if (dynamic_cast<const std::length_error*>(&error)) { cause.kind = "length_error"; }
		else if (dynamic_cast<const std::logic_error*>(&error)) { cause.kind = "logic_error"; }
		else if (dynamic_cast<const std::runtime_error*>(&error)) { cause.kind = "runtime_error"; }
		else { cause.kind = "standard_exception"; }
		if (diagnostic != nullptr && std::strcmp(diagnostic, "Player out of bounds") == 0)
		{
			cause.reason = "player_out_of_bounds";
		}
		else if (diagnostic != nullptr && std::strcmp(diagnostic, "No Direct3D hardware device found") == 0)
		{
			cause.reason = "no_hardware_device";
		}
		this->causes_.push_back(cause);
		const auto* nested = dynamic_cast<const std::nested_exception*>(&error);
		if (nested != nullptr && nested->nested_ptr()) { this->capture(nested->nested_ptr()); }
	}
	catch (const std::nested_exception& error)
	{
		this->append_message("Unknown non-standard exception.");
		this->causes_.push_back(Cause{});
		if (error.nested_ptr()) { this->capture(error.nested_ptr()); }
	}
	catch (...)
	{
		this->append_message("Unknown non-standard exception.");
		this->causes_.push_back(Cause{});
	}
}

const std::string& FatalReport::message() const { return this->message_; }
const char* FatalReport::stderr_prefix() const { return prefix(this->phase_); }
const wchar_t* FatalReport::caption() const
{
	return this->phase_ == FailurePhase::running ?
		L"ColourWars - runtime failure" : L"ColourWars - startup failure";
}

std::wstring FatalReport::dialog_text(bool saved) const
{
	std::string text = "Failure while ";
	text += phase_description(this->phase_);
	text += ".\n\n";
	text += this->message_;
	if (this->message_truncated_) { text += "\n[Exception text truncated]"; }
	text += saved ? "\n\nDiagnostic metadata saved to %LOCALAPPDATA%\\ColourWars\\last-error.log." :
		"\n\nDiagnostic metadata could not be saved.";
	return labrador::widen(text);
}

std::string FatalReport::record_text() const
{
	SYSTEMTIME now{};
	GetSystemTime(&now);
	char utc[32]{};
	std::snprintf(utc, sizeof(utc), "%04u-%02u-%02uT%02u:%02u:%02uZ",
		static_cast<unsigned>(now.wYear), static_cast<unsigned>(now.wMonth),
		static_cast<unsigned>(now.wDay), static_cast<unsigned>(now.wHour),
		static_cast<unsigned>(now.wMinute), static_cast<unsigned>(now.wSecond));
	std::string text = "report_format=1\nutc=";
	text += utc;
	text += "\nphase="; text += phase_name(this->phase_);
	text += "\ncategory="; text += this->stderr_prefix();
	text += "\ngame_revision="; text += build_info::game_revision;
	text += "\nengine_revision="; text += build_info::engine_revision;
	text += "\nconfiguration="; text += build_info::configuration;
	text += "\nrender_backend="; text += build_info::render_backend;
	text += "\naudio_backend="; text += build_info::audio_backend;
	for (std::size_t index = 0; index < this->causes_.size(); ++index)
	{
		const Cause& cause = this->causes_[index];
		const std::string key = "\ncause_" + std::to_string(index);
		text += key + "=" + cause.kind;
		if (cause.code_category != nullptr)
		{
			text += key + "_code_category=" + cause.code_category;
			text += key + "_code=" + std::to_string(cause.code);
		}
		if (cause.reason != nullptr) { text += key + "_reason=" + cause.reason; }
	}
	text += this->causes_truncated_ ? "\ncauses_truncated=true" : "\ncauses_truncated=false";
	text += this->message_truncated_ ? "\nmessage_truncated=true\n" : "\nmessage_truncated=false\n";
	return text;
}

bool FatalReport::write_record(const std::filesystem::path& path) const noexcept
{
	try
	{
		if (path.empty()) { return false; }
		const std::string text = this->record_text();
		if (text.size() > (std::numeric_limits<DWORD>::max)()) { return false; }
		std::error_code error;
		if (!path.parent_path().empty()) { std::filesystem::create_directories(path.parent_path(), error); }
		if (error) { return false; }
		std::filesystem::path temporary = path;
		temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L"." +
			std::to_wstring(GetTickCount64()) + L".tmp";
		const HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) { return false; }
		DWORD written = 0;
		const bool complete = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
			written == text.size() && FlushFileBuffers(file);
		const bool closed = CloseHandle(file) != FALSE;
		if (complete && closed && MoveFileExW(temporary.c_str(), path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { return true; }
		DeleteFileW(temporary.c_str());
	}
	catch (...) { }
	return false;
}

int present_fatal_error(FailurePhase phase, const std::exception_ptr& failure) noexcept
{
	try
	{
		const FatalReport report(phase, failure);
		bool saved = false;
		try
		{
			const std::string path = directory_consts::fatal_report_path();
			if (!path.empty()) { saved = report.write_record(labrador::path_from_utf8(path)); }
		}
		catch (...) { }
		const std::wstring text = report.dialog_text(saved);
		std::fprintf(stderr, "%s failure: %s\n", report.stderr_prefix(), report.message().c_str());
		std::fflush(stderr);
		MessageBoxW(nullptr, text.c_str(), report.caption(), MB_OK | MB_ICONERROR);
	}
	catch (...)
	{
		try
		{
			if (failure) { std::rethrow_exception(failure); }
			fallback(phase, "No exception details available.");
		}
		catch (const std::exception& error) { fallback(phase, error.what()); }
		catch (...) { fallback(phase, "Unknown non-standard exception."); }
	}
	return 1;
}
