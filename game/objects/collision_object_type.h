#pragma once

#include "engine/collision/collision_layer.h"
#include "game/objects/projectile_type.h"

// Game-specific collision tags. The engine carries these opaquely and filters
// contacts using layers and masks.
enum class CollisionObjectType
{
	structure,
	structure_paintable,
	structure_jump_through,
	structure_ramp_left,
	structure_ramp_right,
	player_team_a,
	player_team_b,
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
	none,
	// Appended so existing replay/debug tag values remain unchanged.
	structure_fixed_slab
};

namespace collision_layers
{
	// Dead players keep their layer but use an empty mask.
	constexpr labrador::CollisionLayer STRUCTURE = 1u << 0;

	// Ramps have a separate layer because paint passes through them.
	constexpr labrador::CollisionLayer STRUCTURE_RAMP = 1u << 1;

	constexpr labrador::CollisionLayer PLAYER_TEAM_A = 1u << 2;
	constexpr labrador::CollisionLayer PLAYER_TEAM_B = 1u << 3;
	constexpr labrador::CollisionLayer PROJECTILE_TEAM_A = 1u << 4;
	constexpr labrador::CollisionLayer PROJECTILE_TEAM_B = 1u << 5;

	constexpr labrador::CollisionMask ALL_STRUCTURES =
		STRUCTURE | STRUCTURE_RAMP;
	constexpr labrador::CollisionMask ALL_PLAYERS =
		PLAYER_TEAM_A | PLAYER_TEAM_B;
	constexpr labrador::CollisionMask ALL_PROJECTILES =
		PROJECTILE_TEAM_A | PROJECTILE_TEAM_B;
}

// The tag is the enum, unchanged and uninterpreted in between.
constexpr labrador::CollisionTag to_collision_tag(CollisionObjectType type)
{
	return static_cast<labrador::CollisionTag>(type);
}

constexpr CollisionObjectType to_collision_type(labrador::CollisionTag tag)
{
	return static_cast<CollisionObjectType>(tag);
}

// Layer and mask for each type. Both objects must accept a pair for contact testing.
labrador::CollisionLayer collision_layer(CollisionObjectType type);
labrador::CollisionMask collision_mask(CollisionObjectType type);

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

// Projectile kind without its firing team, used to look up damage tuning.
// Throws if the tag is not a projectile.
ProjectileType to_projectile_type(CollisionObjectType type);

inline bool is_structure(CollisionObjectType type)
{
	return type == CollisionObjectType::structure ||
		type == CollisionObjectType::structure_paintable ||
		type == CollisionObjectType::structure_jump_through ||
		type == CollisionObjectType::structure_ramp_left ||
		type == CollisionObjectType::structure_ramp_right ||
		type == CollisionObjectType::structure_fixed_slab;
}

inline bool is_structure_ramp(CollisionObjectType type)
{
	return type == CollisionObjectType::structure_ramp_left ||
		type == CollisionObjectType::structure_ramp_right;
}
