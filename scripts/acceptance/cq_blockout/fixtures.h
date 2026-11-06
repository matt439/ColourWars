#pragma once

#include "scripts/acceptance/support/route_run.h"
#include "game/contracts/paint_face_geometry.h"

namespace cq_blockout_acceptance
{
    using namespace acceptance::route_run;

    // CQ's jumps hold the button through the whole landing window.
    inline void held_jump_to(Run& run, const Surface& target, float centre_x, int delay)
    {
        jump_to(run, target, centre_x, delay, jump_held_throughout);
    }

    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
}
