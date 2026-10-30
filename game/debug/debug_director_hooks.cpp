#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "engine/render/text_encoding.h"
#include "game/debug/bot_driver.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_screenshot.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
#include "game/match_setup.h"
#include <bit>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

using namespace labrador;

namespace
{
	std::string upper(std::string text)
	{
		for (char& c : text)
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		return text;
	}
}

bool DebugDirectorImpl::has_pending_content() const
{
	return this->pending_tuning_ != nullptr && this->pending_presentation_ != nullptr;
}

bool DebugDirectorImpl::rebuild_queued() const
{
	return this->match_ != nullptr &&
		(!this->request_.empty() || this->has_pending_content());
}

std::string DebugDirectorImpl::queued_rebuild_description() const
{
	return this->request_.empty() ? std::string("reload") : this->request_.describe();
}

// Only ever called with no Level alive, from before_level_build.
void DebugDirectorImpl::apply_pending_content()
{
	if (!this->has_pending_content())
	{
		return;
	}
	const bool feedback_before = this->presentation_->movement_feedback().enabled;
	*this->tuning_ = std::move(*this->pending_tuning_);
	*this->presentation_ = std::move(*this->pending_presentation_);
	this->pending_tuning_.reset();
	this->pending_presentation_.reset();
	this->log_.info("reload: applied tuning.json and presentation.json");
	// What reload read is the new tuning baseline: live edits are gone.
	const std::array<std::string*, 2> texts = { &this->pending_tuning_text_,
		&this->pending_presentation_text_ };
	for (int file = 0; file < 2; ++file)
	{
		try
		{
			if (this->tune_docs_[static_cast<std::size_t>(file)] != nullptr)
			{
				this->tune_docs_[static_cast<std::size_t>(file)]->rebase(*texts[static_cast<std::size_t>(file)]);
			}
			else
			{
				this->tune_docs_[static_cast<std::size_t>(file)] = std::make_unique<TuneDocument>(
					file == 0 ? "tuning" : "presentation", *texts[static_cast<std::size_t>(file)]);
			}
		}
		catch (const std::exception& e)
		{
			this->log_.warning(std::string("tune: cannot follow the reload: ") + e.what());
		}
		texts[static_cast<std::size_t>(file)]->clear();
	}
	this->tune_load_tried_ = true;

	if (this->presentation_->movement_feedback().enabled != feedback_before)
	{
		this->log_.warning(std::string("reload: feedback is now ") +
			(this->presentation_->movement_feedback().enabled ? "on" : "off") +
			", as presentation.json says (the live toggle was replaced)");
	}
}

void DebugDirectorImpl::attach_match(DebugMatchTarget& match)
{
	this->match_hook_active_ = true;
	this->background_inputs_.set_next(nullptr);
	this->match_ = &match;
	this->waiting_notice_shown_ = false;
	// Perf times match frames: not the time spent in the menus before this.
	this->break_perf_interval();
	// A match started by the console was given its plan and the request was
	// consumed; anything still queued was stored in the menus without `stage`
	// (or left by a match that ended) and does not belong to this one.
	this->drop_request("this match was started from the menus");
	// A goto typed in the menus belongs to the match `stage` starts, not to
	// one started by walking the menus.
	if (this->pending_goto_ && !this->starting_from_plan_)
	{
		this->log_.warning("dropped queued goto " + this->pending_goto_->station +
			": this match was started from the menus");
		this->pending_goto_.reset();
	}
	this->starting_from_plan_ = false;
}

void DebugDirectorImpl::detach_match(const DebugMatchTarget& match)
{
	if (this->match_ == &match)
	{
		this->match_hook_active_ = true;
		this->background_inputs_.set_next(nullptr);
		this->break_perf_interval();
		// The level is still alive: a recording ends with its final state.
		this->stop_recording("the match ended");
		this->stop_playing("the match ended");
		this->bot_driver_.reset();
		this->close_live_console("the match ended");
		this->match_ = nullptr;
	}
	// A replay armed for this match's rebuild has no build left to start.
	this->disarm_play("the match ended");
	// A rebuild queued for this match means nothing once it has ended, nor
	// does a goto waiting for that rebuild.
	this->drop_request("the match ended");
	if (this->pending_goto_)
	{
		this->log_.warning("dropped queued goto " + this->pending_goto_->station +
			": the match ended");
		this->pending_goto_.reset();
	}
	this->leave_requested_ = false;
}

