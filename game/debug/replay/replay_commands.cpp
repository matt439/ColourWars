#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/build_info.h"
#include "game/debug/replay/bot_driver.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/replay/replay.h"
#include "game/debug/tune/tune_document.h"
#include "game/debug/replay/hash.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
#include "game/objects/level_loaded_info.h"
#include <algorithm>
#include <bit>
#include <charconv>

#include <cstdio>
#include <cstdlib>
#include <filesystem>

// seed, record, play, replays and checksum, and the update hooks that record
// and replay a match. See docs/debug-console.md, "Seed, record and play".

namespace
{
	DebugMenuTraits replay_traits(DebugMenuTraits::Form form)
	{
		DebugMenuTraits traits;
		traits.category = "replay";
		traits.form = form;
		return traits;
	}

	std::string narrow_build_label()
	{
		std::string label;
		for (const wchar_t* c = build_info::label; *c != 0; ++c)
		{
			label += *c < 0x80 ? static_cast<char>(*c) : '?';
		}
		return label;
	}

	std::filesystem::path replay_path(const std::string& name)
	{
		return std::filesystem::path(labrador::widen(directory_consts::replay_directory())) /
			labrador::widen(name + ".json");
	}

	std::string dt_text(std::uint32_t bits)
	{
		char buffer[48] = {};
		std::snprintf(buffer, sizeof(buffer), "%.9g s (%08x)",
			static_cast<double>(std::bit_cast<float>(bits)), bits);
		return buffer;
	}

	std::vector<std::string> replay_names()
	{
		std::vector<std::string> names;
		std::error_code error;
		const std::filesystem::path directory(labrador::widen(directory_consts::replay_directory()));
		// increment(error), not ++: a listing that fails partway stops there
		// instead of throwing out of replay.list or completion.
		for (std::filesystem::directory_iterator it(directory, error), end;
			!error && it != end; it.increment(error))
		{
			std::error_code kind_error;
			if (it->is_regular_file(kind_error) && it->path().extension() == L".json")
			{
				names.push_back(labrador::path_to_utf8(it->path().filename()));
			}
		}
		std::sort(names.begin(), names.end());
		return names;
	}
}

MatchPlanner::Random DebugDirectorImpl::planner_random()
{
	// Every plan under a seed draws the same choices, as every build under it
	// has the same round seed and bots.
	if (this->seed_)
	{
		this->planner_random_.seed(*this->seed_);
	}
	return [this] { return static_cast<unsigned int>(this->planner_random_()); };
}

bool DebugDirectorImpl::replay_finished() const
{
	// A replay of several rounds has finished when its last one has: until
	// then the next round is armed.
	return this->player_ != nullptr && this->player_->finished && this->play_armed_ == nullptr;
}

std::string DebugDirectorImpl::replay_label(const ReplayPlayer& player) const
{
	const int rounds = this->play_file_ != nullptr ? this->play_file_->round_count() : 1;
	return rounds > 1 ? player.name() + " round " + std::to_string(this->play_round_ + 1) + "/" +
		std::to_string(rounds) : player.name();
}

SeatInputFilter* DebugDirectorImpl::seat_filter()
{
	SeatInputFilter* driver = this->bot_driver_.get();
	if (this->player_ != nullptr)
	{
		driver = this->player_.get();
	}
	else if (this->recorder_ != nullptr)
	{
		driver = this->recorder_.get();
	}
	if (!this->match_hook_active_ && this->background_updates_allowed())
	{
		this->background_inputs_.set_next(driver);
		return &this->background_inputs_;
	}
	return driver;
}

std::optional<MatchFingerprint> DebugDirectorImpl::fingerprint() const
{
	if (this->match_ == nullptr || this->match_->debug_level() == nullptr)
	{
		return std::nullopt;
	}
	return MatchFingerprint::of(*this->match_->debug_level(), this->level_updates_);
}

bool DebugDirectorImpl::drives_slot(int slot) const
{
	if (slot < 0)
	{
		return false;
	}
	const ReplayPlayer* replay = this->player_ != nullptr ? this->player_.get() :
		this->play_armed_.get();
	if (replay != nullptr)
	{
		for (const SeatSetup& seat : replay->file().tape.settings().player_settings())
		{
			if (seat.num == slot)
			{
				return true;
			}
		}
	}
	if (this->match_ != nullptr)
	{
		const std::vector<SeatSetup>& seats = this->match_->debug_settings().player_settings();
		for (std::size_t i = 0; i < seats.size(); ++i)
		{
			if (seats[i].num == slot && !this->bot_for_seat(static_cast<int>(i), slot).empty())
			{
				return true;
			}
		}
	}
	return false;
}

