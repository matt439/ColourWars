#pragma once

#include "editor/level_document.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace colourwars_editor
{
    enum class PaintMechanismKind { gate, ferry };

    // Authored rules from the inspector's current property snapshot. No paint,
    // permission or runtime motion state is observed or predicted.
    struct PaintMechanismGuide
    {
        PaintMechanismKind kind;
        int team_a_heading = 0;
        bool operator==(const PaintMechanismGuide&) const = default;
    };

    struct MechanismStroke { double x1, y1, x2, y2; };
    struct MechanismLabel { double x, y; std::wstring text; };
    struct PaintMechanismDrawing
    {
        Rect panel;
        std::vector<MechanismStroke> strokes;
        std::vector<MechanismLabel> labels;
    };

    std::optional<PaintMechanismGuide> paint_mechanism_guide(
        const std::map<std::string, std::string>& properties);
    // Screen-pixel schematic above the actual drawn body, including its initial
    // motion phase or provisional drag pose. It is outside hit-test geometry.
    // The caller supplies its measured font line height in the same coordinates;
    // the default is a conservative 96-DPI model value for non-raster callers.
    PaintMechanismDrawing paint_mechanism_drawing(const PaintMechanismGuide& guide,
        const Rect& screen_body, double line_height = 24.0);
}
