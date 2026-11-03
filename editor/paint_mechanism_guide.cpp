#include "editor/paint_mechanism_guide.h"
#include "game/contracts/level_format.h"

#include <algorithm>
#include <initializer_list>

namespace colourwars_editor
{
    std::optional<PaintMechanismGuide> paint_mechanism_guide(
        const std::map<std::string, std::string>& properties)
    {
        const auto value = [&properties](const char* key)
        {
            const auto found = properties.find(key);
            return found == properties.end() ? std::string() : found->second;
        };
        if (value(level_format::key::type) != level_format::object_type::structure_paintable) { return std::nullopt; }
        if (value("gate") == "paint")
        { return PaintMechanismGuide{ PaintMechanismKind::gate }; }
        if (value("motion/type") == "horizontal" && value("motion/drive") == "paint")
        {
            const std::string heading = value("motion/team_a_heads");
            if (heading == "left" || heading == "right")
            { return PaintMechanismGuide{ PaintMechanismKind::ferry, heading == "right" ? 1 : -1 }; }
        }
        // A paint lift (ROAD#28): +1 is down, as in the shared drive rule.
        if (value("motion/type") == "vertical" && value("motion/drive") == "paint")
        {
            const std::string heading = value("motion/team_a_heads");
            if (heading == "up" || heading == "down")
            { return PaintMechanismGuide{ PaintMechanismKind::lift, heading == "down" ? 1 : -1 }; }
        }
        return std::nullopt;
    }

    PaintMechanismDrawing paint_mechanism_drawing(const PaintMechanismGuide& guide,
        const Rect& screen_body, double line_height)
    {
        PaintMechanismDrawing drawing;
        const double width = 132.0;
        // A lift's panel has the ferry's two team rows, with vertical arrows.
        const bool ferry = guide.kind != PaintMechanismKind::gate;
        const double first_row = std::max(ferry ? 24.0 : 22.0, 4.0 + line_height + 4.0);
        const double second_row = ferry ? std::max(42.0, first_row + line_height + 4.0) : first_row + 4.0;
        const double gate_feet = std::max(39.0, second_row + line_height + 4.0);
        const double height = ferry ? second_row + line_height + 4.0 : gate_feet + 4.0;
        const double x = screen_body.x + (screen_body.width - width) / 2.0;
        const double y = screen_body.y - height - 6.0;
        drawing.panel = { x, y, width, height };
        if (guide.kind == PaintMechanismKind::gate)
        {
            drawing.labels = { { x + 8.0, y + 4.0, L"PAINT GATE" },
                { x + 48.0, y + second_row, L"A / B" } };
            // A permanent schematic lintel and two posts, never an open/closed
            // gate picture or a claim that either team currently has permission.
            drawing.strokes = { { x + 24.0, y + first_row, x + 108.0, y + first_row },
                { x + 24.0, y + first_row, x + 24.0, y + gate_feet },
                { x + 108.0, y + first_row, x + 108.0, y + gate_feet },
                { x + 24.0, y + gate_feet, x + 39.0, y + gate_feet },
                { x + 93.0, y + gate_feet, x + 108.0, y + gate_feet } };
        }
        else if (guide.kind == PaintMechanismKind::lift)
        {
            drawing.labels = { { x + 8.0, y + 4.0, L"PAINT LIFT" },
                { x + 10.0, y + first_row, L"A" }, { x + 10.0, y + second_row, L"B" } };
            for (const int team : { 1, -1 })
            {
                // Down for +1, up for -1, as a short shaft within the team's row.
                const int heading = team * guide.team_a_heading;
                const double column = x + 66.0;
                const double row = y + (team == 1 ? first_row : second_row);
                const double tail = row + (heading > 0 ? 0.0 : line_height);
                const double tip = row + (heading > 0 ? line_height : 0.0);
                drawing.strokes.push_back({ column, tail, column, tip });
                drawing.strokes.push_back({ column - 5.0, tip - heading * 7.0, column, tip });
                drawing.strokes.push_back({ column + 5.0, tip - heading * 7.0, column, tip });
            }
        }
        else
        {
            drawing.labels = { { x + 8.0, y + 4.0, L"PAINT FERRY" },
                { x + 10.0, y + first_row, L"A" }, { x + 10.0, y + second_row, L"B" } };
            for (const int team : { 1, -1 })
            {
                const int heading = team * guide.team_a_heading;
                const double tail = x + (heading > 0 ? 34.0 : 118.0);
                const double tip = x + (heading > 0 ? 118.0 : 34.0);
                const double row = y + (team == 1 ? first_row : second_row) + line_height * 0.5;
                drawing.strokes.push_back({ tail, row, tip, row });
                drawing.strokes.push_back({ tip - heading * 7.0, row - 5.0, tip, row });
                drawing.strokes.push_back({ tip - heading * 7.0, row + 5.0, tip, row });
            }
        }
        return drawing;
    }
}
