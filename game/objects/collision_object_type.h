#pragma once

#include "engine/collision/collision_layer.h"

// The game's collision vocabulary, in one file: what a thing is, which group
// the engine files it under, and which groups it answers to.
//
// The enum used to be the whole of it, and it reached the engine by sitting in
// the collision interface's own signature - finding #13. It is the opaque tag
// now: engine/collision carries it through as an integer it never looks
// inside, and it is read back only here and in the responses. What the engine
// filters on is the layer and the mask.
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
	none
};

namespace collision_layers
{
	// The six groups worth telling the engine apart. There is no dead-player
	// layer: a dead player is one whose mask is empty, which is the same fact
	// stated where it changes rather than in a second pair of enumerators that
	// nothing ever read.
	constexpr artattack::CollisionLayer STRUCTURE = 1u << 0;

	// Slopes are their own group only because paint flies through them.
	// That is how the game already behaves - a projectile's type list named
	// the three flat structure kinds and omitted the two ramps - and it was a
	// gap in a list rather than a decision anybody wrote down. Merge this bit
	// into STRUCTURE to make ramps solid to paint.
	constexpr artattack::CollisionLayer STRUCTURE_RAMP = 1u << 1;

	constexpr artattack::CollisionLayer PLAYER_TEAM_A = 1u << 2;
	constexpr artattack::CollisionLayer PLAYER_TEAM_B = 1u << 3;
	constexpr artattack::CollisionLayer PROJECTILE_TEAM_A = 1u << 4;
	constexpr artattack::CollisionLayer PROJECTILE_TEAM_B = 1u << 5;

	constexpr artattack::CollisionMask ALL_STRUCTURES =
		STRUCTURE | STRUCTURE_RAMP;
	constexpr artattack::CollisionMask ALL_PLAYERS =
		PLAYER_TEAM_A | PLAYER_TEAM_B;
	constexpr artattack::CollisionMask ALL_PROJECTILES =
		PROJECTILE_TEAM_A | PROJECTILE_TEAM_B;
}

// The tag is the enum, unchanged and uninterpreted in between.
constexpr artattack::CollisionTag to_collision_tag(CollisionObjectType type)
{
	return static_cast<artattack::CollisionTag>(type);
}

constexpr CollisionObjectType to_collision_type(artattack::CollisionTag tag)
{
	return static_cast<CollisionObjectType>(tag);
}

// Which group this type belongs to, and which groups it responds to.
//
// These two replace six hand-written type lists spread over Player,
// Projectile and StructurePaintable, which had already drifted: a player
// asked only whether the other thing was a structure, and a projectile asked
// whether it was a structure or an enemy player, so the pair (player,
// projectile) passed one test and failed the other. Both sides now consult
// the same table, and find_contacts requires both to agree.
artattack::CollisionLayer collision_layer(CollisionObjectType type);
artattack::CollisionMask collision_mask(CollisionObjectType type);

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
