#pragma once

enum class CollisionObjectType
{
	structure,
	structure_paintable,
	structure_jump_through,
	structure_ramp_left,
	structure_ramp_right,
	player_team_a,
	player_team_b,
	player_team_a_dead,
	player_team_b_dead,
	projectile_spray_team_a,
	projectile_spray_team_b,
	projectile_jet_team_a,
	projectile_jet_team_b,
	projectile_rolling_team_a,
	projectile_rolling_team_b,
	projectile_ball_team_a,
	projectile_ball_team_b,
	projectile_mist_team_a,
	projectile_mist_team_b,
	paint_tile,
	power_up,
	none
};

inline bool is_player(CollisionObjectType type)
{
	return type == CollisionObjectType::player_team_a ||
		type == CollisionObjectType::player_team_b;
}

inline bool is_dead_player(CollisionObjectType type)
{
	return type == CollisionObjectType::player_team_a_dead ||
		type == CollisionObjectType::player_team_b_dead;
}

inline bool is_projectile(CollisionObjectType type)
{
	return type == CollisionObjectType::projectile_spray_team_a ||
		type == CollisionObjectType::projectile_spray_team_b ||
		type == CollisionObjectType::projectile_jet_team_a ||
		type == CollisionObjectType::projectile_jet_team_b ||
		type == CollisionObjectType::projectile_rolling_team_a ||
		type == CollisionObjectType::projectile_rolling_team_b ||
		type == CollisionObjectType::projectile_ball_team_a ||
		type == CollisionObjectType::projectile_ball_team_b ||
		type == CollisionObjectType::projectile_mist_team_a ||
		type == CollisionObjectType::projectile_mist_team_b;
}

inline bool is_team_a_projectile(CollisionObjectType type)
{
	return type == CollisionObjectType::projectile_spray_team_a ||
		type == CollisionObjectType::projectile_jet_team_a ||
		type == CollisionObjectType::projectile_rolling_team_a ||
		type == CollisionObjectType::projectile_ball_team_a ||
		type == CollisionObjectType::projectile_mist_team_a;
}

inline bool is_team_b_projectile(CollisionObjectType type)
{
	return type == CollisionObjectType::projectile_spray_team_b ||
		type == CollisionObjectType::projectile_jet_team_b ||
		type == CollisionObjectType::projectile_rolling_team_b ||
		type == CollisionObjectType::projectile_ball_team_b ||
		type == CollisionObjectType::projectile_mist_team_b;
}

inline bool is_structure(CollisionObjectType type)
{
	return type == CollisionObjectType::structure ||
		type == CollisionObjectType::structure_paintable ||
		type == CollisionObjectType::structure_jump_through ||
		type == CollisionObjectType::structure_ramp_left ||
		type == CollisionObjectType::structure_ramp_right;
}

inline bool is_structure_ramp(CollisionObjectType type)
{
	return type == CollisionObjectType::structure_ramp_left ||
		type == CollisionObjectType::structure_ramp_right;
}
