#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot_driver.h"
#include "game/debug/debug_director_impl.h"
#include "game/game_data.h"
#include <cstdio>

namespace
{
	bool parse_seat(const std::string& text, int& seat)
	{
		return DebugArgs::parse_int(text, seat) && seat >= 1 && seat <= 4;
	}
}

// goto, respawn, kill, god and ammo: edits of the live players, made from the
// console or the debug menu while no world update is running.
void DebugDirectorImpl::register_player_commands()
{
	DebugRegistry& registry = this->registry_;
	const auto menu = [](DebugMenuTraits::Form form)
		{
			DebugMenuTraits traits;
			traits.category = "player";
			traits.form = form;
			return traits;
		};

	auto go = std::make_unique<LambdaCommand>("goto",
		"put a seat (1-4, default 1) at a named station of the current stage, at rest",
		std::vector<DebugArgSpec>{
			{ "station", false, [this] { return this->station_names(); } },
			{ "seat", true, [] { return std::vector<std::string>{ "1", "2", "3", "4" }; } } },
		[this](const DebugArgs& args, DebugLog&)
		{
			int seat = 1;
			if (args.has(1) && !parse_seat(args.text(1), seat))
			{
				return DebugStatus::failure("goto: seat must be 1-4, got '" + args.text(1) + "'");
			}
			if (const auto refused = this->refuse_while_replaying("goto"))
			{
				return *refused;
			}
			if (this->match_ == nullptr)
			{
				// Launch arguments and the menus: the first build of the match
				// `stage` starts applies it (and checks the name then).
				this->pending_goto_ = PendingGoto{ args.text(0), seat };
				return DebugStatus::success("goto " + args.text(0) + ": applies when 'stage' "
					"starts a match");
			}
			if (this->rebuild_queued())
			{
				// The queued rebuild puts every player back at a spawn, so the
				// goto waits for it and is checked against the stage it builds.
				this->pending_goto_ = PendingGoto{ args.text(0), seat };
				return DebugStatus::success("goto " + args.text(0) + ": applies after the "
					"queued " + this->queued_rebuild_description());
			}
			return this->apply_goto(*this->match_, args.text(0), seat);
		});
	go->set_menu(menu(DebugMenuTraits::Form::pick));
	registry.add(std::move(go));

	auto respawn = std::make_unique<LambdaCommand>("respawn",
		"respawn a seat (1-4) or every seat at its spawn, health and ammo full",
		std::vector<DebugArgSpec>{ { "seat|all", true, []
			{ return std::vector<std::string>{ "all", "1", "2", "3", "4" }; } } },
		[this](const DebugArgs& args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("respawn: no match");
			}
			if (const auto refused = this->refuse_while_replaying("respawn"))
			{
				return *refused;
			}
			if (this->rebuild_queued())
			{
				return DebugStatus::failure("respawn: the queued " +
					this->queued_rebuild_description() + " respawns every seat anyway");
			}
			const int players = this->match_->debug_settings().player_count();
			int first = 1;
			int last = players;
			if (args.has(0) && args.text(0) != "all")
			{
				if (!parse_seat(args.text(0), first))
				{
					return DebugStatus::failure("respawn: seat must be 1-4 or all");
				}
				last = first;
			}
			std::string done;
			for (int seat = first; seat <= last; ++seat)
			{
				if (!this->match_->debug_respawn(seat - 1))
				{
					return DebugStatus::failure("respawn: seat " + std::to_string(seat) +
						" is not in this match (" + std::to_string(players) + " players)");
				}
				done += " P" + std::to_string(seat);
			}
			return DebugStatus::success("respawn:" + done, true);
		});
	respawn->set_menu(menu(DebugMenuTraits::Form::run));
	registry.add(std::move(respawn));

	auto kill = std::make_unique<LambdaCommand>("kill",
		"splat a seat (1-4) or every seat, god or not, to watch the respawn redraw",
		std::vector<DebugArgSpec>{ { "seat|all", true, []
			{ return std::vector<std::string>{ "all", "1", "2", "3", "4" }; } } },
		[this](const DebugArgs& args, DebugLog&)
		{
			if (this->match_ == nullptr)
			{
				return DebugStatus::failure("kill: no match");
			}
			if (const auto refused = this->refuse_while_replaying("kill"))
			{
				return *refused;
			}
			if (this->rebuild_queued())
			{
				return DebugStatus::failure("kill: the queued " +
					this->queued_rebuild_description() + " rebuilds every seat");
			}
			const int players = this->match_->debug_settings().player_count();
			int first = 1;
			int last = players;
			if (args.has(0) && args.text(0) != "all")
			{
				if (!parse_seat(args.text(0), first))
				{
					return DebugStatus::failure("kill: seat must be 1-4 or all");
				}
				last = first;
			}
			std::string done;
			for (int seat = first; seat <= last; ++seat)
			{
				if (!this->match_->debug_kill(seat - 1))
				{
					return DebugStatus::failure("kill: seat " + std::to_string(seat) +
						" is not in this match (" + std::to_string(players) + " players)");
				}
				done += " P" + std::to_string(seat);
			}
			// Death is noticed by the player's own update, so a countdown holds it.
			return DebugStatus::success("kill:" + done + " (on the next world update)", true);
		});
	kill->set_menu(menu(DebugMenuTraits::Form::run));
	registry.add(std::move(kill));

	auto god = std::make_unique<BoolVariable>("god",
		"no player takes paint damage (paint still lands)", false,
		[this] { return this->god_; },
		[this](bool on)
		{
			if (const auto refused = this->refuse_while_replaying("god"))
			{
				return *refused;
			}
			this->god_ = on;
			// Set over a finished replay's held frame: the choice outlasts it.
			if (this->cheats_before_play_)
			{
				this->cheats_before_play_->first = on;
			}
			this->apply_cheats();
			return DebugStatus::success(std::string("god = ") + (on ? "on" : "off"));
		});
	god->set_menu(menu(DebugMenuTraits::Form::variable));
	registry.add(std::move(god));

	auto ammo = std::make_unique<ChoiceVariable>("ammo",
		"inf keeps every tank full", "normal", std::vector<std::string>{ "normal", "inf" },
		std::vector<std::pair<std::string, std::string>>{ { "infinite", "inf" } },
		[this] { return std::string(this->infinite_ammo_ ? "inf" : "normal"); },
		[this](const std::string& choice)
		{
			if (const auto refused = this->refuse_while_replaying("ammo"))
			{
				return *refused;
			}
			this->infinite_ammo_ = choice == "inf";
			if (this->cheats_before_play_)
			{
				this->cheats_before_play_->second = this->infinite_ammo_;
			}
			this->apply_cheats();
			return DebugStatus::success("ammo = " + choice);
		});
	ammo->set_menu(menu(DebugMenuTraits::Form::variable));
	registry.add(std::move(ammo));
}

