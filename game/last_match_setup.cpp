#include "game/pch.h"
#include "game/last_match_setup.h"
#include "engine/assets/json.h"
#include "game/save.h"
#include "game/states/menu_level_settings.h"
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

using namespace labrador;

namespace
{
	constexpr std::array<ScreenLayout, 4> layouts{
		ScreenLayout::one_player, ScreenLayout::two_player,
		ScreenLayout::three_player, ScreenLayout::four_player };
	constexpr std::array<const char*, 4> layout_keys{
		"one_player", "two_player", "three_player", "four_player" };
	constexpr std::array<WeaponType, 5> weapons{
		WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
		WeaponType::mister, WeaponType::bucket };
	constexpr std::array<const char*, 5> weapon_keys{
		"sprayer", "sniper", "roller", "mister", "bucket" };
	constexpr std::array<BotDifficulty, 3> difficulties{
		BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
	constexpr std::array<const char*, 3> difficulty_keys{ "easy", "normal", "hard" };

	ScreenLayout layout_for_count(size_t count)
	{
		if (count < 1 || count > layouts.size())
		{
			throw std::invalid_argument("Remembered player count must be between 1 and 4");
		}
		return layouts[count - 1];
	}

	bool valid_team(PlayerTeam team)
	{
		return team == PlayerTeam::a || team == PlayerTeam::b;
	}

	bool valid_weapon(WeaponType weapon)
	{
		return std::find(weapons.begin(), weapons.end(), weapon) != weapons.end();
	}

	bool valid_best_of(int best_of)
	{
		return best_of == 1 || best_of == 3 || best_of == 5;
	}

	bool valid_stage_rule(SeriesStageRule rule)
	{
		return rule == SeriesStageRule::same || rule == SeriesStageRule::rotate ||
			rule == SeriesStageRule::losers_pick;
	}

	bool valid_controller(PlayerController controller)
	{
		return controller == PlayerController::human || controller == PlayerController::bot;
	}

	bool valid_difficulty(BotDifficulty difficulty)
	{
		return std::find(difficulties.begin(), difficulties.end(), difficulty) != difficulties.end();
	}

	size_t human_count(const std::vector<LastMatchPlayer>& players)
	{
		return static_cast<size_t>(std::count_if(players.begin(), players.end(),
			[](const LastMatchPlayer& player) { return player.controller == PlayerController::human; }));
	}

	PlayerController read_controller(const JsonValue& player)
	{
		try
		{
			if (player.identifier("controller") == "bot") { return PlayerController::bot; }
		}
		catch (const std::runtime_error&) {}
		return PlayerController::human;
	}

	BotDifficulty read_difficulty(const JsonValue& player)
	{
		try
		{
			const std::string key = player.identifier("difficulty");
			for (size_t index = 0; index < difficulties.size(); ++index)
			{
				if (key == difficulty_keys[index]) { return difficulties[index]; }
			}
		}
		catch (const std::runtime_error&) {}
		return BotDifficulty::normal;
	}

	// Out-of-range values survive to normalised(), which repairs them to none.
	int read_handicap(const JsonValue& player)
	{
		try
		{
			if (player.has("handicap")) { return player.integer("handicap"); }
		}
		catch (const std::runtime_error&) {}
		return 0;
	}

	const char* difficulty_key(BotDifficulty difficulty)
	{
		for (size_t index = 0; index < difficulties.size(); ++index)
		{
			if (difficulty == difficulties[index]) { return difficulty_keys[index]; }
		}
		throw std::invalid_argument("Remembered bot difficulty must be Easy, Normal or Hard");
	}

	bool valid_teams(const std::vector<LastMatchPlayer>& players)
	{
		bool team_a = false;
		bool team_b = false;
		for (const LastMatchPlayer& player : players)
		{
			if (!valid_team(player.team)) { return false; }
			team_a = team_a || player.team == PlayerTeam::a;
			team_b = team_b || player.team == PlayerTeam::b;
		}
		return players.size() == 1 || (team_a && team_b);
	}

	PlayerTeam read_team(const JsonValue& player)
	{
		try
		{
			const std::string key = player.identifier("team");
			if (key == "a") { return PlayerTeam::a; }
			if (key == "b") { return PlayerTeam::b; }
		}
		catch (const std::runtime_error&)
		{
		}
		return PlayerTeam::none;
	}

	WeaponType read_weapon(const JsonValue& player)
	{
		try
		{
			const std::string key = player.identifier("weapon");
			for (size_t index = 0; index < weapons.size(); ++index)
			{
				if (key == weapon_keys[index]) { return weapons[index]; }
			}
		}
		catch (const std::runtime_error&)
		{
		}
		return WeaponType::sprayer;
	}

	const char* weapon_key(WeaponType weapon)
	{
		for (size_t index = 0; index < weapons.size(); ++index)
		{
			if (weapon == weapons[index]) { return weapon_keys[index]; }
		}
		return "sprayer";
	}
}

LastMatchSetup LastMatchSetup::capture(const MenuLevelSettings& settings)
{
	LastMatchSetup setup;
	for (const MenuPlayerSettings& player : settings.player_settings())
	{
		setup.players.push_back({ player.team, player.weapon, player.controller,
			player.controller == PlayerController::bot ? player.difficulty : BotDifficulty::normal,
			player.handicap });
	}
	setup.stage = settings.stage();
	setup.layout = settings.screen_layout();
	setup.duration = settings.duration();
	setup.best_of = settings.best_of();
	setup.stage_rule = settings.series_stage_rule();
	setup.validate();
	return setup;
}

void LastMatchSetup::validate() const
{
	layout_for_count(this->players.size());
	if (this->players.front().controller != PlayerController::human)
	{
		throw std::invalid_argument("Remembered Player 1 must be the human setup host");
	}
	if (this->layout != layout_for_count(human_count(this->players)))
	{
		throw std::invalid_argument("Remembered screen layout must match the human count");
	}
	if (!valid_teams(this->players))
	{
		throw std::invalid_argument("Remembered teams must be valid and oppose each other in multiplayer");
	}
	for (const LastMatchPlayer& player : this->players)
	{
		if (!valid_controller(player.controller) || !valid_difficulty(player.difficulty) ||
			(player.controller == PlayerController::human && player.difficulty != BotDifficulty::normal))
		{
			throw std::invalid_argument("Remembered controllers and bot difficulties must be valid");
		}
		if (!valid_weapon(player.weapon))
		{
			throw std::invalid_argument("Remembered weapons must be resolved primary weapons");
		}
		if (!handicaps::valid(player.handicap))
		{
			throw std::invalid_argument("Remembered handicaps must be none, +1 or +2");
		}
	}
	if (this->stage.empty() || this->stage.find('\0') != std::string::npos)
	{
		throw std::invalid_argument("Remembered stage must be a resolved manifest key");
	}
	if (!valid_best_of(this->best_of) || (this->players.size() == 1 && this->best_of != 1))
	{
		throw std::invalid_argument("Remembered session must be single match for solo, or best of 1, 3 or 5");
	}
	if (this->duration && (!std::isfinite(*this->duration) || *this->duration <= 0.0f))
	{
		throw std::invalid_argument("Remembered duration must be finite and positive");
	}
	if (!valid_stage_rule(this->stage_rule))
	{
		throw std::invalid_argument("Remembered series stage rule must be Same, Rotate or Loser's pick");
	}
}

LastMatchSetup LastMatchSetup::normalised() const
{
	LastMatchSetup setup = *this;
	layout_for_count(setup.players.size());
	for (LastMatchPlayer& player : setup.players)
	{
		if (!valid_controller(player.controller)) { player.controller = PlayerController::human; }
		if (!valid_difficulty(player.difficulty)) { player.difficulty = BotDifficulty::normal; }
		if (!handicaps::valid(player.handicap)) { player.handicap = 0; }
	}
	setup.players.front().controller = PlayerController::human;
	for (LastMatchPlayer& player : setup.players)
	{
		if (player.controller == PlayerController::human) { player.difficulty = BotDifficulty::normal; }
	}
	setup.layout = layout_for_count(human_count(setup.players));
	if (!valid_teams(setup.players))
	{
		for (size_t index = 0; index < setup.players.size(); ++index)
		{
			setup.players[index].team = index % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
		}
	}
	for (LastMatchPlayer& player : setup.players)
	{
		if (!valid_weapon(player.weapon)) { player.weapon = WeaponType::sprayer; }
	}
	if (setup.players.size() == 1 || !valid_best_of(setup.best_of)) { setup.best_of = 1; }
	if (setup.duration && (!std::isfinite(*setup.duration) || *setup.duration <= 0.0f))
	{
		setup.duration.reset();
	}
	if (setup.stage.find('\0') != std::string::npos) { setup.stage.clear(); }
	if (!valid_stage_rule(setup.stage_rule)) { setup.stage_rule = SeriesStageRule::same; }
	return setup;
}

MenuLevelSettings LastMatchSetup::restore(const StageList& stages) const
{
	if (stages.empty())
	{
		throw std::invalid_argument("Cannot restore a match without registered stages");
	}
	const LastMatchSetup setup = this->normalised();
	const auto chosen_stage = std::find_if(stages.begin(), stages.end(),
		[&setup](const Stage& candidate) { return candidate.name == setup.stage; });
	MenuLevelSettings settings;
	settings.set_player_count(static_cast<int>(setup.players.size()));
	for (int index = 0; index < settings.player_count(); ++index)
	{
		const LastMatchPlayer& player = setup.players[static_cast<size_t>(index)];
		settings.set_player_team(index, player.team);
		settings.set_player_weapon(index, player.weapon);
		if (player.controller == PlayerController::bot) { settings.set_player_bot(index, player.difficulty); }
		settings.set_player_handicap(index, player.handicap);
	}
	settings.set_stage(chosen_stage == stages.end() ? stages.front().name : chosen_stage->name);
	settings.set_screen_layout(setup.layout);
	settings.set_duration(setup.duration);
	settings.set_best_of(setup.best_of);
	settings.set_series_stage_rule(setup.stage_rule);
	return settings;
}

void LastMatchSetupSection::reset(SaveData& data) const
{
	data.last_match.reset();
}

void LastMatchSetupSection::read(const JsonValue& root, SaveData& data) const
{
	if (!root.has(this->key())) { return; }
	const JsonValue entry = root.object(this->key());
	const JsonValue players = entry.array("players");
	if (players.size() < 1 || players.size() > layouts.size())
	{
		throw std::runtime_error(players.where() + " must contain between 1 and 4 players");
	}
	LastMatchSetup setup;
	for (size_t index = 0; index < players.size(); ++index)
	{
		const JsonValue player = players.at(index);
		setup.players.push_back({ read_team(player), read_weapon(player),
			read_controller(player), read_difficulty(player), read_handicap(player) });
	}
	try
	{
		setup.stage = entry.identifier("stage");
	}
	catch (const std::runtime_error&)
	{
	}
	try
	{
		const std::string key = entry.identifier("layout");
		for (size_t index = 0; index < layouts.size(); ++index)
		{
			if (key == layout_keys[index]) { setup.layout = layouts[index]; break; }
		}
	}
	catch (const std::runtime_error&)
	{
	}
	try
	{
		setup.best_of = entry.integer("best_of");
	}
	catch (const std::runtime_error&)
	{
	}
	try
	{
		if (entry.has("duration")) { setup.duration = entry.number("duration"); }
	}
	catch (const std::runtime_error&)
	{
	}
	// Saves before version 5 have no rule, and an unknown spelling keeps
	// Same: both play every round on one stage, as those builds did.
	try
	{
		if (entry.has("stage_rule")) { std::ignore = read_series_stage_rule(entry.identifier("stage_rule"), setup.stage_rule); }
	}
	catch (const std::runtime_error&)
	{
	}
	data.last_match = setup.normalised();
}

void LastMatchSetupSection::write(SaveWriter& writer, const SaveData& data) const
{
	if (!data.last_match) { return; }
	const LastMatchSetup& setup = *data.last_match;
	writer.key(this->key());
	writer.begin_object();
	writer.key("players");
	writer.begin_array();
	for (const LastMatchPlayer& player : setup.players)
	{
		writer.begin_object();
		writer.key("team");
		writer.string(player.team == PlayerTeam::b ? "b" : "a");
		writer.key("weapon");
		writer.string(weapon_key(player.weapon));
		writer.key("controller");
		writer.string(player.controller == PlayerController::bot ? "bot" : "human");
		writer.key("difficulty");
		writer.string(difficulty_key(player.difficulty));
		if (player.handicap != 0)
		{
			writer.key("handicap");
			writer.integer(player.handicap);
		}
		writer.end_object();
	}
	writer.end_array();
	writer.key("stage");
	writer.string(setup.stage);
	writer.key("layout");
	writer.string(layout_keys[human_count(setup.players) - 1]);
	writer.key("best_of");
	writer.integer(setup.best_of);
	if (setup.duration)
	{
		writer.key("duration");
		writer.number(*setup.duration);
	}
	if (setup.stage_rule != SeriesStageRule::same)
	{
		writer.key("stage_rule");
		writer.string(series_stage_rule_key(setup.stage_rule));
	}
	writer.end_object();
}
