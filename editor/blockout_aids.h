#pragma once

#include "editor/level_document.h"

#include <filesystem>
#include <optional>
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
    };

    BlockoutTuning load_blockout_tuning(const std::filesystem::path& path,
        const std::filesystem::path& measurements);
    std::vector<ScoringFace> scoring_faces(const LevelDocument& document,
        const BlockoutTuning& tuning);
    // Physical scoring-strip area, clipped to a region when supplied. As in
    // runtime scoring, corners/overlapping strips contribute separately.
    double scoring_area(const std::vector<ScoringFace>& faces,
        const std::optional<Rect>& region = std::nullopt);
}
