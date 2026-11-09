#include "game/pch.h"
#include "game/match/match_setup.h"
#include "game/input/input_slots.h"
#include "game/match/match_rules.h"
#include <array>
#include <stdexcept>
#include <cstdlib>
#include <numeric>
using namespace labrador;

std::optional<float> MatchSetup::duration() const
{
	return this->duration_;
}

void MatchSetup::set_duration(std::optional<float> seconds)
{
	if (seconds && (!std::isfinite(*seconds) || *seconds <= 0.0f))
	{
		throw std::invalid_argument("Match duration must be finite and positive");
	}
	this->duration_ = seconds;
}

MatchVariant MatchSetup::variant() const
{
	return this->variant_;
}

void MatchSetup::set_variant(MatchVariant variant)
{
	if (!valid_match_variant(variant))
	{
		throw std::invalid_argument("Match variant must be Standard or Endless paint");
	}
	this->variant_ = variant;
}

const std::vector<SeatSetup>& MatchSetup::player_settings() const
{
	return this->player_settings_;
}

void MatchSetup::set_player_count(int player_count)
{
	if (!valid_player_count(player_count))
	{
		throw std::invalid_argument("Player count must be " + player_count_range());
	}
	// The vector is the count's single source of truth, including if growth throws.
	this->player_settings_.resize(static_cast<size_t>(player_count));
}

int MatchSetup::player_count() const
{
	return static_cast<int>(this->player_settings_.size());
}

int MatchSetup::human_count() const
{
	return this->player_count() - this->bot_count();
}

int MatchSetup::bot_count() const
{
	return static_cast<int>(std::count_if(this->player_settings_.begin(),
		this->player_settings_.end(), [](const SeatSetup& player)
		{ return player.controller == PlayerController::bot; }));
}

void MatchSetup::set_player_bot(int player_index, BotDifficulty difficulty)
{
	SeatSetup& player = this->player_settings_.at(static_cast<size_t>(player_index));
	if (player_index == 0)
	{
		throw std::invalid_argument("Player 1 must remain the human setup host");
	}
	if (!valid_bot_difficulty(difficulty))
	{
		throw std::invalid_argument("Bot difficulty must be Easy, Normal or Hard");
	}
	player.controller = PlayerController::bot;
	player.difficulty = difficulty;
	player.num = -1;
	this->reset_devices();
}

void MatchSetup::set_player_human(int player_index)
{
	SeatSetup& player = this->player_settings_.at(static_cast<size_t>(player_index));
	if (player.controller == PlayerController::bot)
	{
		player.controller = PlayerController::human;
		player.num = -1;
		this->reset_devices();
	}
}

void MatchSetup::set_stage(const std::string& stage)
{
	this->stage_ = stage;
}

const std::string& MatchSetup::stage() const
{
	return this->stage_;
}

ScreenLayout MatchSetup::screen_layout() const
{
	return this->screen_layout_;
}

void MatchSetup::set_screen_layout(ScreenLayout screen_layout)
{
	this->screen_layout_ = screen_layout;
}

void MatchSetup::set_player_team(int player_index, PlayerTeam team)
{
	this->player_settings_.at(static_cast<size_t>(player_index)).team = team;
}
PlayerTeam MatchSetup::player_team(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).team;
}

bool MatchSetup::shuffle_teams()
{
	const int count = this->player_count();
	if (count < min_versus_players || !this->input_roster_complete()) { return false; }

	std::array<int, max_players> seats{};
	std::iota(seats.begin(), seats.end(), 0);
	for (int last = count - 1; last > 0; --last)
	{
		const int chosen = std::rand() % (last + 1);
		std::swap(seats[static_cast<size_t>(last)], seats[static_cast<size_t>(chosen)]);
	}
	// With three players either side may have the extra seat.
	const int team_a_count = count / 2 + (count % 2 != 0 ? std::rand() % 2 : 0);
	for (int seat = 0; seat < count; ++seat)
	{
		this->player_settings_[static_cast<size_t>(seats[static_cast<size_t>(seat)])].team =
			seat < team_a_count ? PlayerTeam::a : PlayerTeam::b;
	}
	return true;
}

void MatchSetup::set_player_weapon(int player_index, WeaponType weapon)
{
	this->player_settings_.at(static_cast<size_t>(player_index)).weapon = weapon;
}
WeaponType MatchSetup::player_weapon(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).weapon;
}

