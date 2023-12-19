#ifndef COLLISION_OBJECT_TYPE_H
#define COLLISION_OBJECT_TYPE_H

//#include <vector>
//#include "shape_type.h"

enum class collision_object_type
{
	STRUCTURE_WALL,
	STRUCTURE_WALL_PAINTABLE,
	STRUCTURE_FLOOR,
	STRUCTURE_FLOOR_PAINTABLE,
	STRUCTURE,
	STRUCTURE_PAINTABLE,
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
	//PROJECTILE_OFFENSIVE_TEAM_A,
	//PROJECTILE_OFFENSIVE_TEAM_B,
	//PROJECTILE_DEFENSIVE_TEAM_A,
	//PROJECTILE_DEFENSIVE_TEAM_B,
	POWER_UP,
	DEFAULT,
	NONE
};

//enum class collidable_object_shape
//{
//	RECTANGLE,
//	RECTANGLE_ROTATED,
//	CIRCLE,
//	TRIANGLE,
//	NONE
//};

//enum class collision_effect_type
//{
//	COLLIDER_PLAYER_HEALTH_ALTER,
//	COLLIDEE_PLAYER_HEALTH_ALTER,
//	COLLIDER_PLAYER_AMMO_ALTER,
//	PAINTABLE_OBJECT_PAINT_TEAM_A,
//	PAINTABLE_OBJECT_PAINT_TEAM_B,
//	DESTROY_SELF,
//	DESTROY_OTHER,
//	IMPART_VELOCITY,
//	NONE
//};
//
//enum class collision_movement_outcome
//{
//	NONE, // no effect, pass through
//	STOP_VERTICAL, // slide horizontally
//	STOP_HORIZONTAL, // slide vertically
//	REBOUND_HORIZONTAL, // rebound horizontally
//	REBOUND_VERTICAL, // rebound vertically
//	STOP, // stop immediately
//	
//};
//
//struct collision_effect_detail
//{A
//	collision_effect_type type = collision_effect_type::NONE;
//	float value = 0.0f;
//};
//
//struct collidee_object_detail
//{
//	collision_object_type colidee_type = collision_object_type::NONE;
//	collision_movement_outcome movement_outcome = collision_movement_outcome::NONE;
//	//collidable_object_shape colidee_shape = collidable_object_shape::NONE;
//	std::vector<collision_effect_detail> effects;
//};

#endif // !COLLISION_OBJECT_TYPE_H
