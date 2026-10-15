#pragma once

#include "game/debug/match_fingerprint.h"
#include "game/objects/seat_input_filter.h"
#include "game/states/menu_level_settings.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// One recorded match: the setup that rebuilds it, every seat's input on
// every logic step (after slot mapping and mouse aim, so a replay needs no
// device, window or camera), where the match was suspended, checkpoint
// hashes every 60 updates and the final fingerprint. Written as JSON to
// %LOCALAPPDATA%\ColourWars\replays\<name>.json. Debug-only.
//
// Format version 3 (docs/debug-console.md, "Seed, record and play"):
//   settings contains the match setup without the retired workshop art mode.
//   inputs:  one array per seat, each a run-length list of
//            "[count*]flags|x|aim_x|aim_y|angle|lx|ly|rx|ry" with the floats as
//            8-hex-digit bit patterns ("0" for +0.0).
//   dt, hashes and fingerprint floats as hex bit patterns: bit-exact.
class ReplayFile
{
public:
	static constexpr int format_version = 3;
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
	// For a person reading the file: the live tune edits in force when it was
	// recorded. The hashes are what play checks.
	std::vector<std::string> tune_diff;
	bool god = false;
	bool infinite_ammo = false;
	MenuLevelSettings settings;

	int updates = 0;
	// Level updates already run when the match was suspended.
	std::vector<int> suspends;
	// [seat][logic step].
	std::vector<std::vector<PlayerInputData>> inputs;
	std::vector<Checkpoint> checkpoints;
	MatchFingerprint final_print;

	// Throws std::runtime_error naming the path on any I/O failure.
	void write(const std::filesystem::path& path) const;

	// Throws std::runtime_error naming the path and what is wrong: not JSON,
	// another format version, a malformed input or setting.
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

// A file name stem from a requested name: [A-Za-z0-9_-] kept, anything else
// '_'; empty gives "replay-YYYYMMDD-HHMMSS".
std::string replay_name(std::string_view requested);

// Appends every seat's input, after letting `inner` (the bots, or null)
// write theirs. The director counts updates and adds checkpoints.
class ReplayRecorder final : public SeatInputFilter
{
public:
	ReplayRecorder(std::string name, ReplayFile header, SeatInputFilter* inner, int limit);

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

	void note_suspend(int update);
	void add_checkpoint(const MatchFingerprint& print);

	const std::string& name() const { return this->name_; }
	int limit() const { return this->limit_; }
	ReplayFile& file() { return this->file_; }

private:
	std::string name_;
	ReplayFile file_;
	SeatInputFilter* inner_ = nullptr;
	int limit_ = 0;
};

// Replaces every seat's input from a recording; once the recording is
// exhausted, neutral connected input.
class ReplayPlayer final : public SeatInputFilter
{
public:
	ReplayPlayer(std::string name, ReplayFile file);

	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

	const std::string& name() const { return this->name_; }
	const ReplayFile& file() const { return this->file_; }
	bool recorded_suspend(int update) const;

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
	std::size_t step_ = 0;
};
