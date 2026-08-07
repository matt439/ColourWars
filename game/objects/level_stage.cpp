#include "game/pch.h"
#include "game/objects/level_stage.h"

std::string level_asset_name(LevelStage stage)
{
    switch (stage)
    {
    case LevelStage::king_of_the_hill:
        return "king_of_the_hill";
    case LevelStage::turbulence:
        return "turbulence";
    case LevelStage::close_quarters:
        return "close_quarters";
    case LevelStage::random:
        throw std::invalid_argument(
            "LevelStage::random has no level of its own - resolve it to a "
            "stage before asking for one.");
    }

    throw std::invalid_argument("Unknown LevelStage: " +
        std::to_string(static_cast<int>(stage)) + ".");
}
