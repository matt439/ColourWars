#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot_driver.h"
#include "game/objects/level.h"
#include "game/tuning.h"
#include "game/objects/weapon.h"
#include "engine/core/step_timer.h"

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

	// Policy for developer measurements only: a stationary horizontal launch,
	// using the actual fixed-step drag/cap/first-step lifetime convention for
	// at most sixty steps. Gravity, inherited velocity, terrain, diffusion and
	// target motion can all change a real hit. This is not a range certificate.
	float engagement_distance(const Player& player, const GameTuning& tuning)
	{
		const Weapon& weapon = player.primary_weapon();
		const WeaponTuning& tool = tuning.weapon(weapon.type());
		const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
		const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(
			labrador::StepTimer::TicksPerSecond / 60));
		float speed = tool.details.starting_vel_length;
		float distance = 0.0f;
		float elapsed = 0.0f;
		for (int step = 0; step < 60; ++step)
		{
			elapsed += dt;
			if (step > 0 && elapsed > projectile.details.delete_timer) { break; }
			if (std::fabs(speed) > projectile.details.wind_resistance * projectile.wind_stop_factor)
			{
				speed += speed > 0.0f ? -projectile.details.wind_resistance * dt :
					projectile.details.wind_resistance * dt;
			}
			else { speed = 0.0f; }
			speed = std::clamp(speed, -projectile.max_velocity.x, projectile.max_velocity.x);
			distance += std::fabs(speed * dt);
		}
		return distance + mattmath::Vector2F::distance(weapon.nozzle_pos(), player.center()) +
			projectile.details.col_rect_size.x * 0.5f + tuning.player().size.x * 0.5f;
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

BotSeatView BotDriver::view_of(const Level& level, int seat, int step, const GameTuning& tuning,
	bool observe_tool)
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
	if (observe_tool && seat >= 0 && seat < level.player_count())
	{
		const Player& self = level.player(seat);
		view.shot_empty = self.primary_weapon().readiness() == WeaponReadiness::empty;
		view.engage_distance = engagement_distance(self, tuning);
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
			bot->think(view_of(level, seat, this->step_, *this->tuning_, bot->name() == "skirmish")) : Bot::neutral();
	}
	++this->step_;
}
#endif
