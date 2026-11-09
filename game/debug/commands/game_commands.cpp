#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "engine/assets/json.h"
#include "engine/core/file_path.h"
#include "game/config/content_validation.h"
#include "game/debug/script/debug_files.h"
#include "game/states/menu_content_validation.h"
#include "game/debug/replay/bot.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/capture/debug_screenshot.h"
#include "game/game_data.h"
#include "game/config/sound_groups.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "game/debug/replay/bot_driver.h"

using namespace labrador;

namespace
{
	// The directory reload reads: the source tree in a Debug build made from
	// one, so edits apply without a rebuild; otherwise the staged copy.
	std::filesystem::path reload_directory(const std::filesystem::path& source,
		const std::filesystem::path& staged, bool& from_source)
	{
		std::error_code ignored;
		from_source = std::filesystem::is_regular_file(source / L"tuning.json", ignored) &&
			std::filesystem::is_regular_file(source / L"presentation.json", ignored);
		return from_source ? source : staged;
	}

	std::string utf8(const std::filesystem::path& path)
	{
		return labrador::path_to_utf8(path.generic_wstring());
	}

	using Form = DebugMenuTraits::Form;
}

void DebugDirectorImpl::register_commands()
{
	using Args = const DebugArgs&;
	DebugRegistry& registry = this->registry_;
	// Match plan ------------------------------------------------------------

	registry.add_command("stage", "start or switch to a stage (in a match: rebuilds it)",
		{ { "name|next|prev|random", false, [this] { return this->planner().stage_words(); } } },
		[this](Args args, DebugLog&)
		{
			const std::string& word = args.text(0);
			if (!this->planner().is_stage_word(word))
			{
				std::string known;
				for (const std::string& stage : this->planner().stage_words())
				{
					known += " " + stage;
				}
				return DebugStatus::failure("unknown stage '" + word + "'; one of:" + known);
			}
			this->request_.stage = word;
			return DebugStatus::success("stage " + word +
				(this->match_ != nullptr ? this->rebuild_note() : ": starting a match"), true);
		}, DebugMenuTraits::of("match", Form::pick, true));

	registry.add_command("players", "seats 1-4: pads in slot order, then keyboard/mouse, then idle dummies; "
		"a second word puts that bot on every idle dummy (players 4 paint)",
		{ { "1-4", false, [] { return std::vector<std::string>{ "1", "2", "3", "4" }; } },
			{ "bot", true, [] { return Bot::behaviours(); } } },
		[this](Args args, DebugLog& log)
		{
			int players = 0;
			if (!DebugArgs::parse_int(args.text(0), players) || players < 1 || players > 4)
			{
				return DebugStatus::failure("players: expected 1-4, got '" + args.text(0) + "'");
			}
			if (args.has(1) && !Bot::is_behaviour(args.text(1)))
			{
				return DebugStatus::failure("players: unknown bot '" + args.text(1) +
					"'; one of " + join_words(Bot::behaviours(), ", "));
			}
			if (args.has(1))
			{
				if (const auto refused = this->refuse_while_replaying("players " + args.text(0) +
					" " + args.text(1)))
				{
					return *refused;
				}
				// Stored first: the rebuild this queues builds the bots.
				this->dummy_bot_ = args.text(1) == "idle" ? std::string() : args.text(1);
				log.info("bots: every idle dummy " + args.text(1));
			}
			this->request_.players = players;
			if (this->match_ != nullptr)
			{
				return DebugStatus::success("players " + std::to_string(players) +
					this->rebuild_note(), true);
			}
			if (this->request_.stage)
			{
				return DebugStatus::success("players " + std::to_string(players), true);
			}
			return DebugStatus::success("players " + std::to_string(players) +
				": stored for 'stage <name>'; a match started from the menus drops it");
		}, DebugMenuTraits::of("match", Form::pick, true));

	registry.add_command("weapon", "choose a seat's weapon (seat 1-4 or all)",
		{ { "seat|all", false, [] { return std::vector<std::string>{ "all", "1", "2", "3", "4" }; } },
			{ "sprayer|sniper|roller|mister|bucket|random", false, [] { return MatchPlanner::weapon_names(); } } },
		[this](Args args, DebugLog&)
		{
			MatchRequest::WeaponChoice choice;
			if (args.text(0) != "all")
			{
				int seat = 0;
				if (!DebugArgs::parse_int(args.text(0), seat) || seat < 1 || seat > 4)
				{
					return DebugStatus::failure("weapon: seat must be 1-4 or all");
				}
				choice.seat = seat - 1;
			}
			WeaponType weapon = WeaponType::sprayer;
			if (!MatchPlanner::weapon_from_name(args.text(1), weapon))
			{
				return DebugStatus::failure("weapon: unknown weapon '" + args.text(1) + "'");
			}
			choice.weapon = args.text(1);
			this->request_.weapons.push_back(choice);
			const bool applies = this->match_ != nullptr || this->request_.stage.has_value();
			return DebugStatus::success("weapon " + args.text(0) + " " + args.text(1) +
				(this->match_ != nullptr ? this->rebuild_note() :
					applies ? "" : ": stored for 'stage <name>'; a match started from the menus drops it"),
				applies);
		}, DebugMenuTraits::of("match", Form::pick, true, 1, "all"));

	// Console only: three words per seat do not fit one picker.
	registry.add_command("seat", "set a seat's team, shipping bot, Boost or sub: seat <1-4> team a|b, "
		"seat <2-4> bot easy|normal|hard, seat <2-4> human, seat <1-4> boost 0|1|2, "
		"seat <1-4> sub grenade|sprinkler|wall|random "
		"(bot <seat> <behaviour> only drives a seat's input)",
		{ { "1-4", false, [] { return std::vector<std::string>{ "1", "2", "3", "4" }; } },
			{ "team|bot|human|boost|sub", false, [] { return MatchPlanner::seat_fields(); } },
			{ "value", true, [] { return MatchPlanner::seat_values(); } } },
		[this](Args args, DebugLog&)
		{
			MatchRequest::SeatChoice choice;
			if (!DebugArgs::parse_int(args.text(0), choice.seat) || choice.seat < 1 || choice.seat > 4)
			{
				return DebugStatus::failure("seat: seat must be 1-4, got '" + args.text(0) + "'");
			}
			const std::string error = MatchPlanner::parse_seat_choice(args.text(1),
				args.has(2) ? args.text(2) : std::string(), choice);
			if (!error.empty())
			{
				return DebugStatus::failure("seat: " + error);
			}
			if (choice.seat == 1 && choice.field == MatchRequest::SeatChoice::Field::bot)
			{
				return DebugStatus::failure("seat: seat 1 stays human: it is the setup host");
			}
			--choice.seat;
			this->request_.seats.push_back(choice);
			const std::string line = "seat " + args.text(0) + " " + args.text(1) +
				(choice.value.empty() ? std::string() : " " + choice.value);
			const bool applies = this->match_ != nullptr || this->request_.stage.has_value();
			return DebugStatus::success(line +
				(this->match_ != nullptr ? this->rebuild_note() :
					applies ? "" : ": stored for 'stage <name>'; a match started from the menus drops it"),
				applies);
		}, DebugMenuTraits::of("match", Form::hidden, true));

	registry.add(std::make_unique<ChoiceVariable>("intro",
		"how the next match begins: auto (the game's choice), full, quick (from the "
		"preview) or off (straight to the split countdown); record and play use off",
		"auto", std::vector<std::string>{ "auto", "full", "quick", "off" },
		std::vector<std::pair<std::string, std::string>>{},
		[this] { return this->intro_.empty() ? std::string("auto") : this->intro_; },
		[this](const std::string& choice)
		{
			this->intro_ = choice == "auto" ? std::string() : choice;
			return DebugStatus::success("intro = " + choice + " (from the next build)");
		}), DebugMenuTraits::of("match", Form::variable));

	registry.add_command("restart", "rebuild the match with its current settings (keeps the session)", {},
		[this](Args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("restart: no match");
			}
			this->request_.restart = true;
			return DebugStatus::success("restart", true);
		}, DebugMenuTraits::of("match", Form::run));

	registry.add_command("menu", "leave the match for the main menu", {},
		[this](Args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::success("menu: already in the menus");
			}
			this->leave_requested_ = true;
			return DebugStatus::success("menu: leaving the match", true);
		}, DebugMenuTraits::of("match", Form::run));

	// Time ------------------------------------------------------------------

	registry.add(std::make_unique<NumberVariable>("timescale",
		"match speed, 0.25-4 (updates are stepped, never stretched; audio is not scaled)",
		1.0f, DebugClock::min_timescale, DebugClock::max_timescale,
		std::vector<float>{ 0.25f, 0.5f, 1.0f, 2.0f, 4.0f },
		[this] { return this->clock_.timescale(); },
		[this](float scale)
		{
			this->clock_.set_timescale(scale);
			return DebugStatus::success("timescale = " +
				NumberVariable::format(this->clock_.timescale()));
		}), DebugMenuTraits::of("time", Form::variable));

	registry.add_command("pause", "pause or resume the match clock (F6)",
		{ { "on|off|toggle", true, [] { return std::vector<std::string>{ "on", "off", "toggle" }; } } },
		[this](Args args, DebugLog&)
		{
			bool ok = true;
			const bool paused = BoolVariable::parse(args.has(0) ? args.text(0) : "toggle",
				this->clock_.paused(), ok);
			if (!ok)
			{
				return DebugStatus::failure("pause: expected on, off or toggle");
			}
			this->clock_.set_paused(paused);
			// The user's pause now, not a replay's held frame.
			this->replay_hold_ = false;
			if (this->match_ != nullptr)
			{
				this->match_->debug_sync_audio();
			}
			return DebugStatus::success(paused ? "paused" : "running");
		}, DebugMenuTraits::of("time", Form::run));

	registry.add_command("step", "pause, then run n single updates (F7)",
		{ { "n", true, {} } },
		[this](Args args, DebugLog&)
		{
			int count = 1;
			if (args.has(0) && (!DebugArgs::parse_int(args.text(0), count) ||
				count < 1 || count > DebugClock::max_steps))
			{
				return DebugStatus::failure("step: expected 1-600");
			}
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("step: no match");
			}
			this->clock_.request_steps(count);
			this->replay_hold_ = false;
			return DebugStatus::success("step " + std::to_string(count) +
				" (paused; " + std::to_string(this->clock_.pending_steps()) + " pending)", true);
		}, DebugMenuTraits::of("time", Form::run));

	// View ------------------------------------------------------------------

	registry.add(std::make_unique<BoolVariable>("feedback",
		"movement feedback bursts on take-off and landing (F9)",
		this->presentation_->movement_feedback().enabled,
		[this] { return this->presentation_->movement_feedback().enabled; },
		[this](bool enabled)
		{
			this->presentation_->set_movement_feedback_enabled(enabled);
			return DebugStatus::success(std::string("feedback = ") + (enabled ? "on" : "off"));
		}), DebugMenuTraits::of("view", Form::variable));

	// Content ---------------------------------------------------------------

	registry.add_command("reload", "re-read tuning.json and presentation.json (source tree first); "
		"in a match it restarts", {},
		[this](Args, DebugLog&) { return this->reload_content(); },
		DebugMenuTraits::of("content", Form::run));

	registry.add_command("screenshot", "save the window as a PNG in %LOCALAPPDATA%/ColourWars/screens "
		"(timestamped name by default)",
		{ { "name", true, {} } },
		[this](Args args, DebugLog&)
		{
			// Named now, so the message names the file the capture writes (a
			// timestamped default is the time of the request, not the capture).
			const std::string name = screenshot_file_name(args.has(0) ? args.text(0) :
				std::string(), std::chrono::system_clock::now());
			this->pending_screenshot_ = name;
			return DebugStatus::success("screenshot " + name + " queued", true);
		}, DebugMenuTraits::of("content", Form::run));

	// Console ---------------------------------------------------------------

	registry.add_command("status", "what is running: stack, match, seats, clock", {},
		[this](Args, DebugLog& log)
		{
			for (const std::string& line : this->status_lines())
			{
				log.info(line);
			}
			return DebugStatus::success();
		}, DebugMenuTraits::of("console", Form::run));

	registry.add_command("logfile", "mirror the console to a UTF-8 file, or off",
		{ { "path|off", false, {} } },
		[this](Args args, DebugLog& log)
		{
			const std::string path = args.text(0) == "off" ? std::string() : args.text(0);
			if (!log.set_transcript(path))
			{
				return DebugStatus::failure("logfile: cannot open '" + path + "'");
			}
			return DebugStatus::success(path.empty() ? "logfile off" : "logfile " + path);
		}, DebugMenuTraits::of("console", Form::hidden));

	registry.add_command("clear", "clear the console output", {},
		[](Args, DebugLog& log)
		{
			log.clear();
			return DebugStatus::success();
		}, DebugMenuTraits::of("console", Form::hidden));

	registry.add_command("quit", "close the game", {},
		[this](Args, DebugLog&)
		{
			if (this->application_ == nullptr)
			{
				return DebugStatus::failure("quit: no application");
			}
			this->application_->quit();
			return DebugStatus::success("quit", true);
		}, DebugMenuTraits::of("console", Form::run));
}

