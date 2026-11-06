#pragma once

#include "engine/assets/json.h"
#include "game/contracts/simulation.h"
#include "game/config/tuning.h"

class LevelObjectBuilder;

// The engine's anti-tunnelling bound (engine/collision/tunnelling.h) evaluated
// for every mover this game still resolves by sampling endpoints, against one
// stage's real geometry. Throws with the object's source location, the axis,
// the per-step travel and the bound when a mover at its speed cap could cross
// an object it collides with between two steps.
//
// Which movers that is: the player, whose velocity Player clamps per axis to
// player.max_velocity except on the first step of input (x_initial_velocity,
// so x takes the larger of the two) and on a drop-through (drop_down_velocity,
// added to y after the clamp, so y takes the sum). With paint underfoot on,
// both horizontal speeds are multiplied by own_speed_multiplier first, since a
// boost can carry into the next step. No projectile is a mover here. The only
// unswept kind (is_swept) is the roller, which never moves; every other kind
// is exempt, not forgotten: Projectile::resolve_sweep tests its trajectory,
// which is what tunnelling.h tells a discrete client to do.
//
// The bound is exact only for boxes moving along one axis. For ramps (taken
// by their bounding box) and diagonal travel it is a tuning heuristic: a pass
// is not proof that nothing can be stepped through.
void check_stage_tunnelling(const GameTuning& tuning,
	const LevelObjectBuilder& builder,
	const labrador::JsonValue& collision_objects,
	float seconds_per_step = simulation::seconds_per_step);
