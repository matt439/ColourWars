#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/render/text_encoding.h"
#include "game/content_validation.h"
#include "game/debug/bot.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_screenshot.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/source_paths.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "game/debug/bot_driver.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"

using namespace labrador;

namespace
{
	std::string format_number(float value)
	{
		char buffer[32] = {};
		std::snprintf(buffer, sizeof(buffer), "%g", static_cast<double>(value));
		return buffer;
	}

	bool visual_from_name(const std::string& name, PlayerVisualMode& mode)
	{
		if (name == "original") { mode = PlayerVisualMode::original; return true; }
		if (name == "rotating") { mode = PlayerVisualMode::continuous; return true; }
		if (name == "posed") { mode = PlayerVisualMode::authored; return true; }
		return false;
	}

	// The directory reload reads: the source tree in a Debug build made from
	// one, so edits apply without a rebuild; otherwise the staged copy.
	std::filesystem::path reload_directory(bool& from_source)
	{
		const std::filesystem::path source(widen(source_paths::content_directory));
		std::error_code ignored;
		from_source = std::filesystem::is_regular_file(source / L"tuning.json", ignored) &&
			std::filesystem::is_regular_file(source / L"presentation.json", ignored);
		return from_source ? source : std::filesystem::path(widen(executable_directory()));
	}

	std::string utf8(const std::filesystem::path& path)
	{
		const std::u8string text = path.generic_u8string();
		return std::string(text.begin(), text.end());
	}

	constexpr const char* course_stage = "movement_course";

	using Form = DebugMenuTraits::Form;

	DebugMenuTraits traits(const char* category, Form form, bool staged = false,
		std::size_t pick_argument = 0, const char* prefix = "")
	{
		DebugMenuTraits result;
		result.category = category;
		result.form = form;
		result.staged = staged;
		result.pick_argument = pick_argument;
		result.prefix = prefix;
		return result;
	}

	// A variable keeps its variable form; only its category and staging change.
	void place(DebugRegistry& registry, const char* name, const char* category,
		bool staged = false)
	{
		DebugEntry* entry = registry.find(name);
		DebugMenuTraits menu = entry->menu();
		menu.category = category;
		menu.staged = staged;
		entry->set_menu(std::move(menu));
	}
}