void DebugDirectorImpl::log_match(const MatchPlan& plan, const std::string& how)
{
	this->log_.info("match: " + plan.settings.stage() + ", " +
		std::to_string(plan.settings.player_count()) + " player(s)" +
		(plan.reset_session ? " (new session)" : "") + how);
}

void DebugDirectorImpl::before_level_build(DebugMatchTarget& match)
{
	// A rebuild ends a recording or a replay of the level it replaces (the
	// one `record` or `play` queued starts after this build).
	this->stop_recording("the match was rebuilt");
	this->stop_playing("the match was rebuilt");
	this->bot_driver_.reset();

	this->apply_pending_content();

	// A debug rebuild cleared the request before building, so anything still
	// queued here reached the match through a menu's own restart (pause menu
	// Restart, or the end menu after unwinding): this build is the one that
	// applies it.
	MatchRequest queued = this->request_;
	queued.restart = false;
	if (!queued.empty())
	{
		const MatchPlan plan = this->planner().resolve(&match.debug_settings(),
			this->request_, this->connected_pads(), this->planner_random());
		const std::string described = this->request_.describe();
		this->request_ = MatchRequest();
		this->waiting_notice_shown_ = false;
		if (!plan.ok)
		{
			this->log_.error("cannot apply " + described + ": " + plan.error +
				"; building with the current settings");
			this->disarm_play("its match could not be planned");
		}
		else
		{
			match.debug_adopt(plan.settings, plan.reset_session);
			this->log_match(plan, " (adopted by the menu's build)");
		}
	}
	// This build is the restart.
	this->request_.restart = false;

	// The palette and anything else the build draws from rand() repeat with
	// the seed; a replay uses the seed it was recorded with, and the cheats.
	if (this->play_armed_ != nullptr)
	{
		const ReplayFile& file = this->play_armed_->file();
		if (file.seed)
		{
			std::srand(*file.seed);
		}
		if (!this->cheats_before_play_)
		{
			this->cheats_before_play_ = std::make_pair(this->god_, this->infinite_ammo_);
		}
		this->god_ = file.tape.damage_immune();
		this->infinite_ammo_ = file.tape.infinite_ammo();
	}
	else if (this->seed_)
	{
		std::srand(*this->seed_);
	}
	// The build proper - layout and LevelBuilder::build_level - is what
	// blocks on the last menu frame; `perf` reports it (GAP#5.1).
	this->build_began_ms_ = perf_now_ms();
}

OpeningMode DebugDirectorImpl::opening_mode(OpeningMode chosen) const
{
	// A recording starts at the first update of play, and a replay must
	// meet the same frames, so neither has an intro to skip or not.
	if (this->play_armed_ != nullptr || this->record_armed_)
	{
		return OpeningMode::off;
	}
	if (this->intro_ == "full") { return OpeningMode::full; }
	if (this->intro_ == "quick") { return OpeningMode::quick; }
	if (this->intro_ == "off") { return OpeningMode::off; }
	return chosen;
}

void DebugDirectorImpl::after_level_build(DebugMatchTarget& match)
{
	if (this->build_began_ms_ >= 0.0)
	{
		const MatchSetup& built = match.debug_settings();
		this->last_build_ = LevelBuildTime{ perf_now_ms() - this->build_began_ms_,
			built.stage(), built.player_count() };
		this->build_began_ms_ = -1.0;
	}
	match.debug_set_overlays(this->level_overlays());
	// New players start with their text off; only overlay.player turns it on.
	if (this->overlay_player_)
	{
		match.debug_set_player_text(true);
	}
	match.debug_set_cheats(this->god_, this->infinite_ammo_);
	if (this->pending_goto_)
	{
		const PendingGoto pending = *this->pending_goto_;
		this->pending_goto_.reset();
		const DebugStatus status = this->apply_goto(match, pending.station, pending.seat);
		this->log_.write(status.ok ? DebugSeverity::info : DebugSeverity::error, status.message);
	}
	// Bots, a replay or a recording start with the level's first update, and
	// perf's next interval with its first draw (not the build before it).
	this->break_perf_interval();
	this->level_updates_ = 0;
	this->last_print_.reset();
	if (this->play_armed_ != nullptr)
	{
		this->player_ = std::move(this->play_armed_);
		this->log_.info("replay " + this->player_->name() + ": playing " +
			std::to_string(this->player_->file().updates) + " updates");
		// The last replay's held frame is over; a pause the user chose stays.
		if (this->replay_hold_ && this->clock_.paused())
		{
			this->clock_.set_paused(false);
		}
		this->replay_hold_ = false;
	}
	else
	{
		this->rebuild_bots(match);
		if (this->record_armed_)
		{
			const RecordRequest armed = *this->record_armed_;
			this->record_armed_.reset();
			this->recorder_ = std::make_unique<ReplayRecorder>(armed.name,
				this->replay_header(match), this->bot_driver_.get(), armed.limit);
			this->log_.info("record " + armed.name + ": recording" + (armed.limit > 0 ?
				" " + std::to_string(armed.limit) + " updates" : std::string(" until 'record stop'")));
		}
	}

	// Every build gets the HUD labels of the seats a bot or replay drives.
	this->push_seat_labels(match);

	// No partial step or old edge carries into the new level, and its
	// voices start paused if the clock is.
	this->clock_.reset_accumulator();
	if (this->clock_.paused())
	{
		match.debug_sync_audio();
	}
}

