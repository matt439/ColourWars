#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"

namespace press_tower_acceptance
{
    bool segment_hits(const Vector2F& from, const Vector2F& to, const RectangleF& r)
    {
        double t0 = 0.0, t1 = 1.0;
        const double dx = static_cast<double>(to.x) - from.x, dy = static_cast<double>(to.y) - from.y;
        const std::array<std::pair<double, double>, 4> edges{ {
            { -dx, static_cast<double>(from.x) - r.left() }, { dx, static_cast<double>(r.right()) - from.x },
            { -dy, static_cast<double>(from.y) - r.top() }, { dy, static_cast<double>(r.bottom()) - from.y } } };
        for (const auto& [p, q] : edges)
        {
            if (p == 0.0)
            {
                if (q <= 0.0) { return false; }
                continue;
            }
            const double t = q / p;
            if (p < 0.0) { t0 = std::max(t0, t); } else { t1 = std::min(t1, t); }
            if (t0 > t1) { return false; }
        }
        return t1 - t0 > 1e-6;
    }

    float reflected_x(const Services& s, bool right, float x)
    {
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        return right ? x : bounds.left() + bounds.right() - x;
    }

    void drop_once(Run& run, const std::string& expected_support)
    {
        require(supported(*run.player), "Press Tower drop did not start supported");
        const std::string from = run.support();
        const int takeoffs = run.takeoffs;
        for (int tick = 0; tick < 10 && supported(*run.player); ++tick)
        {
            run.tick(input(0.0f, false, false, true));
        }
        require(!supported(*run.player), "Press Tower drop never left one-way support '" + from + "'");
        for (int tick = 0; tick < 180 && !supported(*run.player); ++tick) { run.tick(); }
        require_support(run, expected_support, "drop from '" + from + "'");
        require(run.takeoffs == takeoffs, "Press Tower drop from '" + from + "' requested a jump");
    }

    void require_support(const Run& run, const std::string& name, const std::string& context)
    {
        require(supported(*run.player) && run.support() == name,
            "Press Tower " + context + " missed support '" + name + "'; found '" + run.support() +
            "' at x=" + fixed(run.player->center().x));
    }
}
