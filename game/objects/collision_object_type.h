#ifndef COLLISION_OBJECT_TYPE_H
#define COLLISION_OBJECT_TYPE_H

enum class collision_object_type
{
	STRUCTURE,
	STRUCTURE_PAINTABLE,
	STRUCTURE_JUMP_THROUGH,
	STRUCTURE_RAMP_LEFT,
	STRUCTURE_RAMP_RIGHT,
	PLAYER_TEAM_A,
	PLAYER_TEAM_B,
	PLAYER_TEAM_A_DEAD,
	PLAYER_TEAM_B_DEAD,
	PROJECTILE_SPRAY_TEAM_A,
	PROJECTILE_SPRAY_TEAM_B,
	PROJECTILE_JET_TEAM_A,
	PROJECTILE_JET_TEAM_B,
	PROJECTILE_ROLLING_TEAM_A,
	PROJECTILE_ROLLING_TEAM_B,
	PROJECTILE_BALL_TEAM_A,
	PROJECTILE_BALL_TEAM_B,
	PROJECTILE_MIST_TEAM_A,
	PROJECTILE_MIST_TEAM_B,
	PAINT_TILE,
	POWER_UP,
	DEFAULT,
	NONE
};

inline bool is_player(collision_object_type type)
{
	return type == collision_object_type::PLAYER_TEAM_A ||
		type == collision_object_type::PLAYER_TEAM_B;
}

inline bool is_dead_player(collision_object_type type)
{
	return type == collision_object_type::PLAYER_TEAM_A_DEAD ||
		type == collision_object_type::PLAYER_TEAM_B_DEAD;
}

inline bool is_projectile(collision_object_type type)
{
	return type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
		type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
		type == collision_object_type::PROJECTILE_JET_TEAM_A ||
		type == collision_object_type::PROJECTILE_JET_TEAM_B ||
		type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
		type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
		type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
		type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
		type == collision_object_type::PROJECTILE_MIST_TEAM_A ||
		type == collision_object_type::PROJECTILE_MIST_TEAM_B;
}

inline bool is_team_a_projectile(collision_object_type type)
{
	return type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
		type == collision_object_type::PROJECTILE_JET_TEAM_A ||
		type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
		type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
		type == collision_object_type::PROJECTILE_MIST_TEAM_A;
}

inline bool is_team_b_projectile(collision_object_type type)
{
	return type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
		type == collision_object_type::PROJECTILE_JET_TEAM_B ||
		type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
		type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
		type == collision_object_type::PROJECTILE_MIST_TEAM_B;
}

inline bool is_structure(collision_object_type type)
{
	return type == collision_object_type::STRUCTURE ||
		type == collision_object_type::STRUCTURE_PAINTABLE ||
		type == collision_object_type::STRUCTURE_JUMP_THROUGH ||
		type == collision_object_type::STRUCTURE_RAMP_LEFT ||
		type == collision_object_type::STRUCTURE_RAMP_RIGHT;
}

inline bool is_structure_ramp(collision_object_type type)
{
	return type == collision_object_type::STRUCTURE_RAMP_LEFT ||
		type == collision_object_type::STRUCTURE_RAMP_RIGHT;
}

#endif // !COLLISION_OBJECT_TYPE_H
