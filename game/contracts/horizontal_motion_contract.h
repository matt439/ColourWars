#pragma once

// Engine-free authored motion and envelope arithmetic shared by the runtime
// and level document. All inputs are game floats; keep operation order and
// rounding identical to the admitted runtime path.
#include "game/contracts/simulation.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace horizontal_motion_contract
{
    struct Rectangle
    {
        float x, y, width, height;
        float left() const { return x; }
        float top() const { return y; }
        float right() const { return x + width; }
        float bottom() const { return y + height; }
    };

    // What advances the phase. A cycle runs the triangular round trip forever. A
    // paint drive travels the same path at the same speed, but only while one team
    // holds more of the mover's own paint: toward that team's heading, stopping at
    // either end, and holding still on a tie (including no paint at all).
    enum class Drive : std::uint8_t { cycle, paint };

    // One axis per mover (ROAD#28, E1). A horizontal mover travels travel_x
    // right of its leftmost anchor; a vertical lift travels travel_y down from
    // its topmost anchor. Exactly one of the two is positive and the other is
    // zero; diagonal or two-axis motion is refused. The namespace keeps the
    // name of the capability it began as.
    struct Spec
    {
        Rectangle anchor;
        float travel_x, period_seconds, initial_phase;
        Drive drive = Drive::cycle;
        // Paint drive only: +1 when team A's paint carries the mover right (a
        // lift down), -1 when it carries it left (a lift up). Team B's paint
        // always heads the other way.
        int team_a_heading = 1;
        float travel_y = 0.0f;
        bool vertical() const { return travel_y != 0.0f; }
        float travel() const { return vertical() ? travel_y : travel_x; }
    };

    struct DriveFields { Drive drive = Drive::cycle; int team_a_heading = 1; };

    // The authored text of motion.drive and motion.team_a_heads, nullopt when
    // absent. The game's loader and the level editor both read their JSON
    // through this, so a record one accepts the other accepts; each adds its
    // own location to the message. A horizontal mover heads 'right' or 'left';
    // a vertical lift (ROAD#28) heads 'up' or 'down'. In both, +1 runs toward
    // phase 0.5 (the right end, or the lift's bottom) and -1 toward phase 0.
    inline DriveFields decode_drive(const std::optional<std::string>& drive,
        const std::optional<std::string>& team_a_heads, bool vertical = false)
    {
        const std::string name = drive.value_or("cycle");
        if (name == "cycle")
        {
            if (team_a_heads) { throw std::invalid_argument("team_a_heads belongs only on a paint drive"); }
            return {};
        }
        if (name != "paint") { throw std::invalid_argument("drive must be 'cycle' or 'paint'"); }
        const char* toward = vertical ? "down" : "right";
        const char* back = vertical ? "up" : "left";
        const std::string pair = vertical ? "'up' or 'down'" : "'right' or 'left'";
        if (!team_a_heads)
        {
            throw std::invalid_argument(vertical ? "a paint-driven lift needs team_a_heads " + pair
                : "a paint drive needs team_a_heads " + pair);
        }
        if (*team_a_heads != toward && *team_a_heads != back)
        {
            throw std::invalid_argument(std::string(vertical ? "a lift's " : "") + "team_a_heads must be " + pair);
        }
        return { Drive::paint, *team_a_heads == toward ? 1 : -1 };
    }

    // The authored name of a paint heading: the inverse of decode_drive.
    inline const char* heading_name(bool vertical, int heading)
    {
        if (vertical) { return heading > 0 ? "down" : "up"; }
        return heading > 0 ? "right" : "left";
    }

    inline void validate_drive(const Spec& spec)
    {
        if (spec.drive == Drive::cycle) { return; }
        if (spec.drive != Drive::paint)
        { throw std::invalid_argument("Horizontal motion drive must be a cycle or paint."); }
        // A paint drive keeps to the outgoing leg, where phase 0 is the left
        // end (a lift's top) and 0.5 the right (its bottom), so phase alone
        // still names its pose.
        if (!(spec.initial_phase <= 0.5f))
        {
            throw std::invalid_argument(spec.vertical()
                ? "A paint-driven lift starts at phase 0 (top) to 0.5 (bottom)."
                : "A paint-driven mover starts at phase 0 (left end) to 0.5 (right end).");
        }
        if (spec.team_a_heading != 1 && spec.team_a_heading != -1)
        {
            throw std::invalid_argument(spec.vertical() ? "A paint-driven lift needs team A heading up or down."
                : "A paint-driven mover needs team A heading right or left.");
        }
    }

    struct Box { double left, top, right, bottom; };

    inline void require_rectangle(const Rectangle& rectangle)
    {
        if (!std::isfinite(rectangle.x) || !std::isfinite(rectangle.y) ||
            !std::isfinite(rectangle.width) || !std::isfinite(rectangle.height) ||
            !(rectangle.width > 0.0f) || !(rectangle.height > 0.0f) ||
            !std::isfinite(rectangle.right()) || !std::isfinite(rectangle.bottom()) ||
            !(rectangle.right() > rectangle.left()) || !(rectangle.bottom() > rectangle.top()))
        {
            throw std::invalid_argument("Horizontal motion requires finite representable positive rectangle edges.");
        }
    }

    inline void validate_spec(const Spec& spec)
    {
        require_rectangle(spec.anchor);
        // The idle axis must be exactly zero: a NaN or a second travel is a
        // two-axis request, not a lift.
        const bool vertical = spec.travel_x == 0.0f && std::isfinite(spec.travel_y) && spec.travel_y > 0.0f;
        const bool horizontal = spec.travel_y == 0.0f;
        if (!vertical && !horizontal)
        {
            throw std::invalid_argument("Motion travels on one axis: travel_x for a horizontal mover or travel_y for a vertical lift.");
        }
        const float travel = vertical ? spec.travel_y : spec.travel_x;
        if (!std::isfinite(travel) || !(travel > 0.0f) ||
            !std::isfinite(spec.period_seconds) ||
            static_cast<double>(spec.period_seconds) < 2.0 * static_cast<double>(SECONDS_PER_STEP) ||
            !std::isfinite(spec.initial_phase) ||
            !(spec.initial_phase >= 0.0f && spec.initial_phase < 1.0f))
        {
            throw std::invalid_argument("Horizontal motion needs positive finite travel, period >= two simulation steps and phase in [0,1).");
        }
        Rectangle finish = spec.anchor;
        if (vertical) { finish.y = spec.anchor.y + spec.travel_y; }
        else { finish.x = spec.anchor.x + spec.travel_x; }
        require_rectangle(finish);
        if (vertical ? !(finish.y > spec.anchor.y) : !(finish.x > spec.anchor.x))
        {
            throw std::invalid_argument("Horizontal motion travel must reach a distinct representable endpoint.");
        }
        validate_drive(spec);
    }

    // The scalar offset along the mover's own axis.
    inline float offset_at(const Spec& spec, double phase)
    {
        const double ratio = phase <= 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
        return static_cast<float>(static_cast<double>(spec.travel()) * ratio);
    }

    inline double finite(double value)
    {
        if (!std::isfinite(value) || std::abs(value) >
            static_cast<double>((std::numeric_limits<float>::max)()))
        {
            throw std::invalid_argument("Horizontal motion requires finite representable arithmetic and edges.");
        }
        return value;
    }

    inline double up(double value)
    {
        finite(value);
        if (value < 0.0)
        { throw std::invalid_argument("Horizontal motion requires nonnegative displacement budgets."); }
        float result = static_cast<float>(value);
        if (static_cast<double>(result) < value)
        { result = std::nextafter(result, std::numeric_limits<float>::infinity()); }
        return finite(static_cast<double>(result));
    }

    inline double ulp(double magnitude)
    {
        const float value = static_cast<float>(up(std::abs(magnitude)));
        const float next = std::nextafter(value, std::numeric_limits<float>::infinity());
        finite(static_cast<double>(next));
        return static_cast<double>(next) - static_cast<double>(value);
    }

    inline Box box(const Rectangle& value)
    {
        const Box result{ value.left(), value.top(), value.right(), value.bottom() };
        for (double edge : { result.left, result.top, result.right, result.bottom })
        { finite(edge); }
        if (!std::isfinite(value.width) || !std::isfinite(value.height) ||
            !(value.width > 0.0f) || !(value.height > 0.0f) ||
            !(result.left < result.right) || !(result.top < result.bottom))
        { throw std::invalid_argument("Horizontal motion requires finite positive represented body dimensions."); }
        return result;
    }

    // A scalar axis offset adds to x for a horizontal mover and to y for a lift.
    inline Rectangle pose(const Spec& spec, float offset)
    {
        Rectangle result = spec.anchor;
        if (offset != 0.0f)
        {
            if (spec.vertical()) { result.y += offset; }
            else { result.x += offset; }
        }
        box(result);
        return result;
    }

    // Every pose's union. A horizontal path keeps the anchor's top and bottom
    // and widens left/right; a lift's keeps its left and right and widens down.
    inline Box path(const Spec& spec)
    {
        validate_spec(spec);
        const Box first = box(spec.anchor);
        const Box last = box(pose(spec, spec.travel()));
        if (spec.vertical())
        {
            return { first.left, std::min(first.top, last.top),
                first.right, std::max(first.bottom, last.bottom) };
        }
        return { std::min(first.left, last.left), first.top,
            std::max(first.right, last.right), first.bottom };
    }

    inline double corridor_pad(const Box& path, float body_width, float body_height)
    {
        const double scale = std::max({ std::abs(path.left), std::abs(path.right),
            std::abs(path.top), std::abs(path.bottom),
            static_cast<double>(body_width), static_cast<double>(body_height) });
        return 2.0 * ulp(finite(2.0 * scale));
    }

    // One body height above the path's top, widened by a body each side. For
    // a lift the path's top is its highest pose, so the corridor rises with it
    // (E3a) and no ceiling can close on a rider.
    inline Box standing(const Box& path, float body_width, float body_height)
    {
        const double scale = std::max({ std::abs(path.left), std::abs(path.right),
            std::abs(path.top), static_cast<double>(body_width), static_cast<double>(body_height) });
        const double pad = 2.0 * ulp(finite(2.0 * scale));
        return { finite(path.left - body_width - pad),
            finite(path.top - body_height - pad),
            finite(path.right + body_width + pad), path.top };
    }

    // A lift only (E3a as amended 2026-11-03): the room its lowest pose must
    // leave beneath it, a full body height plus the corridor pad, widened as the
    // standing corridor is. Nothing may stand in it, so a descending underside
    // never closes on a player; a lift that meets its floor flush is refused.
    inline Box underside(const Box& path, float body_width, float body_height)
    {
        const double pad = corridor_pad(path, body_width, body_height);
        return { finite(path.left - body_width - pad), path.bottom,
            finite(path.right + body_width + pad), finite(path.bottom + body_height + pad) };
    }

    // A lift only: the whole column a rider or a player beside it can occupy.
    // It is the shaft widened by a body each side (a rider may overhang the
    // lift's edge by almost a body), from a body above the highest pose to a
    // body below the lowest. It contains the standing corridor and the
    // clearance beneath. With it clear, no step can push a body into geometry,
    // so E3(a) alone rules out a crush and the hold (E3b) should never fire.
    inline Box column(const Box& path, float body_width, float body_height)
    {
        const double pad = corridor_pad(path, body_width, body_height);
        return { finite(path.left - body_width - pad), finite(path.top - body_height - pad),
            finite(path.right + body_width + pad), finite(path.bottom + body_height + pad) };
    }

    inline bool contains(const Box& outer, const Box& inner)
    {
        return inner.left >= outer.left && inner.top >= outer.top &&
            inner.right <= outer.right && inner.bottom <= outer.bottom;
    }

    inline bool intersects(const Box& a, const Box& b)
    {
        return a.left < b.right && a.right > b.left &&
            a.top < b.bottom && a.bottom > b.top;
    }

    // A lift is solid (ROAD#28). A jump-through landing compares the rider's
    // previous feet with the platform's current top, which a rising lift has
    // already moved past, so it could drop its rider through.
    inline void validate_collision(const Spec& spec, bool jump_through)
    {
        if (spec.vertical() && jump_through)
        { throw std::invalid_argument("A vertical lift needs solid paintable collision, not jump-through."); }
    }

    // E3b: where a lift's step from before to after would put a player's body,
    // nullopt when the step leaves it alone. A rider moves with the top; a body
    // the moving lift would overlap is pushed clear of it along the lift's travel.
    inline std::optional<Box> pushed_body(const Box& before, const Box& after,
        const Box& body, bool rider)
    {
        const double dy = after.top - before.top;
        if (dy == 0.0) { return std::nullopt; }
        if (rider) { return Box{ body.left, body.top + dy, body.right, body.bottom + dy }; }
        if (!intersects(after, body)) { return std::nullopt; }
        const double height = body.bottom - body.top;
        if (dy > 0.0) { return Box{ body.left, after.bottom, body.right, after.bottom + height }; }
        return Box{ body.left, after.top - height, body.right, after.top };
    }

    // True when the step would push a player through blocking geometry or out
    // of the scene: the lift then holds its phase for the step. Admission
    // (E3a) leaves room above and below every lift, so this should never fire.
    template<class Obstacles>
    bool closes_on(const Box& before, const Box& after, const Box& body, bool rider,
        const Box& world, const Obstacles& obstacles)
    {
        const std::optional<Box> pushed = pushed_body(before, after, body, rider);
        if (!pushed) { return false; }
        if (!contains(world, *pushed)) { return true; }
        for (const Box& obstacle : obstacles)
        {
            if (intersects(*pushed, obstacle)) { return true; }
        }
        return false;
    }
}
