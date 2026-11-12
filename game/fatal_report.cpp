#include "game/pch.h"
#include "game/fatal_report.h"
#include "game/build_info.h"
#include "game/directory_consts.h"
#include "game/objects/player_out_of_bounds.h"
#include "game/save/atomic_file.h"
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
		if (dynamic_cast<const PlayerOutOfBounds*>(&error)) { cause.reason = "player_out_of_bounds"; }
		// The engine's device failure has no type of its own (engine report 30).
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

std::wstring FatalReport::dialog_text(const std::string& saved_path) const
{
	std::string text = "Failure while ";
	text += phase_description(this->phase_);
	text += ".\n\n";
	text += this->message_;
	if (this->message_truncated_) { text += "\n[Exception text truncated]"; }
	const bool no_hardware = std::any_of(this->causes_.begin(), this->causes_.end(),
		[](const Cause& cause)
		{
			return cause.reason != nullptr && std::strcmp(cause.reason, "no_hardware_device") == 0;
		});
	if (no_hardware)
	{
		text += "\n\nColourWars needs a Direct3D 11 graphics card. Update its driver and try "
			"on the PC directly; Remote Desktop and most virtual machines cannot run it.";
	}
	else if (this->phase_ == FailurePhase::definitions || this->phase_ == FailurePhase::manifest ||
		this->phase_ == FailurePhase::validation)
	{
		text += "\n\nExtract the whole game ZIP into one folder before playing. "
			"If you built the game from source, rebuild it to restore its content.";
	}
	text += saved_path.empty() ? std::string("\n\nA short error report could not be saved.") :
		"\n\nA short error report was saved to " + saved_path + ".";
	text += "\n\nReport this at https://github.com/matt439/ColourWars/issues. "
		"Press Ctrl+C to copy this message and include it in your report.";
	if (!saved_path.empty()) { text += " Attach last-error.log too; it does not contain the error message."; }
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
	text += "\nversion="; text += build_info::version;
	text += "\ngame_revision=";text += build_info::game_revision;
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
		return replace_file_atomically(path, this->record_text());
	}
	catch (...) { }
	return false;
}

int present_fatal_error(FailurePhase phase, const std::exception_ptr& failure) noexcept
{
	try
	{
		const FatalReport report(phase, failure);
		std::string saved_path;
		try
		{
			const std::string path = directory_consts::fatal_report_path();
			if (!path.empty() && report.write_record(labrador::path_from_utf8(path))) { saved_path = path; }
		}
		catch (...) { }
		const std::wstring text = report.dialog_text(saved_path);
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