std::vector<std::wstring> DebugDirectorImpl::seat_labels(const DebugMatchView& match) const
{
	const std::vector<SeatSetup> seats = match.debug_settings().player_settings();
	std::vector<std::wstring> labels(seats.size());
	bool any = false;
	for (std::size_t i = 0; i < seats.size(); ++i)
	{
		if (this->player_ != nullptr)
		{
			labels[i] = L"Replay";
		}
		else if (this->bot_driver_ != nullptr)
		{
			const auto& assigned = this->bot_driver_->assignments();
			const auto found = assigned.find(static_cast<int>(i));
			if (found != assigned.end())
			{
				labels[i] = L"Bot: " + widen(found->second);
			}
		}
		any = any || !labels[i].empty();
	}
	return any ? labels : std::vector<std::wstring>();
}

void DebugDirectorImpl::push_seat_labels(DebugMatchTarget& match)
{
	match.debug_set_seat_labels(this->seat_labels(match));
}

void DebugDirectorImpl::filter_match_inputs(std::vector<SlotInput>& inputs)
{
	for (int slot = 0; slot < input_slots::pad_count &&
		slot < static_cast<int>(inputs.size()); ++slot)
	{
		if (this->chord_.suppressing(slot))
		{
			SlotInput& input = inputs[static_cast<std::size_t>(slot)];
			input.toggle_debug = false;
			input.toggle_pause_menu = false;
			input.skip_intro = false;
		}
	}
	// The live console has the keyboard: the keyboard seat stands still (no
	// move, jump, drop, shot, F1 or pause) while it is open and on the frame
	// it closed, whose Escape release is that seat's pause key. Mouse aim is
	// resolved after this, as seat commands are built, and still follows the cursor. Raw reads are
	// never consumed, so which reader sees the keys is this client's policy.
	const std::size_t keyboard = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
	if ((this->live_open_ || this->blank_keyboard_once_) && keyboard < inputs.size())
	{
		SlotInput idle;
		idle.connected = inputs[keyboard].connected;
		inputs[keyboard] = idle;
	}
	this->blank_keyboard_once_ = false;
}

void DebugDirectorImpl::drop_request(const std::string& reason)
{
	MatchRequest dropped = this->request_;
	dropped.restart = false;
	if (!dropped.empty())
	{
		this->log_.warning("dropped queued " + dropped.describe() + ": " + reason);
	}
	this->request_ = MatchRequest();
	this->waiting_notice_shown_ = false;
	if (dropped.exact)
	{
		this->disarm_play(reason);
	}
}

void DebugDirectorImpl::disarm_play(const std::string& reason)
{
	if (this->play_armed_ != nullptr)
	{
		this->log_.warning("play " + this->play_armed_->name() + ": disarmed (" + reason + ")");
		this->play_armed_.reset();
	}
}

bool DebugDirectorImpl::screenshot_waits() const
{
	// Something that applies at this update would change the picture first:
	// in a match a rebuild or leave, in the menus only a queued stage (players
	// or weapon without one, or reloaded content, wait for a match that may
	// never come).
	return this->match_ != nullptr ? !this->request_.empty() || this->leave_requested_ :
		this->request_.stage.has_value();
}