void DebugDirectorImpl::match_suspended(DebugMatchTarget& match)
{
	if (&match != this->match_)
	{
		return;
	}
	// Focus loss and stack coverage break the interval immediately. Preserve
	// an already-published final sample before interrupting unfinished work,
	// including suspension before the next update/export hook.
	this->break_perf_interval();
	if (this->recorder_ != nullptr)
	{
		this->recorder_->note_suspend(this->level_updates_);
	}
	if (this->player_ != nullptr && !this->player_->finished &&
		this->player_->unrecorded_suspend < 0 &&
		!this->player_->file().suspended_at(this->level_updates_))
	{
		this->player_->unrecorded_suspend = this->level_updates_;
		this->log_.warning("replay " + this->player_->name() + ": the match was suspended at "
			"update " + std::to_string(this->level_updates_) + " (console, menu or focus), "
			"which the recording was not; that clears every jump buffer, so a divergence "
			"after it is expected");
	}
}

void DebugDirectorImpl::match_resumed(DebugMatchTarget& match, const std::vector<PlayerInputData>& seats)
{
	if (&match == this->match_ && this->recorder_ != nullptr)
	{
		this->recorder_->note_resume(this->level_updates_, seats);
	}
}

std::optional<DebugStatus> DebugDirectorImpl::refuse_while_replaying(const std::string& what) const
{
	if (this->recorder_ != nullptr)
	{
		return DebugStatus::failure(what + ": recording '" + this->recorder_->name() +
			"' would not capture this; 'record stop' first");
	}
	if (this->player_ != nullptr && !this->player_->finished)
	{
		return DebugStatus::failure(what + ": replaying '" + this->player_->name() +
			"' would diverge; 'play stop' first");
	}
	return std::nullopt;
}

std::uint64_t DebugDirectorImpl::content_hash(std::string_view file)
{
	const TuneDocument* document = this->tune_document(file);
	return document != nullptr ? document->current_hash() : 0;
}

ReplayFile DebugDirectorImpl::replay_header(const DebugMatchView& match)
{
	ReplayFile file;
	file.build = narrow_build_label();
	file.dt_bits = this->last_dt_bits_;
	file.seed = this->round_seed_;
	file.tuning_hash = this->content_hash("tuning");
	file.presentation_hash = this->content_hash("presentation");
	for (const char* name : { "tuning", "presentation" })
	{
		const TuneDocument* document = this->tune_document(name);
		if (document == nullptr)
		{
			continue;
		}
		for (const auto& [path, edit] : document->edits())
		{
			file.tune_diff.push_back(document->name() + "." + path + " " + edit.baseline_token +
				" -> " + edit.current_token);
		}
	}
	file.tape.start(this->round_seed_, match.debug_settings());
	file.tape.set_cheats(this->god_, this->infinite_ammo_);
	const std::string& stage = file.tape.settings().stage();
	const auto* levels = this->data_ != nullptr ? this->data_->level_infos() : nullptr;
	if (levels == nullptr || !levels->contains(stage))
	{
		throw std::runtime_error("record: stage '" + stage +
			"' has no loaded definition; a recording requires the definition used to build its match");
	}
	// Rescans occur only while no match holds a definition, so this is the
	// same immutable registry entry LevelBuilder just used, never newer disk bytes.
	file.stage_hash = levels->get(stage)->semantic_hash();
	return file;
}

void DebugDirectorImpl::before_level_update(DebugMatchTarget& match, float dt)
{
	if (this->player_ == nullptr || this->player_->finished)
	{
		return;
	}
	const ReplayFile& file = this->player_->file();
	if (this->level_updates_ == 0 && std::bit_cast<std::uint32_t>(dt) != file.dt_bits)
	{
		this->log_.error("replay " + this->player_->name() + ": refused: recorded at dt " +
			dt_text(file.dt_bits) + ", this game steps " +
			dt_text(std::bit_cast<std::uint32_t>(dt)) + "; the seats get live input");
		this->stop_playing("the step differs");
		// As play stop: the restored cheats reach the level, bots their seats,
		// and the HUDs stop saying Replay.
		this->apply_cheats();
		this->rebuild_bots(match);
		this->push_seat_labels(match);
		return;
	}
	// A suspend clears every jump buffer and a resume re-reads the held
	// jumps; repeat each at the Level update the recording had it.
	if (match.debug_level() != nullptr)
	{
		const std::vector<RoundTape::Gap>& gaps = file.tape.gaps();
		for (std::size_t i = 0; i < gaps.size() && i < file.gap_updates.size(); ++i)
		{
			if (file.gap_updates[i] == this->level_updates_)
			{
				match.debug_apply_gap(gaps[i]);
			}
		}
	}
}

