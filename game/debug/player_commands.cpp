#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
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

// goto, respawn, god and ammo: edits of the live players, made from the
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

	auto god = std::make_unique<BoolVariable>("god",
		"no player takes paint damage (paint still lands)", false,
		[this] { return this->god_; },
		[this](bool on)
		{
			this->god_ = on;
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
			this->infinite_ammo_ = choice == "inf";
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
#endif
