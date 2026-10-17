#pragma once

#include "engine/assets/json.h"
#include "game/simulation.h"
#include "game/tuning.h"

class LevelObjectBuilder;

// The engine's anti-tunnelling bound (engine/collision/tunnelling.h) evaluated
// for every mover this game still resolves by sampling endpoints, against one
// stage's real geometry. Throws with the object's source location, the axis,
// the per-step travel and the bound when a mover at its speed cap could cross
// an object it collides with between two steps.
//
// Which movers that is: the player, whose velocity Player clamps per axis to
// player.max_velocity, and any unswept projectile kind (is_swept), of which
// the only one is the roller, which never moves. Every other projectile kind
// is exempt, not forgotten: Projectile::resolve_sweep tests its trajectory,
// which is what tunnelling.h tells a discrete client to do.
//
// The bound is exact only for boxes moving along one axis. For ramps (taken
// by their bounding box) and diagonal travel it is a tuning heuristic: a pass
// is not proof that nothing can be stepped through.
void check_stage_tunnelling(const GameTuning& tuning,
	const LevelObjectBuilder& builder,
	const labrador::JsonValue& collision_objects,
	float seconds_per_step = SECONDS_PER_STEP);
