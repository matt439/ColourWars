#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "engine/core/file_path.h"
#include "game/build_info.h"
#include "game/debug/bot_driver.h"
#include "game/debug/debug_console_state.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_menu_page.h"
#include "game/debug/debug_script.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_target.h"
#include "game/game_data.h"
#include "game/states/menu_context.h"
#include "game/states/menu_input.h"
#include "game/states/menu_level_settings.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <utility>

using namespace labrador;

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
		"F9 feedback; 'binds' lists the key table, 'exec.list' the scripts.");
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

DebugDirectorImpl::~DebugDirectorImpl() = default;

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
	// Writes only after sampling has stopped, before a script can quit or
	// rebuild. Manual captures are serviced even with no script running.
	this->finish_perf_capture();
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
			return this->pending_screenshot_.has_value() || this->perf_capture_path_.has_value();
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
	// A blocking console can cover an already-paused menu, so the match may
	// receive no new on_suspend callback. Never apply this to console.live.
	if (open && this->match_ != nullptr) { this->match_->debug_sync_audio(); }
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
	if (page != nullptr && this->match_ != nullptr) { this->match_->debug_sync_audio(); }
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
			this->data_->keyboard(), this->data_->tuning());
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

DebugChord::Pads DebugDirectorImpl::debug_pads() const
{
	DebugChord::Pads pads{};
	if (this->data_ == nullptr || this->application_ == nullptr)
	{
		return pads;
	}
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
	return pads;
}

bool DebugDirectorImpl::poll_debug_menu_toggle(float dt)
{
	if (this->data_ == nullptr || this->application_ == nullptr)
	{
		return false;
	}
	const DebugChord::Pads pads = this->debug_pads();
	const bool chord = this->chord_.update(pads, dt);
	const bool f10 = this->data_->keyboard()->pressed(Key::f10);
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

#endif
