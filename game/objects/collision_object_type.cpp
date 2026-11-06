#include "game/pch.h"
#include "game/objects/collision_object_type.h"

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
		return collision_layers::structure;

	case CollisionObjectType::structure_ramp_left:
	case CollisionObjectType::structure_ramp_right:
		return collision_layers::structure_ramp;

	case CollisionObjectType::player_team_a:
		return collision_layers::player_team_a;
	case CollisionObjectType::player_team_b:
		return collision_layers::player_team_b;

	case CollisionObjectType::projectile_spray_team_a:
	case CollisionObjectType::projectile_jet_team_a:
	case CollisionObjectType::projectile_rolling_team_a:
	case CollisionObjectType::projectile_ball_team_a:
	case CollisionObjectType::projectile_mist_team_a:
		return collision_layers::projectile_team_a;

	case CollisionObjectType::projectile_spray_team_b:
	case CollisionObjectType::projectile_jet_team_b:
	case CollisionObjectType::projectile_rolling_team_b:
	case CollisionObjectType::projectile_ball_team_b:
	case CollisionObjectType::projectile_mist_team_b:
		return collision_layers::projectile_team_b;

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
		return collision_layers::all_players | collision_layers::all_projectiles;

	// Players collide with geometry and enemy paint, but not other players or friendly paint.
	case CollisionObjectType::player_team_a:
		return collision_layers::all_structures |
			collision_layers::projectile_team_b;
	case CollisionObjectType::player_team_b:
		return collision_layers::all_structures |
			collision_layers::projectile_team_a;

	// Paint passes through ramps, so only flat structures are included.
	case CollisionObjectType::projectile_spray_team_a:
	case CollisionObjectType::projectile_jet_team_a:
	case CollisionObjectType::projectile_rolling_team_a:
	case CollisionObjectType::projectile_ball_team_a:
	case CollisionObjectType::projectile_mist_team_a:
		return collision_layers::structure | collision_layers::player_team_b;

	case CollisionObjectType::projectile_spray_team_b:
	case CollisionObjectType::projectile_jet_team_b:
	case CollisionObjectType::projectile_rolling_team_b:
	case CollisionObjectType::projectile_ball_team_b:
	case CollisionObjectType::projectile_mist_team_b:
		return collision_layers::structure | collision_layers::player_team_a;

	case CollisionObjectType::none:
	default:
		return 0;
	}
}