bool DebugDirectorImpl::can_apply() const
{
	return this->match_ != nullptr ? !this->request_.empty() || this->leave_requested_ :
		this->request_.stage.has_value();
}

void DebugDirectorImpl::run_binds(DebugMatchTarget& match)
{
	if (this->application_ == nullptr || this->data_ == nullptr)
	{
		return;
	}
	const Keyboard& keyboard = *this->data_->keyboard();
	// A copy: a bind may rebind keys.
	const auto fired = this->binds_.fired(
		[&keyboard](Key key) { return keyboard.pressed(key); },
		[&keyboard](Key key) { return keyboard.held(key); });
	for (const auto& [chord, line] : fired)
	{
		this->log_.write(DebugSeverity::echo, "[" + upper(debug_keys::chord_name(chord)) + "] " + line);
		const bool paused = this->clock_.paused();
		const int steps = this->clock_.pending_steps();
		this->execute(line);
		// Pause and step change what the match's voices may do.
		if (this->clock_.paused() != paused || this->clock_.pending_steps() != steps)
		{
			match.debug_sync_audio();
		}
	}
}

void DebugDirectorImpl::capture_screenshot_if_ready()
{
	if (!this->pending_screenshot_ || this->application_ == nullptr)
	{
		return;
	}
	if (this->screenshot_waits())
	{
		return;
	}
	const std::string requested = *this->pending_screenshot_;
	this->pending_screenshot_.reset();
	try
	{
		const std::string name = screenshot_file_name(requested,
			std::chrono::system_clock::now());
		std::filesystem::path directory(widen(directory_consts::screenshot_directory()));
		if (!this->screenshot_dir_.empty())
		{
			directory /= widen(this->screenshot_dir_);
		}
		std::filesystem::create_directories(directory);
		const std::filesystem::path path = directory / widen(name);
		// The capture redraws the stack: no perf sample (and the frame after
		// it starts a new interval rather than timing the capture), and the
		// strip only when screenshot.strip is on.
		this->break_perf_interval();
		this->capturing_ = true;
		ScreenCapture image;
		try
		{
			image = capture_stack(*this->application_);
		}
		catch (...)
		{
			this->capturing_ = false;
			throw;
		}
		this->capturing_ = false;
		std::string error;
		if (!write_png(path, image, error))
		{
			this->log_.error("screenshot failed: " + error);
			return;
		}
		const std::u8string text = path.u8string();
		this->log_.info("screenshot: " + std::string(text.begin(), text.end()) + " (" +
			std::to_string(image.width) + "x" + std::to_string(image.height) + ")");
	}
	catch (const std::exception& e)
	{
		this->log_.error(std::string("screenshot failed: ") + e.what());
	}
}

bool DebugDirectorImpl::before_match_update(DebugMatchTarget& match, float dt, bool active)
{
	this->match_hook_active_ = active;
	if (!active) { this->clock_.discard_input_edges(); }
	++this->hook_frames_;
	if (active && (this->poll_debug_menu_toggle(dt) || this->open_console_if_requested(true)))
	{
		return true;
	}

	this->capture_screenshot_if_ready();
	if (active && this->live_open_ && this->live_view_ != nullptr && this->data_ != nullptr)
	{
		// The live console has the keyboard: no binds while it is open.
		if (this->live_view_->update(*this->data_->keyboard(), dt, *this) ==
			DebugConsoleView::Outcome::close)
		{
			this->close_live_console("");
		}
	}
	else if (active)
	{
		this->run_binds(match);
	}
	// A statement's queued rebuild applies below, in this same hook; a
	// screenshot with nothing queued is taken before this step's update.
	++this->script_frames_;
	this->tick_scripts(true, !active);
	if (!active && !this->background_updates_allowed()) { return true; }

	if (this->leave_requested_)
	{
		this->leave_requested_ = false;
		this->request_ = MatchRequest();
		this->disarm_play("the match was left");
		match.debug_leave();
		return true;
	}

	if (this->request_.empty() && !this->has_pending_content())
	{
		return false;
	}

	const MatchPlan plan = this->planner().resolve(&match.debug_settings(),
		this->request_, this->connected_pads(), this->planner_random());
	const std::string described = this->request_.describe();
	this->request_ = MatchRequest();
	this->waiting_notice_shown_ = false;
	if (!plan.ok)
	{
		this->log_.error("cannot apply " + described + ": " + plan.error);
		this->disarm_play("its match could not be planned");
		// Content that validated still waits for the next rebuild; a goto
		// meant for the stage that will not be built does not.
		if (this->pending_goto_)
		{
			this->log_.warning("dropped queued goto " + this->pending_goto_->station +
				": the rebuild it waited for failed");
			this->pending_goto_.reset();
		}
		return false;
	}

	// The build runs before_level_build and after_level_build.
	match.debug_rebuild(plan.settings, plan.reset_session);
	this->log_match(plan, "");
	return true;
}