void DebugDirectorImpl::apply_cheats()
{
	if (this->match_ != nullptr)
	{
		this->match_->debug_set_cheats(this->god_, this->infinite_ammo_);
	}
}

std::vector<std::string> DebugDirectorImpl::station_names() const
{
	std::vector<std::string> names;
	try
	{
		std::vector<LevelStation> stations;
		if (this->request_.stage)
		{
			// A queued stage is where a goto lands (in a match it waits for the
			// rebuild). Only a concrete stage key: next, prev and random name
			// no level yet, so they list nothing.
			const std::string& stage = *this->request_.stage;
			if (this->data_ == nullptr || this->data_->stages() == nullptr ||
				this->data_->level_infos() == nullptr)
			{
				return names;
			}
			for (const Stage& known : *this->data_->stages())
			{
				if (known.name == stage && this->data_->level_infos()->contains(stage))
				{
					stations = this->data_->level_infos()->get(stage)->stations();
					break;
				}
			}
		}
		else if (this->match_ != nullptr)
		{
			stations = this->match_->debug_stations();
		}
		for (const LevelStation& station : stations)
		{
			names.push_back(station.name);
		}
	}
	catch (const std::exception&)
	{
		names.clear();
	}
	return names;
}

DebugStatus DebugDirectorImpl::apply_goto(DebugMatchTarget& match,
	const std::string& station, int seat)
{
	const std::vector<LevelStation> stations = match.debug_stations();
	const auto found = std::find_if(stations.begin(), stations.end(),
		[&station](const LevelStation& candidate) { return candidate.name == station; });
	if (found == stations.end())
	{
		std::string known;
		for (const LevelStation& candidate : stations)
		{
			known += " " + candidate.name;
		}
		return DebugStatus::failure("goto: no station '" + station + "' on " +
			match.debug_settings().stage() + "; one of:" + known);
	}
	const int players = match.debug_settings().player_count();
	if (seat > players || !match.debug_place(seat - 1, found->position))
	{
		return DebugStatus::failure("goto: seat " + std::to_string(seat) +
			" is not in this match (" + std::to_string(players) + " player" +
			(players == 1 ? "" : "s") + ")");
	}
	char position[64] = {};
	std::snprintf(position, sizeof(position), "(%.0f, %.0f)",
		static_cast<double>(found->position.x), static_cast<double>(found->position.y));
	return DebugStatus::success("goto " + station + ": P" + std::to_string(seat) + " at " +
		position, true);
}

// Bots ------------------------------------------------------------------------

std::string DebugDirectorImpl::bot_for_seat(int seat, int slot, bool shipping_bot) const
{
	const auto explicit_bot = this->seat_bots_.find(seat);
	if (explicit_bot != this->seat_bots_.end())
	{
		return explicit_bot->second;
	}
	return slot < 0 && !shipping_bot ? this->dummy_bot_ : std::string();
}

