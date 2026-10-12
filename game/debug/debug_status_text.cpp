#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "game/debug/bot_driver.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_variable.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/input_slots.h"
#include "game/presentation.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace labrador;

namespace
{
	std::string device_label(int slot)
	{
		if (input_slots::is_keyboard_and_mouse(slot))
		{
			return "kbm";
		}
		if (input_slots::is_pad(slot))
		{
			return "pad" + std::to_string(slot + 1);
		}
		return "idle";
	}

	constexpr const char* course_stage = "movement_course";
}

std::vector<std::string> DebugDirectorImpl::status_lines() const
{
	std::vector<std::string> lines;
	const int depth = this->application_ != nullptr ? this->application_->depth() : 0;
	lines.push_back("stack depth " + std::to_string(depth) +
		(this->match_ != nullptr ? ", in a match" : ", in the menus"));

	if (this->match_ != nullptr)
	{
		const MenuLevelSettings& settings = this->match_->debug_settings();
		lines.push_back("match: stage " + settings.stage() + ", " +
			std::to_string(settings.player_count()) + " player(s), visual " +
			MatchPlanner::visual_name(settings.workshop_visual_mode()) +
			", best of " + std::to_string(settings.best_of()));
		const std::vector<MenuPlayerSettings> players = settings.player_settings();
		const std::vector<std::string> views = this->match_->debug_status();
		for (std::size_t i = 0; i < players.size(); ++i)
		{
			const std::string bot = this->player_ != nullptr ? std::string("replay") :
				this->bot_driver_ != nullptr ? this->bot_for_seat(static_cast<int>(i), players[i].num) :
				std::string();
			lines.push_back("  P" + std::to_string(i + 1) + " " + device_label(players[i].num) +
				" team " + (players[i].team == PlayerTeam::a ? "a" : "b") + " " +
				MatchPlanner::weapon_name(players[i].weapon) +
				(bot.empty() ? std::string() : " bot " + bot) +
				(i < views.size() ? "  " + views[i] : std::string()));
		}
		lines.push_back("updates " + std::to_string(this->level_updates_) + " since the build" +
			(this->recorder_ != nullptr ? ", recording " + this->recorder_->name() : std::string()) +
			(this->player_ != nullptr ? ", replaying " + this->player_->name() : std::string()));
	}
	lines.push_back(std::string("seed ") + (this->seed_ ? std::to_string(*this->seed_) :
		std::string("off (time)")) + "; bots: dummies " +
		(this->dummy_bot_.empty() ? std::string("off") : this->dummy_bot_) +
		(this->seat_bots_.empty() ? std::string() : ", " + std::to_string(this->seat_bots_.size()) +
			" seat(s) named"));

	lines.push_back("clock: timescale " + NumberVariable::format(this->clock_.timescale()) +
		(this->clock_.paused() ? ", paused (" + std::to_string(this->clock_.pending_steps()) +
			" step(s) pending)" : ", running"));
	lines.push_back(std::string("feedback ") +
		(this->presentation_->movement_feedback().enabled ? "on" : "off") +
		", visual " + MatchPlanner::visual_name(this->planned_visual()));
	std::string overlays;
	for (const auto& [name, on] : { std::pair<const char*, bool>{ "player", this->overlay_player_ },
		{ "collision", this->overlay_collision_ }, { "camera", this->overlay_camera_ },
		{ "paint", this->overlay_paint_ } })
	{
		overlays += on ? std::string(overlays.empty() ? "" : " ") + name : "";
	}
	lines.push_back("overlays: " + (overlays.empty() ? std::string("none") : overlays) +
		"; cheats: god " + (this->god_ ? "on" : "off") + ", ammo " +
		(this->infinite_ammo_ ? "inf" : "normal"));
	const std::vector<std::string> stations = this->station_names();
	if (!stations.empty())
	{
		std::string names;
		for (const std::string& station : stations)
		{
			names += " " + station;
		}
		lines.push_back("stations:" + names);
	}
	if (this->pending_goto_)
	{
		lines.push_back("queued: goto " + this->pending_goto_->station + " " +
			std::to_string(this->pending_goto_->seat));
	}
	if (!this->request_.empty())
	{
		lines.push_back("queued: " + this->request_.describe());
	}
	if (this->has_pending_content())
	{
		lines.push_back("queued: reloaded content for the next build");
	}
	if (!this->log_.transcript_path().empty())
	{
		lines.push_back("logfile " + this->log_.transcript_path());
	}
	if (this->scripts_.running())
	{
		lines.push_back("script:");
		for (const std::string& line : this->scripts_.stack_lines(this->wait_context()))
		{
			lines.push_back(line);
		}
	}
	if (this->live_open_)
	{
		lines.push_back("console: live (the keyboard seat is idle)");
	}
	return lines;
}

