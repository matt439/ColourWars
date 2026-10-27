#pragma once

#include "game/objects/player_input_data.h"
#include "game/objects/player_team.h"
#include "game/objects/weapon_type.h"
#include "game/tuning.h"
#include "game/ai/match_bot_support.h"
#include "engine/math/rectanglef.h"
#include <cstdint>
#include <vector>

// Value observations for the shipping decision policy. The driver includes
// only living enemies within the shared sight range and with a clear sight
// line. No unseen enemy position survives as a pursuit target.
struct MatchBotObservation
{
	struct Enemy
	{
		mattmath::Vector2F center;
		mattmath::Vector2F velocity;
	};

	mattmath::Vector2F center;
	mattmath::Vector2F velocity;
	mattmath::Vector2F size;
	bool alive = false;
	bool grounded = false;
	bool on_platform = false;
	bool facing_right = true;
	float ammo = 0.0f;
	bool shot_empty = false;
	float engage_distance = 0.0f;
	float projectile_speed = 0.0f;
	float projectile_gravity = 0.0f;
	std::vector<Enemy> enemies;
	// Sampled visible non-team scoring tiles, refreshed only for a replan.
	std::vector<mattmath::Vector2F> territory;
	std::vector<mattmath::RectangleF> obstacles;
	// Rectangular standing surfaces, distinct from conservative sight bounds.
	// One-way supports may be crossed by a deliberate downward route.
	std::vector<MatchBotSupport> supports;
};

// One opponent's match-lifetime decisions, independent of cameras and devices.
// Local standing/descent and adjacent-support ascents are a foundation for
// arena qualification, not complete platform path finding.
class MatchBot
{
public:
	MatchBot(const GameTuning* tuning, BotDifficulty difficulty, std::uint64_t seed);
	PlayerInputData think(const MatchBotObservation& view);
	bool needs_territory() const;
	std::uint64_t random();

private:
	const GameTuning* tuning_;
	BotDifficulty difficulty_;
	std::uint64_t random_state_;
	int react_in_ = 0;
	int replan_in_ = 0;
	int blocked_steps_ = 0;
	int recovery_left_ = 0;
	int progress_steps_ = 0;
	mattmath::Vector2F progress_origin_;
	int jump_left_ = 0;
	bool jump_was_held_ = false;
	bool refilling_ = false;
	bool had_alive_step_ = false;
	bool has_goal_ = false;
	mattmath::Vector2F goal_;
	bool has_paint_goal_ = false;
	mattmath::Vector2F paint_goal_;
	float heading_ = 1.0f;
	PlayerInputData decision_;
	bool ascending_ = false;
	bool ascent_airborne_ = false;
	bool continue_ascent_ = false;
	int ascent_steps_left_ = 0;
	bool seeking_after_descent_ = false;
	mattmath::RectangleF ascent_support_;
	mattmath::Vector2F ascent_takeoff_;
	std::vector<mattmath::RectangleF> visited_supports_;

	float random_unit();
	void decide(const MatchBotObservation& view);
	void choose_territory(const MatchBotObservation& view);
	bool standing_goal(const MatchBotObservation& view, mattmath::Vector2F aim,
		mattmath::Vector2F& standing) const;
	bool clear_standing(const MatchBotObservation& view, mattmath::Vector2F center) const;
	bool clear_route(const MatchBotObservation& view, mattmath::Vector2F center) const;
	bool clear_route(const MatchBotObservation& view, mattmath::Vector2F from, mattmath::Vector2F to) const;
	void choose_descent(const MatchBotObservation& view);
	void choose_ascent(const MatchBotObservation& view);
	void steer_ascent(const MatchBotObservation& view, PlayerInputData& input);
	void remember_descent(const MatchBotObservation& view, mattmath::Vector2F goal);
	bool obstacle_ahead(const MatchBotObservation& view) const;
	bool ledge_ahead(const MatchBotObservation& view) const;
};
