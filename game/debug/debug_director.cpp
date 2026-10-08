#include "game/pch.h"
#include "game/debug/debug_director.h"
#include "game/debug/debug_tokenizer.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/build_info.h"
#include "game/debug/bot_driver.h"
#include "game/debug/debug_console_state.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_menu_page.h"
#include "game/debug/debug_screenshot.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_target.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
#include "game/states/menu_context.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"
#include <bit>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <utility>

using namespace labrador;

namespace
{
	// F10 never reaches the engine as a key the game can bind: the window
	// procedure hands WM_SYSKEYUP to DefWindowProc, which opens the system
	// menu's modal loop and freezes the game until the next key. A thread
	// message hook, installed only while a Debug director is attached, takes
	// F10 out of the queue before the window sees it and counts the press.
	// Engine report 11 asks for a way to do this without a hook.
	int f10_presses = 0;

	LRESULT CALLBACK f10_message_hook(int code, WPARAM remove, LPARAM message_pointer)
	{
		if (code == HC_ACTION && message_pointer != 0)
		{
			MSG* message = reinterpret_cast<MSG*>(message_pointer);
			const bool key = message->message == WM_SYSKEYDOWN || message->message == WM_SYSKEYUP ||
				message->message == WM_KEYDOWN || message->message == WM_KEYUP;
			// Bit 29 is Alt: Alt+F10 stays the system's.
			const bool alt = (message->lParam & (1 << 29)) != 0;
			if (key && message->wParam == VK_F10 && !alt)
			{
				const bool down = message->message == WM_SYSKEYDOWN || message->message == WM_KEYDOWN;
				// Bit 30: the key was already down (auto-repeat).
				const bool repeat = (message->lParam & (1 << 30)) != 0;
				if (remove == PM_REMOVE && down && !repeat)
				{
					++f10_presses;
				}
				message->message = WM_NULL;
			}
		}
		return CallNextHookEx(nullptr, code, remove, message_pointer);
	}

	bool take_f10_press()
	{
		const bool pressed = f10_presses > 0;
		f10_presses = 0;
		return pressed;
	}

	std::string upper(std::string text)
	{
		for (char& c : text)
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		return text;
	}
}

DebugDirector::DebugDirector() = default;

DebugDirector::DebugDirector(GameTuning* tuning, Presentation* presentation)
{
	if (tuning != nullptr && presentation != nullptr)
	{
		this->impl_ = std::make_unique<DebugDirectorImpl>(tuning, presentation);
	}
}

DebugDirector::~DebugDirector() = default;

DebugDirector& DebugDirector::inert()
{
	static DebugDirector director;
	return director;
}

void DebugDirector::attach(Application* application, GameData* data)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->attach(application, data);
	}
}

void DebugDirector::run_launch_arguments(const std::vector<std::wstring>& arguments)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->run_launch_statements(parse_launch_arguments(arguments));
	}
}

void DebugDirector::attach_match(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->attach_match(match);
	}
}

void DebugDirector::detach_match(const DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->detach_match(match);
	}
}

void DebugDirector::before_level_build(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->before_level_build(match);
	}
}

void DebugDirector::after_level_build(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->after_level_build(match);
	}
}

void DebugDirector::filter_match_inputs(std::vector<PlayerInputData>& inputs) const
{
	if (this->impl_ != nullptr)
	{
		this->impl_->filter_match_inputs(inputs);
	}
}

bool DebugDirector::before_match_update(DebugMatchTarget& match, float dt)
{
	return this->impl_ != nullptr && this->impl_->before_match_update(match, dt);
}

void DebugDirector::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
	if (this->impl_ == nullptr)
	{
		match.debug_update_level(inputs, dt, nullptr);
		return;
	}
	this->impl_->update_match(match, inputs, dt);
}

bool DebugDirector::match_audio_paused() const
{
	return this->impl_ != nullptr && this->impl_->clock().paused();
}

void DebugDirector::match_suspended(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->match_suspended(match);
	}
}

bool DebugDirector::drives_slot(int slot) const
{
	return this->impl_ != nullptr && this->impl_->drives_slot(slot);
}

bool DebugDirector::before_page_update(DebugPageTarget& page, float dt)
{
	return this->impl_ != nullptr && this->impl_->before_page_update(page, dt);
}

void DebugDirector::draw_status(Renderer& renderer) const
{
	if (this->impl_ != nullptr && this->impl_->data() != nullptr)
	{
		this->impl_->draw_status(renderer);
	}
}

