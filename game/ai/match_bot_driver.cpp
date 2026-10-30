#include "game/pch.h"
#include "game/ai/match_bot_driver.h"
#include "game/ai/match_bot_ballistics.h"
#include "game/ai/match_bot_territory.h"
#include "game/objects/level.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/objects/weapon.h"
#include <array>
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

	// A ready deliberate tool requalifies its top shot every step, airborne too.
	bool support_shot(const MatchBot& bot, const Player& player)
	{
		return bot.finishing() || bot.riding() || (bot.deliberate(player.primary_weapon().type()) &&
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

namespace
{
	std::vector<std::optional<BotDifficulty>> roster_bots(const MatchSetup& settings)
	{
		std::vector<std::optional<BotDifficulty>> seats;
		for (const SeatSetup& player : settings.player_settings())
		{
			seats.push_back(player.controller == PlayerController::bot ?
				std::optional<BotDifficulty>(player.difficulty) : std::nullopt);
		}
		return seats;
	}
}

MatchBotDriver::MatchBotDriver(const GameTuning* tuning, const MatchSetup& settings,
	std::uint64_t seed) : MatchBotDriver(tuning, roster_bots(settings), seed)
{
}

MatchBotDriver::MatchBotDriver(const GameTuning* tuning,
	const std::vector<std::optional<BotDifficulty>>& seats, std::uint64_t seed) : tuning_(tuning)
{
	if (tuning == nullptr) { throw std::invalid_argument("A match bot driver requires tuning"); }
	this->bots_.resize(seats.size());
	this->difficulties_.reserve(seats.size());
	for (size_t seat = 0; seat < seats.size(); ++seat)
	{
		this->difficulties_.push_back(seats[seat].value_or(BotDifficulty::normal));
		if (seats[seat])
		{
			this->bots_[seat] = std::make_unique<MatchBot>(tuning, *seats[seat],
				BotRandom::seat_seed(seed, static_cast<int>(seat)));
		}
	}
}

bool MatchBotDriver::drives(int participant) const
{
	return participant >= 0 && participant < static_cast<int>(this->bots_.size()) &&
		this->bots_[static_cast<size_t>(participant)] != nullptr;
}

const MatchBot* MatchBotDriver::bot(int participant) const
{
	return this->drives(participant) ? this->bots_[static_cast<size_t>(participant)].get() : nullptr;
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
	// Input is observed before this step settles gates. React to the permission
	// actually published by the last settlement, including occupied holds.
	const auto gates = level.team_gates();
	this->gate_permissions_.resize(gates.size(), PlayerTeam::none);
	std::array<bool, 2> changed{};
	for (size_t index = 0; index < gates.size(); ++index)
	{
		const PlayerTeam before = this->gate_permissions_[index];
		const PlayerTeam after = gates[index]->open_for();
		if (before == after) { continue; }
		if (before != PlayerTeam::none) { changed[static_cast<size_t>(before)] = true; }
		if (after != PlayerTeam::none) { changed[static_cast<size_t>(after)] = true; }
		this->gate_permissions_[index] = after;
	}
	bool wants_territory = false;
	for (size_t seat = 0; seat < this->bots_.size() && seat < seats.size() &&
		seat < static_cast<size_t>(level.player_count()); ++seat)
	{
		const Player& player = level.player(static_cast<int>(seat));
		if (this->bots_[seat] != nullptr && player.team() != PlayerTeam::none &&
			changed[static_cast<size_t>(player.team())])
		{ this->bots_[seat]->replan_traversal(); }
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
		view.paint_refill = this->tuning_->handicap(player.handicap()).paint_refill;
		const PlayerPaintUnderfootState underfoot = player.paint_underfoot_state();
		view.underfoot = underfoot.enabled;
		view.on_enemy_paint = underfoot.enabled && underfoot.step_team != PlayerTeam::none &&
			underfoot.step_team != player.team();
		view.shot_empty = weapon.readiness() == WeaponReadiness::empty;
		view.engage_distance = engagement_distance(player, *this->tuning_);
		view.weapon = weapon.type();
		view.obstacles = gates.empty() ? obstacles : level.bot_obstacles(player.team());
		view.supports = gates.empty() ? supports : level.bot_supports(player.team());
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
				view.enemies.push_back({ candidate.center(), candidate.velocity(),
					engagement_distance(candidate, *this->tuning_) });
			}
			const bool replan = bot->needs_territory();
			if ((replan || support_shot(*bot, player)) && !surfaces.empty())
			{
				const bool deliberate = bot->deliberate(view.weapon);
				// Guarantee a local scoring face can be considered even when a
				// small deck is missed by all of Easy's global random samples.
				// Keep this separate from movement candidates and their RNG draws.
				float best_support_cost = (std::numeric_limits<float>::max)();
				float best_other_cost = (std::numeric_limits<float>::max)();
				std::optional<match_bot_territory::SupportTop> other;
				for (const PaintSurfaceSample& surface : surfaces)
				{
					if (surface.team == player.team()) { continue; }
					const std::optional<match_bot_territory::SupportTop> top = match_bot_territory::support_top_point(
						view, surface, this->tuning_->paint().thickness);
					if (!top)
					{
						// Off this support, only a slow straight shot that lands qualifies.
						if (!deliberate) { continue; }
						const std::optional<match_bot_territory::SupportTop> lip = match_bot_territory::scoring_top_point(
							view, surface, this->tuning_->paint().thickness);
						if (!lip) { continue; }
						const Vector2F gap = lip->point - view.center;
						const float cost = std::fabs(gap.x) + std::fabs(gap.y) * 2.0f;
						if (cost < best_other_cost && gap.length() <= view.engage_distance &&
							match_bot_territory::top_launch_lands(view, lip->bounds, gap.angle(), *this->tuning_) &&
							level.bot_line_of_sight(view.center, lip->point)) { best_other_cost = cost; other = lip; }
						continue;
					}
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
				// The support underfoot keeps priority over a lower or farther lip.
				if (!view.support_paint && other) { view.support_paint = other->point; view.support_bounds = other->bounds; }
				// Random territory sampling stays replan-only: the same draws.
				if (replan)
				{
					const int count = this->tuning_->bot(this->difficulties_[seat]).territory_candidates;
					for (int index = 0; index < count; ++index)
					{
						const PaintSurfaceSample& surface = surfaces[static_cast<size_t>(bot->random() % surfaces.size())];
						if (surface.team == player.team()) { continue; }
						const Vector2F point = paint_point(surface, this->tuning_->paint().thickness + view.size.x * 0.05f);
						if (Vector2F::distance_squared(point, view.center) > sight * sight) { continue; }
						if (level.bot_line_of_sight(view.center, point)) { view.territory.push_back(point); }
						else if (!gates.empty() && level.bot_traversal_line_of_sight(view.center, point, player.team()))
						{ view.route_territory.push_back(point); }
					}
				}
			}
		}
		seats[seat] = bot->think(view);
	}
}
