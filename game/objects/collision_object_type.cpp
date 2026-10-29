#include "game/pch.h"
#include "game/objects/collision_object_type.h"

#include <stdexcept>
#include <string>

using labrador::CollisionLayer;
using labrador::CollisionMask;

CollisionLayer collision_layer(CollisionObjectType type)
{
	switch (type)
	{
	case CollisionObjectType::structure:
	case CollisionObjectType::structure_paintable:
	case CollisionObjectType::structure_jump_through:
	case CollisionObjectType::structure_fixed_slab:
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
	// Geometry accepts players and projectiles; collision filtering requires both masks to agree.
	case CollisionObjectType::structure:
	case CollisionObjectType::structure_paintable:
	case CollisionObjectType::structure_jump_through:
	case CollisionObjectType::structure_ramp_left:
	case CollisionObjectType::structure_ramp_right:
	case CollisionObjectType::structure_fixed_slab:
		return collision_layers::ALL_PLAYERS | collision_layers::ALL_PROJECTILES;

	// Players collide with geometry and enemy paint, but not other players or friendly paint.
	case CollisionObjectType::player_team_a:
		return collision_layers::ALL_STRUCTURES |
			collision_layers::PROJECTILE_TEAM_B;
	case CollisionObjectType::player_team_b:
		return collision_layers::ALL_STRUCTURES |
			collision_layers::PROJECTILE_TEAM_A;

	// Paint passes through ramps, so only flat structures are included.
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

ProjectileType to_projectile_type(CollisionObjectType type)
{
	switch (type)
	{
	case CollisionObjectType::projectile_spray_team_a:
	case CollisionObjectType::projectile_spray_team_b:
		return ProjectileType::spray;
	case CollisionObjectType::projectile_jet_team_a:
	case CollisionObjectType::projectile_jet_team_b:
		return ProjectileType::jet;
	case CollisionObjectType::projectile_rolling_team_a:
	case CollisionObjectType::projectile_rolling_team_b:
		return ProjectileType::rolling;
	case CollisionObjectType::projectile_mist_team_a:
	case CollisionObjectType::projectile_mist_team_b:
		return ProjectileType::mist;
	case CollisionObjectType::projectile_ball_team_a:
	case CollisionObjectType::projectile_ball_team_b:
		return ProjectileType::ball;
	default:
		throw std::runtime_error("Collision type " +
			std::to_string(static_cast<int>(type)) + " is not paint.");
	}
}
