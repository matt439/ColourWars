#pragma once

#include <optional>
#include <string>
#include <vector>
#include "game/save_section.h"

struct LevelEndInfo;

// Everything one stage has kept across launches, keyed by its manifest name.
// Percentages are 0-100, as the results show them.
struct StageRecord
{
	std::string stage;
	// Two-team matches. A team's win is counted by side, not by player.
	int versus_played = 0;
	int team_a_wins = 0;
	int team_b_wins = 0;
	int draws = 0;
	// The winning team's share of painted surface; a draw sets no best.
	std::optional<float> best_win_percentage;
	// One-team matches (solo practice): the share of the arena painted.
	int practice_played = 0;
	std::optional<float> best_coverage_percentage;
};

// What adding one result did, for the results line. Default-constructed, it
// says nothing was recorded and the results page shows no record line.
struct RecordOutcome
{
	bool recorded = false;
	// The value is coverage (one team played), not a winning share.
	bool coverage = false;
	// This result beat the shown best at the precision the results display.
	bool new_best = false;
	// This result's value; absent for a draw, which has no winning share.
	std::optional<float> value;
	// The stage's best after this result, of the same kind as `value`.
	std::optional<float> best;
};

// Per-stage results. Records for stages the manifest no longer lists are kept,
// so removing and restoring a level does not lose its history.
class MatchRecords
{
public:
	const StageRecord* find(const std::string& stage) const;
	const std::vector<StageRecord>& stages() const;
	// Counts one finished match on `stage`. A best only moves when the new
	// value is higher at the results' one-decimal display, so a gain nobody
	// can see is never called a new best.
	RecordOutcome add(const std::string& stage, const LevelEndInfo& result);
	// Adds a record read from the save. A second record for the same stage is
	// ignored; returns whether this one was kept.
	bool insert(const StageRecord& record);
private:
	std::vector<StageRecord> stages_;
	StageRecord& entry(const std::string& stage);
};

// "records": [ { "stage": "...", ... } ] at the save's root. An array of
// objects rather than an object keyed by stage, because the engine's JSON
// reader has no member iteration. Malformed entries are skipped one by one.
class MatchRecordsSection final : public SaveSection
{
public:
	const char* key() const override { return "records"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