std::string DebugDirectorImpl::status_strip() const
{
	std::string strip;
	const auto add = [&strip](const std::string& part)
		{
			strip += strip.empty() ? "" : "  ";
			strip += part;
		};
	if (this->clock_.timescale() != 1.0f)
	{
		add("x" + NumberVariable::format(this->clock_.timescale()));
	}
	if (this->clock_.paused())
	{
		add("PAUSED");
	}
	// The comparison exists only on the Movement Course.
	const PlayerVisualMode visual = this->planned_visual();
	const bool off_course = this->match_ != nullptr &&
		this->match_->debug_settings().stage() != course_stage;
	if (visual != PlayerVisualMode::original && !off_course)
	{
		add(std::string("visual ") + MatchPlanner::visual_name(visual));
	}
	if (!this->presentation_->movement_feedback().enabled)
	{
		add("feedback off");
	}
	if (this->god_)
	{
		add("GOD");
	}
	if (this->infinite_ammo_)
	{
		add("AMMO INF");
	}
	const std::string bots = this->player_ == nullptr ? this->bot_summary() : std::string();
	if (!bots.empty())
	{
		add(bots);
	}
	if (this->recorder_ != nullptr)
	{
		add("REC " + this->recorder_->name() + " " + std::to_string(this->level_updates_));
	}
	if (this->player_ != nullptr)
	{
		add("PLAY " + this->player_->name() + (this->player_->finished ?
			std::string(" done (") + (this->player_->matched ? "MATCH" : "DIVERGED") + ")" :
			" " + std::to_string(this->level_updates_) + "/" +
				std::to_string(this->player_->file().updates)));
	}
	{
		int edits = 0;
		for (const auto& document : this->tune_docs_)
		{
			edits += document != nullptr ? static_cast<int>(document->edits().size()) : 0;
		}
		if (edits > 0)
		{
			add("TUNED " + std::to_string(edits));
		}
	}
	std::string overlays;
	for (const auto& [name, on] : { std::pair<const char*, bool>{ "collision", this->overlay_collision_ },
		{ "camera", this->overlay_camera_ }, { "paint", this->overlay_paint_ } })
	{
		overlays += on ? std::string(" ") + name : "";
	}
	if (!overlays.empty())
	{
		add("ovl" + overlays);
	}
	// A script that is waiting says so wherever the strip shows, whatever
	// stalled it (a paused clock, a lost focus, an open console).
	const std::string script = this->scripts_.waiting(this->wait_context());
	if (!script.empty() && !this->capturing_)
	{
		add("SCRIPT " + script);
	}
	return strip;
}

std::vector<std::string> DebugDirectorImpl::paint_lines() const
{
	if (!this->overlay_paint_ || this->match_ == nullptr)
	{
		return {};
	}
	const MatchPaint paint = this->match_->debug_paint();
	const auto percent = [&paint](float area)
		{
			char buffer[16] = {};
			std::snprintf(buffer, sizeof(buffer), "%.1f%%", paint.total > 0.0f ?
				static_cast<double>(area / paint.total * 100.0f) : 0.0);
			return std::string(buffer);
		};
	std::vector<std::string> lines;
	lines.push_back("paint  A " + percent(paint.team_a) + "  B " + percent(paint.team_b) +
		"  free " + percent(std::max(0.0f, paint.total - paint.team_a - paint.team_b)));
	const std::vector<MenuPlayerSettings> players = this->match_->debug_settings().player_settings();
	std::string seats;
	for (std::size_t i = 0; i < players.size() && i < paint.seats.size(); ++i)
	{
		seats += (seats.empty() ? "" : "   ") + std::string("P") + std::to_string(i + 1) + " " +
			device_label(players[i].num) + " " + percent(paint.seats[i]);
	}
	if (!seats.empty())
	{
		lines.push_back(seats);
	}
	return lines;
}
#endif
