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
	std::wstring dialog_text(bool saved) const;
	std::string record_text() const;
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
