#include "game/pch.h"
#include "game/ai/match_bot_driver.h"
#include "game/ai/match_bot_ballistics.h"
#include "game/ai/match_bot_territory.h"
#include "game/objects/level.h"
#include "game/objects/weapon.h"
#include "engine/core/step_timer.h"
#include <cmath>
#include <limits>

using namespace mattmath;

namespace
{
	bool grounded(PlayerMoveState state)
	{
		return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
			state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
	}

	// A ready, grounded deliberate tool requalifies its support shot every step.
	bool support_shot(const MatchBot& bot, const Player& player)
	{
		return bot.finishing() || (bot.deliberate(player.primary_weapon().type()) && grounded(player.move_state()) &&
			player.primary_weapon().readiness() == WeaponReadiness::ready);
	}

	Vector2F paint_point(const PaintSurfaceSample& surface, float clearance)
	{
		Vector2F center;
		for (const Vector2F& corner : surface.corners) { center += corner; }
		center *= 0.25f;
		switch (surface.face)
		{
		case PaintSurfaceFace::top: center.y -= clearance; break;
		case PaintSurfaceFace::bottom: center.y += clearance; break;
		case PaintSurfaceFace::left: center.x -= clearance; break;
		case PaintSurfaceFace::right: center.x += clearance; break;
		}
		return center;
	}
}

MatchBotDriver::MatchBotDriver(const GameTuning* tuning, const MenuLevelSettings& settings,
	std::uint64_t seed) : tuning_(tuning)
{
	if (tuning == nullptr) { throw std::invalid_argument("A match bot driver requires tuning"); }
	const std::vector<MenuPlayerSettings>& players = settings.player_settings();
	this->bots_.resize(players.size());
	this->difficulties_.reserve(players.size());
	for (size_t seat = 0; seat < players.size(); ++seat)
	{
		const MenuPlayerSettings& player = players[seat];
		this->difficulties_.push_back(player.difficulty);
		if (player.controller == PlayerController::bot)
		{
			this->bots_[seat] = std::make_unique<MatchBot>(tuning, player.difficulty,
				seed * 0x9E3779B97F4A7C15ull + static_cast<std::uint64_t>(seat) + 1);
		}
	}
}

bool MatchBotDriver::drives(int participant) const
{
	return participant >= 0 && participant < static_cast<int>(this->bots_.size()) &&
		this->bots_[static_cast<size_t>(participant)] != nullptr;
}

float match_bot_ballistics::horizontal_reach(const WeaponTuning& tool,
	const ProjectileTuning& projectile)
{
	const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(
		labrador::StepTimer::TicksPerSecond / 60));
	float speed = tool.details.starting_vel_length;
	float distance = 0.0f;
	float elapsed = 0.0f;
	// A conservative local engagement heuristic, not a projectile-hit promise.
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
	if (projectile.details.gravity != 0.0f)
	{
		// Horizontal speed integrated for a whole second overstates a curved
		// shot's useful range: the bot otherwise stops while the shot falls
		// short. The ideal same-height maximum is v^2 / |g|; drag and short
		// lifetimes may reduce the existing local estimate further.
		const double speed_squared = static_cast<double>(tool.details.starting_vel_length) *
			tool.details.starting_vel_length;
		const double ballistic = speed_squared / std::fabs(static_cast<double>(projectile.details.gravity));
		distance = static_cast<float>(std::min(static_cast<double>(distance), ballistic));
	}
	return distance;
}

float MatchBotDriver::engagement_distance(const Player& player, const GameTuning& tuning)
{
	const Weapon& weapon = player.primary_weapon();
	const WeaponTuning& tool = tuning.weapon(weapon.type());
	const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
	return match_bot_ballistics::horizontal_reach(tool, projectile) +
		Vector2F::distance(weapon.nozzle_pos(), player.center()) +
		projectile.details.col_rect_size.x * 0.5f + tuning.player().size.x * 0.5f;
}

