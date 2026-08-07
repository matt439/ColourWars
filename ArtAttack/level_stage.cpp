#include "pch.h"
#include "level_stage.h"

std::string level_asset_name(level_stage stage)
{
    switch (stage)
    {
    case level_stage::KING_OF_THE_HILL:
        return "king_of_the_hill";
    case level_stage::TURBULENCE:
        return "turbulence";
    case level_stage::CLOSE_QUARTERS:
        return "close_quarters";
    case level_stage::RANDOM:
        throw std::invalid_argument(
            "level_stage::RANDOM has no level of its own - resolve it to a "
            "stage before asking for one.");
    }

    throw std::invalid_argument("Unknown level_stage: " +
        std::to_string(static_cast<int>(stage)) + ".");
}