void MatchSetup::set_player_sub(int player_index, SubType sub)
{
	if (sub_name(sub) == nullptr) { throw std::invalid_argument("Not a sub weapon"); }
	this->player_settings_.at(static_cast<size_t>(player_index)).sub = sub;
}
SubType MatchSetup::player_sub(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).sub;
}

void MatchSetup::set_player_handicap(int player_index, int handicap)
{
	SeatSetup& player = this->player_settings_.at(static_cast<size_t>(player_index));
	if (!handicaps::valid(handicap))
	{
		throw std::invalid_argument("Handicap level must be none, +1 or +2");
	}
	player.handicap = handicap;
}
int MatchSetup::player_handicap(int player_index) const
{
	return this->player_settings_.at(static_cast<size_t>(player_index)).handicap;
}

void MatchSetup::begin_input_claims(int initiating_slot)
{
	if (this->player_settings_.empty() ||
		initiating_slot < 0 || initiating_slot >= input_slots::count)
	{
		throw std::invalid_argument("Cannot bind the player roster to these inputs");
	}
	for (SeatSetup& player : this->player_settings_)
	{
		player.num = -1;
	}
	this->player_settings_.front().num = initiating_slot;
	this->reset_devices();
}

bool MatchSetup::claim_input_slot(int slot)
{
	if (slot < 0 || slot >= input_slots::count ||
		this->player_index_for_slot(slot) != -1)
	{
		return false;
	}
	for (SeatSetup& player : this->player_settings_)
	{
		if (player.controller == PlayerController::human && player.num == -1)
		{
			player.num = slot;
			this->reset_devices();
			return true;
		}
	}
	return false;
}

bool MatchSetup::release_input_slot(int slot)
{
	const int player = this->player_index_for_slot(slot);
	if (player <= 0)
	{
		return false;
	}
	this->player_settings_[static_cast<size_t>(player)].num = -1;
	this->reset_devices();
	return true;
}

bool MatchSetup::input_roster_complete() const
{
	return !this->player_settings_.empty() && std::all_of(
		this->player_settings_.begin(), this->player_settings_.end(),
		[](const SeatSetup& player)
		{ return player.controller == PlayerController::bot || player.num >= 0; });
}

int MatchSetup::player_index_for_slot(int slot) const
{
	if (slot < 0 || slot >= input_slots::count)
	{
		return -1;
	}
	for (int player = 0; player < this->player_count(); ++player)
	{
		if (this->player_settings_[player].num == slot)
		{
			return player;
		}
	}
	return -1;
}

int MatchSetup::device_slot(int slot) const
{
	if (!input_slots::is_pad(slot) || this->devices_[static_cast<size_t>(slot)] < 0)
	{
		return slot;
	}
	return this->devices_[static_cast<size_t>(slot)];
}

std::array<int, input_slots::pad_count> MatchSetup::pad_devices() const
{
	std::array<int, input_slots::pad_count> devices{};
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		devices[static_cast<size_t>(slot)] = this->device_slot(slot);
	}
	return devices;
}

int MatchSetup::player_index_for_device(int device) const
{
	if (device < 0 || device >= input_slots::count)
	{
		return -1;
	}
	for (int player = 0; player < this->player_count(); ++player)
	{
		const int slot = this->player_settings_[player].num;
		if (slot >= 0 && this->device_slot(slot) == device)
		{
			return player;
		}
	}
	return -1;
}

bool MatchSetup::move_to_device(int slot, int device)
{
	const int player = this->player_index_for_slot(slot);
	if (player == -1 || !input_slots::is_pad(slot) || !input_slots::is_pad(device) ||
		this->player_settings_[player].controller != PlayerController::human)
	{
		return false;
	}
	const int reading = this->player_index_for_device(device);
	if (reading != -1 && reading != player)
	{
		return false;
	}
	this->devices_[static_cast<size_t>(slot)] = device == slot ? -1 : device;
	return true;
}

void MatchSetup::reset_devices()
{
	this->devices_.fill(-1);
}

SeriesStageRule MatchSetup::series_stage_rule() const
{
	return this->series_stage_rule_;
}

void MatchSetup::set_series_stage_rule(SeriesStageRule rule)
{
	std::ignore = series_stage_rule_key(rule);
	this->series_stage_rule_ = rule;
}

int MatchSetup::best_of() const
{
	return this->best_of_;
}

void MatchSetup::set_best_of(int best_of)
{
	if (!valid_best_of(best_of))
	{
		throw std::invalid_argument("Session length must be 1, 3 or 5");
	}
	this->best_of_ = best_of;
}

int session_rounds(const MatchSetup& settings)
{
	return settings.player_count() == 1 ? 1 : settings.best_of();
}
