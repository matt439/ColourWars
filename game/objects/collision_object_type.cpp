#include "game/pch.h"
#include "game/objects/collision_object_type.h"

using artattack::CollisionLayer;
using artattack::CollisionMask;

CollisionLayer collision_layer(CollisionObjectType type)
{
	switch (type)
	{
	case CollisionObjectType::structure:
	case CollisionObjectType::structure_paintable:
	case CollisionObjectType::structure_jump_through:
		return collision_layers::STRUCTURE;

	case CollisionObjectType::structure_ramp_left:
	case CollisionObjectType::structure_ramp_right:
		return collision_layers::STRUCTURE_RAMP;

	case CollisionObjectType::player_team_a:
		return collision_layers::PLAYER_TEAM_A;
	case CollisionObjectType::player_team_b:
		return collision_layers::PLAYER_TEAM_B;

	case CollisionObjectType::projectile_spray_team_a:
	case CollisionObjectType::projectile_jet_team_a:
	case CollisionObjectType::projectile_rolling_team_a:
	case CollisionObjectType::projectile_ball_team_a:
	case CollisionObjectType::projectile_mist_team_a:
		return collision_layers::PROJECTILE_TEAM_A;

	case CollisionObjectType::projectile_spray_team_b:
	case CollisionObjectType::projectile_jet_team_b:
	case CollisionObjectType::projectile_rolling_team_b:
	case CollisionObjectType::projectile_ball_team_b:
	case CollisionObjectType::projectile_mist_team_b:
		return collision_layers::PROJECTILE_TEAM_B;

	case CollisionObjectType::none:
	default:
		return 0;
	}
}

CollisionMask collision_mask(CollisionObjectType type)
{
	switch (type)
	{
	// Level geometry answers to everything that moves. It never initiates -
	// Structure::is_colliding returned false unconditionally - but under a
	// symmetric filter "I never initiate" is not a thing an object can say,
	// and it never should have been: whether a pair is tested is a property
	// of the pair.
	case CollisionObjectType::structure:
	case CollisionObjectType::structure_paintable:
	case CollisionObjectType::structure_jump_through:
	case CollisionObjectType::structure_ramp_left:
	case CollisionObjectType::structure_ramp_right:
		return collision_layers::ALL_PLAYERS | collision_layers::ALL_PROJECTILES;

	// Players walk into level geometry, ramps included, and are shot by the
	// other team. Two players never collide with each other, and neither does
	// a player with their own paint.
	case CollisionObjectType::player_team_a:
		return collision_layers::ALL_STRUCTURES |
			collision_layers::PROJECTILE_TEAM_B;
	case CollisionObjectType::player_team_b:
		return collision_layers::ALL_STRUCTURES |
			collision_layers::PROJECTILE_TEAM_A;

	// STRUCTURE and not ALL_STRUCTURES: paint flies through ramps. See
	// collision_layers::STRUCTURE_RAMP.
	case CollisionObjectType::projectile_spray_team_a:
	case CollisionObjectType::projectile_jet_team_a:
	case CollisionObjectType::projectile_rolling_team_a:
	case CollisionObjectType::projectile_ball_team_a:
	case CollisionObjectType::projectile_mist_team_a:
		return collision_layers::STRUCTURE | collision_layers::PLAYER_TEAM_B;

	case CollisionObjectType::projectile_spray_team_b:
	case CollisionObjectType::projectile_jet_team_b:
	case CollisionObjectType::projectile_rolling_team_b:
	case CollisionObjectType::projectile_ball_team_b:
	case CollisionObjectType::projectile_mist_team_b:
		return collision_layers::STRUCTURE | collision_layers::PLAYER_TEAM_A;

	case CollisionObjectType::none:
	default:
		return 0;
	}
}
