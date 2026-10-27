#pragma once

#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include <string>

// A named place in a level a developer can jump a player to (`goto`). The
// position is the character's top-left, like a spawn.
struct LevelStation
{
	std::string name;
	mattmath::Vector2F position;
};

// A station's optional practice prompt (POL#19). Untimed solo practice shows
// it while the player's centre is inside `area`; the first lesson in file
// order wins where areas overlap. Title and text are authored UTF-8.
struct LevelLesson
{
	std::string station;
	mattmath::RectangleF area;
	std::string title;
	std::string text;
};