void DebugDirectorImpl::rebuild_bots(const DebugMatchView& match)
{
	this->bot_driver_.reset();
	const std::vector<SeatSetup> seats = match.debug_settings().player_settings();
	// The same seed gives the same bots on every build; without one each
	// build draws a new one.
	const std::uint64_t seed = this->seed_ ? static_cast<std::uint64_t>(*this->seed_) :
		static_cast<std::uint64_t>(this->planner_random_());
	auto driver = std::make_unique<BotDriver>(this->tuning_, seed);
	for (std::size_t i = 0; i < seats.size(); ++i)
	{
		const std::string behaviour = this->bot_for_seat(static_cast<int>(i), seats[i].num,
			seats[i].controller == PlayerController::bot);
		if (!behaviour.empty())
		{
			driver->assign(static_cast<int>(i), behaviour);
		}
	}
	if (!driver->assignments().empty())
	{
		this->bot_driver_ = std::move(driver);
	}
}

std::string DebugDirectorImpl::bot_summary() const
{
	if (this->bot_driver_ == nullptr)
	{
		return {};
	}
	const std::map<int, std::string>& assigned = this->bot_driver_->assignments();
	std::string first = assigned.begin()->second;
	bool same = true;
	for (const auto& entry : assigned)
	{
		same = same && entry.second == first;
	}
	return "BOTS " + std::to_string(assigned.size()) + (same ? "x " + first : std::string());
}

void DebugDirectorImpl::register_bot_commands()
{
	std::vector<std::string> seats_and_all = { "all", "off", "1", "2", "3", "4" };
	auto bot = std::make_unique<LambdaCommand>("bot",
		"drive seats with scripted input: bot <seat|all> <idle|walk|jump|chase|paint|skirmish|off>; "
		"all = every idle dummy; bot off clears; bot alone lists",
		std::vector<DebugArgSpec>{
			{ "seat|all|off", true, [seats_and_all] { return seats_and_all; } },
			{ "behaviour", true, []
				{
					std::vector<std::string> words = Bot::behaviours();
					words.push_back("off");
					return words;
				} } },
		[this](const DebugArgs& args, DebugLog& log)
		{
			if (!args.has(0))
			{
				log.info(std::string("bots: all dummies ") +
					(this->dummy_bot_.empty() ? "off" : this->dummy_bot_));
				for (const auto& [seat, behaviour] : this->seat_bots_)
				{
					log.info("  seat " + std::to_string(seat + 1) + " " + behaviour);
				}
				return DebugStatus::success(this->bot_driver_ != nullptr ?
					"running: " + this->bot_summary() : std::string("no bot is driving a seat"));
			}
			if (const auto refused = this->refuse_while_replaying("bot"))
			{
				return *refused;
			}
			const std::string& target = args.text(0);
			std::string behaviour = args.has(1) ? args.text(1) : std::string();
			if (target == "off")
			{
				this->seat_bots_.clear();
				this->dummy_bot_.clear();
			}
			else
			{
				if (behaviour.empty())
				{
					return DebugStatus::failure("bot: name a behaviour: idle, walk, jump, chase, "
						"paint, skirmish or off");
				}
				if (behaviour != "off" && !Bot::is_behaviour(behaviour))
				{
					return DebugStatus::failure("bot: unknown behaviour '" + behaviour +
						"'; one of idle, walk, jump, chase, paint, skirmish, off");
				}
				if (target == "all")
				{
					this->dummy_bot_ = behaviour == "off" ? std::string() : behaviour;
				}
				else
				{
					int seat = 0;
					if (!parse_seat(target, seat))
					{
						return DebugStatus::failure("bot: seat must be 1-4, all or off");
					}
					if (behaviour == "off")
					{
						this->seat_bots_.erase(seat - 1);
					}
					else
					{
						this->seat_bots_[seat - 1] = behaviour;
					}
				}
			}
			// Takes over (or hands back) the running match's seats at once.
			if (this->match_ != nullptr)
			{
				this->rebuild_bots(*this->match_);
				this->push_seat_labels(*this->match_);
				return DebugStatus::success("bot " + target + (behaviour.empty() ? "" : " " + behaviour) +
					": " + (this->bot_driver_ != nullptr ? this->bot_summary() :
						std::string("no seat is driven")));
			}
			return DebugStatus::success("bot " + target + (behaviour.empty() ? "" : " " + behaviour) +
				": applies to the next match");
		});
	DebugMenuTraits menu;
	menu.category = "player";
	menu.form = DebugMenuTraits::Form::pick;
	menu.pick_argument = 1;
	menu.prefix = "all";
	bot->set_menu(std::move(menu));
	this->registry_.add(std::move(bot));
}
#endif