void DebugDirectorImpl::after_level_update(DebugMatchTarget& match)
{
	const Level* level = match.debug_level();
	if (level == nullptr)
	{
		return;
	}
	const int interval = ReplayFile::checkpoint_interval;

	if (this->recorder_ != nullptr)
	{
		if (this->level_updates_ == 1)
		{
			this->recorder_->file().dt_bits = this->last_dt_bits_;
		}
		this->last_print_ = MatchFingerprint::of(*level, this->level_updates_);
		if (this->level_updates_ % interval == 0)
		{
			this->recorder_->add_checkpoint(*this->last_print_);
		}
		const int limit = this->recorder_->limit();
		if (limit > 0 && this->level_updates_ >= limit)
		{
			this->stop_recording("reached " + std::to_string(limit) + " updates");
		}
		else if (match.debug_level_finished())
		{
			this->stop_recording("the round ended", true);
		}
	}

	if (this->player_ != nullptr && !this->player_->finished)
	{
		ReplayPlayer& player = *this->player_;
		const ReplayFile& file = player.file();
		while (player.next_checkpoint < file.checkpoints.size() &&
			file.checkpoints[player.next_checkpoint].update < this->level_updates_)
		{
			++player.next_checkpoint;
		}
		if (player.next_checkpoint < file.checkpoints.size() &&
			file.checkpoints[player.next_checkpoint].update == this->level_updates_)
		{
			const ReplayFile::Checkpoint& checkpoint = file.checkpoints[player.next_checkpoint];
			const MatchFingerprint print = MatchFingerprint::of(*level, this->level_updates_);
			if (print.hash != checkpoint.hash && player.diverged_at < 0)
			{
				player.diverged_at = this->level_updates_;
				this->log_.error("replay " + this->replay_label(player) + ": DIVERGED at update " +
					std::to_string(this->level_updates_) + " (checkpoint " +
					std::to_string(player.next_checkpoint + 1) + " of " +
					std::to_string(file.checkpoints.size()) + "): state hash " + hex64(print.hash) +
					", recorded " + hex64(checkpoint.hash));
			}
			++player.next_checkpoint;
		}
		// A round that ends before its recorded last update has diverged; it is
		// compared there, since the level steps no further.
		if (this->level_updates_ >= file.updates || match.debug_level_finished())
		{
			player.finished = true;
			const std::string label = this->replay_label(player);
			const MatchFingerprint print = MatchFingerprint::of(*level, this->level_updates_);
			const std::string difference = MatchFingerprint::difference(file.final_print, print);
			const std::string expected = player.unrecorded_suspend >= 0 ?
				" (expected: suspended at update " + std::to_string(player.unrecorded_suspend) +
				" during playback)" : std::string();
			if (difference.empty() && player.diverged_at < 0)
			{
				player.matched = true;
				this->log_.info("replay " + label + ": MATCH (" +
					std::to_string(this->level_updates_) + " updates, " +
					std::to_string(file.checkpoints.size()) + " checkpoints, checksum " +
					hex64(print.hash) + ", paint " + print.paint_text() + ")");
			}
			else if (player.diverged_at < 0)
			{
				this->log_.error("replay " + label + ": DIVERGED at update " +
					std::to_string(this->level_updates_) + " (final state): " + difference + expected);
			}
			else
			{
				this->log_.error("replay " + label + ": final state differs: " +
					(difference.empty() ? std::string("hash only") : difference) + expected);
			}
			this->play_rounds_matched_ = this->play_rounds_matched_ && player.matched;
			const int rounds = this->play_file_ != nullptr ? this->play_file_->round_count() : 1;
			if (this->play_round_ + 1 < rounds)
			{
				// The next round is built from its own seed and settings, as
				// the End menu built it: a divergence does not carry into it.
				++this->play_round_;
				this->play_armed_ = std::make_unique<ReplayPlayer>(player.name(),
					this->play_file_->round(this->play_round_));
				const MatchSetup& settings = this->play_armed_->file().tape.settings();
				this->request_ = MatchRequest();
				this->request_.exact = settings;
				this->request_.exact_label = player.name();
				this->request_.stage = settings.stage();
				return;
			}
			if (rounds > 1)
			{
				if (this->play_rounds_matched_)
				{
					this->log_.info("replay " + player.name() + ": MATCH in all " +
						std::to_string(rounds) + " rounds");
				}
				else
				{
					this->log_.error("replay " + player.name() + ": DIVERGED in at least one of " +
						std::to_string(rounds) + " rounds");
				}
			}
			// Hold the last frame so it can be looked at (the next play resumes).
			if (!this->clock_.paused())
			{
				this->clock_.set_paused(true);
				this->replay_hold_ = true;
			}
			match.debug_sync_audio();
		}
	}
}

