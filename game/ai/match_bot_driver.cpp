#include "game/pch.h"
#include "game/ai/match_bot_driver.h"
#include "game/ai/match_bot_ballistics.h"
#include "game/ai/match_bot_territory.h"
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace mattmath;

namespace
{
	// A ready deliberate tool requalifies its top shot every step, airborne too.
	bool support_shot(const MatchBot& bot, const BotBody& body)
	{
		return bot.finishing() || bot.riding() || (bot.deliberate(body.weapon) && body.tool_ready);
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

float MatchBotDriver::engagement_distance(const BotBody& body, const GameTuning& tuning)
{
	const WeaponTuning& tool = tuning.weapon(body.weapon);
	const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
	return match_bot_ballistics::horizontal_reach(tool, projectile) +
		body.muzzle_reach +
		projectile.details.col_rect_size.x * 0.5f + tuning.player().size.x * 0.5f;
}

void MatchBotDriver::drive(std::vector<PlayerInputData>& seats, const BotWorld& world)
{
	// Input is observed before this step settles gates. React to the permission
	// actually published by the last settlement, including occupied holds.
	const std::vector<PlayerTeam> gates = world.gate_permissions();
	this->gate_permissions_.resize(gates.size(), PlayerTeam::none);
	std::array<bool, 2> changed{};
	for (size_t index = 0; index < gates.size(); ++index)
	{
		const PlayerTeam before = this->gate_permissions_[index];
		const PlayerTeam after = gates[index];
		if (before == after) { continue; }
		if (before != PlayerTeam::none) { changed[static_cast<size_t>(before)] = true; }
		if (after != PlayerTeam::none) { changed[static_cast<size_t>(after)] = true; }
		this->gate_permissions_[index] = after;
	}
	bool wants_territory = false;
	for (size_t seat = 0; seat < this->bots_.size() && seat < seats.size() &&
		seat < static_cast<size_t>(world.participant_count()); ++seat)
	{
		const BotBody player = world.body(static_cast<int>(seat));
		if (this->bots_[seat] != nullptr && player.team != PlayerTeam::none &&
			changed[static_cast<size_t>(player.team)])
		{ this->bots_[seat]->replan_traversal(); }
		wants_territory = wants_territory || (this->bots_[seat] != nullptr && player.alive &&
			(this->bots_[seat]->needs_territory() || support_shot(*this->bots_[seat], player)));
	}
	const std::vector<PaintSurfaceSample> surfaces = wants_territory ? world.paint_surfaces() :
		std::vector<PaintSurfaceSample>{};
	const std::vector<RectangleF> obstacles = world.obstacles(PlayerTeam::none);
	const std::vector<MatchBotSupport> supports = world.supports(PlayerTeam::none);
	const std::vector<MatchBotRamp> ramps = world.ramps();
	for (size_t seat = 0; seat < this->bots_.size() && seat < seats.size() &&
		seat < static_cast<size_t>(world.participant_count()); ++seat)
	{
		MatchBot* bot = this->bots_[seat].get();
		if (bot == nullptr) { continue; }
		const BotBody player = world.body(static_cast<int>(seat));
		MatchBotObservation view;
		view.center = player.center;
		view.velocity = player.velocity;
		view.size = this->tuning_->player().size;
		view.alive = player.alive && player.health > 0.0f;
		view.grounded = player.grounded;
		view.on_platform = player.on_platform;
		view.facing_right = player.facing_right;
		view.ammo = player.ammo / this->tuning_->starting_ammo();
		view.paint_refill = this->tuning_->handicap(player.handicap).paint_refill;
		view.underfoot = player.underfoot_enabled;
		view.on_enemy_paint = player.underfoot_enabled && player.underfoot_team != PlayerTeam::none &&
			player.underfoot_team != player.team;
		view.shot_empty = player.tool_empty;
		view.engage_distance = engagement_distance(player, *this->tuning_);
		view.weapon = player.weapon;
		view.obstacles = gates.empty() ? obstacles : world.obstacles(player.team);
		view.supports = gates.empty() ? supports : world.supports(player.team);
		view.ramps = ramps;
		const float sight = this->tuning_->bots().sight_distance;
		if (view.alive)
		{
			for (int other = 0; other < world.participant_count(); ++other)
			{
				const BotBody candidate = world.body(other);
				if (other == static_cast<int>(seat) || candidate.team == player.team ||
					!candidate.alive || candidate.health <= 0.0f ||
					Vector2F::distance_squared(candidate.center, view.center) > sight * sight ||
					!world.line_of_sight(view.center, candidate.center)) { continue; }
				view.enemies.push_back({ candidate.center, candidate.velocity,
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
					if (surface.team == player.team) { continue; }
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
							world.line_of_sight(view.center, lip->point)) { best_other_cost = cost; other = lip; }
						continue;
					}
					const Vector2F gap = top->point - view.center;
					const float cost = std::fabs(gap.x) + std::fabs(gap.y) * 2.0f;
					if (gap.length() > view.engage_distance) { continue; }
					// A deliberate tool also needs this nominal angle to land.
					const bool aimable = cost < best_support_cost &&
						match_bot_territory::top_launch_clear(view, top->point, *this->tuning_, this->difficulties_[seat]) &&
						(!deliberate || match_bot_territory::top_launch_lands(view, top->bounds, gap.angle(), *this->tuning_));
					if ((!aimable && view.support_open) || !world.line_of_sight(view.center, top->point)) { continue; }
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
						if (surface.team == player.team) { continue; }
						const Vector2F point = paint_point(surface, this->tuning_->paint().thickness + view.size.x * 0.05f);
						if (Vector2F::distance_squared(point, view.center) > sight * sight) { continue; }
						if (world.line_of_sight(view.center, point)) { view.territory.push_back(point); }
						else if (!gates.empty() && world.traversal_line_of_sight(view.center, point, player.team))
						{ view.route_territory.push_back(point); }
					}
				}
			}
		}
		seats[seat] = bot->think(view);
	}
}
