#pragma once

#include "editor/level_json.h"
#include "game/horizontal_motion_contract.h"

namespace colourwars_editor
{
    struct MotionPlayerSize { float width, height; };
    horizontal_motion_contract::Rectangle motion_rectangle(const level_json::Json& object);
    horizontal_motion_contract::Spec decode_horizontal_motion(const level_json::Json& object);
    MotionPlayerSize load_motion_player_size(const std::filesystem::path& tuning);
    MotionGuide horizontal_motion_guide(const level_json::Json& object, const MotionPlayerSize& player,
        const std::optional<Rect>& preview = std::nullopt);
    void validate_motion_geometry(const level_json::Document& document,
        const std::filesystem::path& tuning);
}
