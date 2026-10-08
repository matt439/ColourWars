#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// One console statement of a script: where it came from, for every echo and
// error ("review_feedback_ab.cfg:14"), and its text, already split on ';'.
struct DebugScriptStatement
{
	std::string origin;
	// 1-based; 0 for a launch argument, which has no line.
	int line = 0;
	std::string text;

	// "name.cfg:12", or the bare origin for a statement with no line.
	std::string where() const;
};

struct ParsedScript
{
	bool ok = true;
	// "name.cfg:3: unterminated quote", or the argument that was refused.
	std::string error;
	std::vector<DebugScriptStatement> statements;
};

namespace debug_script
{
	// A script is a short list of console lines, not a program.
	inline constexpr std::size_t max_bytes = 64 * 1024;
	inline constexpr std::size_t max_lines = 2000;

	// The file a script name means: [A-Za-z0-9_-]+ with an optional ".cfg".
	// False for anything else, so a name can never be a path.
	bool file_name(std::string_view name, std::string& file);

	// The line with a // comment removed; // inside double quotes is text.
	std::string strip_comment(std::string_view line);
}

// Parses a .cfg: one console line per text line, ';' separating statements as
// at the prompt, // comments outside quotes, blank lines ignored, CRLF and a
// UTF-8 byte-order mark accepted. $1..$9 are replaced by `args` before the
// line is split ($$ is a literal $); an argument may not carry ';', '"', //
// or whitespace, so it can never add a statement. Every statement is
// tokenized here, so a bad byte or an open quote fails before anything runs.
// Pure: it reads no file.
ParsedScript parse_script(std::string_view origin, std::string_view text,
	const std::vector<std::string>& args);