void DebugDirector::begin_match_draw() const
{
	if (this->impl_ != nullptr)
	{
		this->impl_->begin_match_draw();
	}
}

// ---------------------------------------------------------------------------

DebugDirectorImpl::DebugDirectorImpl(GameTuning* tuning, Presentation* presentation) :
	tuning_(tuning), presentation_(presentation), planner_random_(std::random_device{}())
{
	std::string label;
	for (const wchar_t* c = build_info::label; *c != 0; ++c)
	{
		label += *c < 0x80 ? static_cast<char>(*c) : '?';
	}
	this->log_.info("ColourWars developer console - " + label);
	this->log_.info("` toggles this console; type help. F10/F11 (or hold pad Back+Start) "
		"opens the debug menu. F2-F4 overlays, F5 restart, F6 pause, F7 step, "
		"F8 visual, F9 feedback; 'binds' lists the key table, 'exec.list' the scripts.");
	this->binds_.reset();
	this->register_commands();
	this->register_view_commands();
	this->register_player_commands();
	this->register_bot_commands();
	this->register_replay_commands();
	this->register_tune_commands();
	this->register_script_commands();
	this->tune_targets_[0] = std::make_unique<TuningTarget>(tuning, presentation);
	this->tune_targets_[1] = std::make_unique<PresentationTarget>(presentation, tuning);
}

DebugDirectorImpl::~DebugDirectorImpl()
{
	if (this->f10_hook_ != nullptr)
	{
		UnhookWindowsHookEx(static_cast<HHOOK>(this->f10_hook_));
	}
}

DebugStatus DebugDirectorImpl::submit_console_line(const std::string& line)
{
	this->log_.write(DebugSeverity::echo, "] " + line);
	const DebugStatus status = this->execute(line);
	this->save_history();
	return status;
}

void DebugDirectorImpl::attach(Application* application, GameData* data)
{
	this->application_ = application;
	this->data_ = data;
	// The running game's baseline is the staged files main.cpp just loaded.
	if (application != nullptr)
	{
		this->load_tune_documents();
	}
	if (application != nullptr && this->f10_hook_ == nullptr)
	{
		this->f10_hook_ = SetWindowsHookExW(WH_GETMESSAGE, f10_message_hook, nullptr,
			GetCurrentThreadId());
		if (this->f10_hook_ == nullptr)
		{
			this->log_.warning("F10 is the system's (the message hook failed); use F11 "
				"for the debug menu");
		}
	}
}

DebugStatus DebugDirectorImpl::execute(std::string_view line)
{
	return this->registry_.execute(line, this->log_);
}

// The launch arguments are a script frame that keeps going after an error, as
// launch statements always have; autoexec.cfg, when the profile has one, is a
// file frame above it, so it runs first. One synchronous tick runs both up to
// their first wait, so `+stage x +players 4` is all queued before the first
// page update, as before.
void DebugDirectorImpl::run_launch_statements(const std::vector<std::string>& statements)
{
	this->load_persistent();
	if (!statements.empty())
	{
		DebugScriptRunner::Frame launch;
		launch.name = "launch";
		launch.keep_going = true;
		for (const std::string& statement : statements)
		{
			launch.statements.push_back({ "launch", 0, statement });
		}
		this->scripts_.push(std::move(launch), true);
	}
	if (const std::optional<std::filesystem::path> directory = this->user_directory())
	{
		const std::filesystem::path path = *directory / L"autoexec.cfg";
		std::error_code ignored;
		if (std::filesystem::is_regular_file(path, ignored))
		{
			std::ifstream stream(path, std::ios::binary);
			const std::string text((std::istreambuf_iterator<char>(stream)),
				std::istreambuf_iterator<char>());
			const ParsedScript parsed = parse_script("autoexec.cfg", text, {});
			if (!parsed.ok)
			{
				this->log_.error("autoexec: " + parsed.error + "; not run");
			}
			else
			{
				this->log_.info("autoexec: " + labrador::path_to_utf8(path));
				DebugScriptRunner::Frame frame;
				frame.name = "autoexec.cfg";
				frame.statements = parsed.statements;
				this->scripts_.push(std::move(frame), true);
			}
		}
	}
	this->tick_scripts(false);
}