void DebugDirectorImpl::register_commands()
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

	// Match plan ------------------------------------------------------------

	add("stage", "start or switch to a stage (in a match: rebuilds it)",
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
			// A stage chosen here uses the remembered Workshop comparison.
			if (!this->request_.visual)
			{
				this->request_.visual = this->remembered_visual_;
			}
			return DebugStatus::success("stage " + word +
				(this->match_ != nullptr ? this->rebuild_note() : ": starting a match"), true);
		}, traits("match", Form::pick, true));

	add("players", "seats 1-4: pads in slot order, then keyboard/mouse, then idle dummies; "
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
					"'; one of idle, walk, jump, chase, paint");
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
		}, traits("match", Form::pick, true));

	add("weapon", "choose a seat's weapon (seat 1-4 or all)",
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
		}, traits("match", Form::pick, true, 1, "all"));

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
		}));
	place(registry, "intro", "match");

	registry.add(std::make_unique<ChoiceVariable>("visual",
		"Movement Course art: original, rotating (continuous tool) or posed (authored)",
		"original", std::vector<std::string>{ "original", "rotating", "posed" },
		std::vector<std::pair<std::string, std::string>>{
			{ "continuous", "rotating" }, { "authored", "posed" } },
		[this] { return std::string(MatchPlanner::visual_name(this->planned_visual())); },
		[this](const std::string& choice)
		{
			PlayerVisualMode mode = PlayerVisualMode::original;
			visual_from_name(choice, mode);
			if (this->match_ != nullptr && !this->request_.stage &&
				this->match_->debug_settings().stage() != course_stage)
			{
				// Only the Movement Course has the comparison: do not rebuild.
				this->remembered_visual_ = mode;
				return DebugStatus::success("visual = " + choice + " (no effect on " +
					this->match_->debug_settings().stage() + "; used on the Movement Course)");
			}
			if (this->match_ == nullptr && !this->request_.stage)
			{
				// Remembered, not queued: `stage` picks it up, and a match
				// started from the menus keeps the page's own comparison.
				this->remembered_visual_ = mode;
				return DebugStatus::success("visual = " + choice +
					" (for the next match 'stage <name>' starts)");
			}
			this->queue_visual(mode);
			return DebugStatus::success("visual = " + choice + (this->match_ != nullptr ?
				this->rebuild_note() : std::string(" (for the queued match)")), true);
		}));
	place(registry, "visual", "match", true);

	add("restart", "rebuild the match with its current settings (keeps the session)", {},
		[this](Args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("restart: no match");
			}
			this->request_.restart = true;
			return DebugStatus::success("restart", true);
		}, traits("match", Form::run));

	add("menu", "leave the match for the main menu", {},
		[this](Args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::success("menu: already in the menus");
			}
			this->leave_requested_ = true;
			return DebugStatus::success("menu: leaving the match", true);
		}, traits("match", Form::run));

	// Time ------------------------------------------------------------------

	registry.add(std::make_unique<NumberVariable>("timescale",
		"match speed, 0.25-4 (updates are stepped, never stretched; audio is not scaled)",
		1.0f, DebugClock::min_timescale, DebugClock::max_timescale,
		std::vector<float>{ 0.25f, 0.5f, 1.0f, 2.0f, 4.0f },
		[this] { return this->clock_.timescale(); },
		[this](float scale)
		{
			this->clock_.set_timescale(scale);
			return DebugStatus::success("timescale = " + format_number(this->clock_.timescale()));
		}));
	place(registry, "timescale", "time");

	add("pause", "pause or resume the match clock (F6)",
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
		}, traits("time", Form::run));

	add("step", "pause, then run n single updates (F7)",
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
		}, traits("time", Form::run));

	// View ------------------------------------------------------------------

	registry.add(std::make_unique<BoolVariable>("feedback",
		"movement feedback bursts on take-off and landing (F9)",
		this->presentation_->movement_feedback().enabled,
		[this] { return this->presentation_->movement_feedback().enabled; },
		[this](bool enabled)
		{
			this->presentation_->set_movement_feedback_enabled(enabled);
			return DebugStatus::success(std::string("feedback = ") + (enabled ? "on" : "off"));
		}));
	place(registry, "feedback", "view");

	// Content ---------------------------------------------------------------

	add("reload", "re-read tuning.json and presentation.json (source tree first); "
		"in a match it restarts", {},
		[this](Args, DebugLog&) { return this->reload_content(); },
		traits("content", Form::run));

	add("screenshot", "save the window as a PNG in %LOCALAPPDATA%/ColourWars/screens "
		"(timestamped name by default)",
		{ { "name", true, {} } },
		[this](Args args, DebugLog&)
		{
			const std::string requested = args.has(0) ? args.text(0) : std::string();
			this->pending_screenshot_ = requested;
			const std::string name = screenshot_file_name(requested,
				std::chrono::system_clock::now());
			return DebugStatus::success("screenshot " + name + " queued", true);
		}, traits("content", Form::run));

	// Console ---------------------------------------------------------------

	add("status", "what is running: stack, match, seats, clock", {},
		[this](Args, DebugLog& log)
		{
			for (const std::string& line : this->status_lines())
			{
				log.info(line);
			}
			return DebugStatus::success();
		}, traits("console", Form::run));

	add("logfile", "mirror the console to a UTF-8 file, or off",
		{ { "path|off", false, {} } },
		[this](Args args, DebugLog& log)
		{
			const std::string path = args.text(0) == "off" ? std::string() : args.text(0);
			if (!log.set_transcript(path))
			{
				return DebugStatus::failure("logfile: cannot open '" + path + "'");
			}
			return DebugStatus::success(path.empty() ? "logfile off" : "logfile " + path);
		}, traits("console", Form::hidden));

	add("clear", "clear the console output", {},
		[](Args, DebugLog& log)
		{
			log.clear();
			return DebugStatus::success();
		}, traits("console", Form::hidden));

	add("quit", "close the game", {},
		[this](Args, DebugLog&)
		{
			if (this->application_ == nullptr)
			{
				return DebugStatus::failure("quit: no application");
			}
			this->application_->quit();
			return DebugStatus::success("quit", true);
		}, traits("console", Form::run));
}

DebugStatus DebugDirectorImpl::reload_content()
{
	if (this->data_ == nullptr || this->data_->application() == nullptr)
	{
		return DebugStatus::failure("reload: the game is not running");
	}

	bool from_source = false;
	const std::filesystem::path directory = reload_directory(from_source);
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
		auto tuning = std::make_unique<GameTuning>(tuning_path.c_str());
		auto presentation = std::make_unique<Presentation>(presentation_path.c_str());
		validate_game_content(*tuning, *presentation, this->data_->render_resources(),
			this->data_->audio_resources(), *this->data_->level_infos(),
			*this->data_->stages(), this->data_->save()->play_settings());
		// What the reader just accepted becomes live tuning's baseline.
		std::string texts[2];
		for (int file = 0; file < 2; ++file)
		{
			std::ifstream stream(directory / (file == 0 ? L"tuning.json" : L"presentation.json"),
				std::ios::binary);
			texts[file].assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		this->pending_tuning_ = std::move(tuning);
		this->pending_presentation_ = std::move(presentation);
		this->pending_tuning_text_ = std::move(texts[0]);
		this->pending_presentation_text_ = std::move(texts[1]);
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
