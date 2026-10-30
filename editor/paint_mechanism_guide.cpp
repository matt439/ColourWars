#include "editor/paint_mechanism_guide.h"
#include "game/level_format.h"

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
        return std::nullopt;
    }

    PaintMechanismDrawing paint_mechanism_drawing(const PaintMechanismGuide& guide,
        const Rect& screen_body, double line_height)
    {
        PaintMechanismDrawing drawing;
        const double width = 132.0;
        const bool ferry = guide.kind == PaintMechanismKind::ferry;
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
