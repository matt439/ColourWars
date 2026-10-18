#pragma once

#include <string>
#include <string_view>
#include <vector>

// When a tuned value reaches the game. Debug-only.
enum class TuneScope
{
	live,        // read every step through a pointer into the live object
	new_objects, // copied when a projectile is made: the next shot
	rebuild,     // copied when the match is built: restart applies it
	next_screen, // read when a menu page is built
};

const char* tune_scope_name(TuneScope scope);

// What a successful tune reports after the value: "(live)", "(applies to new
// shots)", "(applies on rebuild: restart, or tune ... restart)", ...
const char* tune_scope_note(TuneScope scope);

// The scope of a prefixed path ("tuning.player.gravity"). The first matching
// row of the table in tune_scope.cpp wins; a path no row matches is
// `rebuild`, which is always true if sometimes more than needed.
TuneScope tune_scope_of(std::string_view path);

// Only these existing presentation strings have audited colour consumers.
// A general JSON string (including a font/resource name) is never implied by
// the numeric scope wildcards. `path` includes the presentation. prefix.
bool tune_colour_path(std::string_view path);

// The table's patterns, so the acceptance case can check that every row
// matches a path the files actually have. '*' is one segment, '**' any depth.
std::vector<std::string> tune_scope_patterns();
bool tune_pattern_matches(std::string_view pattern, std::string_view path);

// One Left/Right step from `value`. A decimal token steps by one unit of the
// digit below its leading one (1800 by 100, 0.15 by 0.01, 0 by 0.1); an
// integer token ("4", "12", no '.' or exponent) by at least 1, so it stays
// an integer.
double tune_step(double value, bool integer_token);

// `value` written in the style of `previous`: an integer token stays an
// integer when the value is integral; a decimal token keeps a '.'
// ("240.0" -> "250.0", never "250"). Shortest round-trip digits otherwise.
std::string tune_format(double value, std::string_view previous);


// Whether a token was written as an integer (no '.', 'e' or 'E').
bool tune_integer_token(std::string_view token);

// The children of `prefix` among `paths`, in first-seen order: a leaf's full
// path, or an inner node's path ending in '.' or '[' (so completion and the
// debug menu walk the tree one segment at a time). `prefix` is "" or ends in
// '.' or '['.
std::vector<std::string> tune_children(const std::vector<std::string>& paths,
	std::string_view prefix);