void DebugDirectorImpl::stop_recording(const std::string& reason, bool round_ended)
{
	if (this->recorder_ == nullptr)
	{
		this->end_record_series(reason);
		return;
	}
	ReplayFile& file = this->recorder_->file();
	file.updates = this->level_updates_;
	if (this->last_print_)
	{
		file.final_print = *this->last_print_;
	}
	else
	{
		file.final_print = MatchFingerprint();
		file.final_print.updates = this->level_updates_;
	}
	const std::string name = this->recorder_->name();
	const int limit = this->record_series_ != nullptr ? this->record_series_limit_ :
		this->recorder_->limit();
	const int updates = (this->record_series_ != nullptr ? this->record_series_updates_ : 0) +
		file.updates;
	const std::uint64_t checksum = file.final_print.hash;
	// A round the End menu carried the recording into joins the rounds saved
	// before it.
	ReplayFile saved;
	if (this->record_series_ != nullptr)
	{
		saved = std::move(*this->record_series_);
		saved.append_round(std::move(file));
	}
	else
	{
		saved = std::move(file);
	}
	this->record_series_.reset();
	this->recorder_.reset();
	this->last_print_.reset();

	const int rounds = saved.round_count();
	const std::filesystem::path path = replay_path(name);
	bool written = false;
	try
	{
		saved.write(path);
		written = true;
		this->log_.info("record: saved " + labrador::path_to_utf8(path) + " (" +
			std::to_string(updates) + " updates" +
			(rounds > 1 ? " in " + std::to_string(rounds) + " rounds" : std::string()) +
			", checksum " + hex64(checksum) + "; " + reason + ")");
	}
	catch (const std::exception& e)
	{
		this->log_.error(std::string("record: not saved: ") + e.what());
	}
	if (round_ended && written && (limit <= 0 || updates < limit))
	{
		this->record_series_ = std::make_unique<ReplayFile>(std::move(saved));
		this->record_series_name_ = name;
		this->record_series_limit_ = limit;
		this->record_series_updates_ = updates;
		this->log_.info("record " + name + ": the end menu's next round carries this recording "
			"on ('record stop' ends it here)");
	}
}

void DebugDirectorImpl::end_record_series(const std::string& reason)
{
	if (this->record_series_ == nullptr)
	{
		return;
	}
	const int rounds = this->record_series_->round_count();
	this->log_.info("record " + this->record_series_name_ + ": ends with " + std::to_string(rounds) +
		(rounds == 1 ? " round" : " rounds") + ", as saved (" + reason + ")");
	this->record_series_.reset();
	this->record_series_name_.clear();
	this->record_series_limit_ = 0;
	this->record_series_updates_ = 0;
}

