#pragma once

// Engine-free rules for a level's review stations and their lessons, shared by
// the game's loader and the level editor, so the editor refuses exactly the
// stations the game refuses, in the game's words. Each caller adds its own
// location to the message. Lesson words are checked by the game alone: it owns
// the fonts that decide what can be drawn.
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace station_contract
{
    struct Point
    {
        float x, y;
    };

    struct Rectangle
    {
        float x, y, width, height;
    };

    // Inclusive on every edge, as the engine's rectangle_point_intersect.
    inline bool contains(const Rectangle& rectangle, const Point& point)
    {
        return point.x >= rectangle.x && point.x <= rectangle.x + rectangle.width &&
            point.y >= rectangle.y && point.y <= rectangle.y + rectangle.height;
    }

    // Spelling, the reserved spawn names and repeats; `seen` holds the names
    // before this one and gains it.
    inline void validate_name(const std::string& name, std::vector<std::string>& seen)
    {
        const bool spelled = !name.empty() && std::all_of(name.begin(), name.end(),
            [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; });
        if (!spelled)
        {
            throw std::invalid_argument("names a station '" + name +
                "'; station names are lower-case letters, digits and '_', so they can be typed.");
        }
        if (name == "spawn_a" || name == "spawn_b")
        {
            throw std::invalid_argument("names a station '" + name +
                "', which every level already has (its team's first spawn).");
        }
        if (std::find(seen.begin(), seen.end(), name) != seen.end())
        {
            throw std::invalid_argument("repeats the station name '" + name + "'.");
        }
        seen.push_back(name);
    }

    // A player built outside out_of_bounds is retired on its first tick.
    inline void validate_position(const std::string& name, const Point& station,
        const Rectangle& out_of_bounds)
    {
        if (contains(out_of_bounds, station)) { return; }
        throw std::invalid_argument("puts a station '" + name + "' at (" +
            std::to_string(station.x) + ", " + std::to_string(station.y) + "), outside its own "
            "out_of_bounds rectangle - a player built there is retired on the "
            "first tick of the match.");
    }

    // The lesson area's own geometry; the message follows the area's location.
    inline void validate_lesson_area(const std::string& name, const Point& station,
        const Rectangle& area, const Rectangle& camera)
    {
        if (!std::isfinite(area.x) || !std::isfinite(area.y) ||
            !std::isfinite(area.width) || !std::isfinite(area.height) ||
            area.width <= 0.0f || area.height <= 0.0f)
        {
            throw std::invalid_argument("needs finite coordinates and positive width and height");
        }
        if (area.x < camera.x || area.y < camera.y ||
            area.x + area.width > camera.x + camera.width ||
            area.y + area.height > camera.y + camera.height)
        {
            throw std::invalid_argument("leaves the camera bounds; no player could stand in all of it.");
        }
        // `goto <station>` must land inside the station's own lesson.
        if (!contains(area, station))
        {
            throw std::invalid_argument("does not contain station '" + name +
                "'; going to the station must show its lesson.");
        }
    }

    // A lesson's title and text need something to read.
    inline void validate_lesson_words(const std::string& words)
    {
        if (words.find_first_not_of(" \n") == std::string::npos)
        {
            throw std::invalid_argument("is empty; a lesson needs a title and a line to read.");
        }
    }
}