int DebugDirectorImpl::match_steps(const std::vector<SlotInput>& inputs, float dt)
{
	this->last_dt_bits_ = std::bit_cast<std::uint32_t>(dt);
	this->step_dt_ = dt;
	const int runs = this->clock_.updates_this_step();
	this->latched_ = this->clock_.latch(inputs, runs);
	return runs;
}

bool DebugDirectorImpl::before_match_step(DebugMatchTarget& match, int run, MatchStep& step)
{
	// A round that ends mid fast-forward stops there; GameLevel opens
	// results on its next update, as it would at normal speed.
	if (match.debug_level_finished())
	{
		return false;
	}
	this->replay_was_running_ = this->player_ != nullptr && !this->player_->finished;
	this->before_level_update(match, this->step_dt_);
	if (this->recorder_ != nullptr)
	{
		this->recorder_->set_dt(this->step_dt_);
	}
	if (run == 0)
	{
		step.inputs = &this->latched_;
	}
	else
	{
		this->later_step_ = DebugClock::without_edges(this->latched_);
		step.inputs = &this->later_step_;
	}
	// The perf overlay times the game's own step, seat filter (bots, record,
	// play) included, from here to after_match_step.
	this->step_began_ms_ = perf_now_ms();
	step.seats = this->seat_filter();
	return true;
}

bool DebugDirectorImpl::after_match_step(DebugMatchTarget& match)
{
	this->perf_.add_update(perf_now_ms() - this->step_began_ms_);
	++this->level_updates_;
	++this->total_updates_;
	this->after_level_update(match);
	// A replay that just reached its last update holds that frame, even
	// mid fast-forward.
	if (this->replay_was_running_ && this->player_ != nullptr && this->player_->finished)
	{
		return false;
	}
	// A script waiting for this update (or the end of the countdown)
	// runs before the next one, even at timescale 4.
	return !this->scripts_.should_stop(this->wait_context());
}

void DebugDirectorImpl::end_match_steps(DebugMatchTarget& match, int runs)
{
	if (runs > 0 && this->clock_.paused())
	{
		// A stepped update can start the music or a weapon voice.
		match.debug_sync_audio();
	}
}

void DebugDirectorImpl::cancel_capture_work()
{
	const bool pending = this->scripts_.running() || !this->request_.empty() || this->leave_requested_ ||
		this->pending_goto_.has_value() || this->pending_screenshot_.has_value() ||
		this->pending_tuning_ != nullptr || this->pending_presentation_ != nullptr ||
		this->record_armed_.has_value() || this->play_armed_ != nullptr;
	if (!pending) { return; }
	// Capture starts from page input, after the script tick has returned. Keep
	// one cancellation notice rather than a separate warning for every queue.
	DebugLog quiet;
	quiet.set_mirror_to_debugger(false);
	this->scripts_.stop("raw keyboard capture", quiet);
	this->request_ = MatchRequest();
	this->leave_requested_ = false;
	this->pending_goto_.reset();
	this->pending_screenshot_.reset();
	this->pending_tuning_.reset();
	this->pending_presentation_.reset();
	this->pending_tuning_text_.clear();
	this->pending_presentation_text_.clear();
	this->record_armed_.reset();
	this->play_armed_.reset();
	this->waiting_notice_shown_ = false;
	this->log_.warning("keyboard capture: discarded queued developer work; active recording/replay and settings retained");
}

void DebugDirectorImpl::begin_keyboard_capture()
{
	this->keyboard_capture_ = true;
	this->cancel_capture_work();
	this->chord_.consume(this->debug_pads());
}

void DebugDirectorImpl::capture_input_frame(DebugPageTarget& page)
{
	++this->hook_frames_;
	this->keyboard_capture_ = true;
	this->close_live_console("raw keyboard capture");
	this->break_perf_interval();
	this->cancel_capture_work();
	this->chord_.consume(this->debug_pads());
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->chord_.suppressing(slot)) { page.debug_reset_input(slot); }
	}
}

