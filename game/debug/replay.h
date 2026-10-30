#pragma once

#include "game/debug/match_fingerprint.h"
#include "game/objects/seat_input_filter.h"
#include "game/round_tape.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// One recorded match: a RoundTape (game/round_tape.h) - the setup that
// rebuilds it, the cheats, every seat's input on every logic step (after slot
// mapping and mouse aim, so a replay needs no device, window or camera) and
// where the match was suspended and resumed - with what checks a replay:
// checkpoint hashes every 60 updates, the final fingerprint and the content
// hashes. Written as JSON to %LOCALAPPDATA%\ColourWars\replays\<name>.json.
// Debug-only.
//
// Format version 5 (versions 3 and 4 are refused: a replay holds only on the
// build that recorded it, DBG#2):
//   settings preserves each seat's controller kind, bot difficulty and
//   handicap (written only when a seat has one);
//   layout counts human views, while inputs remain indexed by participant.
//   inputs:  one array per participant, all the same length, each a
//            run-length list of
//            "[count*]flags|x|aim_x|aim_y|angle|lx|ly|rx|ry" with the floats as
//            8-hex-digit bit patterns ("0" for +0.0).
//   gaps:    every suspend ({ "step", "update" }) and resume ({ "step",
//            "update", "resume": the seat commands it re-read, encoded as
//            inputs }), in order. "step" is the tape step it preceded (an
//            index into inputs; the countdown reads no seats, so it is all
//            step 0), "update" the Level updates already run, as checkpoints
//            count them.
//   dt, hashes and fingerprint floats as hex bit patterns: bit-exact. Every
//   step was taken at the one dt.
//   stage_hash identifies the loaded semantic stage definition; play requires
//   it.
// The tape's own seed is not written: `seed` is the developer seed the round's
// seed is drawn from, and playback reseeds that sequence before the build.
class ReplayFile
{
public:
	static constexpr int format_version = 5;
	static constexpr int checkpoint_interval = 60;

	struct Checkpoint
	{
		int update = 0;
		std::uint64_t hash = 0;
	};

	std::string build;
	std::uint32_t dt_bits = 0;
	std::optional<unsigned int> seed;
	std::uint64_t tuning_hash = 0;
	std::uint64_t presentation_hash = 0;
	std::optional<std::uint64_t> stage_hash;
	// For a person reading the file: the live tune edits in force when it was
	// recorded. The hashes are what play checks.
	std::vector<std::string> tune_diff;

	// The setup, cheats, inputs and gaps. A header's tape is started and has
	// its cheats; the recorder fills it.
	RoundTape tape;
	// The Level update each of the tape's gaps was noted at, one per gap.
	// Playback repeats a gap there: a tape step alone cannot place one made
	// after play, when the zoom-out and reveal read no seats.
	std::vector<int> gap_updates;
	int updates = 0;
	std::vector<Checkpoint> checkpoints;
	MatchFingerprint final_print;

	// The tape's inputs by participant: [seat][logic step].
	std::vector<std::vector<PlayerInputData>> inputs_by_seat() const;
	// Whether a suspend was noted at Level update `update`.
	bool suspended_at(int update) const;

	// Throws std::runtime_error naming the path on any I/O failure, when the
	// tape's steps were not all taken at `dt_bits`, or when `gap_updates` does
	// not give each gap a Level update, in order.
	void write(const std::filesystem::path& path) const;

	// Throws std::runtime_error naming the path and what is wrong: not JSON,
	// another format version, a malformed input, gap or setting.
	static ReplayFile read(const std::filesystem::path& path);
};

// PlayerInputData <-> text, bit-exact.
namespace replay_codec
{
	std::string encode(const PlayerInputData& input);
	// False when `text` is not one encoded input.
	bool decode(std::string_view text, PlayerInputData& input);
	bool same(const PlayerInputData& a, const PlayerInputData& b);

	std::vector<std::string> run_length(const std::vector<PlayerInputData>& inputs);
	// Throws std::runtime_error on a malformed entry.
	std::vector<PlayerInputData> expand(const std::vector<std::string>& entries);
}

// A file name stem: keep [A-Za-z0-9_-], replace anything else with '_',
// then remove one case-sensitive trailing "_json" when a stem remains.
// Reserved Windows stems receive a leading '_'; empty gives
// "replay-YYYYMMDD-HHMMSS".
std::string replay_name(std::string_view requested);

// Tapes every seat's input, after letting `inner` (the bots, or null) write
// theirs, at the dt the director hands it (the header's until then). The
// director counts updates, adds checkpoints and notes suspends and resumes.
class ReplayRecorder final : public SeatInputFilter
{
public:
	// A header whose tape was not started gets a fresh one for its settings.
	ReplayRecorder(std::string name, ReplayFile header, SeatInputFilter* inner, int limit);

	ReplayRecorder(const ReplayRecorder&) = delete;
	ReplayRecorder& operator=(const ReplayRecorder&) = delete;

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

	// The dt of the steps from now on.
	void set_dt(float dt) { this->dt_ = dt; }
	// `update`: the Level updates already run.
	void note_suspend(int update);
	void note_resume(int update, const std::vector<PlayerInputData>& seats);
	void add_checkpoint(const MatchFingerprint& print);

	const std::string& name() const { return this->name_; }
	int limit() const { return this->limit_; }
	ReplayFile& file() { return this->file_; }

private:
	std::string name_;
	ReplayFile file_;
	RoundTapeRecorder recorder_{ &this->file_.tape };
	SeatInputFilter* inner_ = nullptr;
	float dt_ = 0.0f;
	int limit_ = 0;
};

// Replaces every seat's input from a recording's tape; once the tape is
// exhausted, neutral connected input.
class ReplayPlayer final : public SeatInputFilter
{
public:
	ReplayPlayer(std::string name, ReplayFile file);

	ReplayPlayer(const ReplayPlayer&) = delete;
	ReplayPlayer& operator=(const ReplayPlayer&) = delete;

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;
	bool replaces_bot_inputs() const override { return true; }

	const std::string& name() const { return this->name_; }
	const ReplayFile& file() const { return this->file_; }
	// The next tape step filter_seats will supply.
	int step() const { return this->player_.step(); }

	// Comparison state, kept by the director.
	std::size_t next_checkpoint = 0;
	bool finished = false;
	// The update of the first failed checkpoint, or -1.
	int diverged_at = -1;
	// Finished with every checkpoint and the final state equal.
	bool matched = false;

	// The first live suspend the recording did not have, or -1.
	int unrecorded_suspend = -1;

private:
	std::string name_;
	ReplayFile file_;
	RoundTapePlayer player_{ &this->file_.tape, 0 };
};
