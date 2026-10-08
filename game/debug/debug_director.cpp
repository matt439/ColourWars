#include "game/pch.h"
#include "game/debug/debug_director.h"
#include "game/debug/debug_tokenizer.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "game/build_info.h"
#include "game/debug/debug_console_state.h"
#include "game/debug/debug_director_impl.h"
#include "game/game_data.h"
#include "game/states/menu_page.h"
#include <random>
#include <utility>

using namespace labrador;

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

void DebugDirector::before_level_build()
{
	if (this->impl_ != nullptr)
	{
		this->impl_->before_level_build();
	}
}

bool DebugDirector::before_match_update(DebugMatchTarget& match)
{
	return this->impl_ != nullptr && this->impl_->before_match_update(match);
}

void DebugDirector::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
	if (this->impl_ == nullptr)
	{
		match.debug_update_level(inputs, dt);
		return;
	}
	this->impl_->update_match(match, inputs, dt);
}

bool DebugDirector::match_audio_paused() const
{
	return this->impl_ != nullptr && this->impl_->clock().paused();
}

bool DebugDirector::before_page_update(MenuPage& page)
{
	return this->impl_ != nullptr && this->impl_->before_page_update(page);
}

void DebugDirector::draw_status(Renderer& renderer) const
{
	if (this->impl_ != nullptr && !this->impl_->console_open() &&
		this->impl_->data() != nullptr)
	{
		DebugConsoleState::draw_strip(renderer, *this->impl_->data(),
			this->impl_->status_strip());
	}
}

// ---------------------------------------------------------------------------

DebugDirectorImpl::DebugDirectorImpl(GameTuning* tuning, Presentation* presentation) :
	tuning_(tuning), presentation_(presentation)
{
	std::string label;
	for (const wchar_t* c = build_info::label; *c != 0; ++c)
	{
		label += *c < 0x80 ? static_cast<char>(*c) : '?';
	}
	this->log_.info("ColourWars developer console - " + label);
	this->log_.info("` toggles this console; type help. F5 restart, F6 pause, "
		"F7 step, F8 visual, F9 feedback.");
	this->register_commands();
}

DebugDirectorImpl::~DebugDirectorImpl() = default;

void DebugDirectorImpl::attach(Application* application, GameData* data)
{
	this->application_ = application;
	this->data_ = data;
}

DebugStatus DebugDirectorImpl::execute(std::string_view line)
{
	return this->registry_.execute(line, this->log_);
}

void DebugDirectorImpl::run_launch_statements(const std::vector<std::string>& statements)
{
	for (const std::string& statement : statements)
	{
		this->log_.write(DebugSeverity::echo, "launch: " + statement);
		this->execute(statement);
	}
}

void DebugDirectorImpl::set_console_open(bool open)
{
	this->console_open_ = open;
}

