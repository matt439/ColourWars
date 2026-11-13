#pragma once

#include <cstdint>

// ROAD#44: the collision shape a round shot (the bucket's ball, a sub's
// grenade and every drop either sub releases) is built with. rectangle is
// what has always shipped; circle collides the same shot as the disc
// inscribed in its collision square and sweeps it as one
// (Projectile::resolve_sweep). The author decides on hit feel from the
// measurement in docs/feature-roadmap.md, so until then circle is reached
// only from the Debug console (shots.round) and the Debug balance runner
// (--round-shots).
//
// The shape is how a shot collides, not a number to tune, so it is code and
// never a tuning.json leaf. It is one process-wide value rather than a Level
// option because shots are built by weapons, subs and other shots, none of
// which hold the Level: a shot reads it once, when it is constructed, and
// keeps that shape for life. A seeded tape therefore replays bit for bit only
// under the setting it was recorded with.
enum class RoundShotShape : std::uint8_t
{
	rectangle,
	circle
};

RoundShotShape round_shot_shape();
void set_round_shot_shape(RoundShotShape shape);
