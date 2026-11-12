#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_director_impl.h"
#include "game/debug/capture/debug_screenshot.h"
#include "game/debug/replay/timelapse.h"
#include <chrono>

// replay.timelapse: the finished round's whole tape, played again from one
// arena camera (REV#48). The command arms GameLevel's instant replay
// (game/states/game_level_replay.cpp) with a speed; the page hook then takes
// the round's Results and End pages one step a frame to Watch Replay, and
// GameLevel calls timelapse_frame after each frame it plays, where the
// requested stills are saved. See docs/debug-console.md, "Time-lapse".

namespace
{
	// "20261112-143005-123" from the screenshot default's
	// "colourwars-20261112-143005-123.png".
	std::string run_stamp()
	{
		std::string name = screenshot_file_name(std::string(), std::chrono::system_clock::now());
		const std::string prefix = "colourwars-";
		if (name.rfind(prefix, 0) == 0) { name.erase(0, prefix.size()); }
		if (name.size() > 4) { name.resize(name.size() - 4); }
		return name;
	}
}

bool DebugDirectorImpl::timelapse_active() const
{
	if (!this->timelapse_) { return false; }
	// Playing until GameLevel stops showing it, which a confirm, back or
	// pause press may do before the last step.
	return !this->timelapse_->started ||
		(this->match_ != nullptr && this->match_->debug_timelapse_running());
}

void DebugDirectorImpl::drop_timelapse(const std::string& reason)
{
	if (!this->timelapse_) { return; }
	if (!this->timelapse_->started)
	{
		this->log_.warning("replay.timelapse: disarmed (" + reason + ")");
	}
	this->timelapse_.reset();
	this->timelapse_notice_shown_ = false;
	if (this->match_ != nullptr) { this->match_->debug_arm_timelapse(0); }
}

bool DebugDirectorImpl::advance_timelapse(DebugPageTarget& page)
{
	if (!this->timelapse_ || this->match_ == nullptr) { return false; }
	if (this->timelapse_->started)
	{
		// A page is on top again: the time-lapse is over. One that reached
		// its last step has already said so and gone.
		if (!this->match_->debug_timelapse_running())
		{
			this->log_.info("replay.timelapse: ended early, " + std::to_string(this->timelapse_->saved) +
				" of " + std::to_string(this->timelapse_->stills) + " still(s) saved");
			this->timelapse_.reset();
		}
		return false;
	}
	if (page.debug_watch_replay())
	{
		this->timelapse_notice_shown_ = false;
		return true;
	}
	if (this->match_->debug_level_finished() && !this->match_->debug_round_taped())
	{
		this->drop_timelapse("the round's tape cannot be replayed");
		return false;
	}
	if (!this->timelapse_notice_shown_)
	{
		this->log_.info("queued: replay.timelapse starts from the round's End menu");
		this->timelapse_notice_shown_ = true;
	}
	return false;
}

void DebugDirectorImpl::timelapse_frame(DebugMatchTarget& /*match*/, int step, int steps)
{
	if (!this->timelapse_) { return; }
	Timelapse& run = *this->timelapse_;
	if (!run.started)
	{
		run.started = true;
		this->log_.info("replay.timelapse: playing " + std::to_string(steps) + " taped steps at x" +
			std::to_string(run.speed) + "; confirm, back or pause ends it");
	}
	while (run.next < run.stills && step >= timelapse::still_step(run.next, run.stills, steps))
	{
		if (this->write_screenshot(timelapse::still_name(run.stamp, run.next))) { ++run.saved; }
		++run.next;
	}
	if (step >= steps)
	{
		this->log_.info("replay.timelapse: done, " + std::to_string(steps) + " taped steps at x" +
			std::to_string(run.speed) + ", " + std::to_string(run.saved) + " of " +
			std::to_string(run.stills) + " still(s) saved");
		this->timelapse_.reset();
	}
}

void DebugDirectorImpl::register_timelapse_commands()
{
	using Args = const DebugArgs&;
	this->registry_.add_command("replay.timelapse",
		"play the round's whole tape again from one arena camera when its End menu opens: "
		"[speed] taped steps a frame (1-64, default 16), [stills] evenly spaced PNGs (0-32); off disarms",
		{ { "speed|off", true, [] { return std::vector<std::string>{ "off" }; } },
			{ "stills", true, {} } },
		[this](Args args, DebugLog&)
		{
			if (args.has(0) && args.text(0) == "off")
			{
				if (!this->timelapse_)
				{
					return DebugStatus::failure("replay.timelapse off: none is armed");
				}
				if (this->timelapse_->started)
				{
					return DebugStatus::failure("replay.timelapse off: it is playing; confirm, back "
						"or pause ends it");
				}
				this->drop_timelapse("replay.timelapse off");
				return DebugStatus::success("replay.timelapse: disarmed");
			}
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("replay.timelapse: no match (it plays a round's tape)");
			}
			if (this->timelapse_)
			{
				return DebugStatus::failure("replay.timelapse: one is already armed "
					"(replay.timelapse off first)");
			}
			int speed = timelapse::default_speed;
			if (args.has(0) && (!DebugArgs::parse_int(args.text(0), speed) || speed < 1 ||
				speed > timelapse::max_speed))
			{
				return DebugStatus::failure("replay.timelapse: speed must be 1-" +
					std::to_string(timelapse::max_speed) + " taped steps a frame, or off");
			}
			int stills = 0;
			if (args.has(1) && (!DebugArgs::parse_int(args.text(1), stills) || stills < 0 ||
				stills > timelapse::max_stills))
			{
				return DebugStatus::failure("replay.timelapse: stills must be 0-" +
					std::to_string(timelapse::max_stills));
			}
			if (!this->match_->debug_round_taped())
			{
				return DebugStatus::failure("replay.timelapse: this round has no playable tape "
					"(untimed practice keeps none; place, respawn, kill or a late cheat change spoil it)");
			}
			this->timelapse_ = Timelapse{ speed, stills, 0, 0, run_stamp(), false };
			this->timelapse_notice_shown_ = false;
			this->match_->debug_arm_timelapse(speed);
			const std::string what = "replay.timelapse x" + std::to_string(speed) +
				(stills > 0 ? ", " + std::to_string(stills) + " still(s) as " +
					timelapse::still_name(this->timelapse_->stamp, 0) + "..." : std::string());
			return DebugStatus::success(what + (this->match_->debug_level_finished() ?
				": starting from the End menu" : ": starts when the round's End menu opens"), true);
		}, DebugMenuTraits::of("replay", DebugMenuTraits::Form::run));
}
#endif