bool DebugDirectorImpl::console_open() const
{
	return this->console_open_;
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

bool DebugDirectorImpl::open_console_if_requested()
{
	if (this->console_open_ || this->data_ == nullptr || this->application_ == nullptr ||
		!this->data_->keyboard()->pressed(Key::grave))
	{
		return false;
	}
	// Deferred: issued from inside the top state's update.
	this->application_->push(std::make_unique<DebugConsoleState>(this->data_, this));
	return true;
}

bool DebugDirectorImpl::has_pending_content() const
{
	return this->pending_tuning_ != nullptr && this->pending_presentation_ != nullptr;
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
	if (match.debug_settings().stage() == "movement_course")
	{
		// A comparison chosen on the stage-select page carries into commands.
		this->remembered_visual_ = match.debug_settings().workshop_visual_mode();
	}
	// A match started by the console was given its plan and the request was
	// consumed; anything still queued was stored in the menus without `stage`
	// (or left by a match that ended) and does not belong to this one.
	this->drop_request("this match was started from the menus");
}

void DebugDirectorImpl::detach_match(const DebugMatchTarget& match)
{
	if (this->match_ == &match)
	{
		this->match_ = nullptr;
	}
	// A rebuild queued for this match means nothing once it has ended.
	this->drop_request("the match ended");
	this->leave_requested_ = false;
}

void DebugDirectorImpl::before_level_build()
{
	this->apply_pending_content();
	// This build is the restart.
	this->request_.restart = false;
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
	if (keyboard.pressed(Key::f5))
	{
		this->log_.write(DebugSeverity::echo, "[F5] restart");
		this->execute("restart");
	}
	if (keyboard.pressed(Key::f6))
	{
		this->log_.write(DebugSeverity::echo, "[F6] pause toggle");
		this->execute("pause toggle");
		match.debug_sync_audio();
	}
	if (keyboard.pressed(Key::f7))
	{
		this->log_.write(DebugSeverity::echo, "[F7] step");
		this->execute("step 1");
		match.debug_sync_audio();
	}
	if (keyboard.pressed(Key::f8))
	{
		this->log_.write(DebugSeverity::echo, "[F8] visual next");
		this->execute("visual next");
	}
	if (keyboard.pressed(Key::f9))
	{
		this->log_.write(DebugSeverity::echo, "[F9] feedback toggle");
		this->execute("feedback toggle");
	}
}

bool DebugDirectorImpl::before_match_update(DebugMatchTarget& match)
{
	if (this->open_console_if_requested())
	{
		return true;
	}

	this->run_binds(match);

	if (this->leave_requested_)
	{
		this->leave_requested_ = false;
		this->request_ = MatchRequest();
		match.debug_leave();
		return true;
	}

	if (this->request_.empty() && !this->has_pending_content())
	{
		return false;
	}

	std::mt19937 engine(std::random_device{}());
	const MatchPlan plan = this->planner().resolve(&match.debug_settings(),
		this->request_, this->connected_pads(),
		[&engine] { return static_cast<unsigned int>(engine()); });
	const std::string described = this->request_.describe();
	this->request_ = MatchRequest();
	if (!plan.ok)
	{
		this->log_.error("cannot apply " + described + ": " + plan.error);
		// Content that validated still waits for the next rebuild.
		return false;
	}

	this->remembered_visual_ = plan.settings.workshop_visual_mode();
	match.debug_rebuild(plan.settings, plan.reset_session);
	this->clock_.reset_accumulator();
	if (this->clock_.paused())
	{
		// The new level's voices start paused too.
		match.debug_sync_audio();
	}
	this->log_.info("match: " + plan.settings.stage() + ", " +
		std::to_string(plan.settings.player_count()) + " player(s), visual " +
		MatchPlanner::visual_name(plan.settings.workshop_visual_mode()) +
		(plan.reset_session ? " (new session)" : ""));
	return true;
}

void DebugDirectorImpl::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
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
		match.debug_update_level(run == 0 ? latched : DebugClock::without_edges(latched), dt);
	}
	if (runs > 0 && this->clock_.paused())
	{
		// A stepped update can start the music or a weapon voice.
		match.debug_sync_audio();
	}
}

bool DebugDirectorImpl::before_page_update(MenuPage& page)
{
	if (this->open_console_if_requested())
	{
		return true;
	}

	if (!this->request_.stage)
	{
		return false;
	}

	if (this->match_ != nullptr)
	{
		if (!this->waiting_notice_shown_)
		{
			this->log_.info("queued: " + this->request_.describe() +
				" applies when the match resumes");
			this->waiting_notice_shown_ = true;
		}
		return false;
	}

	// Resolve now: the pads have been polled and random choices are fresh.
	MatchRequest request = this->request_;
	if (!request.visual)
	{
		request.visual = this->remembered_visual_;
	}
	std::mt19937 engine(std::random_device{}());
	const MatchPlan plan = this->planner().resolve(nullptr, request,
		this->connected_pads(), [&engine] { return static_cast<unsigned int>(engine()); });
	if (!plan.ok)
	{
		this->log_.error("cannot start " + request.describe() + ": " + plan.error);
		this->request_ = MatchRequest();
		return false;
	}

	// Consumed before the page starts the match, so the match's attach_match
	// finds nothing left over however soon it runs.
	const MatchRequest consumed = std::exchange(this->request_, MatchRequest());
	if (!page.debug_start_match(plan.settings))
	{
		this->request_ = consumed;
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
