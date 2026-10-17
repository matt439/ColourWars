#include "game/pch.h"
#include "game/match_records.h"
#include "game/save.h"
#include "game/objects/level_end_info.h"
#include "engine/assets/json.h"
#include <cmath>

using namespace labrador;

const StageRecord* MatchRecords::find(const std::string& stage) const
{
	for (const StageRecord& record : this->stages_)
	{
		if (record.stage == stage)
		{
			return &record;
		}
	}
	return nullptr;
}

const std::vector<StageRecord>& MatchRecords::stages() const
{
	return this->stages_;
}

StageRecord& MatchRecords::entry(const std::string& stage)
{
	for (StageRecord& record : this->stages_)
	{
		if (record.stage == stage)
		{
			return record;
		}
	}
	StageRecord record;
	record.stage = stage;
	this->stages_.push_back(record);
	return this->stages_.back();
}

bool MatchRecords::insert(const StageRecord& record)
{
	if (record.stage.empty() || this->find(record.stage) != nullptr)
	{
		return false;
	}
	this->stages_.push_back(record);
	return true;
}

RecordOutcome MatchRecords::add(const std::string& stage, const LevelEndInfo& result)
{
	RecordOutcome outcome;
	if (stage.empty())
	{
		return outcome;
	}
	StageRecord& record = this->entry(stage);
	outcome.recorded = true;

	std::optional<float>* best = nullptr;
	float value = 0.0f;
	if (result.one_team_only())
	{
		// Solo practice: the coverage line the results show.
		outcome.coverage = true;
		++record.practice_played;
		value = 100.0f * result.coverage_ratio();
		best = &record.best_coverage_percentage;
	}
	else
	{
		++record.versus_played;
		switch (result.winning_team())
		{
		case PlayerTeam::a:
			++record.team_a_wins;
			value = result.team_a_percentage();
			break;
		case PlayerTeam::b:
			++record.team_b_wins;
			value = result.team_b_percentage();
			break;
		default:
			// A draw has no winning share, so it cannot set or beat a best.
			++record.draws;
			outcome.best = record.best_win_percentage;
			return outcome;
		}
		best = &record.best_win_percentage;
	}

	value = std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, 100.0f);
	outcome.value = value;
	// Compare what the results print. A first result needs something on the
	// board: an unpainted practice run is not a best.
	const int previous = best->has_value() ? percentage_tenths(**best) : 0;
	if (percentage_tenths(value) > previous)
	{
		outcome.new_best = true;
		*best = value;
	}
	outcome.best = *best;
	return outcome;
}

void MatchRecordsSection::reset(SaveData& data) const
{
	data.records = MatchRecords();
}

void MatchRecordsSection::read(const JsonValue& root, SaveData& data) const
{
	if (!root.has(this->key()))
	{
		return;
	}
	const JsonValue entries = root.array(this->key());
	for (size_t index = 0; index < entries.size(); ++index)
	{
		// One malformed entry is skipped; the rest of the board survives.
		try
		{
			const JsonValue entry = entries.at(index);
			StageRecord record;
			record.stage = entry.identifier("stage");
			const auto count = [&entry](const char* name, int& destination)
			{
				try
				{
					destination = std::max(0, entry.integer(name));
				}
				catch (const std::runtime_error&)
				{
				}
			};
			const auto percentage = [&entry](const char* name, std::optional<float>& destination)
			{
				try
				{
					if (!entry.has(name))
					{
						return;
					}
					const float value = entry.number(name);
					if (std::isfinite(value))
					{
						destination = std::clamp(value, 0.0f, 100.0f);
					}
				}
				catch (const std::runtime_error&)
				{
				}
			};
			count("versus_played", record.versus_played);
			count("team_a_wins", record.team_a_wins);
			count("team_b_wins", record.team_b_wins);
			count("draws", record.draws);
			percentage("best_win_percentage", record.best_win_percentage);
			count("practice_played", record.practice_played);
			percentage("best_coverage_percentage", record.best_coverage_percentage);
			data.records.insert(record);
		}
		catch (const std::runtime_error&)
		{
		}
	}
}

void MatchRecordsSection::write(SaveWriter& writer, const SaveData& data) const
{
	writer.key(this->key());
	writer.begin_array();
	for (const StageRecord& record : data.records.stages())
	{
		writer.begin_object();
		writer.key("stage");
		writer.string(record.stage);
		writer.key("versus_played");
		writer.integer(record.versus_played);
		writer.key("team_a_wins");
		writer.integer(record.team_a_wins);
		writer.key("team_b_wins");
		writer.integer(record.team_b_wins);
		writer.key("draws");
		writer.integer(record.draws);
		if (record.best_win_percentage)
		{
			writer.key("best_win_percentage");
			writer.number(*record.best_win_percentage);
		}
		writer.key("practice_played");
		writer.integer(record.practice_played);
		if (record.best_coverage_percentage)
		{
			writer.key("best_coverage_percentage");
			writer.number(*record.best_coverage_percentage);
		}
		writer.end_object();
	}
	writer.end_array();
}