void DebugDirectorImpl::tick_scripts(bool from_hook)
{
	if (!this->scripts_.running())
	{
		return;
	}
	this->inline_capture_ = from_hook;
	this->scripts_.tick(
		[this](const DebugScriptStatement& statement)
		{
			return this->registry_.execute(statement.text, this->log_);
		},
		[this] { return this->wait_context(); },
		[this]
		{
			if (this->pending_screenshot_ && this->inline_capture_)
			{
				this->capture_screenshot_if_ready();
			}
			return this->pending_screenshot_.has_value();
		},
		this->log_);
	this->inline_capture_ = false;
}

DebugWaitContext DebugDirectorImpl::wait_context() const
{
	DebugWaitContext context;
	context.updates = this->total_updates_;
	context.frames = this->script_frames_;
	context.in_match = this->match_ != nullptr;
	context.round_over = this->match_ != nullptr && this->match_->debug_level_finished();
	context.build_pending = this->match_ != nullptr ?
		this->rebuild_queued() && !this->leave_requested_ :
		this->request_.stage.has_value() || this->starting_from_plan_;
	context.match_ready = this->match_ != nullptr && !this->rebuild_queued() &&
		!this->leave_requested_ && this->match_->debug_countdown_over();
	context.replay_active = this->play_armed_ != nullptr ||
		(this->player_ != nullptr && !this->player_->finished);
	context.recording = this->recorder_ != nullptr || this->record_armed_.has_value();
	context.record_armed = this->recorder_ == nullptr && this->record_armed_.has_value();
	context.paused = this->clock_.paused();
	// GameData forwards the keyboard to the application: ask only with one.
	context.window_active = this->application_ == nullptr || this->data_ == nullptr ||
		this->data_->keyboard() == nullptr || this->data_->keyboard()->focused();
	return context;
}

void DebugDirectorImpl::set_console_open(bool open)
{
	this->console_open_ = open;
}

bool DebugDirectorImpl::console_open() const
{
	return this->console_open_ || this->live_open_;
}

void DebugDirectorImpl::open_live_console()
{
	this->live_view_ = std::make_unique<DebugConsoleView>(true);
	this->live_view_->init(*this->data_);
	this->live_open_ = true;
	this->log_.info("console: live (the match keeps running; the keyboard seat is idle while "
		"it is open)");
}

void DebugDirectorImpl::close_live_console(const std::string& reason)
{
	if (!this->live_open_)
	{
		return;
	}
	// The view is kept until the next open: this may run inside its update.
	this->live_open_ = false;
	this->blank_keyboard_once_ = true;
	this->log_.info("console: live closed" + (reason.empty() ? std::string() : " (" + reason + ")"));
}

void DebugDirectorImpl::set_live_console_open(bool open)
{
	if (open)
	{
		this->live_open_ = true;
	}
	else
	{
		this->close_live_console("");
	}
}

void DebugDirectorImpl::set_debug_menu(DebugMenuPage* page)
{
	this->debug_menu_ = page;
	if (page == nullptr)
	{
		this->waiting_notice_shown_ = false;
	}
}

MenuContext* DebugDirectorImpl::menu_context()
{
	if (this->menu_context_ == nullptr && this->data_ != nullptr)
	{
		this->menu_input_ = std::make_unique<MenuInput>(this->data_->gamepads(),
			this->data_->keyboard());
		this->menu_settings_ = std::make_unique<MenuLevelSettings>();
		this->menu_context_ = std::make_unique<MenuContext>(this->data_,
			this->menu_input_.get(), this->menu_settings_.get());
	}
	return this->menu_context_.get();
}

MatchPlanner DebugDirectorImpl::planner() const
{
	return MatchPlanner(this->data_ != nullptr ? this->data_->stages() : nullptr);
}

MatchPlanner::ConnectedPads DebugDirectorImpl::connected_pads() const
{
	MatchPlanner::ConnectedPads pads{};
	if (this->data_ != nullptr && this->data_->application() != nullptr)
	{
		for (int slot = 0; slot < static_cast<int>(pads.size()); ++slot)
		{
			pads[static_cast<std::size_t>(slot)] = this->data_->gamepads()->connected(slot);
		}
	}
	return pads;
}

bool DebugDirectorImpl::open_console_if_requested(bool match_hook)
{
	// An open live console reads grave itself (to close).
	if (this->console_open_ || this->live_open_ || this->data_ == nullptr ||
		this->application_ == nullptr || !this->data_->keyboard()->pressed(Key::grave))
	{
		return false;
	}
	if (match_hook && this->live_console_)
	{
		// Over a running match only; the update goes ahead this frame.
		this->open_live_console();
		return false;
	}
	// Deferred: issued from inside the top state's update.
	this->application_->push(std::make_unique<DebugConsoleState>(this->data_, this));
	return true;
}