void DebugDirectorImpl::continue_recording(const DebugMatchView& match)
{
	ReplayFile header;
	try
	{
		header = this->replay_header(match);
	}
	catch (const std::exception& e)
	{
		this->log_.error(std::string("record: ") + e.what());
		this->end_record_series("this round cannot be recorded");
		return;
	}
	const ReplayFile& saved = *this->record_series_;
	// One header serves every round: the hashes play checks and the cheats
	// it re-applies must hold for all of them.
	std::string changed;
	if (header.tuning_hash != saved.tuning_hash) { changed = "tuning"; }
	else if (header.presentation_hash != saved.presentation_hash) { changed = "presentation.json"; }
	else if (header.tape.damage_immune() != saved.tape.damage_immune() ||
		header.tape.infinite_ammo() != saved.tape.infinite_ammo()) { changed = "the cheats"; }
	if (!changed.empty())
	{
		this->log_.warning("record " + this->record_series_name_ + ": not carried into this round: " +
			changed + " changed since its first round");
		this->end_record_series(changed + " changed");
		return;
	}
	header.build = saved.build;
	header.dt_bits = saved.dt_bits;
	header.tune_diff = saved.tune_diff;
	const int number = saved.round_count() + 1;
	const int remaining = this->record_series_limit_ > 0 ?
		this->record_series_limit_ - this->record_series_updates_ : 0;
	this->recorder_ = std::make_unique<ReplayRecorder>(this->record_series_name_, std::move(header),
		this->bot_driver_.get(), remaining);
	this->log_.info("record " + this->record_series_name_ + ": recording round " +
		std::to_string(number) + (remaining > 0 ? ", " + std::to_string(remaining) + " updates left" :
			std::string()));
}

void DebugDirectorImpl::stop_playing(const std::string& reason)
{
	if (this->player_ == nullptr)
	{
		return;
	}
	if (!this->player_->finished)
	{
		this->log_.warning("replay " + this->player_->name() + ": stopped at update " +
			std::to_string(this->level_updates_) + " of " +
			std::to_string(this->player_->file().updates) + " (" + reason + ")");
	}
	this->player_.reset();
	this->restore_cheats_after_play();
	// A next round already armed keeps the recording it comes from.
	if (this->play_armed_ == nullptr)
	{
		this->play_file_.reset();
	}
}

void DebugDirectorImpl::restore_cheats_after_play()
{
	if (this->cheats_before_play_)
	{
		this->god_ = this->cheats_before_play_->first;
		this->infinite_ammo_ = this->cheats_before_play_->second;
		this->cheats_before_play_.reset();
	}
}

