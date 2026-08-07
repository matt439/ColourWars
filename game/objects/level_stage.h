#pragma once

#include <string>

enum class LevelStage
{
    king_of_the_hill,
    turbulence,
    close_quarters,
    random,
    max_stage = random,
};

// The registry name a stage's level definition is loaded and looked up under.
// The engine keys every resource by name and never interprets one, so this
// table - the only place that knows King of the Hill is a file called
// king_of_the_hill - belongs to the game.
//
// Throws std::invalid_argument for RANDOM: it is a menu choice, not a stage,
// and has to be resolved to a real one before anything asks for its level.
std::string level_asset_name(LevelStage stage);