bool DebugDirectorImpl::poll_debug_menu_toggle(float dt)
{
	if (this->data_ == nullptr || this->application_ == nullptr)
	{
		return false;
	}
	DebugChord::Pads pads{};
	const Gamepads& gamepads = *this->data_->gamepads();
	for (int slot = 0; slot < static_cast<int>(pads.size()); ++slot)
	{
		DebugChord::Pad& pad = pads[static_cast<std::size_t>(slot)];
		pad.connected = gamepads.connected(slot);
		if (pad.connected)
		{
			pad.back = gamepads.state(slot).is_down(GamepadButton::back);
			pad.start = gamepads.state(slot).is_down(GamepadButton::start);
		}
	}
	const bool chord = this->chord_.update(pads, dt);
	const bool f10 = take_f10_press();
	const bool f11 = this->data_->keyboard()->pressed(Key::f11);
	if (!chord && !f10 && !f11)
	{
		return false;
	}
	if (this->debug_menu_ != nullptr)
	{
		this->debug_menu_->request_close();
	}
	else
	{
		this->open_debug_menu();
	}
	return true;
}

void DebugDirectorImpl::forget_menu_key()
{
	take_f10_press();
}

void DebugDirectorImpl::open_debug_menu()
{
	MenuContext* context = this->menu_context();
	if (context == nullptr || this->application_ == nullptr)
	{
		return;
	}
	// F10 typed into the live console is still the debug menu's key; the
	// menu is a page over the match, so the live console closes.
	this->close_live_console("the debug menu opened");
	// Deferred, like the console: pushed from inside the top state's update,
	// with no result, so it can sit over any state.
	this->application_->push(std::make_unique<DebugMenuPage>(context, this));
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
	this->match_ = &match;
	this->waiting_notice_shown_ = false;
	// Perf times match frames: not the time spent in the menus before this.
	this->perf_.break_interval();
	if (match.debug_settings().stage() == "movement_course")
	{
		// A comparison chosen on the stage-select page carries into commands.
		this->remembered_visual_ = match.debug_settings().workshop_visual_mode();
	}
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
		std::to_string(plan.settings.player_count()) + " player(s), visual " +
		MatchPlanner::visual_name(plan.settings.workshop_visual_mode()) +
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
			this->remembered_visual_ = plan.settings.workshop_visual_mode();
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
		this->god_ = file.god;
		this->infinite_ammo_ = file.infinite_ammo;
	}
	else if (this->seed_)
	{
		std::srand(*this->seed_);
	}
}

