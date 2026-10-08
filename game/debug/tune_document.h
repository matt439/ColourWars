#pragma once

#include "game/debug/json_spans.h"
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// One live edit: the token the baseline file had and the one now in force.
struct TuneEdit
{
	std::string baseline_token;
	std::string current_token;
};

// What tune.save would write into a file, or why it would not.
struct TuneSplice
{
	std::string text;
	std::vector<std::string> conflicts; // "path: why", one per refused edit
	int written = 0;
};

// One content file as live tuning sees it: the text the live object was
// loaded from (the baseline), the text with every edit applied (current),
// and the edits between the two. Paths here are the file's own
// ("player.gravity"); commands prefix the file name. Debug-only.
class TuneDocument
{
public:
	// `name` is "tuning" or "presentation"; throws what JsonSpanDocument throws.
	TuneDocument(std::string name, std::string baseline_text);

	const std::string& name() const { return this->name_; }
	const JsonSpanDocument& current() const { return this->current_; }
	const JsonSpanDocument& baseline() const { return this->baseline_; }
	const std::map<std::string, TuneEdit>& edits() const { return this->edits_; }

	// Every number and boolean, prefixed with the file name, in file order.
	std::vector<std::string> prefixed_paths() const;

	// The current text with one leaf's token replaced: a candidate, not yet
	// committed.
	std::string edited(const JsonLeaf& leaf, std::string_view token) const;

	// Makes a validated candidate the current text and records the edit (or
	// drops it, when the token is the baseline's again).
	void commit(std::string candidate_text, const std::string& path);

	// The candidate text with `path` (or every edit, for an empty path) back
	// at its baseline token.
	std::string reverted(const std::string& path) const;

	// A new baseline: after reload, or after tune.save wrote the edits.
	void rebase(std::string text);

	// The edits spliced into `disk_text` (the source file as it is now),
	// last span first so earlier offsets stay valid. An edit whose disk token
	// is no longer the baseline token is a conflict unless `force`; a path
	// the disk no longer has always is. Throws when disk_text is not JSON.
	TuneSplice splice(const std::string& disk_text, bool force) const;

	// FNV-1a over the current text: what a replay header records.
	std::uint64_t current_hash() const;

private:
	std::string name_;
	JsonSpanDocument baseline_;
	JsonSpanDocument current_;
	std::map<std::string, TuneEdit> edits_;
};

// FNV-1a, 64-bit.
std::uint64_t fnv1a(std::string_view bytes, std::uint64_t hash = 0xcbf29ce484222325ull);
std::string hex64(std::uint64_t value);

// Folders named ColourWars-tune-<digits> directly under %TEMP% that are empty:
// what earlier builds' live tuning left behind. Removing an empty folder
// cannot delete anyone's data; anything else there is left alone.
std::vector<std::filesystem::path> stale_tune_directories(const std::filesystem::path& temp);
