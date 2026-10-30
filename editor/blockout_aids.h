#pragma once

#include "editor/level_document.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace colourwars_editor
{
    struct BlockoutTuning
    {
        double player_width = 0.0;
        double player_height = 0.0;
        double paint_thickness = 0.0;
        bool measured_movement_matches = false;
        double tap_rise = 0.0;
        double tap_travel = 0.0;
        double held_rise = 0.0;
        double held_travel = 0.0;
        double measured_speed = 0.0;
    };

    struct ScoringFace
    {
        Entry entry;
        Rect strip;
        bool along_x = false;
        bool leading = false;
        // Slab strips retain authoritative local area in strip and their real
        // world quad here. Flat/moving strips keep the original rectangle path.
        std::optional<PaintPolygon> polygon;
    };

    BlockoutTuning load_blockout_tuning(const std::filesystem::path& path,
        const std::filesystem::path& measurements);
    std::vector<ScoringFace> scoring_faces(const LevelDocument& document,
        const BlockoutTuning& tuning);
    // Physical scoring-strip area, clipped to a region when supplied. As in
    // runtime scoring, corners/overlapping strips contribute separately.
    double scoring_area(const std::vector<ScoringFace>& faces,
        const std::optional<Rect>& region = std::nullopt);

    // Scoring area either side of the out_of_bounds centre (ED#25). Strips
    // that straddle the centre split by region, so left + right is the total.
    struct SideScoring
    {
        double left = 0.0;
        double right = 0.0;
    };
    SideScoring side_scoring_areas(const std::vector<ScoringFace>& faces, const Rect& out_of_bounds);

    // The blockout summary's symmetry line: left and right scoring area,
    // spawns and collision objects with no mirror partner, unpartnered
    // decorations apart, and paint ferries as authored.
    std::string symmetry_summary(const LevelDocument& document, const std::vector<ScoringFace>& faces);
}