void DebugDirectorImpl::after_level_build(DebugMatchTarget& match)
{
	match.debug_set_overlays({ this->overlay_collision_, this->overlay_camera_ });
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
	this->perf_.break_interval();
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

std::vector<std::wstring> DebugDirectorImpl::seat_labels(const DebugMatchTarget& match) const
{
	const std::vector<MenuPlayerSettings> seats = match.debug_settings().player_settings();
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

void DebugDirectorImpl::filter_match_inputs(std::vector<PlayerInputData>& inputs) const
{
	for (int slot = 0; slot < input_slots::pad_count &&
		slot < static_cast<int>(inputs.size()); ++slot)
	{
		if (this->chord_.suppressing(slot))
		{
			PlayerInputData& input = inputs[static_cast<std::size_t>(slot)];
			input.toggle_debug = false;
			input.toggle_pause_menu = false;
		}
	}
	// The live console has the keyboard: the keyboard seat stands still (no
	// move, jump, drop, shot, F1 or pause) while it is open and on the frame
	// it closed, whose Escape release is that seat's pause key. Mouse aim is
	// read by the level directly and still follows the cursor (engine
	// report 9).
	const std::size_t keyboard = static_cast<std::size_t>(input_slots::keyboard_and_mouse);
	if ((this->live_open_ || this->blank_keyboard_once_) && keyboard < inputs.size())
	{
		PlayerInputData idle;
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

PlayerVisualMode DebugDirectorImpl::planned_visual() const
{
	if (this->request_.visual)
	{
		return *this->request_.visual;
	}
	if (this->match_ != nullptr && this->match_->debug_settings().stage() == "movement_course")
	{
		return this->match_->debug_settings().workshop_visual_mode();
	}
	return this->remembered_visual_;
}

void DebugDirectorImpl::queue_visual(PlayerVisualMode mode)
{
	this->request_.visual = mode;
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
		this->perf_.break_interval();
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

bool DebugDirectorImpl::before_match_update(DebugMatchTarget& match, float dt)
{
	++this->hook_frames_;
	if (this->poll_debug_menu_toggle(dt) || this->open_console_if_requested(true))
	{
		return true;
	}

	this->capture_screenshot_if_ready();
	if (this->live_open_ && this->live_view_ != nullptr && this->data_ != nullptr)
	{
		// The live console has the keyboard: no binds while it is open.
		if (this->live_view_->update(*this->data_->keyboard(), dt, *this) ==
			DebugConsoleView::Outcome::close)
		{
			this->close_live_console("");
		}
	}
	else
	{
		this->run_binds(match);
	}
	// A statement's queued rebuild applies below, in this same hook; a
	// screenshot with nothing queued is taken before this step's update.
	++this->script_frames_;
	this->tick_scripts(true);

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

	this->remembered_visual_ = plan.settings.workshop_visual_mode();
	// The build runs before_level_build and after_level_build.
	match.debug_rebuild(plan.settings, plan.reset_session);
	this->log_match(plan, "");
	return true;
}

void DebugDirectorImpl::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
	this->last_dt_bits_ = std::bit_cast<std::uint32_t>(dt);
	const int runs = this->clock_.updates_this_step();
	const std::vector<PlayerInputData> latched = this->clock_.latch(inputs, runs);
	for (int run = 0; run < runs; ++run)
	{
		// A round that ends mid fast-forward stops there; GameLevel opens
		// results on its next update, as it would at normal speed.
		if (match.debug_level_finished())
		{
			break;
		}
		const bool replay_was_running = this->player_ != nullptr && !this->player_->finished;
		this->before_level_update(match, dt);
		// The perf overlay times the game's own Level::update, seat filter
		// (bots, record, play) included.
		const double start = perf_now_ms();
		match.debug_update_level(run == 0 ? latched : DebugClock::without_edges(latched), dt,
			this->seat_filter());
		this->perf_.add_update(perf_now_ms() - start);
		++this->level_updates_;
		++this->total_updates_;
		this->after_level_update(match);
		// A replay that just reached its last update holds that frame, even
		// mid fast-forward.
		if (replay_was_running && this->player_ != nullptr && this->player_->finished)
		{
			break;
		}
		// A script waiting for this update (or the end of the countdown)
		// runs before the next one, even at timescale 4.
		if (this->scripts_.should_stop(this->wait_context()))
		{
			break;
		}
	}
	if (runs > 0 && this->clock_.paused())
	{
		// A stepped update can start the music or a weapon voice.
		match.debug_sync_audio();
	}
}

bool DebugDirectorImpl::before_page_update(DebugPageTarget& page, float dt)
{
	++this->hook_frames_;
	// A page is on top: the live console exists only over a running match,
	// and perf times only frames with the match on top (a page may cover it).
	this->close_live_console("a menu opened");
	this->perf_.break_interval();
	if (this->poll_debug_menu_toggle(dt))
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
	if (this->open_console_if_requested(false))
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
	MatchRequest request = this->request_;
	if (!request.visual)
	{
		request.visual = this->remembered_visual_;
	}
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
	this->remembered_visual_ = plan.settings.workshop_visual_mode();
	this->log_.info("match: starting " + plan.settings.stage() + ", " +
		std::to_string(plan.settings.player_count()) + " player(s), visual " +
		MatchPlanner::visual_name(plan.settings.workshop_visual_mode()));
	return true;
}

#else

// Release: no implementation exists; the type is complete only so the
// unique_ptr member can be destroyed.
class DebugDirectorImpl
{
};

DebugDirector::DebugDirector() = default;

DebugDirector::DebugDirector(GameTuning*, Presentation*)
{
}

DebugDirector::~DebugDirector() = default;

DebugDirector& DebugDirector::inert()
{
	static DebugDirector director;
	return director;
}

void DebugDirector::run_launch_arguments(const std::vector<std::wstring>& arguments)
{
	if (has_launch_statements(arguments))
	{
		std::fputs("developer arguments ignored: Release build\n", stderr);
	}
}

#endif
