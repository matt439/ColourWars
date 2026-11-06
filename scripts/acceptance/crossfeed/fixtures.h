#pragma once

#include "scripts/acceptance/support/route_run.h"

namespace crossfeed_acceptance
{
    using namespace acceptance::route_run;

    void geometry_checks(Services& s, const std::filesystem::path& output);
    void spawn_checks(Services& s);
    void route_checks(Services& s);
    void tool_checks(Services& s);
    void combat_checks(Services& s);
    void pressure_checks(Services& s);
    void reciprocal_checks(Services& s);
    void trajectory_checks(Services& s);
    void supply_checks(Services& s);
    void painting_checks(Services& s);
    void repaint_checks(Services& s);
    void home_loop_checks(Services& s);
    void opposed_loop_checks(Services& s);
    void opening_checks(Services& s);
}
