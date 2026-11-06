#pragma once

#include "scripts/acceptance/support/route_run.h"

namespace press_tower_acceptance
{
    using namespace acceptance::route_run;

    // Liang-Barsky. A segment meets the closed rectangle only over a
    // positive length; grazing a corner does not count.
    bool segment_hits(const Vector2F& from, const Vector2F& to, const RectangleF& r);

    // The stage mirrors about the out-of-bounds centre (X 1800). Team A's
    // routes are authored left to right; `right == false` reflects them.
    float reflected_x(const Services& s, bool right, float x);
    // jump_to that also refuses a landing that took off more than once.
    inline void jump_once(Run& run, const Surface& target, float centre_x, int delay,
        int hold_ticks = default_jump_hold)
    {
        jump_to(run, target, centre_x, delay, hold_ticks, true);
    }
    // Leaves the current one-way support by holding Down only until the body
    // is airborne, then releases, so the next one-way tier below catches it.
    void drop_once(Run& run, const std::string& expected_support);
    void require_support(const Run& run, const std::string& name, const std::string& context);
    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
    void painting_checks(Services& s);
    void live_fire_checks(Services& s);
}
