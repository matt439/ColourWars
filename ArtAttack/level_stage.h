#ifndef LEVEL_STAGE_H
#define LEVEL_STAGE_H

#include <string>

enum class level_stage
{
    KING_OF_THE_HILL,
    TURBULENCE,
    CLOSE_QUARTERS,
    RANDOM,
    MAX_STAGE = RANDOM,
};

// The registry name a stage's level definition is loaded and looked up under.
// The engine keys every resource by name and never interprets one, so this
// table - the only place that knows King of the Hill is a file called
// king_of_the_hill - belongs to the game.
//
// Throws std::invalid_argument for RANDOM: it is a menu choice, not a stage,
// and has to be resolved to a real one before anything asks for its level.
std::string level_asset_name(level_stage stage);

#endif // !LEVEL_STAGE_H