void DebugDirectorImpl::register_replay_commands()
{
	using Args = const DebugArgs&;
	DebugRegistry& registry = this->registry_;
	const auto add = [&registry](std::string name, std::string help,
		std::vector<DebugArgSpec> args, LambdaCommand::Body body, DebugMenuTraits menu)
		{
			auto command = std::make_unique<LambdaCommand>(std::move(name), std::move(help),
				std::move(args), std::move(body));
			command->set_menu(std::move(menu));
			registry.add(std::move(command));
		};

	add("seed", "fix the round seed of every build (palette, intro, bots), the menus' rand() and the planner, or off",
		{ { "n|off", true, [] { return std::vector<std::string>{ "off" }; } } },
		[this](Args args, DebugLog&)
		{
			if (!args.has(0))
			{
				return DebugStatus::success(this->seed_ ? "seed " + std::to_string(*this->seed_) :
					std::string("seed off (time-seeded)"));
			}
			if (args.text(0) == "off")
			{
				this->seed_.reset();
				this->planner_random_.seed(std::random_device{}());
				return DebugStatus::success("seed off: the next build draws new random choices");
			}
			unsigned int seed = 0;
			const std::string& text = args.text(0);
			const auto result = std::from_chars(text.data(), text.data() + text.size(), seed);
			if (text.empty() || result.ec != std::errc() || result.ptr != text.data() + text.size())
			{
				return DebugStatus::failure("seed: expected a whole number 0-4294967295 or off");
			}
			this->seed_ = seed;
			std::srand(seed);
			this->planner_random_.seed(seed);
			return DebugStatus::success("seed " + text + (this->match_ != nullptr ?
				": applies when the match is next built (restart)" : ""));
		}, replay_traits(DebugMenuTraits::Form::hidden));

	add("record", "record every seat's input from a fresh build: record [name] [updates], "
		"record stop (bare record while recording stops)",
		{ { "name|stop", true, [] { return std::vector<std::string>{ "stop" }; } },
			{ "updates", true, {} } },
		[this](Args args, DebugLog&)
		{
			const bool stop = (args.has(0) && args.text(0) == "stop") ||
				(!args.has(0) && (this->recorder_ != nullptr || this->record_series_ != nullptr));
			if (stop)
			{
				if (this->record_armed_ && this->recorder_ == nullptr)
				{
					const std::string name = this->record_armed_->name;
					this->record_armed_.reset();
					return DebugStatus::success("record " + name + ": disarmed");
				}
				if (this->recorder_ == nullptr && this->record_series_ != nullptr)
				{
					this->end_record_series("record stop");
					return DebugStatus::success();
				}
				if (this->recorder_ == nullptr)
				{
					return DebugStatus::failure("record stop: not recording");
				}
				this->stop_recording("record stop");
				return DebugStatus::success();
			}
			if (this->recorder_ != nullptr)
			{
				return DebugStatus::failure("record: already recording '" +
					this->recorder_->name() + "'; record stop first");
			}
			if (this->player_ != nullptr || this->play_armed_ != nullptr)
			{
				return DebugStatus::failure("record: a replay is running; play stop first");
			}
			int limit = 0;
			if (args.has(1) && (!DebugArgs::parse_int(args.text(1), limit) || limit < 1))
			{
				return DebugStatus::failure("record: updates must be a whole number above 0");
			}
			const std::string name = replay_name(args.has(0) ? args.text(0) : std::string());
			this->record_armed_ = RecordRequest{ name, limit };
			if (this->match_ != nullptr)
			{
				// From a fresh build, so the recording starts where a replay will.
				this->request_.restart = true;
				return DebugStatus::success("record " + name + ": restarting the match to record "
					"from its first update", true);
			}
			return DebugStatus::success("record " + name + ": starts with the next match");
		}, replay_traits(DebugMenuTraits::Form::run));

	add("play", "rebuild a recorded match and feed it its recorded input; play stop",
		{ { "name|stop", false, [] {
				std::vector<std::string> names = replay_names();
				names.insert(names.begin(), "stop");
				return names; } },
			{ "force", true, [] { return std::vector<std::string>{ "force" }; } } },
		[this](Args args, DebugLog& log)
		{
			if (args.text(0) == "stop")
			{
				if (this->play_armed_ != nullptr && this->player_ == nullptr)
				{
					this->play_armed_.reset();
					// Its rebuild goes too: the recorded roster without the replay.
					this->drop_request("play stop");
					return DebugStatus::success("play: disarmed");
				}
				if (this->player_ == nullptr)
				{
					return DebugStatus::failure("play stop: no replay is running");
				}
				if (this->play_armed_ != nullptr)
				{
					// Between rounds: the next round's build goes too.
					this->play_armed_.reset();
					this->drop_request("play stop");
				}
				this->stop_playing("play stop");
				this->apply_cheats();
				if (this->match_ != nullptr)
				{
					// Bots take their seats back at once.
					this->rebuild_bots(*this->match_);
					this->push_seat_labels(*this->match_);
				}
				return DebugStatus::success("play stopped; the seats get live input" +
					(this->bot_driver_ != nullptr ? " (" + this->bot_summary() + ")" : std::string()));
			}
			const bool force = args.has(1) && args.text(1) == "force";
			if (args.has(1) && !force)
			{
				return DebugStatus::failure("play: the second word may only be force");
			}
			if (this->recorder_ != nullptr || this->record_armed_)
			{
				return DebugStatus::failure("play: a recording is running or armed; record stop first");
			}
			const std::string name = replay_name(args.text(0));
			const std::filesystem::path path = replay_path(name);
			std::error_code error;
			if (!std::filesystem::is_regular_file(path, error))
			{
				return DebugStatus::failure("play: no replay '" + name + "' in " +
					directory_consts::replay_directory() + " (try replays)");
			}
			ReplayFile file;
			try
			{
				file = ReplayFile::read(path);
			}
			catch (const std::exception& e)
			{
				return DebugStatus::failure(std::string("play: ") + e.what());
			}
			// Every round is built from its own stage: each must be loaded and
			// be the definition it was recorded on.
			for (int index = 0; index < file.round_count(); ++index)
			{
				const ReplayFile recorded = file.round(index);
				const std::string which = file.round_count() > 1 ?
					" (round " + std::to_string(index + 1) + ")" : std::string();
				const std::string& stage_name = recorded.tape.settings().stage();
				if (!recorded.stage_hash)
				{
					return DebugStatus::failure("play: legacy recording '" + name +
						"' has no stage fingerprint for '" + stage_name +
						"'; record a new run with this build" + which);
				}
				const StageList* stages = this->data_ != nullptr ? this->data_->stages() : nullptr;
				if (stages == nullptr || std::none_of(stages->begin(), stages->end(),
					[&stage_name](const Stage& stage) { return stage.name == stage_name; }))
				{
					return DebugStatus::failure("play: the recording's stage '" + stage_name +
						"' is not loaded" + which);
				}
				const auto* levels = this->data_ != nullptr ? this->data_->level_infos() : nullptr;
				if (levels == nullptr || !levels->contains(stage_name))
				{
					return DebugStatus::failure("play: the recording's stage '" + stage_name +
						"' has no loaded definition; restore the stage definition or record a new run" +
						which);
				}
				const std::uint64_t stage_hash = levels->get(stage_name)->semantic_hash();
				if (*recorded.stage_hash != stage_hash)
				{
					return DebugStatus::failure("play: stage '" + stage_name +
						"' differs from the recording (recorded " + hex64(*recorded.stage_hash) +
						", loaded " + hex64(stage_hash) +
						"); restore the recorded stage definition or record a new run" + which);
				}
			}
			if (this->last_dt_bits_ != 0 && this->last_dt_bits_ != file.dt_bits)
			{
				return DebugStatus::failure("play: recorded at dt " + dt_text(file.dt_bits) +
					", this game steps " + dt_text(this->last_dt_bits_) +
					"; a replay never matches across step sizes");
			}
			const std::uint64_t tuning_hash = this->content_hash("tuning");
			const std::uint64_t presentation_hash = this->content_hash("presentation");

			if (file.tuning_hash != tuning_hash)
			{
				if (!force)
				{
					return DebugStatus::failure("play: recorded with other tuning (" +
						hex64(file.tuning_hash) + ", now " + hex64(tuning_hash) +
						"); tune.reset or reload to match it, or 'play " + name + " force'");
				}
				log.warning("play: tuning differs from the recording; expect DIVERGED");
			}
			if (file.presentation_hash != presentation_hash)
			{
				log.warning("play: presentation.json differs from the recording (camera and "
					"layout only; the simulation does not read it)");
			}
			if (file.build != narrow_build_label())
			{
				log.warning("play: recorded by " + file.build + "; a replay is exact only on the "
					"build that made it");
			}
			for (const std::string& line : file.tune_diff)
			{
				log.info("play: recorded with " + line);
			}
			// A recording that crossed round boundaries plays each round in
			// turn, from the first.
			this->play_file_ = std::make_unique<ReplayFile>(std::move(file));
			this->play_round_ = 0;
			this->play_rounds_matched_ = true;
			this->play_armed_ = std::make_unique<ReplayPlayer>(name, this->play_file_->round(0));
			const int rounds = this->play_file_->round_count();
			const MatchSetup& settings = this->play_armed_->file().tape.settings();
			this->request_ = MatchRequest();
			this->request_.exact = settings;
			this->request_.exact_label = name;
			this->request_.stage = settings.stage();
			return DebugStatus::success("play " + name + ": " + settings.stage() + ", " +
				std::to_string(settings.player_count()) + " player(s), " +
				std::to_string(this->play_armed_->file().updates) + " updates" +
				(rounds > 1 ? " in round 1 of " + std::to_string(rounds) : std::string()) +
				(this->match_ != nullptr ? ": rebuilding the match" : ": starting a match"), true);
		}, replay_traits(DebugMenuTraits::Form::pick));

	add("replays", "list the recorded matches", {},
		[](Args, DebugLog& log)
		{
			const std::vector<std::string> names = replay_names();
			for (const std::string& name : names)
			{
				log.info("  " + name);
			}
			return DebugStatus::success(std::to_string(names.size()) + " replay(s) in " +
				directory_consts::replay_directory());
		}, replay_traits(DebugMenuTraits::Form::run));

	add("checksum", "the running match's state hash and paint (what a replay compares)", {},
		[this](Args, DebugLog&)
		{
			const std::optional<MatchFingerprint> print = this->fingerprint();
			if (!print)
			{
				return DebugStatus::failure("checksum: no match");
			}
			// Live movement-feedback bursts: cosmetic, outside the hash, but
			// what an A/B screenshot of the feedback needs on screen.
			int bursts = 0;
			if (const Level* level = this->match_->debug_level())
			{
				for (int view = 0; view < level->player_count(); ++view)
				{
					bursts += static_cast<int>(level->player(view).movement_feedback().effects().size());
				}
			}
			return DebugStatus::success("checksum " + hex64(print->hash) + " at update " +
				std::to_string(print->updates) + ", paint " + print->paint_text() + ", bursts " +
				std::to_string(bursts));
		}, replay_traits(DebugMenuTraits::Form::run));
}
#endif