void DebugDirectorImpl::end_keyboard_capture()
{
	this->cancel_capture_work();
	this->chord_.consume(this->debug_pads());
	this->keyboard_capture_ = false;
}

bool DebugDirectorImpl::before_page_update(DebugPageTarget& page, float dt, bool active)
{
	this->match_hook_active_ = true;
	this->background_inputs_.set_next(nullptr);
	// Existing default hooks retain their policy. Opting in permits only
	// ordinary inactive work; native hide/minimise/power gates still hold it.
	if (!active && this->background_scripts_ && !this->background_updates_allowed()) { return false; }
	if (this->keyboard_capture_)
	{
		this->capture_input_frame(page);
		return false;
	}
	++this->hook_frames_;
	// A page is on top: the live console exists only over a running match,
	// and perf times only frames with the match on top (a page may cover it).
	this->close_live_console("a menu opened");
	this->break_perf_interval();
	if (active && this->poll_debug_menu_toggle(dt))
	{
		return true;
	}
	// Every frame of a chord, including the release frames: Start's release
	// must never reach this page as `pause`.
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->chord_.suppressing(slot))
		{
			page.debug_reset_input(slot);
		}
	}
	if (active && this->open_console_if_requested(false))
	{
		return true;
	}

	this->capture_screenshot_if_ready();
	// Not under the debug menu: someone is browsing it, and a script's lines
	// would race what it stages. The script waits for it to close.
	if (this->debug_menu_ == nullptr)
	{
		++this->script_frames_;
		this->tick_scripts(true);
	}

	// While the debug menu is open it is the page on top (only the console can
	// sit above it, and the console is not a page): what it stages, or what
	// the console queued over it, waits for it to close.
	if (this->debug_menu_ != nullptr)
	{
		const bool queued = this->match_ != nullptr ?
			!this->request_.empty() || this->leave_requested_ : this->request_.stage.has_value();
		if (queued && !this->waiting_notice_shown_)
		{
			this->log_.info("queued: " + this->request_.describe() +
				" applies when the debug menu closes");
			this->waiting_notice_shown_ = true;
		}
		return false;
	}

	if (this->match_ != nullptr)
	{
		if (this->request_.empty() && !this->leave_requested_)
		{
			return false;
		}
		// A pause, controls, results or end page is over the match: close it
		// the way its own controls would, one page a frame, until the match
		// is on top and applies the request (or a menu's restart adopts it).
		if (page.debug_dismiss(this->leave_requested_))
		{
			return true;
		}
		if (!this->waiting_notice_shown_)
		{
			this->log_.info("queued: " + (this->leave_requested_ ? std::string("menu") :
				this->request_.describe()) + " applies when the match resumes");
			this->waiting_notice_shown_ = true;
		}
		return false;
	}

	if (!this->request_.stage)
	{
		return false;
	}

	// Resolve now: the pads have been polled and random choices are fresh.
	const MatchRequest request = this->request_;
	const MatchPlan plan = this->planner().resolve(nullptr, request,
		this->connected_pads(), this->planner_random());
	if (!plan.ok)
	{
		this->log_.error("cannot start " + request.describe() + ": " + plan.error);
		this->request_ = MatchRequest();
		this->disarm_play("its match could not be planned");
		return false;
	}

	// Consumed before the page starts the match, so the match's attach_match
	// finds nothing left over however soon it runs.
	const MatchRequest consumed = std::exchange(this->request_, MatchRequest());
	this->starting_from_plan_ = true;
	if (!page.debug_start_match(plan.settings))
	{
		this->starting_from_plan_ = false;
		this->request_ = consumed;
		// A page with no way to start a match (controls over options) is
		// closed, and the page beneath it starts the match next frame.
		if (page.debug_dismiss(false))
		{
			return true;
		}
		if (!this->waiting_notice_shown_)
		{
			this->log_.info("queued: close this page to start " + request.describe());
			this->waiting_notice_shown_ = true;
		}
		return false;
	}

	this->waiting_notice_shown_ = false;
	this->log_.info("match: starting " + plan.settings.stage() + ", " +
		std::to_string(plan.settings.player_count()) + " player(s)");
	return true;
}

#endif
