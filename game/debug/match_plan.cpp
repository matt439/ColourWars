#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_plan.h"
#include "game/input_slots.h"

labrador::ScreenLayout MatchPlanner::layout_for(int players)
{
	switch (players)
	{
	case 2: return labrador::ScreenLayout::two_player;
	case 3: return labrador::ScreenLayout::three_player;
	case 4: return labrador::ScreenLayout::four_player;
	default: return labrador::ScreenLayout::one_player;
	}
}

bool MatchRequest::empty() const
{
	return !this->stage && !this->players && this->weapons.empty() &&
		!this->restart && !this->exact;
}

std::string MatchRequest::describe() const
{
	std::string text;
	const auto add = [&text](const std::string& part)
		{
			text += text.empty() ? "" : ", ";
			text += part;
		};
	if (this->exact)
	{
		add("replay " + this->exact_label);
		return text;
	}
	if (this->stage) { add("stage " + *this->stage); }
	if (this->players) { add("players " + std::to_string(*this->players)); }
	for (const WeaponChoice& choice : this->weapons)
	{
		add("weapon " + (choice.seat < 0 ? std::string("all") :
			std::to_string(choice.seat + 1)) + " " + choice.weapon);
	}
	if (this->restart) { add("restart"); }
	return text.empty() ? "nothing" : text;
}

MatchPlanner::MatchPlanner(const StageList* stages) : stages_(stages)
{
}

std::vector<std::string> MatchPlanner::stage_words() const
{
	std::vector<std::string> words;
	if (this->stages_ != nullptr)
	{
		for (const Stage& stage : *this->stages_)
		{
			words.push_back(stage.name);
		}
	}
	words.push_back("next");
	words.push_back("prev");
	words.push_back("random");
	return words;
}

bool MatchPlanner::is_stage_word(const std::string& word) const
{
	const std::vector<std::string> words = this->stage_words();
	return std::find(words.begin(), words.end(), word) != words.end();
}

std::vector<std::string> MatchPlanner::weapon_names()
{
	return { "sprayer", "sniper", "roller", "mister", "bucket", "random" };
}

bool MatchPlanner::weapon_from_name(const std::string& name, WeaponType& weapon)
{
	static const std::pair<const char*, WeaponType> names[] = {
		{ "sprayer", WeaponType::sprayer }, { "sniper", WeaponType::sniper },
		{ "roller", WeaponType::roller }, { "mister", WeaponType::mister },
		{ "bucket", WeaponType::bucket }, { "random", WeaponType::random_primary },
	};
	for (const auto& entry : names)
	{
		if (name == entry.first)
		{
			weapon = entry.second;
			return true;
		}
	}
	return false;
}

const char* MatchPlanner::weapon_name(WeaponType weapon)
{
	switch (weapon)
	{
	case WeaponType::sprayer: return "sprayer";
	case WeaponType::sniper: return "sniper";
	case WeaponType::roller: return "roller";
	case WeaponType::mister: return "mister";
	case WeaponType::bucket: return "bucket";
	default: return "other";
	}
}

void MatchPlanner::assign_devices(MenuLevelSettings& settings, int players,
	const ConnectedPads& pads)
{
	settings.set_player_count(players);
	std::vector<int> devices;
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (pads[static_cast<std::size_t>(slot)])
		{
			devices.push_back(slot);
		}
	}
	devices.push_back(input_slots::keyboard_and_mouse);

	settings.begin_input_claims(devices.front());
	for (std::size_t device = 1; device < devices.size(); ++device)
	{
		settings.claim_input_slot(devices[device]);
	}

	settings.set_screen_layout(layout_for(players));
	for (int seat = 0; seat < players; ++seat)
	{
		settings.set_player_team(seat, players == 1 || seat % 2 == 0 ?
			PlayerTeam::a : PlayerTeam::b);
	}
}

MatchPlan MatchPlanner::resolve(const MenuLevelSettings* current,
	const MatchRequest& request, const ConnectedPads& pads, const Random& random) const
{
	MatchPlan plan;
	if (this->stages_ == nullptr || this->stages_->empty())
	{
		plan.error = "no stages are loaded";
		return plan;
	}
	const StageList& stages = *this->stages_;
	if (request.exact)
	{
		const std::string& wanted = request.exact->stage();
		if (std::find_if(stages.begin(), stages.end(),
			[&wanted](const Stage& stage) { return stage.name == wanted; }) == stages.end())
		{
			plan.error = "unknown stage '" + wanted + "'";
			return plan;
		}
		plan.settings = *request.exact;
		plan.reset_session = true;
		plan.ok = true;
		return plan;
	}
	const bool fresh = current == nullptr || current->player_count() == 0;
	MenuLevelSettings settings = current != nullptr ? *current : MenuLevelSettings();

	// Stage.
	std::size_t index = 0;
	for (std::size_t i = 0; i < stages.size(); ++i)
	{
		if (stages[i].name == settings.stage())
		{
			index = i;
		}
	}
	if (request.stage)
	{
		const std::string& word = *request.stage;
		if (word == "next")
		{
			index = settings.stage().empty() ? 0 : (index + 1) % stages.size();
		}
		else if (word == "prev")
		{
			index = settings.stage().empty() ? stages.size() - 1 :
				(index + stages.size() - 1) % stages.size();
		}
		else if (word == "random")
		{
			try
			{
				index = random_stage_index(stages, random());
			}
			catch (const std::exception& e)
			{
				plan.error = e.what();
				return plan;
			}
		}
		else
		{
			const auto found = std::find_if(stages.begin(), stages.end(),
				[&word](const Stage& stage) { return stage.name == word; });
			if (found == stages.end())
			{
				plan.error = "unknown stage '" + word + "'";
				return plan;
			}
			index = static_cast<std::size_t>(found - stages.begin());
		}
	}
	const std::string stage = stages[index].name;
	if (stage != settings.stage())
	{
		plan.reset_session = true;
	}
	settings.set_stage(stage);

	// Roster: devices are claimed now, from the pads connected at apply time.
	if (request.players || fresh)
	{
		const int players = request.players.value_or(1);
		if (players < 1 || players > input_slots::pad_count)
		{
			plan.error = "players must be 1-4";
			return plan;
		}
		assign_devices(settings, players, pads);
		plan.reset_session = true;
	}

	// Weapons, in request order.
	for (const MatchRequest::WeaponChoice& choice : request.weapons)
	{
		WeaponType weapon = WeaponType::sprayer;
		if (!weapon_from_name(choice.weapon, weapon))
		{
			plan.error = "unknown weapon '" + choice.weapon + "'";
			return plan;
		}
		if (choice.seat >= settings.player_count())
		{
			plan.error = "weapon: seat " + std::to_string(choice.seat + 1) +
				" is not in a " + std::to_string(settings.player_count()) + "-player match";
			return plan;
		}
		for (int seat = 0; seat < settings.player_count(); ++seat)
		{
			if (choice.seat >= 0 && seat != choice.seat)
			{
				continue;
			}
			WeaponType chosen = weapon;
			if (chosen == WeaponType::random_primary)
			{
				chosen = static_cast<WeaponType>(
					random() % static_cast<unsigned int>(WeaponType::max_prim_wep));
			}
			settings.set_player_weapon(seat, chosen);
		}
	}

	plan.settings = settings;
	plan.ok = true;
	return plan;
}
#endif
