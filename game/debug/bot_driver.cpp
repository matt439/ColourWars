#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot_driver.h"
#include "game/objects/level.h"
#include "game/tuning.h"

namespace
{
	bool on_ground(PlayerMoveState state)
	{
		switch (state)
		{
		case PlayerMoveState::on_ground:
		case PlayerMoveState::on_drop_down_ground:
		case PlayerMoveState::on_ramp_left:
		case PlayerMoveState::on_ramp_right:
			return true;
		default:
			return false;
		}
	}

	BotSeatView::Seat seat_of(const Player& player, const GameTuning& tuning)
	{
		BotSeatView::Seat seat;
		seat.center = player.center();
		seat.velocity = player.velocity();
		seat.team = player.team();
		seat.alive = player.state() == PlayerState::alive;
		seat.on_ground = on_ground(player.move_state());
		seat.facing_right = player.facing_right();
		const float health = tuning.player().starting_health;
		const float ammo = tuning.starting_ammo();
		seat.health = health > 0.0f ? player.health() / health : 0.0f;
		seat.ammo = ammo > 0.0f ? player.weapon_ammo() / ammo : 0.0f;
		return seat;
	}
}

BotDriver::BotDriver(const GameTuning* tuning, std::uint64_t seed) :
	tuning_(tuning), seed_(seed)
{
}

void BotDriver::assign(int seat, const std::string& behaviour)
{
	this->behaviours_[seat] = behaviour;
	this->bots_[seat] = Bot::make(behaviour, this->seed_, seat);
}

bool BotDriver::drives(int seat) const
{
	return this->behaviours_.find(seat) != this->behaviours_.end();
}

BotSeatView BotDriver::view_of(const Level& level, int seat, int step, const GameTuning& tuning)
{
	BotSeatView view;
	view.step = step;
	view.player_height = tuning.player().size.y;
	for (int other = 0; other < level.player_count(); ++other)
	{
		const BotSeatView::Seat state = seat_of(level.player(other), tuning);
		if (other == seat)
		{
			view.self = state;
		}
		else
		{
			view.others.push_back(state);
		}
	}
	return view;
}

void BotDriver::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	for (auto& [seat, bot] : this->bots_)
	{
		if (seat < 0 || seat >= static_cast<int>(seats.size()) || seat >= level.player_count())
		{
			continue;
		}
		seats[static_cast<std::size_t>(seat)] = bot != nullptr ?
			bot->think(view_of(level, seat, this->step_, *this->tuning_)) : Bot::neutral();
	}
	++this->step_;
}
#endif
