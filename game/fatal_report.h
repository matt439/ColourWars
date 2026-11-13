#pragma once

#include <cstddef>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

enum class FailurePhase
{
	preferences,
	definitions,
	initialization,
	manifest,
	validation,
	launch,
	running
};

// The record's phase= and category= values ("run_loop", "runtime" and so on).
const char* failure_phase_name(FailurePhase phase) noexcept;
const char* failure_category(FailurePhase phase) noexcept;

// The fixed head of every last-error.log record, report_format= through
// audio_backend=, with no trailing newline. It allocates nothing, so the crash
// handlers share it (crash_record.h). Returns the length, or -1 when it does
// not fit.
inline constexpr std::size_t record_header_capacity = 1024;
int format_record_header(char* buffer, std::size_t size, FailurePhase phase) noexcept;

// The original diagnostic is for stderr and the local dialog. The automatic
// file contains only controlled metadata, never arbitrary exception text.
class FatalReport
{
public:
	static constexpr std::size_t max_causes = 16;
	static constexpr std::size_t max_message_bytes = 65536;

	FatalReport(FailurePhase phase, const std::exception_ptr& failure);
	const std::string& message() const;
	const char* stderr_prefix() const;
	const wchar_t* caption() const;
	// saved_path is the UTF-8 path the record was written to, or empty when it
	// was not; the dialog names it.
	std::wstring dialog_text(const std::string& saved_path) const;
	std::string record_text() const;
	// The cause_N fields and causes_truncated, each line led by its newline.
	std::string cause_fields() const;
	// A failed write preserves the previous complete file and the original error.
	bool write_record(const std::filesystem::path& path) const noexcept;

private:
	struct Cause
	{
		const char* kind = "non_standard_exception";
		const char* code_category = nullptr;
		int code = 0;
		const char* reason = nullptr;
	};
	FailurePhase phase_;
	std::string message_;
	std::vector<Cause> causes_;
	bool causes_truncated_ = false;
	bool message_truncated_ = false;

	void capture(const std::exception_ptr& failure);
	void append_message(const char* text);
};

// Last boundary in wWinMain. Report construction, path lookup and log I/O may
// all fail; none may replace the caught cause or turn failure into exit zero.
int present_fatal_error(FailurePhase phase, const std::exception_ptr& failure) noexcept;
