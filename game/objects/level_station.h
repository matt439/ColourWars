#pragma once

#include "engine/math/vector2f.h"
#include <string>

// A named place in a level a developer can jump a player to (`goto`). The
// position is the character's top-left, like a spawn.
struct LevelStation
{
	std::string name;
	mattmath::Vector2F position;
};
