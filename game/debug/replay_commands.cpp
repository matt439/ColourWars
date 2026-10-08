#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/build_info.h"
#include "game/debug/bot_driver.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
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
		for (const auto& entry : std::filesystem::directory_iterator(directory, error))
		{
			if (entry.is_regular_file(error) && entry.path().extension() == L".json")
			{
				const std::string stem = labrador::path_to_utf8(entry.path().stem());
				names.push_back(stem);
			}
		}
		std::sort(names.begin(), names.end());
		return names;
	}
}

MatchPlanner::Random DebugDirectorImpl::planner_random()
{
	// Every plan under a seed draws the same choices, as every build under it
	// draws the same rand() and bots.
	if (this->seed_)
	{
		this->planner_random_.seed(*this->seed_);
	}
	return [this] { return static_cast<unsigned int>(this->planner_random_()); };
}

bool DebugDirectorImpl::replay_finished() const
{
	return this->player_ != nullptr && this->player_->finished;
}

SeatInputFilter* DebugDirectorImpl::seat_filter() const
{
	if (this->player_ != nullptr)
	{
		return this->player_.get();
	}
	if (this->recorder_ != nullptr)
	{
		return this->recorder_.get();
	}
	return this->bot_driver_.get();
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
		for (const MenuPlayerSettings& seat : replay->file().settings.player_settings())
		{
			if (seat.num == slot)
			{
				return true;
			}
		}
	}
	if (this->match_ != nullptr)
	{
		const std::vector<MenuPlayerSettings> seats = this->match_->debug_settings().player_settings();
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
	if (&match != this->match_ || this->injecting_suspend_)
	{
		return;
	}
	if (this->recorder_ != nullptr)
	{
		this->recorder_->note_suspend(this->level_updates_);
	}
	if (this->player_ != nullptr && !this->player_->finished &&
		this->player_->unrecorded_suspend < 0 &&
		!this->player_->recorded_suspend(this->level_updates_))
	{
		this->player_->unrecorded_suspend = this->level_updates_;
		this->log_.warning("replay " + this->player_->name() + ": the match was suspended at "
			"update " + std::to_string(this->level_updates_) + " (console, menu or focus), "
			"which the recording was not; that clears every jump buffer, so a divergence "
			"after it is expected");
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

ReplayFile DebugDirectorImpl::replay_header(const DebugMatchTarget& match)
{
	ReplayFile file;
	file.build = narrow_build_label();
	file.dt_bits = this->last_dt_bits_;
	file.seed = this->seed_;
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
	file.god = this->god_;
	file.infinite_ammo = this->infinite_ammo_;
	file.settings = match.debug_settings();
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
		// As play stop: the restored cheats reach the level, bots their seats.
		this->apply_cheats();
		this->rebuild_bots(match);
		return;
	}
	// A suspend clears every jump buffer; do it where the recording had one.
	if (this->player_->recorded_suspend(this->level_updates_) && match.debug_level() != nullptr)
	{
		this->injecting_suspend_ = true;
		match.debug_level()->suspend();
		match.debug_sync_audio();
		this->injecting_suspend_ = false;
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
			this->stop_recording("the round ended");
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
				this->log_.error("replay " + player.name() + ": DIVERGED at update " +
					std::to_string(this->level_updates_) + " (checkpoint " +
					std::to_string(player.next_checkpoint + 1) + " of " +
					std::to_string(file.checkpoints.size()) + "): state hash " + hex64(print.hash) +
					", recorded " + hex64(checkpoint.hash));
			}
			++player.next_checkpoint;
		}
		if (this->level_updates_ >= file.updates)
		{
			player.finished = true;
			const MatchFingerprint print = MatchFingerprint::of(*level, this->level_updates_);
			const std::string difference = MatchFingerprint::difference(file.final_print, print);
			const std::string expected = player.unrecorded_suspend >= 0 ?
				" (expected: suspended at update " + std::to_string(player.unrecorded_suspend) +
				" during playback)" : std::string();
			if (difference.empty() && player.diverged_at < 0)
			{
				this->log_.info("replay " + player.name() + ": MATCH (" +
					std::to_string(this->level_updates_) + " updates, " +
					std::to_string(file.checkpoints.size()) + " checkpoints, checksum " +
					hex64(print.hash) + ", paint " + print.paint_text() + ")");
			}
			else if (player.diverged_at < 0)
			{
				this->log_.error("replay " + player.name() + ": DIVERGED at update " +
					std::to_string(this->level_updates_) + " (final state): " + difference + expected);
			}
			else
			{
				this->log_.error("replay " + player.name() + ": final state differs: " +
					(difference.empty() ? std::string("hash only") : difference) + expected);
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

void DebugDirectorImpl::stop_recording(const std::string& reason)
{
	if (this->recorder_ == nullptr)
	{
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
	const std::filesystem::path path = replay_path(this->recorder_->name());
	try
	{
		file.write(path);
		this->log_.info("record: saved " + labrador::path_to_utf8(path) + " (" +
			std::to_string(file.updates) + " updates, checksum " + hex64(file.final_print.hash) +
			"; " + reason + ")");
	}
	catch (const std::exception& e)
	{
		this->log_.error(std::string("record: not saved: ") + e.what());
	}
	this->recorder_.reset();
	this->last_print_.reset();
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

	add("seed", "fix the random seed (rand(), the planner, bots) for the next build, or off",
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
				(!args.has(0) && this->recorder_ != nullptr);
			if (stop)
			{
				if (this->record_armed_ && this->recorder_ == nullptr)
				{
					const std::string name = this->record_armed_->name;
					this->record_armed_.reset();
					return DebugStatus::success("record " + name + ": disarmed");
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
				this->stop_playing("play stop");
				this->apply_cheats();
				if (this->match_ != nullptr)
				{
					// Bots take their seats back at once.
					this->rebuild_bots(*this->match_);
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
			if (!this->planner().is_stage_word(file.settings.stage()))
			{
				return DebugStatus::failure("play: the recording's stage '" + file.settings.stage() +
					"' is not loaded");
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
			this->play_armed_ = std::make_unique<ReplayPlayer>(name, std::move(file));
			const MenuLevelSettings& settings = this->play_armed_->file().settings;
			this->request_ = MatchRequest();
			this->request_.exact = settings;
			this->request_.exact_label = name;
			this->request_.stage = settings.stage();
			return DebugStatus::success("play " + name + ": " + settings.stage() + ", " +
				std::to_string(settings.player_count()) + " player(s), " +
				std::to_string(this->play_armed_->file().updates) + " updates" +
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
			return DebugStatus::success("checksum " + hex64(print->hash) + " at update " +
				std::to_string(print->updates) + ", paint " + print->paint_text());
		}, replay_traits(DebugMenuTraits::Form::run));
}
#endif
