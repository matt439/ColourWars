#pragma once

#include <string>
#include <string_view>
#include <vector>

// Words of one console statement. Whitespace separates words; "double quotes"
// group them, and \" inside quotes is a literal quote. The console font is
// ASCII-only, so anything outside printable ASCII is an error.
struct DebugTokens
{
	bool ok = true;
	std::string error;
	std::vector<std::string> words;
};

DebugTokens tokenize(std::string_view statement);

// Splits a line on ';' outside quotes. Empty statements are dropped.
std::vector<std::string> split_statements(std::string_view line);

// Quotes a word when it would not survive tokenize() on its own.
std::string quote_word(const std::string& word);

// Launch arguments, after the program name. Every token that starts with '+'
// followed by a letter begins a statement; every other token is an argument of
// the current statement, so "+timescale +0.5" keeps "+0.5" as a number. Tokens
// before the first statement are ignored (reserved for a future -flag grammar).
//
//     +stage movement_course +players 4 +visual posed
//         -> { "stage movement_course", "players 4", "visual posed" }
std::vector<std::string> parse_launch_arguments(
	const std::vector<std::wstring>& arguments);

// Whether any argument would start a statement: Release reports these once.
bool has_launch_statements(const std::vector<std::wstring>& arguments);