DebugStatus DebugDirectorImpl::reload_content()
{
	if (this->data_ == nullptr || this->data_->application() == nullptr)
	{
		return DebugStatus::failure("reload: the game is not running");
	}

	bool from_source = false;
	const std::filesystem::path directory = reload_directory(this->source_content_directory(),
		this->staged_content_directory(), from_source);
	const std::string tuning_path = utf8(directory / L"tuning.json");
	const std::string presentation_path = utf8(directory / L"presentation.json");
	this->log_.info("reload: reading " + utf8(directory) +
		(from_source ? " (source tree)" : " (staged copy)"));

	if (const auto refused = this->refuse_while_replaying("reload"))
	{
		return *refused;
	}

	try
	{
		// Each file is read once: the text the readers accept is the text that
		// becomes live tuning's baseline, even if the file changes meanwhile.
		const auto read = [](const std::filesystem::path& path, const std::string& shown)
			{
				bool ok = false;
				std::string text = debug_files::read_file(path, ok);
				if (!ok)
				{
					throw std::runtime_error("cannot read '" + shown + "'");
				}
				return text;
			};
		std::string tuning_text = read(directory / L"tuning.json", tuning_path);
		std::string presentation_text = read(directory / L"presentation.json", presentation_path);
		auto tuning = std::make_unique<GameTuning>(parse_json(tuning_text, tuning_path));
		// In the run's language, as at startup.
		const ActiveLocale* locale = this->data_->locale();
		auto presentation = std::make_unique<Presentation>(locale != nullptr ?
			locale->presentation(presentation_text, presentation_path) :
			parse_json(presentation_text, presentation_path));
		check_stage_names_agree(parse_json(presentation_text, presentation_path).root(),
			*this->data_->stages());
		validate_shell_content(*tuning, *presentation, this->data_->render_resources(),
			this->data_->audio_resources(), *this->data_->level_infos(),
			*this->data_->stages(), this->data_->settings_session()->play_settings());
		// A sound the candidate names for the first time joins its group now.
		sound_groups::assign(*this->data_->audio_resources(), *tuning, *presentation,
			*this->data_->level_infos(), *this->data_->stages());
		this->pending_tuning_ = std::move(tuning);
		this->pending_presentation_ = std::move(presentation);
		this->pending_tuning_text_ = std::move(tuning_text);
		this->pending_presentation_text_ = std::move(presentation_text);
	}
	catch (const std::exception& e)
	{
		return DebugStatus::failure(std::string("reload failed; keeping the current values: ") +
			e.what());
	}

	if (this->match_ != nullptr)
	{
		this->request_.restart = true;
		return DebugStatus::success("reload: valid; restarting the match", true);
	}
	return DebugStatus::success("reload: valid; applies to the next match");
}
#endif