void MatchBotDriver::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	bool wants_territory = false;
	for (size_t seat = 0; seat < this->bots_.size() && seat < seats.size() &&
		seat < static_cast<size_t>(level.player_count()); ++seat)
	{
		const Player& player = level.player(static_cast<int>(seat));
		wants_territory = wants_territory || (this->bots_[seat] != nullptr && player.state() == PlayerState::alive &&
			(this->bots_[seat]->needs_territory() || support_shot(*this->bots_[seat], player)));
	}
	const std::vector<PaintSurfaceSample> surfaces = wants_territory ? level.paint_surfaces() :
		std::vector<PaintSurfaceSample>{};
	const std::vector<RectangleF> obstacles = level.bot_obstacles();
	const std::vector<MatchBotSupport> supports = level.bot_supports();
	const std::vector<MatchBotRamp> ramps = level.bot_ramps();
	for (size_t seat = 0; seat < this->bots_.size() && seat < seats.size() &&
		seat < static_cast<size_t>(level.player_count()); ++seat)
	{
		MatchBot* bot = this->bots_[seat].get();
		if (bot == nullptr) { continue; }
		const Player& player = level.player(static_cast<int>(seat));
		const Weapon& weapon = player.primary_weapon();
		MatchBotObservation view;
		view.center = player.center();
		view.velocity = player.velocity();
		view.size = this->tuning_->player().size;
		view.alive = player.state() == PlayerState::alive && player.health() > 0.0f;
		view.grounded = grounded(player.move_state());
		view.on_platform = player.move_state() == PlayerMoveState::on_drop_down_ground;
		view.facing_right = player.facing_right();
		view.ammo = player.weapon_ammo() / this->tuning_->starting_ammo();
		view.shot_empty = weapon.readiness() == WeaponReadiness::empty;
		view.engage_distance = engagement_distance(player, *this->tuning_);
		view.weapon = weapon.type();
		view.obstacles = obstacles;
		view.supports = supports;
		view.ramps = ramps;
		const float sight = this->tuning_->bots().sight_distance;
		if (view.alive)
		{
			for (int other = 0; other < level.player_count(); ++other)
			{
				const Player& candidate = level.player(other);
				if (other == static_cast<int>(seat) || candidate.team() == player.team() ||
					candidate.state() != PlayerState::alive || candidate.health() <= 0.0f ||
					Vector2F::distance_squared(candidate.center(), view.center) > sight * sight ||
					!level.bot_line_of_sight(view.center, candidate.center())) { continue; }
				view.enemies.push_back({ candidate.center(), candidate.velocity() });
			}
			const bool replan = bot->needs_territory();
			if ((replan || support_shot(*bot, player)) && !surfaces.empty())
			{
				const bool deliberate = bot->deliberate(view.weapon);
				// Guarantee a local scoring face can be considered even when a
				// small deck is missed by all of Easy's global random samples.
				// Keep this separate from movement candidates and their RNG draws.
				float best_support_cost = (std::numeric_limits<float>::max)();
				for (const PaintSurfaceSample& surface : surfaces)
				{
					if (surface.team == player.team()) { continue; }
					const std::optional<match_bot_territory::SupportTop> top = match_bot_territory::support_top_point(
						view, surface, this->tuning_->paint().thickness);
					if (!top) { continue; }
					const Vector2F gap = top->point - view.center;
					const float cost = std::fabs(gap.x) + std::fabs(gap.y) * 2.0f;
					if (gap.length() > view.engage_distance) { continue; }
					// A deliberate tool also needs this nominal angle to land.
					const bool aimable = cost < best_support_cost &&
						match_bot_territory::top_launch_clear(view, top->point, *this->tuning_, this->difficulties_[seat]) &&
						(!deliberate || match_bot_territory::top_launch_lands(view, top->bounds, gap.angle(), *this->tuning_));
					if ((!aimable && view.support_open) || !level.bot_line_of_sight(view.center, top->point)) { continue; }
					view.support_open = true;
					if (aimable) { view.support_paint = top->point; view.support_bounds = top->bounds; best_support_cost = cost; }
				}
				// Random territory sampling stays replan-only: the same draws.
				if (replan)
				{
					const int count = this->tuning_->bot(this->difficulties_[seat]).territory_candidates;
					for (int index = 0; index < count; ++index)
					{
						const PaintSurfaceSample& surface = surfaces[static_cast<size_t>(bot->random() % surfaces.size())];
						if (surface.team == player.team()) { continue; }
						const Vector2F point = paint_point(surface, this->tuning_->paint().thickness + view.size.x * 0.05f);
						if (Vector2F::distance_squared(point, view.center) <= sight * sight &&
							level.bot_line_of_sight(view.center, point)) { view.territory.push_back(point); }
					}
				}
			}
		}
		seats[seat] = bot->think(view);
	}
}
