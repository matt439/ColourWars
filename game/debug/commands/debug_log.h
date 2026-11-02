#pragma once

#include <cstddef>
#include <deque>
#include <fstream>
#include <string>
#include <string_view>

enum class DebugSeverity
{
	info,
	echo,
	warning,
	error,
};

struct DebugLogLine
{
	DebugSeverity severity = DebugSeverity::info;
	std::string text;
};

// The developer console's output: a bounded list of recent lines, mirrored to
// the debugger and, when asked, to a transcript file.
class DebugLog
{
public:
	static constexpr std::size_t capacity = 256;

	// Multi-line text is split; each line is kept and mirrored separately.
	void write(DebugSeverity severity, std::string_view text);
	void info(std::string_view text);
	void warning(std::string_view text);
	void error(std::string_view text);

	const std::deque<DebugLogLine>& lines() const;
	void clear();

	// Mirror the kept lines, then every following line, to a UTF-8 file;
	// empty path stops mirroring.
	// Returns false (and keeps no file) when the path cannot be opened.
	bool set_transcript(const std::string& path);
	const std::string& transcript_path() const;

	// When false, lines are not sent to OutputDebugStringA (acceptance runs).
	void set_mirror_to_debugger(bool mirror);

private:
	std::deque<DebugLogLine> lines_;
	std::ofstream transcript_;
	std::string transcript_path_;
	bool mirror_to_debugger_ = true;
};
