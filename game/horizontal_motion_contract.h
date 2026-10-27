#pragma once

// Engine-free authored motion and envelope arithmetic shared by the runtime
// and level document. All inputs are game floats; keep operation order and
// rounding identical to the admitted runtime path.
#include "game/simulation.h"
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

    struct Spec
    {
        Rectangle anchor;
        float travel_x, period_seconds, initial_phase;
        Drive drive = Drive::cycle;
        // Paint drive only: +1 when team A's paint carries the mover right, -1
        // when it carries it left. Team B's paint always heads the other way.
        int team_a_heading = 1;
    };

    struct DriveFields { Drive drive = Drive::cycle; int team_a_heading = 1; };

    // The authored text of motion.drive and motion.team_a_heads, nullopt when
    // absent. The game's loader and the level editor both read their JSON
    // through this, so a record one accepts the other accepts; each adds its
    // own location to the message.
    inline DriveFields decode_drive(const std::optional<std::string>& drive,
        const std::optional<std::string>& team_a_heads)
    {
        const std::string name = drive.value_or("cycle");
        if (name == "cycle")
        {
            if (team_a_heads) { throw std::invalid_argument("team_a_heads belongs only on a paint drive"); }
            return {};
        }
        if (name != "paint") { throw std::invalid_argument("drive must be 'cycle' or 'paint'"); }
        if (!team_a_heads) { throw std::invalid_argument("a paint drive needs team_a_heads 'right' or 'left'"); }
        if (*team_a_heads != "right" && *team_a_heads != "left")
        { throw std::invalid_argument("team_a_heads must be 'right' or 'left'"); }
        return { Drive::paint, *team_a_heads == "right" ? 1 : -1 };
    }

    inline void validate_drive(const Spec& spec)
    {
        if (spec.drive == Drive::cycle) { return; }
        if (spec.drive != Drive::paint)
        { throw std::invalid_argument("Horizontal motion drive must be a cycle or paint."); }
        // A paint drive keeps to the outgoing leg, where phase 0 is the left end
        // and 0.5 the right, so phase alone still names its pose.
        if (!(spec.initial_phase <= 0.5f))
        { throw std::invalid_argument("A paint-driven mover starts at phase 0 (left end) to 0.5 (right end)."); }
        if (spec.team_a_heading != 1 && spec.team_a_heading != -1)
        { throw std::invalid_argument("A paint-driven mover needs team A heading right or left."); }
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
        if (!std::isfinite(spec.travel_x) || !(spec.travel_x > 0.0f) ||
            !std::isfinite(spec.period_seconds) ||
            static_cast<double>(spec.period_seconds) < 2.0 * static_cast<double>(SECONDS_PER_STEP) ||
            !std::isfinite(spec.initial_phase) ||
            !(spec.initial_phase >= 0.0f && spec.initial_phase < 1.0f))
        {
            throw std::invalid_argument("Horizontal motion needs positive finite travel, period >= two simulation steps and phase in [0,1).");
        }
        Rectangle finish = spec.anchor;
        finish.x = spec.anchor.x + spec.travel_x;
        require_rectangle(finish);
        if (!(finish.x > spec.anchor.x))
        {
            throw std::invalid_argument("Horizontal motion travel must reach a distinct representable endpoint.");
        }
        validate_drive(spec);
    }

    inline float offset_at(const Spec& spec, double phase)
    {
        const double ratio = phase <= 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0;
        return static_cast<float>(static_cast<double>(spec.travel_x) * ratio);
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

    inline Rectangle pose(const Spec& spec, float offset)
    {
        Rectangle result = spec.anchor;
        if (offset != 0.0f) { result.x += offset; }
        box(result);
        return result;
    }

    inline Box path(const Spec& spec)
    {
        validate_spec(spec);
        const Box first = box(spec.anchor);
        const Box last = box(pose(spec, spec.travel_x));
        return { std::min(first.left, last.left), first.top,
            std::max(first.right, last.right), first.bottom };
    }

    inline Box standing(const Box& path, float body_width, float body_height)
    {
        const double scale = std::max({ std::abs(path.left), std::abs(path.right),
            std::abs(path.top), static_cast<double>(body_width), static_cast<double>(body_height) });
        const double pad = 2.0 * ulp(finite(2.0 * scale));
        return { finite(path.left - body_width - pad),
            finite(path.top - body_height - pad),
            finite(path.right + body_width + pad), path.top };
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
}
